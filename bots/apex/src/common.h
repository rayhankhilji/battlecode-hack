// Shared includes, tiny helpers and fuel/log discipline for the apex bot.
#pragma once
#define APEX_TRACE 1
#include <cambc.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace apex {

using cambc::bitboard;
using cambc::champion_type;
using cambc::distance_field;
using cambc::team;
using cambc::vec2;

// Fuel discipline. Measured (cambc 2.2.5, release): printf of a 40-byte line ~2,000 fuel,
// engine host calls 30-1,500 fuel, a plain loop iteration ~9 fuel, full-map distance field ~1k-20k.
constexpr std::uint64_t FUEL_RESERVE = 15000;      // keep at least this before issuing actions
constexpr std::uint64_t FUEL_OPTIONAL = 35000;     // skip optional work below this

inline std::uint64_t fuel() { return cambc::fuel_remaining(); }
inline bool fuel_ok(std::uint64_t need = FUEL_RESERVE) { return fuel() >= need; }

inline int cheb(vec2 a, vec2 b) { return static_cast<int>(a.chebyshev_distance(b)); }

// Roles used by the plan and the play state machine.
enum Role : std::uint8_t { ROLE_TANK = 0, ROLE_DPS = 1, ROLE_CASTER = 2 };

inline const char* role_name(int r) {
    switch (r) { case ROLE_TANK: return "tank"; case ROLE_DPS: return "dps"; case ROLE_CASTER: return "caster"; }
    return "?";
}

// Default role by champion design (used when no plan is available).
inline Role default_role(champion_type t) {
    switch (t) {
    case champion_type::ryth: case champion_type::varg: case champion_type::marcella: return ROLE_TANK;
    case champion_type::ren: case champion_type::issac: case champion_type::vendetta: return ROLE_DPS;
    default: return ROLE_CASTER;
    }
}

inline bool is_ranged_type(champion_type t) {
    switch (t) {
    case champion_type::vesper_vale: case champion_type::cassi: case champion_type::stellar: case champion_type::prism:
    case champion_type::willow: case champion_type::tavi: return true;
    default: return false;
    }
}

// Static per-camp design numbers (verified from the engine via sdk_camp_design on 18 Oct).
inline float camp_hp(cambc::camp_type k) {
    switch (k) {
    case cambc::camp_type::wolf: return 1350; case cambc::camp_type::elk: return 2400;
    case cambc::camp_type::gem_cluster: case cambc::camp_type::gold_cluster: return 500;
    case cambc::camp_type::monolith: return 20000;
    }
    return 1000;
}
inline float camp_dmg(cambc::camp_type k) {
    switch (k) {
    case cambc::camp_type::wolf: return 20; case cambc::camp_type::elk: return 60;
    case cambc::camp_type::monolith: return 200; default: return 0;
    }
}
constexpr int CAMP_RANGE = 3;

} // namespace apex
