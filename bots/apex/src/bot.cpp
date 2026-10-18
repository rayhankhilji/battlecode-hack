// Champion economy state machine for apex. See README.md for the architecture.
#include "bot.h"
#include "combat.h"

namespace apex {

using cambc::champion_state;
using cambc::buff_restrictions;

namespace {

#ifdef APEX_TRACE
#define TRACE(...) do { if (apex_trace_on) std::printf(__VA_ARGS__); } while (0)
bool apex_trace_on = false;
#else
#define TRACE(...) do {} while (0)
#endif

bitboard dilate(const bitboard& src, int r) {
    bitboard out;
    for (auto p : src.positions()) out.draw_chebyshev(p, r);
    return out;
}

template <class F>
bool guarded(const char* what, F&& f) {
    try { return f(); }
    catch (const std::exception& e) { std::printf("act %s failed: %s\n", what, e.what()); return false; }
}

} // namespace

// ----------------------------------------------------------------------------------------------
// Initialisation (first play turn, 2x fuel)

void Bot::init() {
    const std::uint64_t i0 = fuel();
    auto& me = cambc::self();
    type = me.champion_name();
    body = model_body(type);
    ranged = is_ranged_type(type);
    attack_range = me.base_stats().base_attack_range;

    allies = cambc::champions(team::ally);
    for (std::size_t i = 0; i < allies.size(); ++i)
        if (static_cast<cambc::handle_champion_t>(allies[i]) == static_cast<cambc::handle_champion_t>(me)) slot = static_cast<int>(i);

    plan_ok = decode_plan(plan);
    ally_slotplan.assign(allies.size(), 255);
    if (plan_ok) {
        for (std::size_t i = 0; i < allies.size(); ++i) {
            const auto t = static_cast<std::uint8_t>(allies[i].champion_name());
            for (int s = 0; s < plan.bodies; ++s) {
                bool used = false;
                for (std::size_t j = 0; j < i; ++j) if (ally_slotplan[j] == s) used = true;
                if (!used && plan.s[s].type == t) { ally_slotplan[i] = static_cast<std::uint8_t>(s); break; }
            }
        }
        if (slot >= 0 && ally_slotplan[slot] != 255) { mine = plan.s[ally_slotplan[slot]]; role = static_cast<Role>(mine.role); }
        else { plan_ok = false; }
    }
    if (!plan_ok) { role = default_role(type); mine = SlotPlan{}; mine.type = static_cast<std::uint8_t>(type); mine.role = role; mine.rank = role; mine.squad = static_cast<std::uint8_t>(slot); }
    for (const auto& a : allies) ally_body.push_back(model_body(a.champion_name()));

    const std::uint64_t i1 = fuel();
    terrain = cambc::map::walls() | cambc::map::holes() | cambc::map::altars() | cambc::map::spawn_region(team::enemy);
    spawn = cambc::map::spawn_region(team::ally);
    altars = cambc::map::altars().positions();
    for (auto a : altars) altar_goals.draw_chebyshev(a, 1);
    altar_goals &= ~terrain;

    const std::uint64_t i2 = fuel();
    groups = cambc::map::camp_groups();
    const std::uint64_t i3 = fuel();
    const int W = cambc::map::width(), H = cambc::map::height();
    tile_group.assign(static_cast<std::size_t>(W * H), 0);
    gspawn.reserve(groups.size()); gcenter.reserve(groups.size()); ghp.reserve(groups.size()); gdanger.reserve(groups.size());
    gdmax.reserve(groups.size()); ggems.reserve(groups.size()); ggold.reserve(groups.size()); gresp.reserve(groups.size());
    for (std::size_t gi = 0; gi < groups.size() && gi < MAX_GROUPS; ++gi) {
        const auto& g = groups[gi];
        gspawn.emplace_back(cambc::uninitialized);
        g.spawn_tiles_into(gspawn.back());
        int sx = 0, sy = 0, n = 0; float hp = 0, dmg = 0, dmax = 0;
        for (const auto& c : g) {
            const vec2 p = c.spawn_position();
            sx += p.x; sy += p.y; ++n; hp += camp_hp(c.kind()); dmg += camp_dmg(c.kind()); dmax = std::max(dmax, camp_dmg(c.kind()));
            tile_group[static_cast<std::size_t>(p.y * W + p.x)] = static_cast<std::uint8_t>(gi + 1);
        }
        gcenter.push_back({ sx / std::max(n, 1), sy / std::max(n, 1) });
        ghp.push_back(hp); gdanger.push_back(dmg); gdmax.push_back(dmax);
        ggems.push_back(static_cast<int>(g.gem_value())); ggold.push_back(static_cast<int>(g.gold_value())); gresp.push_back(g.max_respawn_timer());
        if (static_cast<std::uint32_t>(static_cast<cambc::handle_camp_group_t>(g)) != gi) std::printf("warn: group handle %u != index %zu\n", (unsigned)static_cast<cambc::handle_camp_group_t>(g), gi);
    }
    if (plan_ok && plan.groups != groups.size()) { std::printf("warn: plan groups %d != %zu, ignoring plan\n", plan.groups, groups.size()); plan_ok = false; }

    const std::uint64_t i4 = fuel();
    const auto abilities = me.abilities();
    for (int i = 0; i < 2; ++i) {
        ab[i] = abilities[i];
        if (!ab[i]->info()) continue;
        if (ab[i]->is_attack_reset()) ab_reset = i;
        else if (ab[i]->is_dash()) ab_dash = i;
        else if (ab[i]->is_shield()) ab_shield = i;
        else if (ab[i]->is_damage()) ab_damage = i;
        else if (ab[i]->is_aoe_damage()) ab_aoe = i;
        else if (ab[i]->is_cc()) ab_cc = i;
        else if (ab[i]->is_heal()) ab_heal = i;
    }
    me.set_priority(role == ROLE_TANK ? 20 : role == ROLE_DPS ? 10 : 0);
    std::printf("init slot=%d type=%d role=%s plan=%d squad=%d circ=%d groups=%zu fuel=%llu cost=%llu/%llu/%llu/%llu/%llu\n", slot, static_cast<int>(type), role_name(role),
        plan_ok, mine.squad, mine.n, groups.size(), (unsigned long long)fuel(),
        (unsigned long long)(i0 - i1), (unsigned long long)(i1 - i2), (unsigned long long)(i2 - i3), (unsigned long long)(i3 - i4), (unsigned long long)(i4 - fuel()));
    init_done = true;
}

// ----------------------------------------------------------------------------------------------
// Per-turn observation

void Bot::refresh_state() {
    auto& me = cambc::self();
    w.st = me.state();
}

void Bot::snapshot() {
    auto& me = cambc::self();
    w.pos = me.position();
    w.hp = me.health(); w.maxhp = me.max_health();
    w.gems = me.gem_count(); w.gold = me.gold_count();
    w.st = me.state();
    w.restr = me.restrictions();
    w.at_spawn = spawn.is_set(w.pos);
    w.moved = w.attacked = w.casted = w.exclusive_cast = false;
    cambc::vision::visible_tiles_into(w.visible);
    cambc::vision::camp_tiles_into(w.camp_tiles);
    cambc::vision::champion_tiles_into(w.champ_tiles);
    w.blocked = terrain | w.camp_tiles | w.champ_tiles | cambc::vision::teleport_tiles();
    w.blocked.set(w.pos, false);
    w.monsters.clear();
    const int W = cambc::map::width();
    // Only load camps that matter this turn: my target group and anything that can hit me.
    static std::vector<vec2> tiles; tiles.resize(64);
    const std::uint32_t n = w.camp_tiles.positions_into(tiles);
    for (std::uint32_t i = 0; i < n && i < tiles.size(); ++i) {
        const vec2 p = tiles[i];
        const int g = static_cast<int>(tile_group[static_cast<std::size_t>(p.y * W + p.x)]) - 1;
        if (g < 0) continue;
        if (g != target && cheb(p, w.pos) > CAMP_RANGE + 1) continue;
        try {
            cambc::camp c = cambc::vision::camp_at(p);
            Monster m{ c, p, c.health(), g, c.is_awake() };
            w.monsters.push_back(std::move(m));
        } catch (...) {}
    }
    w.enemies = cambc::vision::champions(team::enemy);
    ally_alive.resize(allies.size());
    for (std::size_t i = 0; i < allies.size(); ++i) ally_alive[i] = static_cast<int>(i) == slot ? true : allies[i].is_alive();
}

void Bot::read_claims() {
    std::array<std::uint8_t, 8> raw{};
    cambc::shared_memory.read(0, std::span{raw});
    for (int i = 0; i < 8; ++i) claims[i] = unpack_claim(raw[i]);
    for (int i = 0; i < 8; ++i) if (claims[i].group >= groups.size() && claims[i].group != NO_GROUP) claims[i] = { NO_GROUP, PH_IDLE };
}

void Bot::write_claim(std::uint8_t phase, int group) {
    my_claim = { static_cast<std::uint8_t>(group < 0 ? NO_GROUP : group), phase };
}

void Bot::update_known() {
    for (std::size_t g = 0; g < groups.size(); ++g) {
        if (cheb(gcenter[g], w.pos) > 11) continue;
        bool seen = false;
        for (const auto& c : groups[g]) if (w.visible.is_set(c.spawn_position())) { seen = true; break; }
        if (!seen) continue;
        bool occupied = false;
        for (const auto& c : groups[g]) if (w.camp_tiles.is_set(c.spawn_position())) { occupied = true; break; }
        if (occupied) { known[g].state = 1; known[g].round = w.round; continue; }
        // Non-target groups are refreshed every third round to bound the cost on dense maps.
        if (static_cast<int>(g) != target && (w.round + g) % 3 != 0 && known[g].round + 3 > w.round) continue;
        try {
            const auto st = groups[g].state();
            Known& k = known[g];
            k.round = w.round;
            if (std::holds_alternative<cambc::map::camp_group_alive>(st)) k.state = 1;
            else if (std::holds_alternative<cambc::map::camp_group_reward_available>(st)) k.state = 2;
            else { k.state = 3; k.respawn_round = w.round + std::get<cambc::map::camp_group_respawning>(st).timer; }
        } catch (...) {}
    }
    // Ally captures imply a respawn soon.
    for (std::size_t s = 0; s < allies.size() && s < 8; ++s) {
        if (static_cast<int>(s) == slot || squad_of_slot(static_cast<int>(s)) == squad_of_slot(slot)) continue;
        const Claim c = claims[s];
        if (c.phase == PH_CAPTURE && c.group < groups.size() && known[c.group].state != 3) {
            known[c.group].state = 3; known[c.group].round = w.round; known[c.group].respawn_round = w.round + gresp[c.group] + 4;
        }
    }
}

std::uint32_t Bot::team_gems() const { return cambc::global_submitted_gems(team::ally); }

int Bot::squad_of_slot(int s) const {
    if (!plan_ok || s < 0 || s >= static_cast<int>(ally_slotplan.size()) || ally_slotplan[s] == 255) return 100 + s;
    return plan.s[ally_slotplan[s]].squad;
}
int Bot::rank_of_slot(int s) const {
    if (!plan_ok || s < 0 || s >= static_cast<int>(ally_slotplan.size()) || ally_slotplan[s] == 255) return default_role(allies[s].champion_name());
    return plan.s[ally_slotplan[s]].rank;
}
Role Bot::role_of_slot(int s) const {
    if (!plan_ok || s < 0 || s >= static_cast<int>(ally_slotplan.size()) || ally_slotplan[s] == 255) return default_role(allies[s].champion_name());
    return static_cast<Role>(plan.s[ally_slotplan[s]].role);
}

// Damage per round from living monsters of group g that can reach `at`.
float Bot::incoming_damage_at(vec2 at, int g) const {
    float d = 0;
    for (const auto& m : w.monsters) if ((g < 0 || m.group == g) && cheb(m.pos, at) <= CAMP_RANGE) d += camp_dmg(m.camp.kind());
    return d;
}

// ----------------------------------------------------------------------------------------------
// Movement

bool Bot::move_to(vec2 tile) {
    auto& me = cambc::self();
    bool ok = false;
    guarded("move", [&] { ok = me.move_absolute(tile); return ok; });
    if (ok) { w.moved = true; w.pos = tile; w.blocked.set(tile, false); refresh_state(); }
    return ok;
}

// Step one tile along a shortest path towards any free goal tile. Returns true when at a goal or moved.
bool Bot::step_towards(const bitboard& goals_in, bool allow_dash) {
    const std::uint64_t pf = fuel();
    struct Done { Bot* b; std::uint64_t pf; ~Done() { b->prof[5] = std::max(b->prof[5], pf - fuel()); } } done{this, pf};
    bitboard goals = goals_in & ~w.blocked;
    if (goals.is_set(w.pos)) return true;
    if (!w.can_move() || goals.is_empty()) return false;
    auto& me = cambc::self();
    distance_field& field = path_field;
    static bitboard visited, origin;
    std::uint16_t here = distance_field::unreachable;
    // Reuse the last field for a few rounds when the goal set is unchanged (the local step still
    // avoids currently blocked tiles); recompute when stale or when we fell off the field.
    if (path_round + 6 > w.round && path_goals == goals_in) here = field.get(w.pos);
    if (here == distance_field::unreachable) {
        origin.clear(); origin.set(w.pos);
        field.compute_to(w.blocked, goals, origin, visited);
        here = field.get(w.pos);
        if (here == distance_field::unreachable) {
            // Units may be blocking the corridor: plan through them and let the local step avoid them.
            const bitboard loose = terrain & ~goals;
            field.compute_to(loose, goals, origin, visited);
            here = field.get(w.pos);
            if (here == distance_field::unreachable) { path_round = 0; return false; }
        }
        path_goals = goals_in; path_round = w.round;
    }
    // Dash first when the goal is far and the dash is ready.
    if (allow_dash && ab_dash >= 0 && here >= 3 && w.can_cast() && !w.restr.contains(buff_restrictions::cast)) {
        bool ready = false;
        guarded("dash-cd", [&] { ready = !ab[ab_dash]->is_on_cooldown(); return ready; });
        if (ready && fuel_ok(FUEL_RESERVE + 5000)) {
            bitboard area; area.draw_chebyshev(w.pos, 2); area &= ~w.blocked;
            vec2 best = w.pos; std::uint16_t bd = here;
            for (auto t : area.positions()) { const auto d = field.get(t); if (d + 2 <= here && d < bd) { bd = d; best = t; } }
            if (best != w.pos) {
                bool ok = false;
                guarded("dash", [&] { ok = ab[ab_dash]->use(best); return ok; });
                if (ok) { w.casted = true; w.pos = me.position(); w.blocked.set(w.pos, false); here = field.get(w.pos); refresh_state(); if (goals.is_set(w.pos)) return true; }
            }
        }
    }
    if (!w.can_move()) return w.moved;
    for (int attempt = 0; attempt < 2; ++attempt) {
        vec2 best = w.pos; std::uint16_t bd = here;
        for (auto t : w.blocked.reachable_tiles(w.pos, 1).positions()) {
            const auto d = field.get(t);
            if (d < bd) { bd = d; best = t; }
        }
        if (best == w.pos) return false;
        if (move_to(best)) return true;
        w.blocked.set(best, true); // hidden blocker revealed; try the next best tile once
    }
    return false;
}

const distance_field& Bot::home_field() {
    if (!f_home_ready) { bitboard visited; f_home.compute(terrain, spawn, visited); f_home_ready = true; }
    return f_home;
}

bool Bot::in_danger_from_champions() const {
    if (w.hp > 0.35f * w.maxhp) return false;
    for (const auto& e : w.enemies) if (cheb(e.position(), w.pos) <= 4) return true;
    return false;
}

// Move towards spawn, preferring tiles farther from visible enemies.
bool Bot::step_away_from_threats() {
    if (!w.can_move()) return false;
    vec2 best = w.pos; float best_score = -1e9f;
    for (auto t : w.blocked.reachable_tiles(w.pos, 1).positions()) {
        float score = -static_cast<float>(home_field().get(t));
        int nearest = 99;
        for (const auto& e : w.enemies) nearest = std::min(nearest, cheb(e.position(), t));
        score += 3.0f * nearest;
        if (score > best_score) { best_score = score; best = t; }
    }
    if (best == w.pos) return false;
    return move_to(best);
}

bool Bot::try_attack_champion(const cambc::champion& target) {
    if (!w.can_attack()) return false;
    auto& me = cambc::self();
    const bool ok = guarded("attack-champ", [&] { me.attack(target); return true; });
    if (ok) { w.attacked = true; refresh_state(); }
    return ok;
}

// ----------------------------------------------------------------------------------------------
// Kit

bool Bot::attack_monster(const Monster& m) {
    if (!w.can_attack() || m.hp <= 0 || cheb(m.pos, w.pos) > attack_range) return false;
    auto& me = cambc::self();
    const bool ok = guarded("attack", [&] { me.attack(m.camp); return true; });
    if (ok) {
        w.attacked = true; refresh_state();
        // Refresh the health of the target and its neighbours (splash) so follow-up actions skip corpses.
        for (auto& o : w.monsters) {
            if (o.group != m.group || cheb(o.pos, m.pos) > 1) continue;
            o.hp = o.camp.is_visible() ? o.camp.health() : 0;
        }
    }
    return ok;
}

const Monster* Bot::choose_attack_target(int g, int range) {
    const Monster* best = nullptr; float best_key = -1e9f;
    for (const auto& m : w.monsters) {
        if (m.group != g || m.hp <= 0 || cheb(m.pos, w.pos) > range) continue;
        float key = -m.hp;                                 // finish low targets first
        if (m.camp.kind() == cambc::camp_type::elk) key += 300; // remove the big hitter sooner when close
        if (body.splash > 1) {                              // Varg: maximise splash coverage
            int n = 0; for (const auto& o : w.monsters) if (o.group == g && cheb(o.pos, m.pos) <= 1) ++n;
            key += 2000.0f * n;
        }
        if (key > best_key) { best_key = key; best = &m; }
    }
    return best;
}

bool Bot::attack_reset_and_attack(int g) {
    if (ab_reset < 0 || !w.can_cast()) return false;
    const Monster* t = choose_attack_target(g, attack_range);
    if (!t) return false;
    bool ready = false;
    guarded("reset-cd", [&] { ready = !ab[ab_reset]->is_on_cooldown(); return ready; });
    if (!ready) return false;
    bool ok = false;
    guarded("reset", [&] { ok = ab[ab_reset]->use(); return ok; });
    if (!ok) return false;
    w.casted = true; refresh_state();
    return attack_monster(*t);
}

bool Bot::cast_damage_on_monster(const Monster& m) {
    if (ab_damage < 0 || !w.can_cast()) return false;
    const auto info = ab[ab_damage]->get<cambc::ability_damage>();
    if (info.exclusive_with_movement && w.moved) return false;
    if (cheb(m.pos, w.pos) > static_cast<int>(info.range)) return false;
    bool ready = false;
    guarded("dmg-cd", [&] { ready = !ab[ab_damage]->is_on_cooldown(); return ready; });
    if (!ready) return false;
    bool ok = false;
    guarded("dmg", [&] { ok = ab[ab_damage]->use(m.camp); return ok; });
    if (ok) { w.casted = true; w.exclusive_cast = info.exclusive_with_movement; refresh_state(); }
    return ok;
}

bool Bot::cast_aoe_on_monsters(int g) {
    if (ab_aoe < 0 || !w.can_cast()) return false;
    const auto info = ab[ab_aoe]->get<cambc::ability_aoe_damage>();
    if (info.exclusive_with_movement && w.moved) return false;
    bool ready = false;
    guarded("aoe-cd", [&] { ready = !ab[ab_aoe]->is_on_cooldown(); return ready; });
    if (!ready) return false;
    // Centre on the living monster covering the most group members.
    const Monster* best = nullptr; int best_n = 0;
    for (const auto& m : w.monsters) {
        if (m.group != g || cheb(m.pos, w.pos) > static_cast<int>(info.range)) continue;
        int n = 0; for (const auto& o : w.monsters) if (o.group == g && cheb(o.pos, m.pos) <= static_cast<int>(info.radius)) ++n;
        if (n > best_n) { best_n = n; best = &m; }
    }
    if (!best) return false;
    bool ok = false;
    guarded("aoe", [&] { ok = ab[ab_aoe]->use(best->pos); return ok; });
    if (ok) { w.casted = true; w.exclusive_cast = info.exclusive_with_movement; refresh_state(); }
    return ok;
}

bool Bot::cast_cc_on_monster(const Monster& m) {
    if (ab_cc < 0 || !w.can_cast()) return false;
    const auto info = ab[ab_cc]->get<cambc::ability_cc>();
    if (info.exclusive_with_movement && w.moved) return false;
    if (cheb(m.pos, w.pos) > static_cast<int>(info.range)) return false;
    bool ready = false;
    guarded("cc-cd", [&] { ready = !ab[ab_cc]->is_on_cooldown(); return ready; });
    if (!ready) return false;
    bool ok = false;
    guarded("cc", [&] { ok = ab[ab_cc]->use(m.camp); return ok; });
    if (ok) { w.casted = true; w.exclusive_cast = info.exclusive_with_movement; refresh_state(); }
    return ok;
}

bool Bot::cast_shield(float incoming) {
    if (ab_shield < 0 || !w.can_cast() || incoming < 40) return false;
    auto& me = cambc::self();
    bool has = false;
    guarded("buffs", [&] { has = me.buffs().has_shield(); return true; });
    if (has) return false;
    bool ready = false;
    guarded("shield-cd", [&] { ready = !ab[ab_shield]->is_on_cooldown(); return ready; });
    if (!ready) return false;
    bool ok = false;
    guarded("shield", [&] { ok = ab[ab_shield]->use(); return ok; });
    if (ok) { w.casted = true; refresh_state(); }
    return ok;
}

// ----------------------------------------------------------------------------------------------
// Shopping and recovery

void Bot::shop() {
    auto& me = cambc::self();
    guarded("shop", [&] {
        const std::uint32_t gold = me.gold_count();
        if (gold >= 10 && me.consumables()[cambc::consumable_type::altar_lock].count() == 0 && me.can_buy(cambc::consumable_type::altar_lock))
            me.buy(cambc::consumable_type::altar_lock);
        const cambc::item_type shard = role == ROLE_CASTER ? cambc::item_type::monster_shard_of_focus
            : role == ROLE_TANK ? cambc::item_type::jungle_shard_of_health : cambc::item_type::monster_shard_of_strength;
        const std::uint32_t g2 = me.gold_count();
        const std::uint32_t count = g2 / 30;
        if (count > 0 && me.can_buy(shard, count)) me.buy(shard, count);
        return true;
    });
}

bool Bot::recover() {
    auto& me = cambc::self();
    if (w.at_spawn) {
        shop();
        if (w.hp < 0.95f * w.maxhp) { mode = Mode::RECOVER; return true; }
        recovering = false;
        return false;
    }
    if (!recovering && w.hp < 0.35f * w.maxhp) { recovering = true; drop_target(); }
    if (!recovering) return false;
    mode = Mode::RECOVER;
    // Safe to recall: no awake monster within reach and no enemy nearby.
    bool threatened = false;
    for (const auto& m : w.monsters) if (m.awake && cheb(m.pos, w.pos) <= CAMP_RANGE + 1) threatened = true;
    for (const auto& e : w.enemies) if (cheb(e.position(), w.pos) <= 5) threatened = true;
    if (!threatened) {
        guarded("recall", [&] { me.recall(); return true; });
        return true;
    }
    step_away_from_threats();
    return true;
}

// ----------------------------------------------------------------------------------------------
// Delivery

bool Bot::bank_now() {
    if (w.gems == 0) return false;
    const std::uint8_t threshold = plan_ok ? plan.bank_threshold : 40;
    if (w.gems >= threshold) return true;
    if (team_gems() + w.gems >= cambc::constants::winning_gems) return true;
    if (w.round + 40 >= cambc::constants::max_rounds) return true;
    int near = 99; for (auto a : altars) near = std::min(near, cheb(a, w.pos));
    if (w.gems >= 20 && near <= 5) return true;
    return false;
}

void Bot::deliver() {
    const std::uint64_t pf = fuel();
    struct Done { Bot* b; std::uint64_t pf; ~Done() { b->prof[7] = std::max(b->prof[7], pf - fuel()); } } done{this, pf};
    auto& me = cambc::self();
    mode = Mode::DELIVER;
    write_claim(PH_DELIVER, -1);
    for (auto a : altars) {
        if (cheb(a, w.pos) > cambc::constants::submit_gems_range) continue;
        bool locked = false;
        guarded("lock?", [&] { locked = cambc::vision::has_altar_lock(a); return true; });
        if (locked) {
            bool enemy_lock = false;
            cambc::altar_lock lock = cambc::vision::altar_lock_at(a);
            enemy_lock = lock.is_enemy();
            if (enemy_lock) {
                if (w.can_attack() && cheb(a, w.pos) <= attack_range) {
                    if (guarded("attack-lock", [&] { me.attack(lock); return true; })) { w.attacked = true; refresh_state(); }
                    return;
                }
                continue; // try another altar
            }
        } else {
            bool enemy_near = false;
            for (const auto& e : w.enemies) if (cheb(e.position(), w.pos) <= 8) enemy_near = true;
            if (enemy_near) guarded("place-lock", [&] {
                if (me.consumables()[cambc::consumable_type::altar_lock].count() > 0) return me.consumables()[cambc::consumable_type::altar_lock].use(a);
                return false;
            });
        }
        if (guarded("submit", [&] { me.submit_gems(a); return true; })) { mode = Mode::TRIBUTE; return; }
    }
    step_towards(altar_goals, true);
}

// ----------------------------------------------------------------------------------------------
// Farming

void Bot::drop_target() { target = -1; wait_rounds = 0; }

bool Bot::target_available(int g, int travel) const {
    if (g < 0 || g >= static_cast<int>(groups.size())) return false;
    if (plan_ok && (plan.g[g].flags & GF_EXCLUDED)) return false;
    if (!plan_ok) { for (const auto& c : groups[g]) if (c.kind() == cambc::camp_type::monolith) return false; }
    const Known& k = known[g];
    if (k.state == 3 && k.respawn_round > w.round + travel + 2) return false;
    return true;
}

int Bot::pick_target() {
    if (!fuel_ok(FUEL_OPTIONAL)) return -1;
    if (w.round < next_pick_round) return -1;
    const std::uint64_t pf = fuel();
    struct Done { Bot* b; std::uint64_t pf; ~Done() { b->prof[4] = std::max(b->prof[4], pf - fuel()); } } done{this, pf};
    static distance_field field; static bitboard visited, origin;
    origin.clear(); origin.set(w.pos);
    field.compute(terrain, origin, visited);
    prof[7] = std::max(prof[7], pf - fuel());
    const int my_squad = squad_of_slot(slot);
    // Squad composition (alive members) for feasibility.
    float dps = 0, poke = 0, tank_hp = 0, stun8 = 0, shield8 = 0; bool melee = false;
    for (std::size_t s = 0; s < allies.size(); ++s) {
        if (squad_of_slot(static_cast<int>(s)) != my_squad) continue;
        if (!ally_alive[s]) continue;
        if (static_cast<int>(s) != slot && claims[s].phase == PH_DELIVER) continue;
        const Body& b = ally_body[s];
        dps += b.dps(); poke += b.poke; stun8 += b.stun8;
        if (!b.ranged) { melee = true; const float hp = static_cast<int>(s) == slot ? w.hp : b.hp; if (hp > tank_hp) { tank_hp = hp; shield8 = b.shield8; } }
    }
    const float gw = (plan_ok ? plan.gold_weight_pct : 30) / 100.0f;
    int best = -1; float best_score = 0;
    static bool dbg_done = false;
    for (std::size_t g = 0; g < groups.size(); ++g) {
        const std::uint64_t d0 = fuel();
        int travel = 65535;
        for (const auto& c : groups[g]) travel = std::min<int>(travel, field.get(c.spawn_position()));
        const std::uint64_t d1 = fuel();
        if (travel >= 60000) continue;
        if (!target_available(static_cast<int>(g), travel)) continue;
        const std::uint64_t d2 = fuel();
        if (!dbg_done && g == 5) { dbg_done = true; std::printf("pickdbg get=%llu avail=%llu\n", (unsigned long long)(d0 - d1), (unsigned long long)(d1 - d2)); }
        bool other_squad = false;
        for (std::size_t s = 0; s < allies.size() && s < 8; ++s) {
            if (static_cast<int>(s) == slot || squad_of_slot(static_cast<int>(s)) == my_squad) continue;
            if (claims[s].group == g && (claims[s].phase == PH_CLEAR || claims[s].phase == PH_CAPTURE) && ally_alive[s]) other_squad = true;
        }
        if (other_squad) continue;
        const std::uint64_t d3 = fuel();
        const Fight f = estimate_fight(ghp[g], gdanger[g], gdmax[g], dps, poke, melee, tank_hp, stun8, shield8);
        const std::uint64_t d4 = fuel();
        if (g == 5) std::printf("pickdbg squadloop=%llu fight=%llu\n", (unsigned long long)(d2 - d3), (unsigned long long)(d3 - d4));
        if (!f.feasible) continue;
        float value = ggems[g] + gw * ggold[g];
        if (plan_ok) {
            if (plan.g[g].flags & GF_HOME) value *= 1.2f;
            for (int c = 0; c < mine.n; ++c) if (mine.circuit[c] == g) { value *= 1.5f; break; }
        }
        float wait = 0;
        if (known[g].state == 3 && known[g].respawn_round > w.round + travel) wait = static_cast<float>(known[g].respawn_round - w.round - travel);
        const float dist_altar = plan_ok ? plan.g[g].dist_altar : 10;
        const float cyc = travel + wait + f.rounds + 6 + 0.5f * (dist_altar + ggems[g] / 10.0f);
        const float score = value / cyc;
        if (score > best_score) { best_score = score; best = static_cast<int>(g); }
        if (g == 5) std::printf("pickdbg rest=%llu\n", (unsigned long long)(d2 - fuel()));
    }
    if (best < 0) next_pick_round = w.round + 4;
    return best;
}

void Bot::approach_group(int g) {
    mode = Mode::TRAVEL;
    bitboard goals = dilate(gspawn[g], ranged ? 3 : 1) & ~terrain & ~gspawn[g];
    step_towards(goals, true);
}

// Capture: every present squad member stands on a cleared tile (rewards split evenly, and the
// gold funds everyone's shards). Members without a free tile wait next to the group.
void Bot::capture_group(int g) {
    mode = Mode::CAPTURE;
    write_claim(PH_CAPTURE, g);
    if (gspawn[g].is_set(w.pos)) return; // hold position
    bitboard free_tiles = gspawn[g] & ~w.blocked;
    if (!free_tiles.is_empty()) { step_towards(free_tiles, true); return; }
    if (cheb(gcenter[g], w.pos) <= 2) return;
    step_towards(dilate(gspawn[g], 1) & ~gspawn[g] & ~terrain, true);
}

void Bot::engage_group(int g) {
    const std::uint64_t pf = fuel();
    struct Done { Bot* b; std::uint64_t pf; ~Done() { b->prof[6] = std::max(b->prof[6], pf - fuel()); } } done{this, pf};
    mode = Mode::CLEAR;
    std::vector<const Monster*> living;
    bool any_awake = false;
    for (const auto& m : w.monsters) if (m.group == g && m.hp > 0) { living.push_back(&m); any_awake |= m.awake; }
    // Claimants (squad mates working on this group) and present power.
    std::vector<int> claimants{ slot };
    for (std::size_t s = 0; s < allies.size() && s < 8; ++s) {
        if (static_cast<int>(s) == slot || claims[s].group != g || claims[s].phase != PH_CLEAR || !ally_alive[s]) continue;
        claimants.push_back(static_cast<int>(s));
    }
    // The opener is the present melee claimant with the most health (it takes the aggro).
    float dps = body.dps(); float poke = body.poke; float stun8 = body.stun8; float shield8 = body.ranged ? 0 : body.shield8; bool melee = !body.ranged;
    int opener = ranged ? -1 : slot; float opener_hp = ranged ? -1 : w.hp;
    bool tank_adjacent = false;
    int absent = static_cast<int>(claimants.size()) - 1;
    {
        bitboard near_group; near_group.draw_chebyshev(gcenter[g], 8);
        for (const auto& a : cambc::vision::champions(team::ally)) {
            int s = -1;
            for (std::size_t i = 0; i < allies.size(); ++i) if (static_cast<cambc::handle_champion_t>(allies[i]) == static_cast<cambc::handle_champion_t>(a)) s = static_cast<int>(i);
            if (s < 0 || s == slot) continue;
            const vec2 p = a.position();
            if (!near_group.is_set(p)) continue;
            if (claims[s].group != g || claims[s].phase != PH_CLEAR) continue;
            --absent;
            dps += ally_body[s].dps(); poke += ally_body[s].poke; stun8 += ally_body[s].stun8;
            if (!ally_body[s].ranged) {
                melee = true;
                const float hp = a.health();
                if (hp > opener_hp || (hp == opener_hp && s < opener)) { opener_hp = hp; opener = s; shield8 = ally_body[s].shield8; }
            }
            for (const Monster* m : living) if (cheb(m->pos, p) <= 1) tank_adjacent = true;
        }
    }
    const bool opener_present = opener >= 0;
    float live_hp = 0, live_danger = 0, live_max = 0;
    for (const Monster* m : living) { live_hp += m->hp; live_danger += camp_dmg(m->camp.kind()); live_max = std::max(live_max, camp_dmg(m->camp.kind())); }
    const float incoming = incoming_damage_at(w.pos, g);
    const bool i_open = opener == slot;
    const Fight fight = estimate_fight(live_hp, live_danger, live_max, dps, poke, melee, opener_hp, stun8, shield8);
    TRACE("  engage: living=%zu awake=%d opener=%d absent=%d tankadj=%d dps=%.0f feas=%d rounds=%.0f dmg=%.0f wait=%d\n", living.size(), any_awake, opener, absent, tank_adjacent, dps, fight.feasible, fight.rounds, fight.tank_damage, wait_rounds);
    bool allowed;
    if (live_danger <= 0 || any_awake || tank_adjacent) allowed = true;
    else if (i_open) allowed = fight.feasible;
    else if (!opener_present && ranged) allowed = true; // poke-only squad
    else allowed = false;
    if (allowed) wait_rounds = 0;
    else if (i_open) {
        // I would take the aggro but cannot survive it: heal if hurt, otherwise wait for the squad.
        if (w.hp < 0.85f * w.maxhp) { recovering = true; drop_target(); recover(); return; }
        if (absent == 0 || ++wait_rounds > 20) { known[g].state = 3; known[g].respawn_round = w.round + 40; drop_target(); return; }
    } else if (!opener_present) {
        if (++wait_rounds > 30) { known[g].state = 3; known[g].respawn_round = w.round + 40; drop_target(); return; }
    }

    // Bail out when the fight turned bad for me specifically.
    if (any_awake && incoming > 0 && w.hp < std::max(0.25f * w.maxhp, 3 * incoming + 50)) { recovering = true; drop_target(); recover(); return; }

    // Positions.
    bitboard outer, inner;
    const int want = allowed ? (ranged ? 3 : 1) : 4;
    const int keep = allowed ? (ranged ? 2 : 1) : 4;
    for (const Monster* m : living) { outer.draw_chebyshev(m->pos, want); if (keep > 1) inner.draw_chebyshev(m->pos, keep - 1); }
    bitboard goals = outer & ~inner & ~w.blocked;
    if (allowed && ranged) goals &= w.visible;
    if (!allowed && !ranged) { // waiting melee: idle at ring 4-5 (no walking needed when already outside range)
        int mind = 99; for (const Monster* m : living) mind = std::min(mind, cheb(m->pos, w.pos));
        if (mind >= 4 && mind <= 6) return;
        bitboard o2; for (const Monster* m : living) o2.draw_chebyshev(m->pos, 5);
        goals = o2 & ~inner & ~w.blocked;
    }
    if (!allowed) {
        // Casters poke from range 4 while waiting.
        if (ranged) {
            const Monster* t = choose_attack_target(g, 4);
            if (t && w.can_cast() && !w.moved) { if (!cast_damage_on_monster(*t)) cast_aoe_on_monsters(g); }
        }
        int mind = 99; for (const Monster* m : living) mind = std::min(mind, cheb(m->pos, w.pos));
        if (mind < 4 || mind > 6) step_towards(goals, false);
        return;
    }

    // Engaged. Ranged: spells first (they exclude walking), then attack, else reposition.
    const bool in_pos = goals.is_set(w.pos);
    TRACE("  engaged in_pos=%d goals=%u canmove=%d canatk=%d cancast=%d\n", in_pos, goals.count(), w.can_move(), w.can_attack(), w.can_cast());
    if (ranged) {
        const Monster* t4 = choose_attack_target(g, 4);
        const Monster* t3 = choose_attack_target(g, attack_range);
        if (t4 && w.can_cast() && !w.moved && (in_pos || t3)) {
            if (ab_damage >= 0 && ab_aoe >= 0) {
                int n = 0; for (const Monster* m : living) if (cheb(m->pos, t4->pos) <= 1) ++n;
                if (n >= 2) { if (!cast_aoe_on_monsters(g)) cast_damage_on_monster(*t4); }
                else { if (!cast_damage_on_monster(*t4)) cast_aoe_on_monsters(g); }
            } else if (ab_damage >= 0) cast_damage_on_monster(*t4);
            else if (ab_aoe >= 0) cast_aoe_on_monsters(g);
        }
        if (t4 && any_awake && ab_cc >= 0 && w.can_cast()) {
            const Monster* big = nullptr; for (const Monster* m : living) if (m->awake && cheb(m->pos, w.pos) <= 4 && (!big || camp_dmg(m->camp.kind()) > camp_dmg(big->camp.kind()))) big = m;
            if (big && camp_dmg(big->camp.kind()) > 0) cast_cc_on_monster(*big);
        }
        if (t3 && w.can_attack() && (in_pos || w.exclusive_cast)) {
            attack_monster(*t3);
            attack_reset_and_attack(g);
            return;
        }
        if (!w.exclusive_cast && !in_pos) step_towards(goals, true);
        else if (t3 && w.can_attack()) { attack_monster(*t3); attack_reset_and_attack(g); }
        return;
    }
    // Melee: close in first (dash allowed), then attack, then spend the cast.
    if (!in_pos) step_towards(goals, true);
    const Monster* t = choose_attack_target(g, attack_range);
    if (!t) return;
    attack_monster(*t);
    const float incoming_now = incoming_damage_at(w.pos, g);
    if (cast_shield(incoming_now)) return;
    if (!w.moved) {
        // Stationary: damaging/stunning spells beat a reset when several monsters are awake.
        const Monster* big = nullptr; for (const Monster* m : living) if (cheb(m->pos, w.pos) <= 4 && (!big || camp_dmg(m->camp.kind()) > camp_dmg(big->camp.kind()))) big = m;
        if (ab_aoe >= 0 && living.size() >= 1 && cast_aoe_on_monsters(g)) return;
        if (ab_cc >= 0 && big && cast_cc_on_monster(*big)) return;
        if (ab_damage >= 0 && cast_damage_on_monster(*t)) return;
    }
    attack_reset_and_attack(g);
}

void Bot::farm() {
    // The squad leader is the lowest-rank (tank first) alive member with an active claim; everyone
    // else follows its group. Without any claim I lead and choose for myself.
    const int my_squad = squad_of_slot(slot);
    int leader = -1, leader_rank = 99;
    for (std::size_t s = 0; s < allies.size() && s < 8; ++s) {
        if (squad_of_slot(static_cast<int>(s)) != my_squad) continue;
        if (claims[s].group == NO_GROUP || (claims[s].phase != PH_CLEAR && claims[s].phase != PH_CAPTURE)) continue;
        if (!ally_alive[s]) continue;
        const int r = rank_of_slot(static_cast<int>(s));
        if (r < leader_rank || (r == leader_rank && static_cast<int>(s) < leader)) { leader_rank = r; leader = static_cast<int>(s); }
    }
    if (leader >= 0 && leader != slot) {
        const int follow = claims[leader].group;
        if (follow != target && target_available(follow, 0)) { target = follow; target_since = w.round; wait_rounds = 0; }
    }
    if (target >= 0 && !target_available(target, 0)) drop_target();
    for (int iteration = 0; iteration < 2; ++iteration) {
        if (target < 0) {
            target = pick_target();
            target_since = w.round; wait_rounds = 0;
            if (target < 0) {
                mode = Mode::IDLE;
                write_claim(PH_IDLE, -1);
                if (w.gems > 0) { delivering = true; deliver(); return; }
                if (w.hp < 0.8f * w.maxhp && !w.at_spawn) { recovering = true; recover(); return; }
                // Wait near the group that respawns soonest.
                int soon = -1; std::uint32_t best = ~0u;
                for (std::size_t g = 0; g < groups.size(); ++g)
                    if (known[g].state == 3 && known[g].respawn_round < best && !(plan_ok && (plan.g[g].flags & GF_EXCLUDED))) { best = known[g].respawn_round; soon = static_cast<int>(g); }
                if (soon >= 0) { bitboard goals = dilate(gspawn[soon], 5) & ~dilate(gspawn[soon], 3) & ~terrain; step_towards(goals, false); }
                return;
            }
        }
        const int g = target;
        write_claim(PH_CLEAR, g);
        bool visible = false;
        for (const auto& c : groups[g]) if (w.visible.is_set(c.spawn_position())) { visible = true; break; }
        bool living = false;
        for (const auto& m : w.monsters) if (m.group == g && m.hp > 0) { living = true; break; }
        TRACE("r=%u s=%d pos=(%d,%d) hp=%.0f tgt=%d vis=%d living=%d known=%d nmon=%zu st=%u\n", w.round, slot, w.pos.x, w.pos.y, w.hp, g, visible, living, known[g].state, w.monsters.size(), w.st.bits());
        if (living) { engage_group(g); return; }
        if (!visible) { approach_group(g); return; }
        // Visible and no living monster: capture or move on.
        const Known& k = known[g];
        if (k.state == 2) { capture_group(g); if (target >= 0) return; continue; }
        if (k.state == 3) {
            if (mode == Mode::CAPTURE && gspawn[g].is_set(w.pos)) {
                auto& me = cambc::self();
                guarded("ward", [&] { if (me.ward_cooldown() == 0) me.place_ward(w.pos); return true; });
            }
            drop_target(); continue;
        }
        // Alive by state but nothing visible: get closer to see the members.
        approach_group(g); return;
    }
    mode = Mode::IDLE;
}

// ----------------------------------------------------------------------------------------------
// Turn

void Bot::play_turn() {
    const std::uint64_t f0 = fuel();
    w.round = cambc::round_number();
#ifdef APEX_TRACE
    apex_trace_on = (w.round >= 1 && w.round <= 60);
#endif
    if (have_prev && w.round > prev_round + 1) ++susp;
    prev_round = w.round; have_prev = true;
    if (!init_done) { init(); if (!fuel_ok(70000)) return; }
    auto& me = cambc::self();
    my_claim = { NO_GROUP, PH_IDLE };
    if (!me.is_alive()) { shop(); mode = Mode::RECOVER; delivering = false; recovering = false; drop_target(); return; }
    const std::uint64_t f1 = fuel();
    snapshot();
    const std::uint64_t f2 = fuel();
    read_claims();
    update_known();
    const std::uint64_t f3 = fuel();
    prof[0] = std::max(prof[0], f0 - f1); prof[1] = std::max(prof[1], f1 - f2); prof[2] = std::max(prof[2], f2 - f3);
    prof_start = f3;
    if (!fuel_ok(FUEL_RESERVE)) return;

    // Channels: never issue anything that would cancel them.
    if (w.st.contains(champion_state::submitting_gems)) { mode = Mode::TRIBUTE; write_claim(PH_DELIVER, -1); return; }
    if (w.st.contains(champion_state::recalling) || w.st.contains(champion_state::teleporting)) { mode = Mode::RECOVER; return; }
    // Stunned or otherwise fully restricted.
    if (w.restr.contains(buff_restrictions::move) && w.restr.contains(buff_restrictions::attack)) return;
    if (!w.can_move() && !w.can_attack() && !w.can_cast()) return;

    if (combat_hook(*this)) return;
    if (recover()) return;
    if (delivering) {
        if (w.gems == 0) delivering = false;
        else { deliver(); return; }
    }
    if (bank_now()) { delivering = true; drop_target(); deliver(); return; }
    farm();
}

void Bot::end_of_turn() {
    if (!init_done) return;
    const std::uint64_t f = fuel();
    fmin = std::min(fmin, f);
    if (slot >= 0 && slot < 8) {
        const std::uint8_t b = pack_claim(my_claim);
        cambc::shared_memory.write(static_cast<std::size_t>(slot), std::span<const std::uint8_t>{&b, 1});
    }
    if (prof_start) { prof[3] = std::max(prof[3], prof_start - f); prof_start = 0; }
    if (w.round % 100 == 0 && f > 6000) {
        std::printf("T r=%u slot=%d role=%s fmin=%llu susp=%u gems=%u banked=%u mode=%d tgt=%d prof=%llu/%llu/%llu/%llu pick=%llu step=%llu eng=%llu del=%llu\n", w.round, slot, role_name(role),
            (unsigned long long)fmin, susp, w.gems, cambc::global_submitted_gems(team::ally), static_cast<int>(mode), target,
            (unsigned long long)prof[0], (unsigned long long)prof[1], (unsigned long long)prof[2], (unsigned long long)prof[3],
            (unsigned long long)prof[4], (unsigned long long)prof[5], (unsigned long long)prof[6], (unsigned long long)prof[7]);
        for (auto& v : prof) v = 0;
    }
}

} // namespace apex
