// Combat / denial module for apex. See combat.h for the contract.
//
// Budget: the threat model costs ~550 fuel per visible enemy (name, stats, state, gems, two
// cooldowns); the whole hook stays under ~10k except when it issues a path step (2-6k).
// Every champion runs in its own WASM instance, so file-level statics are per champion.
#include "combat.h"
#ifndef APEX_CX
#define APEX_CX 9
#endif

namespace apex {

namespace {

template <class F>
bool cguard(F&& f) {
    try { return f(); } catch (...) { return false; }
}

// Static kit facts per champion type (engine designs; cached once per instance).
struct Design {
    bool ok{};
    float str{}, foc{};
    int range{1};
    bool ranged{};
    int reset_slot{-1}, dash_slot{-1}, cc_slot{-1}, dmg_slot{-1}, aoe_slot{-1};
    float cc_mod{}, dmg_mod{}, aoe_mod{};
    bool cc_excl{}, cc_stuns{};
    bool marcella{};
};

const Design& design_of(champion_type t) {
    static Design cache[12];
    Design& d = cache[static_cast<std::size_t>(t) % 12];
    if (d.ok) return d;
    const auto& des = cambc::champion_designs[static_cast<std::size_t>(t)];
    const auto s = des.base_stats();
    d.str = s.base_strength; d.foc = s.base_focus; d.range = s.base_attack_range;
    d.ranged = is_ranged_type(t);
    d.marcella = t == champion_type::marcella;
    const auto abs = des.abilities();
    for (int i = 0; i < 2; ++i) {
        const auto& info = abs[i].info();
        if (!info) continue;
        std::visit([&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, cambc::ability_attack_reset>) d.reset_slot = i;
            else if constexpr (std::is_same_v<T, cambc::ability_dash>) d.dash_slot = i;
            else if constexpr (std::is_same_v<T, cambc::ability_cc>) { d.cc_slot = i; d.cc_mod = v.modifier; d.cc_excl = v.exclusive_with_movement; d.cc_stuns = v.duration > 0; }
            else if constexpr (std::is_same_v<T, cambc::ability_damage>) { d.dmg_slot = i; d.dmg_mod = v.modifier; }
            else if constexpr (std::is_same_v<T, cambc::ability_aoe_damage>) { d.aoe_slot = i; d.aoe_mod = v.modifier; }
        }, *info);
    }
    d.ok = true;
    return d;
}

// Chase / focus state (per champion instance).
vec2 g_chase_pos{-1, -1};
std::uint32_t g_chase_round{};
bool g_chasing{};
bitboard g_enemy_spawn_zone; bool g_zone_ready{};

const bitboard& enemy_spawn_zone() {
    if (!g_zone_ready) {
        for (auto p : cambc::map::spawn_region(team::enemy).positions()) g_enemy_spawn_zone.draw_chebyshev(p, 2);
        g_zone_ready = true;
    }
    return g_enemy_spawn_zone;
}

} // namespace

float ThreatModel::incoming_at(vec2 p) const {
    float d = 0;
    for (const auto& th : t) {
        if (th.stunned) continue;
        const int c = cheb(th.pos, p);
        if (c <= th.reach_a) d += th.melee_burst;
        if (c <= th.reach_s) d += th.spell_burst;
    }
    return d;
}

float ThreatModel::incoming2_at(vec2 p) const {
    float d = 0;
    for (const auto& th : t) {
        const int c = cheb(th.pos, p);
        if (c <= th.reach_a + 1) d += th.melee_burst + th.str;
        if (c <= th.reach_s + 1) d += th.spell_burst;
    }
    return d;
}

float ThreatModel::sustained_at(vec2 p, int turns) const {
    float d = 0;
    for (const auto& th : t) {
        if (th.stunned) continue;
        const int c = cheb(th.pos, p);
        if (c <= th.reach_a) d += th.melee_burst + th.str * static_cast<float>(std::max(turns - 1, 0));
        if (c <= th.reach_s) d += th.spell_burst;
    }
    return d;
}

bool ThreatModel::stun_reaches(vec2 p) const {
    for (const auto& th : t) if (th.stun_ready && !th.stunned && cheb(th.pos, p) <= th.reach_s) return true;
    return false;
}

