// Static champion model derived from engine designs; shared by the draft planner and the play bot.
#pragma once
#include "common.h"

namespace apex {

struct Body {
    champion_type type{};
    Role role{};
    bool ranged{};
    float hp{}, str{}, foc{};
    float basic{};     // basic-attack damage per round incl. reset
    float spell{};     // spell damage per round (all spells on cooldown rotation)
    float poke{};      // spell damage per round from range >= 4 only (safe poke)
    float stun8{};     // stuns per 8 rounds
    float shield8{};   // shield points per 8 rounds (self)
    float splash{1};   // basic-attack multiplier against groups (Varg)
    float dps() const { return basic * splash + spell; }
};

inline Body model_body(champion_type t) {
    Body b; b.type = t; b.role = default_role(t); b.ranged = is_ranged_type(t);
    const auto& d = cambc::champion_designs[static_cast<std::size_t>(t)];
    const auto s = d.base_stats();
    b.hp = s.base_health; b.str = s.base_strength; b.foc = s.base_focus;
    bool reset = false;
    for (const auto& ab : d.abilities()) {
        if (!ab.info()) continue;
        std::visit([&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, cambc::ability_attack_reset>) reset = true;
            else if constexpr (std::is_same_v<T, cambc::ability_damage>) { b.spell += v.modifier * b.foc / 8; if (v.range >= 4) b.poke += v.modifier * b.foc / 8; }
            else if constexpr (std::is_same_v<T, cambc::ability_aoe_damage>) { b.spell += 1.3f * v.modifier * b.foc / 8; if (v.range >= 4) b.poke += 1.3f * v.modifier * b.foc / 8; }
            else if constexpr (std::is_same_v<T, cambc::ability_cc>) { b.spell += v.modifier * b.foc / 8; b.stun8 += 1; if (v.range >= 4) b.poke += v.modifier * b.foc / 8; }
            else if constexpr (std::is_same_v<T, cambc::ability_shield>) { b.shield8 += v.modifier * b.foc; }
        }, *ab.info());
    }
    b.basic = b.str * (reset ? 1.125f : 1.0f);
    if (d.passive() == cambc::passive_type::splash_attack) b.splash = 1.6f;
    return b;
}

// Expected rounds to clear a group with the given team dps, and the damage the front line absorbs.
// Monsters hit only the nearest champion, so the melee bodies rotate the front: the usable health
// pool is the sum over melee bodies of (health - 30% reserve). `pool` is that sum.
struct Fight { float rounds{}; float tank_damage{}; bool feasible{}; bool poke_only{}; };
inline Fight estimate_fight(float group_hp, float group_danger, float group_max_danger, float dps, float poke,
                            bool melee_present, float pool, float stun8, float shield8) {
    Fight f;
    if (group_danger <= 0) { f.rounds = std::ceil(group_hp / std::max(1.0f, dps)); f.feasible = true; return f; }
    if (melee_present) {
        f.rounds = std::ceil(group_hp / std::max(1.0f, dps));
        f.tank_damage = std::max(0.0f, group_danger * f.rounds - std::min(stun8, 4.0f) * f.rounds / 8 * group_max_danger - shield8 * f.rounds / 8);
        f.feasible = f.tank_damage <= pool;
        if (f.feasible) return f;
    }
    // Nobody can stand in front: spells from range 4 take no damage at all.
    if (poke > 0) { f.rounds = std::ceil(group_hp / poke); f.tank_damage = 0; f.feasible = true; f.poke_only = true; }
    else { f.rounds = 1e6f; f.feasible = false; }
    return f;
}

} // namespace apex
