// Sparring bot champion logic. One file shared by spar-farmer / spar-hunter /
// spar-locker; config.h selects which roles exist. main.cpp dispatches here.
#include <cambc.h>
#include "bot.h"
#include "config.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <span>
#include <variant>

using cambc::team;
using cambc::vec2;

namespace {

#ifndef SPAR_TRACE_SPLIT
#define SPAR_TRACE_SPLIT 0
#endif
constexpr std::uint64_t FUEL_BAIL = 20000;   // skip optional work below this
constexpr int BANK_AT = 40;                   // gems carried before banking
constexpr float RECALL_FRACTION = 0.35f;

int cheb(vec2 a, vec2 b) { return static_cast<int>(a.chebyshev_distance(b)); }

bool low_fuel() { return cambc::fuel_remaining() < FUEL_BAIL; }

cambc::bitboard ring(vec2 centre, int radius) {
    cambc::bitboard b;
    b.clear();
    b.draw_chebyshev(centre, static_cast<float>(radius));
    return b;
}

vec2 centre_of(const cambc::bitboard& b) {
    long sx = 0, sy = 0, n = 0;
    for (auto p : b.positions()) { sx += p.x; sy += p.y; ++n; }
    if (n == 0) return {0, 0};
    return {static_cast<int>(sx / n), static_cast<int>(sy / n)};
}

bool is_ranged_type(cambc::champion_type t) {
    using T = cambc::champion_type;
    return t == T::cassi || t == T::stellar || t == T::prism || t == T::willow || t == T::tavi || t == T::vesper_vale;
}

bool ability_is_exclusive(const cambc::ability& a) {
    const auto& info = a.info();
    if (!info) return false;
    return std::visit([](const auto& v) { return v.exclusive_with_movement; }, *info);
}

float ability_range(const cambc::ability& a) {
    const auto& info = a.info();
    if (!info) return 0;
    return std::visit([](const auto& v) -> float {
        if constexpr (requires { v.range; }) return v.range;
        else return 0;
    }, *info);
}

// Health of a unit that may have died / left vision since the turn started; -1 when unreadable.
float safe_hp(const cambc::unit& u) {
    try { return u.is_visible() ? u.health() : -1.0f; } catch (...) { return -1.0f; }
}

// Position of a unit that may have vanished; (-99,-99) when unreadable.
vec2 safe_pos(const cambc::unit& u) {
    try { return u.position(); } catch (...) { return {-99, -99}; }
}

}  // namespace

// ---------------------------------------------------------------- setup

void Bot::init() {
    inited = true;
    auto& me = cambc::self();
    kind = me.champion_name();
    ranged = is_ranged_type(kind);
    const auto roster = cambc::champions(team::ally);
    for (std::size_t i = 0; i < roster.size(); ++i)
        if (roster[i] == me) slot = static_cast<int>(i);

    terrain = cambc::map::walls() | cambc::map::holes() | cambc::map::altars() | cambc::map::spawn_region(team::enemy);
    spawn_ally = cambc::map::spawn_region(team::ally);
    altars = cambc::map::altars().positions();
    enemy_spawn_centre = centre_of(cambc::map::spawn_region(team::enemy));
    ally_spawn_centre = centre_of(spawn_ally);

    cambc::camp_stats kind_stats[8]{};
    bool kind_known[8]{};
    for (const auto& g : cambc::map::camp_groups()) {
        Group info{g, g.spawn_tiles()};
        info.tiles = info.spawns.positions();
        info.centre = centre_of(info.spawns);
        info.gems = static_cast<int>(g.gem_value());
        info.gold = static_cast<int>(g.gold_value());
        for (const auto& c : g.camps()) {
            const int k = static_cast<int>(c.kind());
            if (!kind_known[k]) { kind_stats[k] = c.base_stats(); kind_known[k] = true; }
            info.hp += kind_stats[k].base_health;
            info.dps += kind_stats[k].base_strength;
            info.size++;
            if (c.is_monolith()) info.monolith = true;
        }
        groups.push_back(std::move(info));
    }

    // Role assignment.
    role = Role::farm;
#if SPAR_HUNTER
    if (kind == cambc::champion_type::ren || kind == cambc::champion_type::ryth) role = Role::hunt;
#endif
#if SPAR_LOCKER
    {
        // One locker: Varg if drafted, otherwise Ren, otherwise the last body.
        int locker_slot = -1;
        for (std::size_t i = 0; i < roster.size(); ++i)
            if (roster[i].champion_name() == cambc::champion_type::varg) locker_slot = static_cast<int>(i);
        if (locker_slot < 0)
            for (std::size_t i = 0; i < roster.size(); ++i)
                if (roster[i].champion_name() == cambc::champion_type::ren) locker_slot = static_cast<int>(i);
        if (locker_slot < 0) locker_slot = static_cast<int>(roster.size()) - 1;
        if (slot == locker_slot) role = Role::lock;
    }
#endif

    // Hunter patrol: the two altars closest to the enemy spawn.
    {
        std::vector<vec2> sorted = altars;
        std::sort(sorted.begin(), sorted.end(), [&](vec2 a, vec2 b) {
            return cheb(a, enemy_spawn_centre) < cheb(b, enemy_spawn_centre);
        });
        for (std::size_t i = 0; i < sorted.size() && i < 2; ++i) patrol.push_back(sorted[i]);
        if (!sorted.empty()) lock_altar = sorted[0];
        if (!patrol.empty()) patrol_idx = slot % static_cast<int>(patrol.size());
    }

    std::printf("init slot=%d champ=%d role=%d groups=%d altars=%d fuel=%llu\n", slot, static_cast<int>(kind),
                static_cast<int>(role), static_cast<int>(groups.size()), static_cast<int>(altars.size()),
                static_cast<unsigned long long>(cambc::fuel_remaining()));
}