struct CombatCtx {
    Bot& b;
    World& w;
    ThreatModel tm;
    float margin{};

    explicit CombatCtx(Bot& bot) : b(bot), w(bot.w) { margin = 0.15f * w.maxhp; }

    void build() {
        tm.t.clear();
        tm.t.reserve(w.enemies.size());
        for (std::size_t i = 0; i < w.enemies.size(); ++i) {
            const auto& e = w.enemies[i];
            Threat th; th.idx = static_cast<int>(i); th.pos = w.enemy_pos[i]; th.hp = w.enemy_hp[i];
            th.type = e.champion_name();
            const Design& d = design_of(th.type);
            th.ranged = d.ranged;
            float str = d.str, foc = d.foc;
            bool reset_ready = false, dash_ready = false, cc_ready = false, dmg_ready = false, aoe_ready = false;
            cguard([&] {
                str = e.strength(); foc = e.focus();
                th.gems = e.gem_count();
                th.submitting = e.state().contains(cambc::champion_state::submitting_gems);
                const auto abs = e.abilities();
                int cd[2] = { 0, 0 };
                for (int k = 0; k < 2; ++k) if (abs[k].info()) cd[k] = abs[k].cooldown();
                reset_ready = d.reset_slot >= 0 && cd[d.reset_slot] == 0;
                dash_ready = d.dash_slot >= 0 && cd[d.dash_slot] == 0;
                cc_ready = d.cc_slot >= 0 && cd[d.cc_slot] == 0;
                dmg_ready = d.dmg_slot >= 0 && cd[d.dmg_slot] == 0;
                aoe_ready = d.aoe_slot >= 0 && cd[d.aoe_slot] == 0;
                th.stunned = e.buffs().has_stun();
                return true;
            });
            th.str = str;
            th.melee_burst = str * (reset_ready ? 2.0f : 1.0f);
            // One successful cast per turn: the spell burst is the best ready spell, not the sum.
            th.spell_burst = std::max({ cc_ready ? d.cc_mod * foc : 0.0f, dmg_ready ? d.dmg_mod * foc : 0.0f, aoe_ready ? d.aoe_mod * foc : 0.0f });
            th.stun_ready = (cc_ready && d.cc_stuns) || (d.marcella && reset_ready);
            th.reach_a = 1 + d.range + (dash_ready ? 2 : 0);
            th.reach_s = (cc_ready && !d.cc_excl) ? 5 : 4;
            if (th.submitting || th.stunned) { th.reach_a = d.range; th.reach_s = 4; }
            tm.t.push_back(th);
        }
    }

    bool reset_ready() const {
        if (b.ab_reset < 0) return false;
        bool r = false; cguard([&] { r = !b.ab[b.ab_reset]->is_on_cooldown(); return true; }); return r;
    }
    bool slot_ready(int s) const {
        if (s < 0) return false;
        bool r = false; cguard([&] { r = !b.ab[s]->is_on_cooldown(); return true; }); return r;
    }

    // My damage this turn against a target at distance `dist` (stationary cast assumed).
    float my_burst(int dist, bool allow_spells) const {
        float d = 0;
        if (dist <= b.attack_range && w.can_attack()) d += b.body.str * (reset_ready() && w.can_cast() ? 2.0f : 1.0f);
        if (allow_spells && w.can_cast() && dist <= 4) {
            float best = 0;
            if (slot_ready(b.ab_cc)) best = std::max(best, b.ab[b.ab_cc]->get<cambc::ability_cc>().modifier * b.body.foc);
            if (slot_ready(b.ab_damage)) best = std::max(best, b.ab[b.ab_damage]->get<cambc::ability_damage>().modifier * b.body.foc);
            if (slot_ready(b.ab_aoe)) best = std::max(best, b.ab[b.ab_aoe]->get<cambc::ability_aoe_damage>().modifier * b.body.foc);
            d += best;
        }
        return d;
    }

