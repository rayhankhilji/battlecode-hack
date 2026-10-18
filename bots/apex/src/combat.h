// Combat hook: called before economy decisions every actionable turn. Trivial implementation for
// the economy wave; the combat wave replaces the body of combat_hook().
// Contract: returns true when the turn is fully consumed (the economy must not act).
#pragma once
#include "bot.h"

namespace apex {

inline bool combat_hook(Bot& bot) {
    World& w = bot.w;
    if (w.enemies.empty()) return false;
    // Retreat under lethal threat: low health with an enemy champion that can reach us.
    if (bot.in_danger_from_champions()) {
        if (bot.step_away_from_threats()) return true;
    }
    // Opportunistic basic attack on an enemy already in range (never abandons a channel: channels
    // are handled before this hook runs). Ranged champions give up their walk by attacking.
    if (!w.can_attack()) return false;
    const cambc::champion* best = nullptr;
    float best_hp = 1e9f;
    for (const auto& e : w.enemies) {
        if (cheb(e.position(), w.pos) > bot.attack_range) continue;
        const float hp = e.health();
        if (hp <= 0) continue;
        if (hp < best_hp) { best_hp = hp; best = &e; }
    }
    if (best && (!bot.ranged || best_hp <= 2.0f * w.hp)) bot.try_attack_champion(*best);
    return false;
}

} // namespace apex