void Bot::telemetry(std::uint64_t fuel_min) {
    unsigned gems = 0;
    vec2 p{-1, -1};
    try {
        auto& me = cambc::self();
        if (me.is_alive()) { gems = me.gem_count(); p = me.position(); }
    } catch (...) {}
    std::printf("T r=%u slot=%d fmin=%llu gems=%u banked=%u pos=%d,%d tgt=%d role=%d hp=%.0f ph=%llu/%llu/%llu/%llu\n",
                static_cast<unsigned>(cambc::round_number()), slot, static_cast<unsigned long long>(fuel_min), gems,
                static_cast<unsigned>(cambc::global_submitted_gems(team::ally)), p.x, p.y, target, static_cast<int>(role), hp,
                static_cast<unsigned long long>(probe_max[0]), static_cast<unsigned long long>(probe_max[1]),
                static_cast<unsigned long long>(probe_max[2]), static_cast<unsigned long long>(probe_max[3]));
    for (auto& v : probe_max) v = 0;
}

void Bot::write_claim(int group_index) {
    if (slot < 0 || slot >= 8) return;
    const std::uint8_t v = static_cast<std::uint8_t>(group_index + 1);
    cambc::shared_memory.write(static_cast<std::size_t>(slot), std::span<const std::uint8_t>{&v, 1});
}

int Bot::read_claim(int other_slot) const {
    if (other_slot < 0 || other_slot >= 8) return -1;
    std::uint8_t v = 0;
    cambc::shared_memory.read(static_cast<std::size_t>(other_slot), std::span<std::uint8_t>{&v, 1});
    return static_cast<int>(v) - 1;
}

void Bot::shop() {
    auto& me = cambc::self();
    const bool focus_user = kind == cambc::champion_type::stellar || kind == cambc::champion_type::cassi;
    const auto item = focus_user ? cambc::item_type::monster_shard_of_focus : cambc::item_type::monster_shard_of_strength;
#if SPAR_LOCKER
    if (role == Role::lock) {
        if (me.consumables()[cambc::consumable_type::altar_lock].count() == 0 && me.can_buy(cambc::consumable_type::altar_lock))
            me.buy(cambc::consumable_type::altar_lock);
        // Keep 10 gold in reserve for the next lock.
        const unsigned gold = me.gold_count();
        const unsigned reserve = me.consumables()[cambc::consumable_type::altar_lock].count() ? 10 : 20;
        const unsigned count = gold > reserve ? (gold - reserve) / item.cost() : 0;
        if (count && me.can_buy(item, count)) me.buy(item, count);
        return;
    }
#endif
    const unsigned count = me.gold_count() / item.cost();
    if (count && me.can_buy(item, count)) me.buy(item, count);
}

void Bot::begin_turn() {
    auto& me = cambc::self();
    round = cambc::round_number();
    pos = me.position();
    hp = me.health();
    max_hp = me.max_health();
    at_spawn = spawn_ally.is_set(pos);
    cast_done = false;
    visible = cambc::vision::visible_tiles();
    blocked = terrain | cambc::vision::champion_tiles() | cambc::vision::camp_tiles() | cambc::vision::teleport_tiles();
    blocked.set(pos, false);
    enemies = cambc::vision::champions(team::enemy);
    allies = cambc::vision::champions(team::ally);
    camps = cambc::vision::camps();
    if (pos == last_pos) ++stuck; else stuck = 0;
    last_pos = pos;
}

// ---------------------------------------------------------------- movement