    // Visible allies near a point: approximate two-turn burst from the static model.
    float ally_force_near(vec2 p, int radius, const std::vector<cambc::champion>& vis_allies, bool& melee_adjacent, vec2 adj_to) const {
        float f = 0;
        for (const auto& a : vis_allies) {
            vec2 ap; float ahp = 0;
            if (!cguard([&] { ap = a.position(); ahp = a.health(); return true; })) continue;
            if (ap == w.pos) continue;
            if (cheb(ap, p) > radius || ahp <= 0) continue;
            int s = -1;
            for (std::size_t i = 0; i < b.allies.size(); ++i) if (static_cast<cambc::handle_champion_t>(b.allies[i]) == static_cast<cambc::handle_champion_t>(a)) s = static_cast<int>(i);
            const Body& bd = s >= 0 ? b.ally_body[s] : b.body;
            f += 2.0f * bd.basic + 4.0f * bd.spell;
            if (!bd.ranged && cheb(ap, adj_to) <= 1) melee_adjacent = true;
        }
        return f;
    }

    // --- actions -----------------------------------------------------------------------

    bool cast_stun_on(const Threat& th) {
        if (b.ab_cc < 0 || !w.can_cast()) return false;
        const auto info = b.ab[b.ab_cc]->get<cambc::ability_cc>();
        if (info.exclusive_with_movement && w.moved) return false;
        if (cheb(th.pos, w.pos) > static_cast<int>(info.range)) return false;
        if (!slot_ready(b.ab_cc)) return false;
        const auto& e = w.enemies[th.idx];
        if (!e.is_visible()) return false;
        bool ok = false;
        cguard([&] { ok = b.ab[b.ab_cc]->use(e); return ok; });
        if (ok) { w.casted = true; w.exclusive_cast = info.exclusive_with_movement; b.refresh_state(); }
        return ok;
    }

    bool cast_damage_on(const Threat& th) {
        if (!w.can_cast()) return false;
        const auto& e = w.enemies[th.idx];
        if (b.ab_damage >= 0 && slot_ready(b.ab_damage)) {
            const auto info = b.ab[b.ab_damage]->get<cambc::ability_damage>();
            if (!(info.exclusive_with_movement && w.moved) && cheb(th.pos, w.pos) <= static_cast<int>(info.range) && e.is_visible()) {
                bool ok = false; cguard([&] { ok = b.ab[b.ab_damage]->use(e); return ok; });
                if (ok) { w.casted = true; w.exclusive_cast = info.exclusive_with_movement; b.refresh_state(); return true; }
            }
        }
        if (b.ab_aoe >= 0 && slot_ready(b.ab_aoe)) {
            const auto info = b.ab[b.ab_aoe]->get<cambc::ability_aoe_damage>();
            if (!(info.exclusive_with_movement && w.moved) && cheb(th.pos, w.pos) <= static_cast<int>(info.range)) {
                bool ok = false; cguard([&] { ok = b.ab[b.ab_aoe]->use(th.pos); return ok; });
                if (ok) { w.casted = true; w.exclusive_cast = info.exclusive_with_movement; b.refresh_state(); return true; }
            }
        }
        return false;
    }

    bool basic_and_reset(const Threat& th) {
        const auto& e = w.enemies[th.idx];
        if (cheb(th.pos, w.pos) > b.attack_range || !e.is_visible()) return false;
        bool hit = false;
        if (w.can_attack()) hit = b.try_attack_champion(e);
        if (hit && b.ab_reset >= 0 && w.can_cast() && reset_ready() && w.enemy_hp[th.idx] > b.body.str) {
            bool ok = false; cguard([&] { ok = b.ab[b.ab_reset]->use(); return ok; });
            if (ok) { w.casted = true; b.refresh_state(); if (e.is_visible()) b.try_attack_champion(e); }
        }
        return hit;
    }

