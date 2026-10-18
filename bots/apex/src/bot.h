// Per-champion economy state machine: TRAVEL -> CLEAR -> CAPTURE -> DELIVER -> TRIBUTE -> RECOVER.
#pragma once
#include "common.h"
#include "model.h"
#include "plan.h"

namespace apex {

enum class Mode : std::uint8_t { TRAVEL, CLEAR, CAPTURE, DELIVER, TRIBUTE, RECOVER, IDLE };

struct Monster {
    cambc::camp camp;
    vec2 pos;
    float hp;
    int group;
    bool awake;
};

// Per-turn snapshot of everything the decision code reads more than once.
struct World {
    std::uint32_t round{};
    vec2 pos{};
    float hp{}, maxhp{};
    std::uint32_t gems{}, gold{};
    cambc::champion_state st{};
    cambc::buff_restrictions restr{};
    bool at_spawn{};
    bitboard visible, blocked, camp_tiles, champ_tiles;
    std::vector<Monster> monsters;          // visible monsters
    std::vector<cambc::champion> enemies;   // visible enemy champions
    bool moved{}, attacked{}, casted{}, exclusive_cast{};
    bool can_move() const { return st.contains(cambc::champion_state::can_move) && !restr.contains(cambc::buff_restrictions::move) && !moved; }
    bool can_attack() const { return st.contains(cambc::champion_state::can_attack) && !restr.contains(cambc::buff_restrictions::attack); }
    bool can_cast() const { return st.contains(cambc::champion_state::can_cast) && !restr.contains(cambc::buff_restrictions::cast) && !casted; }
};

struct Known { std::uint8_t state{}; std::uint32_t round{}; std::uint32_t respawn_round{}; }; // state: 0 unknown 1 alive 2 reward 3 respawning

class Bot {
public:
    void play_turn();
    void end_of_turn();

    // --- context visible to combat.h ---
    World w;
    bool ranged{};
    int attack_range{1};
    Role role{ROLE_DPS};

    bool try_attack_champion(const cambc::champion& target);
    bool step_towards(const bitboard& goals, bool allow_dash = false);
    bool step_away_from_threats();
    bool in_danger_from_champions() const;
    void refresh_state();

private:
    // init
    bool init_done{};
    void init();
    void snapshot();
    void read_claims();
    void update_known();
    void shop();

    // economy
    void farm();
    int pick_target();
    bool target_available(int g, int travel) const;
    void approach_group(int g);
    void engage_group(int g);
    void capture_group(int g);
    void deliver();
    bool bank_now();
    bool recover();
    void drop_target();

    // kit
    bool cast_damage_on_monster(const Monster& m);
    bool cast_aoe_on_monsters(int g);
    bool cast_cc_on_monster(const Monster& m);
    bool cast_shield(float incoming);
    bool attack_monster(const Monster& m);
    bool attack_reset_and_attack(int g);
    const Monster* choose_attack_target(int g, int range);

    // helpers
    bool move_to(vec2 tile);
    std::uint32_t team_gems() const;
    int squad_of_slot(int s) const;
    int rank_of_slot(int s) const;
    Role role_of_slot(int s) const;
    float incoming_damage_at(vec2 at, int g) const;
    void write_claim(std::uint8_t phase, int group);

    // static context
    int slot{-1};
    champion_type type{};
    Plan plan{};
    bool plan_ok{};
    SlotPlan mine{};
    Body body{};
    std::vector<cambc::map::camp_group> groups;
    std::vector<bitboard> gspawn;
    std::vector<vec2> gcenter;
    std::vector<float> ghp, gdanger;
    std::vector<int> ggems, ggold, gresp;
    std::vector<std::uint8_t> tile_group;
    std::vector<vec2> altars;
    bitboard altar_goals;
    bitboard terrain;      // walls | holes | altars | enemy spawn
    bitboard spawn;
    distance_field f_home; // distance to our spawn
    std::vector<cambc::champion> allies;
    std::vector<Body> ally_body;
    std::vector<std::uint8_t> ally_slotplan; // index into plan.s or 255
    std::array<std::optional<cambc::ability>, 2> ab;
    int ab_reset{-1}, ab_dash{-1}, ab_shield{-1}, ab_damage{-1}, ab_aoe{-1}, ab_cc{-1}, ab_heal{-1};
    Known known[MAX_GROUPS];

    // dynamic
    int target{-1};
    std::uint32_t target_since{};
    int wait_rounds{};
    bool delivering{};
    bool recovering{};
    Claim claims[8]{};
    Claim my_claim{NO_GROUP, PH_IDLE};
    Mode mode{Mode::IDLE};

    // telemetry
    std::uint64_t fmin{~0ull};
    std::uint32_t susp{};
    std::uint32_t prev_round{};
    bool have_prev{};
};

} // namespace apex