// Step one tile along a shortest path to the nearest free goal tile.
bool Bot::step_towards(const cambc::bitboard& goals_in) {
    auto& me = cambc::self();
    if (!me.can_move() || goals_in.is_set(pos)) return false;
    auto goals = goals_in & ~blocked;
    if (goals.is_empty()) return false;

    cambc::distance_field field;
    cambc::bitboard visited, origin;
    origin.clear();
    origin.set(pos);
    field.compute_to(blocked, goals, origin, visited);

    const auto here = field.get(pos);
    std::array<std::pair<std::uint16_t, vec2>, 8> cand{};
    int n = 0;
    for (auto tile : blocked.reachable_tiles(pos, 1).positions()) {
        if (tile == pos) continue;
        const auto d = field.get(tile);
        if (here == cambc::distance_field::unreachable ? d != cambc::distance_field::unreachable : d < here)
            cand[n++] = {d, tile};
    }
    if (n == 0) {
        // No path through visible blockers: fall back to the engine pathfinder, then wiggle if stuck.
        const auto gp = goals.positions();
        vec2 best = gp[0];
        for (auto g : gp) if (cheb(g, pos) < cheb(best, pos)) best = g;
        if (me.move_towards(best)) { pos = me.position(); return true; }
        if (stuck >= 6) {
            for (auto tile : blocked.reachable_tiles(pos, 1).positions())
                if (tile != pos && (tile.x + tile.y + static_cast<int>(round)) % 3 == 0 && me.move_absolute(tile)) { pos = tile; return true; }
        }
        return false;
    }
    std::sort(cand.begin(), cand.begin() + n, [](const auto& a, const auto& b) { return a.first < b.first; });
#ifdef SPAR_TRACE
    if (slot == 0 && round >= 100 && round < 106) {
        std::printf("trace r=%u pos=%d,%d here=%u n=%d:", (unsigned)round, pos.x, pos.y, (unsigned)here, n);
        for (int i = 0; i < n; ++i) std::printf(" (%d,%d)=%u", cand[i].second.x, cand[i].second.y, (unsigned)cand[i].first);
        std::printf(" goals=%u tgt=%d\n", goals.count(), target);
    }
#endif
    for (int i = 0; i < n && i < 3; ++i) {
        if (me.move_absolute(cand[i].second)) { pos = cand[i].second; return true; }  // false = hidden blocker; try the next
#ifdef SPAR_TRACE
        std::printf("movefalse r=%u slot=%d to=%d,%d canmove_after=%d\n", (unsigned)round, slot, cand[i].second.x, cand[i].second.y, (int)me.can_move());
#endif
    }
    return false;
}

bool Bot::step_away_from(const std::vector<vec2>& threats) {
    auto& me = cambc::self();
    if (!me.can_move() || threats.empty()) return false;
    auto score = [&](vec2 t) {
        int m = 99;
        for (auto th : threats) m = std::min(m, cheb(t, th));
        return m * 4 - cheb(t, ally_spawn_centre) / 8;
    };
    vec2 best = pos;
    int best_score = score(pos);
    for (auto tile : blocked.reachable_tiles(pos, 1).positions()) {
        if (tile == pos) continue;
        const int s = score(tile);
        if (s > best_score) { best_score = s; best = tile; }
    }
    if (best == pos || !me.move_absolute(best)) return false;
    pos = best;
    return true;
}

bool Bot::enemy_within(int range) const {
    for (const auto& e : enemies) if (cheb(e.position(), pos) <= range) return true;
    return false;
}

bool Bot::awake_camp_within(int range) const {
    for (const auto& c : camps)
        if (cheb(c.position(), pos) <= range && c.base_stats().base_strength > 0 && c.is_awake()) return true;
    return false;
}

// Recall when hurt, unless something would cancel it. Returns true if the turn is spent.
bool Bot::want_recall(float fraction) {
    auto& me = cambc::self();
    if (at_spawn || hp >= max_hp * fraction) return false;
    if (enemy_within(1)) return false;  // fight it out: recall would be cancelled anyway
    std::vector<vec2> threats;
    for (const auto& c : camps)
        if (c.base_stats().base_strength > 0 && cheb(c.position(), pos) <= 4 && c.is_awake()) threats.push_back(c.position());
    for (const auto& e : enemies) if (cheb(e.position(), pos) <= 5) threats.push_back(e.position());
    if (!threats.empty()) {
        step_away_from(threats);
        return true;
    }
    me.recall();
    return true;
}

// ---------------------------------------------------------------- combat

bool Bot::attack_camp_in_range(const std::vector<const cambc::camp*>& living) {
    auto& me = cambc::self();
    if (!me.can_attack()) return false;
    const int range = me.attack_range();
    const cambc::camp* best = nullptr;
    float best_hp = 1e9f;
    for (auto c : living) {
        if (cheb(safe_pos(*c), pos) > range) continue;
        const float h = safe_hp(*c);
        if (h > 0 && h < best_hp) { best_hp = h; best = c; }
    }
    if (!best) return false;
    me.attack(*best);
    return true;
}

