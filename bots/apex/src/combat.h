// Combat / denial module: threat model, carrier safety, kill-steal, fight-or-flee, kiting.
// Called from Bot::play_turn() before the economy decisions (combat_hook) and while a tribute
// channel is running (combat_tribute_check). All logic lives in combat.cpp; Bot grants access
// through `friend struct CombatCtx`.
#pragma once
#include "bot.h"

namespace apex {

struct Threat {
    int idx{};                 // index into World::enemies
    vec2 pos{};
    float hp{};
    std::uint32_t gems{};
    float str{};               // sustained basic-attack damage per turn
    float melee_burst{};       // basic attack damage next turn (x2 when a reset is ready)
    float spell_burst{};       // ready damage spells next turn
    int reach_a{};             // tiles from which the basic attack can land next turn (move + range + dash)
    int reach_s{};             // tiles from which a spell can land next turn
    bool stun_ready{};         // a stun can be cast next turn
    bool submitting{};
    bool stunned{};
    bool ranged{};
    champion_type type{};
};

struct ThreatModel {
    std::vector<Threat> t;
    // Damage that can land on `p` next turn.
    float incoming_at(vec2 p) const;
    // Damage over the next two turns from threats that can reach `p` within two moves.
    float incoming2_at(vec2 p) const;
    // Damage over `turns` turns on a champion standing still at `p` (basics every turn, one spell
    // and one reset per enemy).
    float sustained_at(vec2 p, int turns) const;
    bool stun_reaches(vec2 p) const;
    bool any_reaches(vec2 p) const { return incoming_at(p) > 0; }
};

// Returns true when the turn is fully consumed (the economy must not act).
bool combat_hook(Bot& bot);
// While channelling a tribute: cancels and flees when the channel cannot be survived. Returns
// true when it acted.
bool combat_tribute_check(Bot& bot);

} // namespace apex
