#pragma once
#include <cambc.h>
#include <cstdint>
#include <vector>

enum class Role { farm, hunt, lock };

// Static facts about one camp group, computed once on the first turn.
struct Group {
    cambc::map::camp_group group;
    cambc::bitboard spawns;
    std::vector<cambc::vec2> tiles;
    cambc::vec2 centre{};
    int gems = 0, gold = 0, size = 0;
    float hp = 0, dps = 0;
    bool monolith = false;
    std::uint32_t blocked_until = 0;  // round until which we treat it as unavailable
};

class Bot {
public:
    void play_turn();
    void telemetry(std::uint64_t fuel_min);

private:
    // --- persistent state ---
    bool inited = false;
    int slot = -1;
    Role role = Role::farm;
    bool ranged = false;
    cambc::champion_type kind{};
    std::vector<Group> groups;
    cambc::bitboard terrain;       // walls | holes | altars | enemy spawn
    cambc::bitboard spawn_ally;
    std::vector<cambc::vec2> altars;
    cambc::vec2 enemy_spawn_centre{}, ally_spawn_centre{};
    int target = -1;
    std::uint32_t target_since = 0;
    cambc::vec2 last_pos{-1, -1};
    int stuck = 0;
    // hunter
    std::vector<cambc::vec2> patrol;
    int patrol_idx = 0;
    cambc::vec2 chase_pos{-1, -1};
    std::uint32_t chase_seen = 0;
    bool chasing = false;
    // locker
    cambc::vec2 lock_altar{-1, -1};
    bool our_lock_seen = false;   // last known state of our lock when the altar tile was visible

    // fuel probes: max fuel consumed per phase in the current telemetry window
    std::uint64_t probe_last = 0;
    std::uint64_t probe_max[4] = {0, 0, 0, 0};
    void probe_start() { probe_last = cambc::fuel_remaining(); }
    void probe(int phase) {
        const auto now = cambc::fuel_remaining();
        const auto used = probe_last > now ? probe_last - now : 0;
        if (used > probe_max[phase]) probe_max[phase] = used;
        probe_last = now;
    }

    // --- per-turn scratch ---
    std::uint32_t round = 0;
    cambc::vec2 pos{};
    float hp = 0, max_hp = 0;
    bool at_spawn = false;
    bool cast_done = false;
    cambc::bitboard visible;
    cambc::bitboard blocked;       // terrain + visible units (own tile cleared)
    std::vector<cambc::champion> enemies, allies;
    std::vector<cambc::camp> camps;

    void init();
    void begin_turn();
    void shop();
    void write_claim(int group_index);
    int read_claim(int other_slot) const;

    bool step_towards(const cambc::bitboard& goals);
    bool step_away_from(const std::vector<cambc::vec2>& threats);
    bool want_recall(float fraction);
    bool enemy_within(int range) const;
    // Is there a lock of `side` within 2 tiles of this altar (locks are recorded on their own tile)?
    bool altar_locked_by(cambc::vec2 altar, cambc::team side) const;
    bool awake_camp_within(int range) const;

    // combat
    bool attack_camp_in_range(const std::vector<const cambc::camp*>& living);
    void cast_on_camps(const std::vector<const cambc::camp*>& living, bool allow_exclusive);
    bool attack_enemy_in_range(const cambc::champion* preferred);
    void cast_on_enemies(const cambc::champion* preferred, bool allow_exclusive, bool prefer_stun);
    void try_shield();
    void try_reset_and_attack(const cambc::unit* unit_target);

    // roles
    void do_farm();
    void do_hunt();
    void do_lock();
    bool bank_gems(bool force);
    bool choose_target();
    bool target_viable(const Group& g, bool relaxed, float dpr) const;
    float my_dpr() const;
    void act_on_target();
};