// Use damage / cc / aoe abilities on camps. Only one successful cast per turn.
void Bot::cast_on_camps(const std::vector<const cambc::camp*>& living, bool allow_exclusive) {
    if (cast_done || living.empty()) return;
    auto& me = cambc::self();
    if (!me.can_cast()) return;
    for (const auto& ab : me.abilities()) {
        if (cast_done) return;
        if (!ab.info() || ab.is_attack_reset() || ab.is_dash() || ab.is_heal() || ab.is_shield()) continue;
        if (ab.is_on_cooldown()) continue;
        if (!allow_exclusive && ability_is_exclusive(ab)) continue;
        const int range = static_cast<int>(ability_range(ab));
        if (range <= 0) continue;
        const auto targets = ab.valid_targets();
        const bool camps_ok = targets.contains(cambc::ability_target_mask::camps);
        const bool ground_ok = targets.contains(cambc::ability_target_mask::ground);
        if (ab.is_aoe_damage() || (ab.is_cc() && ground_ok && !camps_ok)) {
            // Ground cast: tile maximising living members in radius 1.
            const cambc::camp* best = nullptr;
            int best_n = 0;
            for (auto c : living) {
                const auto cp = safe_pos(*c);
                if (cheb(cp, pos) > range || safe_hp(*c) <= 0) continue;
                int n = 0;
                for (auto o : living) if (cheb(safe_pos(*o), cp) <= 1 && safe_hp(*o) > 0) ++n;
                if (n > best_n) { best_n = n; best = c; }
            }
            if (best && ground_ok && ab.use(safe_pos(*best))) cast_done = true;
        } else if (camps_ok) {
            const cambc::camp* best = nullptr;
            float best_hp = 1e9f;
            for (auto c : living) {
                if (cheb(safe_pos(*c), pos) > range) continue;
                const float h = safe_hp(*c);
                if (h > 0 && h < best_hp) { best_hp = h; best = c; }
            }
            if (best && ab.use(*best)) cast_done = true;
        }
    }
}

bool Bot::attack_enemy_in_range(const cambc::champion* preferred) {
    auto& me = cambc::self();
    if (!me.can_attack()) return false;
    const int range = me.attack_range();
    if (preferred && cheb(safe_pos(*preferred), pos) <= range && safe_hp(*preferred) > 0) {
        me.attack(*preferred);
        return true;
    }
    const cambc::champion* best = nullptr;
    float best_hp = 1e9f;
    for (const auto& e : enemies) {
        if (cheb(safe_pos(e), pos) > range) continue;
        const float h = safe_hp(e);
        if (h <= 0) continue;
        if (h < best_hp) { best_hp = h; best = &e; }
    }
    if (!best) return false;
    me.attack(*best);
    return true;
}

void Bot::cast_on_enemies(const cambc::champion* preferred, bool allow_exclusive, bool prefer_stun) {
    if (cast_done || enemies.empty()) return;
    auto& me = cambc::self();
    if (!me.can_cast()) return;
    auto abilities = me.abilities();
    // Two passes: stuns first when asked, then anything damaging.
    for (int pass = 0; pass < 2 && !cast_done; ++pass) {
        for (const auto& ab : abilities) {
            if (cast_done) return;
            if (!ab.info() || ab.is_attack_reset() || ab.is_dash() || ab.is_heal() || ab.is_shield()) continue;
            if (pass == 0 && !(prefer_stun && ab.is_cc())) continue;
            if (ab.is_on_cooldown()) continue;
            if (!allow_exclusive && ability_is_exclusive(ab)) continue;
            const int range = static_cast<int>(ability_range(ab));
            if (range <= 0) continue;
            const auto targets = ab.valid_targets();
            const bool enemies_ok = targets.contains(cambc::ability_target_mask::enemies);
            const bool ground_ok = targets.contains(cambc::ability_target_mask::ground);
            const cambc::champion* t = nullptr;
            if (preferred && cheb(safe_pos(*preferred), pos) <= range && safe_hp(*preferred) > 0) t = preferred;
            if (!t) {
                float best_hp = 1e9f;
                for (const auto& e : enemies) {
                    const float h = safe_hp(e);
                    if (h > 0 && cheb(safe_pos(e), pos) <= range && h < best_hp) { best_hp = h; t = &e; }
                }
            }
            if (!t) continue;
            bool ok = false;
            if (enemies_ok) ok = ab.use(*t);
            else if (ground_ok) ok = ab.use(safe_pos(*t));
            if (ok) cast_done = true;
        }
    }
}

void Bot::try_shield() {
    if (cast_done) return;
    auto& me = cambc::self();
    if (!me.can_cast()) return;
    for (const auto& ab : me.abilities()) {
        if (ab.is_shield() && !ab.is_on_cooldown() && ab.use()) { cast_done = true; return; }
    }
}

// attack -> reset -> attack, when a reset is ready and the target is in range.
void Bot::try_reset_and_attack(const cambc::unit* unit_target) {
    if (cast_done || !unit_target) return;
    auto& me = cambc::self();
    if (me.can_attack() || !me.can_cast()) return;  // only useful once the basic attack is spent
    if (safe_hp(*unit_target) <= 0 || cheb(safe_pos(*unit_target), pos) > me.attack_range()) return;
    for (const auto& ab : me.abilities()) {
        if (!ab.is_attack_reset() || ab.is_on_cooldown()) continue;
        if (!ab.use()) return;
        cast_done = true;
        if (me.can_attack() && safe_hp(*unit_target) > 0) me.attack(*unit_target);
        return;
    }
}

