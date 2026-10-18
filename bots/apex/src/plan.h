// Draft-memory plan layout (<= 1024 bytes) shared between the draft bot and champion bots.
// Group identity = index into cambc::map::camp_groups() (verified equal to the engine handle and
// identical in draft and play on every map in maps/).
#pragma once
#include "common.h"

namespace apex {

constexpr std::uint8_t PLAN_VERSION = 2;
constexpr int MAX_BODIES = 6;
constexpr int MAX_GROUPS = 64;
constexpr int MAX_CIRCUIT = 24;
constexpr std::uint8_t NO_GROUP = 63;

enum GroupFlag : std::uint8_t { GF_EXCLUDED = 1, GF_HOME = 2 };

#pragma pack(push, 1)
struct GroupPlan {
    std::uint8_t altar;        // nearest altar index (by path)
    std::uint8_t dist_spawn;   // path distance from our spawn (capped 255)
    std::uint8_t dist_altar;   // path distance to the nearest altar neighbour (capped 255)
    std::uint8_t flags;        // GroupFlag bits
};
struct SlotPlan {
    std::uint8_t type;         // champion_type of this body
    std::uint8_t role;         // Role
    std::uint8_t squad;        // squad id
    std::uint8_t rank;         // capture rank within the squad (higher captures)
    std::uint8_t n;            // circuit length
    std::uint8_t circuit[MAX_CIRCUIT];
};
struct Plan {
    std::uint8_t version;
    std::uint8_t bodies;
    std::uint8_t groups;
    std::uint8_t squads;
    std::uint8_t gold_weight_pct; // value of 1 gold in hundredths of a gem
    std::uint8_t bank_threshold;  // gems carried before banking
    std::uint8_t reserved[2];
    GroupPlan g[MAX_GROUPS];
    SlotPlan s[MAX_BODIES];
};
#pragma pack(pop)
static_assert(sizeof(Plan) <= 1024, "plan must fit draft memory");

inline std::vector<std::uint8_t> encode_plan(const Plan& p) {
    std::vector<std::uint8_t> out(sizeof(Plan));
    std::memcpy(out.data(), &p, sizeof(Plan));
    return out;
}

inline bool decode_plan(Plan& p) {
    if (cambc::draft_memory.size() < sizeof(Plan)) return false;
    std::memcpy(&p, cambc::draft_memory.data(), sizeof(Plan));
    return p.version == PLAN_VERSION && p.bodies >= 1 && p.bodies <= MAX_BODIES;
}

// Live shared memory (8 bytes): byte i (< 6) is slot i's record: phase << 6 | group (63 = none).
enum Phase : std::uint8_t { PH_IDLE = 0, PH_CLEAR = 1, PH_CAPTURE = 2, PH_DELIVER = 3 };
struct Claim { std::uint8_t group; std::uint8_t phase; };
inline Claim unpack_claim(std::uint8_t b) { return { static_cast<std::uint8_t>(b & 63), static_cast<std::uint8_t>(b >> 6) }; }
inline std::uint8_t pack_claim(Claim c) { return static_cast<std::uint8_t>((c.phase << 6) | (c.group & 63)); }

} // namespace apex