    // Step to the neighbouring tile (or stay) that minimises next-turn incoming damage, then
    // distance from threats, then distance home. Uses the dash when it is ready and helps.
    bool flee() {
        if (!w.can_move() && !(b.ab_dash >= 0 && w.can_cast())) return false;
        const auto& home = b.home_field();
        auto score = [&](vec2 t) {
            float s = -tm.incoming_at(t) * 4.0f - monster_damage_at(t) * 4.0f;
            int nearest = 99; for (const auto& th : tm.t) nearest = std::min(nearest, cheb(th.pos, t));
            s += 30.0f * std::min(nearest, 6);
            s -= 2.0f * static_cast<float>(std::min<int>(home.get(t), 200));
            return s;
        };
        vec2 best = w.pos; float best_s = score(w.pos);
        if (w.can_move()) {
            for (auto t : w.blocked.reachable_tiles(w.pos, 1).positions()) {
                if (t == w.pos) continue;
                const float s = score(t);
                if (s > best_s) { best_s = s; best = t; }
            }
        }
        // Dash: two tiles away, same scoring, only when strictly better than the walk.
        if (b.ab_dash >= 0 && w.can_cast() && slot_ready(b.ab_dash) && fuel_ok(FUEL_RESERVE + 4000)) {
            bitboard dash_walls = w.blocked & ~cambc::map::holes();
            bitboard area = dash_walls.reachable_tiles(w.pos, 2) & ~w.blocked;
            vec2 db = w.pos; float ds = best_s + 20.0f;
            for (auto t : area.positions()) { const float s = score(t); if (s > ds) { ds = s; db = t; } }
            if (db != w.pos) {
                bool ok = false; cguard([&] { ok = b.ab[b.ab_dash]->use(db); return ok; });
                if (ok) { w.casted = true; cguard([&] { w.pos = cambc::self().position(); return true; }); w.blocked.set(w.pos, false); b.refresh_state(); return true; }
            }
        }
        if (best == w.pos) return false;
        return b.move_to(best);
    }

    float monster_damage_at(vec2 t) const {
        float d = 0;
        for (const auto& m : w.monsters) if (m.awake && m.hp > 0 && cheb(m.pos, t) <= CAMP_RANGE) d += camp_dmg(m.camp.kind());
        return d;
    }
    // Never chase into a monster strike zone or towards the enemy spawn.
    void block_hazards() {
        w.blocked |= enemy_spawn_zone();
        for (const auto& m : w.monsters) if (m.awake && m.hp > 0 && camp_dmg(m.camp.kind()) > 0) { bitboard z; z.draw_chebyshev(m.pos, CAMP_RANGE); z.set(w.pos, false); w.blocked |= z; }
    }

    // Tiles a carrier must not walk through: within reach of threats whose burst threatens the bag.
    void mark_danger(float lethal) {
        float total = 0;
        for (const auto& th : tm.t) if (cheb(th.pos, w.pos) <= th.reach_a + 2) total += th.melee_burst + th.spell_burst;
        const bool group_lethal = total >= lethal;
        bitboard zone;
        bool any = false;
        for (const auto& th : tm.t) {
            if (th.stunned) continue;
            const bool lethal_alone = th.melee_burst + th.spell_burst >= lethal;
            if (lethal_alone || group_lethal) { zone.draw_chebyshev(th.pos, th.reach_a); any = true; }
        }
        if (!any) return;
        zone.set(w.pos, false);
        w.blocked |= zone;
    }

    // Engage one target. Returns true when the turn is spent.
    bool engage(const Threat& th, bool fronted) {
        int dist = cheb(th.pos, w.pos);
        const bool kill_now = th.hp <= my_burst(dist, true);
        // Stun first: it interrupts tributes and freezes the target for the focus fire.
        if (!w.exclusive_cast) cast_stun_on(th);
        if (!b.ranged) {
            if (dist > b.attack_range && w.can_move() && !w.exclusive_cast) {
                bitboard goals; goals.draw_chebyshev(th.pos, b.attack_range);
                bitboard blocked_save = w.blocked;
                block_hazards();
                b.step_towards(goals, true);
                w.blocked = blocked_save;
                dist = cheb(th.pos, w.pos);
            }
            if (b.ab_shield >= 0 && !w.casted) b.cast_shield(tm.incoming_at(w.pos));
            if (!w.casted && !w.moved) cast_damage_on(th);
            basic_and_reset(th);
            return true;
        }
        // Ranged: attack when the target cannot reach me next turn (or an ally fronts it, or it dies
        // now); otherwise kite one step away (a ranged walk and attack exclude each other).
        const bool reachable = tm.incoming_at(w.pos) > 0;
        if (dist <= b.attack_range && (!reachable || fronted || kill_now || w.hp > 0.7f * w.maxhp)) {
            if (!w.moved) cast_damage_on(th);
            basic_and_reset(th);
            return true;
        }
        if (reachable && !fronted) {
            if (!w.moved && dist <= 4 && w.hp > 0.5f * w.maxhp && (kill_now || th.hp <= 1.5f * my_burst(dist, true))) { cast_damage_on(th); basic_and_reset(th); return true; }
            flee();
            return true;
        }
        if (dist > b.attack_range) {
            if (!w.moved && dist <= 4) cast_damage_on(th);
            if (!w.exclusive_cast && w.can_move()) {
                bitboard goals; goals.draw_chebyshev(th.pos, b.attack_range);
                // Do not step into the melee reach of any threat.
                bitboard zone; for (const auto& t2 : tm.t) if (!t2.stunned && !t2.submitting) zone.draw_chebyshev(t2.pos, t2.reach_a - 1);
                goals &= ~zone;
                bitboard blocked_save = w.blocked;
                block_hazards();
                b.step_towards(goals, false);
                w.blocked = blocked_save;
            }
            return true;
        }
        return false;
    }