// ---------------------------------------------------------------- banking

// Walk to the nearest altar without an enemy lock and tribute. Returns true if the turn is spent.
bool Bot::bank_gems(bool force) {
    auto& me = cambc::self();
    const unsigned gems = me.gem_count();
    if (gems == 0) return false;
    if (!force && gems < BANK_AT) return false;

    // Choose the nearest altar that is not enemy-locked.
    vec2 best{-1, -1};
    int best_d = 1 << 30;
    for (auto a : altars) {
        if (cambc::vision::has_altar_lock(a) && cambc::vision::altar_lock_at(a).is_enemy()) continue;
        const int d = cheb(a, pos);
        if (d < best_d) { best_d = d; best = a; }
    }
    if (best.x < 0) return false;

    if (best_d <= cambc::constants::submit_gems_range) {
        // Free ward next to the altar first (ward is a separate action).
        try {
            if (me.ward_cooldown() <= 0) {
                bool have_near = false;
                for (auto w : me.ward_positions()) if (cheb(w, best) <= 3) have_near = true;
                if (!have_near) me.place_ward(pos);
            }
        } catch (...) {}
        me.submit_gems(best);
        return true;
    }
    step_towards(ring(best, cambc::constants::submit_gems_range));
    return true;
}

// ---------------------------------------------------------------- farming

float Bot::my_dpr() const {
    auto& me = cambc::self();
    const float str = me.strength();
    float dpr = str;
    for (const auto& ab : me.abilities()) {
        if (!ab.info()) continue;
        if (ab.is_attack_reset()) dpr += str / 8.0f;
        else if (ab.is_damage() || ab.is_aoe_damage() || ab.is_cc()) {
            try { dpr += ab.damage_amount() / 8.0f; } catch (...) {}
        }
    }
    return dpr < 1 ? 1 : dpr;
}

bool Bot::target_viable(const Group& g, bool relaxed, float dpr) const {
    if (g.monolith) return false;
    if (g.blocked_until > round) return false;
    if (g.gems == 0 && !relaxed) return false;
    // Rough damage race: rounds to clear vs damage taken while the members are awake.
    const float rounds = g.hp / dpr;
    const float damage = g.dps * rounds * 0.6f;
    const float budget = relaxed ? max_hp * 1.2f : hp * 0.8f;
    return damage <= budget;
}

bool Bot::choose_target() {
    // Pass 0: a gem group this champion can clear alone, not claimed by an ally.
    // Pass 1: join a group an ally has claimed (teamwork on elk/wolf groups).
    // Pass 2: rally: any non-monolith group, nearest to our spawn and to us.
    int best = -1;
    float best_score = 1e9f;
    // Claims of the other slots, read once.
    int claim_of[8];
    for (int s = 0; s < 8; ++s) claim_of[s] = s == slot ? -1 : read_claim(s);
    const float dpr = my_dpr();
    const bool need_gold = role == Role::lock && cambc::self().gold_count() < cambc::consumable_type::altar_lock.cost();
    // True walking distances over terrain (camps and units ignored) from our tile.
    cambc::distance_field dist;
    {
        cambc::bitboard visited, origin;
        origin.clear();
        origin.set(pos);
        dist.compute(terrain, origin, visited);
    }
    std::vector<vec2> enemy_pos;
    for (const auto& e : enemies) enemy_pos.push_back(e.position());
    for (int pass = 0; pass < 3 && best < 0; ++pass) {
        for (std::size_t i = 0; i < groups.size(); ++i) {
            if (low_fuel()) break;
            auto& g = groups[i];
            if (g.monolith || g.blocked_until > round) continue;
            if (need_gold && g.gold == 0) continue;
            std::uint16_t d = cambc::distance_field::unreachable;
            for (auto t : g.tiles) d = std::min(d, dist.get(t));
            if (d == cambc::distance_field::unreachable) {
                g.blocked_until = round + 300;  // sealed off (holes/walls); try again much later
                continue;
            }
            int claims = 0;
            for (int s = 0; s < 8; ++s) if (claim_of[s] == static_cast<int>(i)) ++claims;
            bool enemy_there = false;
            for (auto ep : enemy_pos) if (cheb(ep, g.centre) <= 2) enemy_there = true;
            if (enemy_there && pass < 2) continue;
            float score = static_cast<float>(d);
            if (pass == 0) {
                if (g.gems == 0 || claims > 0 || !target_viable(g, false, dpr)) continue;
                bool ally_there = false;
                for (const auto& a : allies)
                    if (!(a == cambc::self()) && cheb(a.position(), g.centre) <= 2) ally_there = true;
                if (ally_there) continue;
                score += g.dps * 0.1f - std::min(g.gems, 60) * 0.1f;
                // Spread the opening picks: each slot slightly prefers a different quadrant.
                if (round < 5 && slot >= 0) {
                    const bool east = g.centre.x >= cambc::map::width() / 2, north = g.centre.y >= cambc::map::height() / 2;
                    if ((east ? 1 : 0) + (north ? 2 : 0) != slot % 4) score += 3;
                }
            } else if (pass == 1) {
                if (claims == 0) continue;
                score -= claims * 4;
                if (g.gems == 0) score += 40;
            } else {
                // Rally: everyone scores from the spawn so the first picks agree.
                score += static_cast<float>(cheb(g.centre, ally_spawn_centre));
                if (g.gems == 0) score += 40;
                score += g.dps * 0.05f - std::min(g.gems, 100) * 0.3f;
            }
            if (score < best_score) { best_score = score; best = static_cast<int>(i); }
        }
    }
    target = best;
    target_since = round;
    write_claim(best);
    return best >= 0;
}

