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
    int best = -1;
    float best_hp = 1e9f;
    for (std::size_t i = 0; i < w.enemies.size(); ++i) {
        if (cheb(w.enemy_pos[i], w.pos) > bot.attack_range) continue;
        const float hp = w.enemy_hp[i];
        if (hp <= 0) continue;
        if (hp < best_hp) { best_hp = hp; best = static_cast<int>(i); }
    }
    if (best >= 0 && (!bot.ranged || best_hp <= 2.0f * w.hp)) { bot.try_attack_champion(w.enemies[best]); return false; }
    // Ranged and hurt with an enemy close: back off towards spawn/allies instead of trading.
    if (bot.ranged && w.hp < 0.6f * w.maxhp) {
        for (const auto p : w.enemy_pos) if (cheb(p, w.pos) <= 3) { if (bot.step_away_from_threats()) return true; break; }
    }
    // Melee: punish an enemy standing close to our work when healthy; one step, then attack.
    if (!bot.ranged && w.hp > 0.5f * w.maxhp && w.can_move()) {
        int near = -1; int nd = 99;
        for (std::size_t i = 0; i < w.enemies.size(); ++i) { const int d = cheb(w.enemy_pos[i], w.pos); if (d <= 3 && d < nd) { nd = d; near = static_cast<int>(i); } }
        if (near >= 0) {
            bitboard goals; goals.draw_chebyshev(w.enemy_pos[near], 1);
            if (bot.step_towards(goals, false) && cheb(w.enemy_pos[near], w.pos) <= bot.attack_range && w.enemies[near].is_visible())
                bot.try_attack_champion(w.enemies[near]);
        }
    }
    return false;
}

} // namespace apex