    void write_focus(vec2 p) {
        cguard([&] { cambc::shared_memory[6] = static_cast<std::uint8_t>(p.x + 1); cambc::shared_memory[7] = static_cast<std::uint8_t>(p.y + 1); return true; });
    }
    vec2 read_focus() const {
        vec2 f{-1, -1};
        cguard([&] { const int x = cambc::shared_memory[6]; const int y = cambc::shared_memory[7]; if (x > 0 && y > 0) f = { x - 1, y - 1 }; return true; });
        return f;
    }

    // --- the hook -----------------------------------------------------------------------

    // Baseline behaviour (measured 23-11 vs spar-hunter) plus opt-in features (APEX_CX bits).
    bool baseline() {
        if (b.in_danger_from_champions()) { if (b.step_away_from_threats()) return true; }
        if (!w.can_attack()) return false;
        int best = -1; float best_hp = 1e9f;
        for (std::size_t i = 0; i < w.enemies.size(); ++i) {
            if (cheb(w.enemy_pos[i], w.pos) > b.attack_range) continue;
            const float hp = w.enemy_hp[i]; if (hp <= 0) continue;
            if (hp < best_hp) { best_hp = hp; best = static_cast<int>(i); }
        }
        if (best >= 0 && (!b.ranged || best_hp <= 2.0f * w.hp)) {
            if (APEX_CX & 16) basic_and_reset(tm.t[best]); else b.try_attack_champion(w.enemies[best]);
            return false;
        }
        if (b.ranged && w.hp < 0.6f * w.maxhp) {
            for (const auto p : w.enemy_pos) if (cheb(p, w.pos) <= 3) { if (b.step_away_from_threats()) return true; break; }
        }
        if (!b.ranged && w.hp > 0.5f * w.maxhp && w.can_move()) {
            int near = -1; int nd = 99;
            for (std::size_t i = 0; i < w.enemies.size(); ++i) { const int d = cheb(w.enemy_pos[i], w.pos); if (d <= 3 && d < nd) { nd = d; near = static_cast<int>(i); } }
            if (near >= 0) {
                bitboard goals; goals.draw_chebyshev(w.enemy_pos[near], 1);
                if (b.step_towards(goals, false) && cheb(w.enemy_pos[near], w.pos) <= b.attack_range && w.enemies[near].is_visible())
                    b.try_attack_champion(w.enemies[near]);
            }
        }
        return false;
    }