void Bot::act_on_target() {
    auto& me = cambc::self();
    auto& g = groups[target];
    const bool seen = !(g.spawns & visible).is_empty();
    if (!seen) {
        // Approach: path to the spawn tiles themselves (camps there are not yet visible).
        if (!step_towards(ring(g.centre, 1) | g.spawns)) {
            if (stuck > 10) { g.blocked_until = round + 30; target = -1; }
        }
        return;
    }
    const auto state = g.group.state();
    if (std::holds_alternative<cambc::map::camp_group_respawning>(state)) {
#ifdef SPAR_TRACE
        std::printf("cap r=%u slot=%d group=%d respawning timer=%d gems=%u on_tile=%d\n", (unsigned)round, slot, target,
                    std::get<cambc::map::camp_group_respawning>(state).timer, me.gem_count(), (int)g.spawns.is_set(pos));
#endif
        g.blocked_until = round + std::get<cambc::map::camp_group_respawning>(state).timer + 1;
        target = -1;
        write_claim(-1);
        return;
    }
    if (std::holds_alternative<cambc::map::camp_group_reward_available>(state)) {
        // Capture: stand on a spawn tile until the reward arrives (state flips to respawning).
#ifdef SPAR_TRACE
        std::printf("cap r=%u slot=%d group=%d reward_available gems=%u on_tile=%d\n", (unsigned)round, slot, target, me.gem_count(), (int)g.spawns.is_set(pos));
#endif
        if (g.spawns.is_set(pos)) return;
        // An ally already standing on the region captures it; splitting only delays banking.
        for (const auto& a : allies)
            if (!SPAR_TRACE_SPLIT && !(a == cambc::self()) && g.spawns.is_set(a.position())) {
                g.blocked_until = round + cambc::constants::capture_time + 3;
                target = -1;
                write_claim(-1);
                return;
            }
        if (!step_towards(g.spawns) && stuck > 8) { g.blocked_until = round + 20; target = -1; write_claim(-1); }
        return;
    }
    // Alive: fight the living members.
    std::vector<const cambc::camp*> living;
    for (const auto& c : camps) if (g.spawns.is_set(c.position()) && c.health() > 0) living.push_back(&c);
    if (living.empty()) {
        // Members alive but not visible (vision gap): walk closer.
        step_towards(g.spawns);
        return;
    }
    const int arange = me.attack_range();
    bool in_attack_range = false;
    int nearest = 99;
    for (auto c : living) { const int d = cheb(c->position(), pos); nearest = std::min(nearest, d); if (d <= arange) in_attack_range = true; }

    // Shield up when the group can hurt us.
    if (g.dps > 0 && nearest <= 3 && hp < max_hp * 0.7f) try_shield();

    // Ranged champions only close to basic-attack range of a biting group once a melee ally is
    // engaged (monsters hit the nearest champion). Otherwise hold at spell range 4, outside range 3.
    if (ranged && g.dps > 0) {
        bool melee_engaged = false;
        for (const auto& a : allies) {
            if (a == cambc::self()) continue;
            for (auto c : living) if (cheb(a.position(), c->position()) <= 1) melee_engaged = true;
        }
        if (!melee_engaged) {
            if (nearest <= 3) {
                std::vector<vec2> threats;
                for (auto c : living) threats.push_back(c->position());
                if (step_away_from(threats)) return;
            }
            if (nearest <= 4) { cast_on_camps(living, true); return; }
            cambc::bitboard goals;
            goals.clear();
            for (auto c : living) goals |= ring(c->position(), 4);
            for (auto c : living) goals &= ~ring(c->position(), 3);
            step_towards(goals);
            return;
        }
    }

    if (in_attack_range) {
        attack_camp_in_range(living);
        cast_on_camps(living, true);
        // Reset + second hit on the lowest member in range.
        const cambc::camp* lowest = nullptr;
        float lowest_hp = 1e9f;
        for (auto c : living) {
            const float h = safe_hp(*c);
            if (h > 0 && cheb(safe_pos(*c), pos) <= arange && h < lowest_hp) { lowest_hp = h; lowest = c; }
        }
        try_reset_and_attack(lowest);
        return;
    }
    // Not in basic range: a ready ranged spell is worth standing still for.
    if (nearest <= 4) {
        cast_on_camps(living, true);
        if (cast_done) return;
    }
    // Approach the living members.
    cambc::bitboard goals;
    goals.clear();
    for (auto c : living) goals |= ring(c->position(), arange);
    if (!step_towards(goals) && stuck > 10) { g.blocked_until = round + 30; target = -1; write_claim(-1); return; }
    // Melee can still swing after walking.
    if (!ranged) {
        attack_camp_in_range(living);
        cast_on_camps(living, false);
    }
}

void Bot::do_farm() {
    auto& me = cambc::self();
    if (want_recall(ranged ? RECALL_FRACTION + 0.1f : RECALL_FRACTION)) return;

    // Enemy in reach: trade hits (melee keeps farming afterwards; ranged spends the attack).
    const cambc::champion* adjacent = nullptr;
    for (const auto& e : enemies) if (cheb(e.position(), pos) <= me.attack_range() && e.health() > 0) adjacent = &e;
    if (adjacent) {
        attack_enemy_in_range(adjacent);
        cast_on_enemies(adjacent, true, true);
        try_reset_and_attack(adjacent);
        if (ranged) return;
    }

    // Bank when the bag is big enough, or when nothing is farmable nearby.
    probe_start();
    const bool banking = bank_gems(false);
    probe(3);
    if (banking) return;

    if (target >= 0) {
        if (groups[target].blocked_until > round || round - target_since > 200) { target = -1; write_claim(-1); }
    }
    probe_start();
    const bool chosen = target >= 0 || choose_target();
    probe(1);
    if (!chosen) {
        if (bank_gems(true)) return;
        // Nothing to do: drift towards the map centre so we see more.
        step_towards(ring({cambc::map::width() / 2, cambc::map::height() / 2}, 2));
        return;
    }
    probe_start();
    if (target >= 0) act_on_target();
    probe(2);
    // Target just invalidated: re-plan only if the fuel budget still comfortably allows it.
    if (target < 0 && cambc::fuel_remaining() > 45000 && me.gem_count() > 0 && !choose_target()) bank_gems(true);
}

// ---------------------------------------------------------------- hunter

void Bot::do_hunt() {
    auto& me = cambc::self();
    // Retreat when hurt.
    if (hp < max_hp * 0.40f && !at_spawn) {
        if (enemy_within(4)) { step_away_from([&] { std::vector<vec2> t; for (auto& e : enemies) t.push_back(e.position()); return t; }()); return; }
        me.recall();
        return;
    }
    // Pick a victim: submitting enemies first, then the fattest bag, then anyone in range.
    const cambc::champion* victim = nullptr;
    int victim_score = -1;
    for (const auto& e : enemies) {
        if (e.health() <= 0) continue;
        int score = 0;
        try {
            if (e.is_submitting_gems()) score += 1000;
            score += static_cast<int>(e.gem_count()) * 5;
        } catch (...) {}
        score -= cheb(e.position(), pos);
        if (cheb(e.position(), pos) <= me.attack_range() + 1) score += 20;
        if (score > victim_score) { victim_score = score; victim = &e; }
    }
    if (victim) {
        chase_pos = victim->position();
        chase_seen = round;
        chasing = true;
        const bool in_range = cheb(victim->position(), pos) <= me.attack_range();
        bool submitting = false;
        try { submitting = victim->is_submitting_gems(); } catch (...) {}
        if (in_range) {
            attack_enemy_in_range(victim);
            cast_on_enemies(victim, true, true);
            try_reset_and_attack(victim);
            return;
        }
        // Stun a channelling enemy from spell range, even if that costs the walk.
        if (submitting) {
            cast_on_enemies(victim, true, true);
            if (cast_done) return;
        }
        step_towards(ring(victim->position(), me.attack_range()));
        if (!ranged) { attack_enemy_in_range(victim); cast_on_enemies(victim, false, true); }
        return;
    }
    // Lost sight: chase for at most 4 turns beyond last sighting.
    if (chasing) {
        if (round - chase_seen <= 4) { step_towards(ring(chase_pos, 1)); return; }
        chasing = false;
    }
    // Bank any stolen gems before resuming patrol.
    if (me.gem_count() >= 20 && bank_gems(true)) return;
    // Patrol between the altars closest to the enemy spawn.
    if (patrol.empty()) { do_farm(); return; }
    const vec2 goal = patrol[patrol_idx];
    if (cheb(goal, pos) <= 1) { patrol_idx = (patrol_idx + 1) % static_cast<int>(patrol.size()); }
    // Ward near the patrol altar once.
    try {
        if (me.ward_cooldown() <= 0 && cheb(goal, pos) <= 2) {
            bool have_near = false;
            for (auto w : me.ward_positions()) if (cheb(w, goal) <= 3) have_near = true;
            if (!have_near) me.place_ward(pos);
        }
    } catch (...) {}
    step_towards(ring(patrol[patrol_idx], 1));
}