    bool run() {
        if (w.enemies.empty()) {
            if ((APEX_CX & 1) && g_chasing && w.round <= g_chase_round + 3 && w.gems < 20 && w.hp > 0.5f * w.maxhp && w.can_move()) {
                bitboard goals; goals.draw_chebyshev(g_chase_pos, 1);
                goals &= ~enemy_spawn_zone();
                b.step_towards(goals, false);
                return true;
            }
            g_chasing = false;
            return false;
        }
        build();
        const float inc = tm.incoming_at(w.pos);
        const bool carrier = w.gems >= 20;
        const int dist_alt = [&] { int d = 99; for (auto a : b.altars) d = std::min(d, cheb(a, w.pos)); return d; }();

        // Lethal next turn: get out.
        if ((APEX_CX & 4) && inc >= w.hp - margin) {
            if (b.ab_shield >= 0 && w.can_cast() && inc < w.hp + 200) b.cast_shield(inc);
            if (flee()) return true;
        }

        // Carrier safety: a stun + kill combo in reach, or a tribute that cannot be survived.
        if ((APEX_CX & 2) && carrier) {
            const float inc2 = tm.incoming2_at(w.pos);
            const int channel = std::max(1, static_cast<int>((w.gems + 9) / 10));
            const bool stun_kill = tm.stun_reaches(w.pos) && inc2 >= w.hp - margin;
            const bool tribute_lethal = dist_alt <= 1 && tm.sustained_at(w.pos, channel) >= w.hp - margin;
            if (stun_kill || tribute_lethal) { if (flee()) return true; }
            mark_danger(w.hp - margin);
        }

        // Kill-steal: submitting enemies (stun interrupts), fat bags we can kill, free kills.
        if ((APEX_CX & 1) && !carrier && w.hp > 0.4f * w.maxhp) {
            std::vector<cambc::champion> vis_allies;
            if (fuel_ok(FUEL_RESERVE + 12000)) vis_allies = cambc::vision::champions(team::ally);
            const vec2 focus = read_focus();
            int best = -1; float best_score = 0; float my_now = 0;
            for (const auto& th : tm.t) {
                const int dist = cheb(th.pos, w.pos);
                if (dist > 8 || th.hp <= 0) continue;
                float score = 0;
                if (th.submitting) score += 1000;
                score += 5.0f * static_cast<float>(th.gems);
                const float burst = my_burst(dist, true);
                if (th.hp <= burst) score += 600;
                if (focus.x >= 0 && cheb(focus, th.pos) <= 2) score += 300;
                if (th.stunned) score += 150;
                score -= 15.0f * dist;
                if (score > best_score) { best_score = score; best = th.idx; my_now = burst; }
            }
            if (best >= 0) {
                const Threat& th = tm.t[best];
                const int dist = cheb(th.pos, w.pos);
                bool fronted = false;
                float theirs = 0;
                for (const auto& t2 : tm.t) if (cheb(t2.pos, w.pos) <= t2.reach_a + 1 || cheb(t2.pos, w.pos) <= t2.reach_s + 1) theirs += 1.5f * t2.melee_burst + t2.spell_burst;
                float ours = 2.0f * my_burst(1, true) + b.body.spell * 2 + ally_force_near(th.pos, 4, vis_allies, fronted, th.pos);
                const bool fat = th.gems >= 30 || th.submitting;
                const bool interrupt = th.submitting && dist <= 5 && (tm.t.size() == 1 || ours >= theirs);
                const bool kill = th.hp <= my_now || (fat && th.hp <= ours && ours >= theirs);
                if (interrupt || kill) {
                    write_focus(th.pos); g_chasing = true; g_chase_pos = th.pos; g_chase_round = w.round;
                    if (engage(th, fronted)) return true;
                }
            }
        }
        g_chasing = false;
        return baseline();
    }

    bool tribute_check() {
        if (w.enemies.empty()) return false;
        build();
        const float inc = tm.incoming_at(w.pos);
        if (inc <= 0) return false;
        int rem = 1;
        cguard([&] { rem = cambc::self().remaining_channel_time(); return true; });
        rem = std::max(rem, 1);
        if (tm.sustained_at(w.pos, rem) < w.hp - 0.05f * w.maxhp) return false; // damage does not cancel: hold
        bool ok = false;
        cguard([&] { cambc::self().cancel_submit_gems(); ok = true; return true; });
        if (!ok) return false;
        b.refresh_state();
        w.restr = cambc::self().restrictions();
        flee();
        return true;
    }
};

bool combat_hook(Bot& bot) {
    if (!fuel_ok(FUEL_RESERVE + 10000)) return false;
    CombatCtx c(bot);
    return c.run();
}

bool combat_tribute_check(Bot& bot) {
    if (!(APEX_CX & 8)) return false;
    if (!fuel_ok(FUEL_RESERVE + 10000)) return false;
    CombatCtx c(bot);
    return c.tribute_check();
}

} // namespace apex