// ---------------------------------------------------------------- locker

void Bot::do_lock() {
    auto& me = cambc::self();
    if (lock_altar.x < 0) { do_farm(); return; }
    const bool have_lock = me.consumables()[cambc::consumable_type::altar_lock].count() > 0;
    bool ours_placed = false, enemy_lock = false;
    if (cambc::vision::has_altar_lock(lock_altar)) {
        const auto lock = cambc::vision::altar_lock_at(lock_altar);
        ours_placed = lock.is_ally();
        enemy_lock = lock.is_enemy();
    }
    if (want_recall(RECALL_FRACTION)) return;

    // Enemies near the altar (or us) get hit; prefer the ones adjacent to the altar (hitting the lock).
    const cambc::champion* victim = nullptr;
    int victim_score = -1;
    for (const auto& e : enemies) {
        if (e.health() <= 0) continue;
        int score = 100 - cheb(e.position(), pos);
        if (cheb(e.position(), lock_altar) <= 1) score += 50;
        if (score > victim_score) { victim_score = score; victim = &e; }
    }
    if (victim && cheb(victim->position(), lock_altar) <= 5) {
        if (cheb(victim->position(), pos) <= me.attack_range()) {
            attack_enemy_in_range(victim);
            cast_on_enemies(victim, true, true);
            try_reset_and_attack(victim);
            return;
        }
        if (cheb(victim->position(), pos) <= 4) { cast_on_enemies(victim, true, true); if (cast_done) return; }
        step_towards(ring(victim->position(), me.attack_range()));
        if (!ranged) { attack_enemy_in_range(victim); cast_on_enemies(victim, false, true); }
        return;
    }

    // Need a lock and have none: earn the 10 gold by farming, then go home to buy one.
    if (!have_lock && !ours_placed) {
        if (me.gold_count() < cambc::consumable_type::altar_lock.cost()) { do_farm(); return; }
        if (at_spawn) return;  // shop() already ran this turn; the lock arrives next turn
        if (cheb(pos, ally_spawn_centre) > 8 && !enemy_within(5) && !awake_camp_within(4)) { me.recall(); return; }
        step_towards(spawn_ally);
        return;
    }
    // Walk into placement range of the altar.
    if (cheb(pos, lock_altar) > 2) {
        step_towards(ring(lock_altar, 2));
        if (cheb(pos, lock_altar) > 2 || !have_lock) return;
    }
    if (enemy_lock) {
        // Break the enemy lock (3 basic attacks), then place ours.
        if (cheb(pos, lock_altar) <= me.attack_range() && me.can_attack()) {
            me.attack(cambc::vision::altar_lock_at(lock_altar));
            try_reset_and_attack(nullptr);
        } else {
            step_towards(ring(lock_altar, me.attack_range()));
        }
        return;
    }
    if (have_lock && !ours_placed) {
        if (me.consumables()[cambc::consumable_type::altar_lock].use(lock_altar)) {
            std::printf("lock placed r=%u at (%d,%d)\n", static_cast<unsigned>(round), lock_altar.x, lock_altar.y);
        }
        return;
    }
    // Guard: keep the ward on the altar, stay within 2 tiles, hit whatever camps are adjacent for value.
    try {
        if (me.ward_cooldown() <= 0) {
            bool have_near = false;
            for (auto w : me.ward_positions()) if (cheb(w, lock_altar) <= 3) have_near = true;
            if (!have_near) me.place_ward(pos);
        }
    } catch (...) {}
    // Bank anything we picked up while standing here.
    if (me.gem_count() > 0 && cheb(pos, lock_altar) <= 1) { me.submit_gems(lock_altar); return; }
    if (cheb(pos, lock_altar) > 1) step_towards(ring(lock_altar, 1));
}

// ---------------------------------------------------------------- turn

void Bot::play_turn() {
    if (!inited) {
        init();
        // The first turn's doubled budget mostly went into init; do not stack a full turn on it.
        if (cambc::fuel_remaining() < 60000) return;
    }
    auto& me = cambc::self();
    round = cambc::round_number();
    if (!me.is_alive()) {
        shop();
        target = -1;
        write_claim(-1);
        chasing = false;
        return;
    }
    // Channels: never move while recalling / tributing / teleporting.
    if (me.is_recalling() || me.is_submitting_gems() || me.is_teleporting()) return;
    probe_start();
    begin_turn();
    probe(0);
    if (at_spawn) {
        shop();
        // Stay to heal to full on the turn we arrive (spawn heals at turn end).
        if (hp < max_hp - 1.0f) return;
    }
    if (!me.can_move() && !me.can_attack() && !me.can_cast()) return;  // stunned

    switch (role) {
        case Role::farm: do_farm(); break;
        case Role::hunt: do_hunt(); break;
        case Role::lock: do_lock(); break;
    }
}
