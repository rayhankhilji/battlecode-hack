# SPEC — Cambridge Battlecode 2026 Autumn: rules, numbers and C++ API (engine-verified)

Every number here is tagged with its source:

- **[probe]** printed by `bots/probe` (C++ bot) in the real engine, `cambc 2.2.5`, `--release`, run on all 17 maps.
  The constant/design/item dump is byte-identical on all 17 maps (one md5). Raw output lives in the replay
  bundles' `team-a.stdout`; the probe source is `bots/probe/src/{draft,bot}.cpp`.
- **[cambc.h]** the C++ SDK header `sdk/cpp/cambc.h` (and `imports.h`), i.e. the API contract and its doc comments.
- **[rules]** the official handbook export `docs/official/GameRules.md`; **[api-doc]** `docs/official/BotAPICpp.md`.
- **[playbook]** the third-party rules digest (`.../ext/Battlecode/docs/strategy-playbook.md`), only where it disagrees.

Where sources disagree the engine ([probe]) wins; every disagreement found is listed in section 11.

---

## 1. Engine settings (`cambc::constants::*`) [probe]

| Setting | Value | Meaning [cambc.h doc comment] |
|---|---|---|
| `capture_time` | **5** | rounds standing uncontested on a cleared group's spawn tiles to receive the reward |
| `camp_sleep_time` | **2** | rounds without an in-range champion before an awake camp sleeps |
| `respawn_time` | **60** | rounds before a dead champion respawns |
| `recall_time` | **5** | base recall channel (Home emblem: 3) |
| `submit_gem_duration_multiplier` | **0.1** | rounds per gem, rounded up → `ceil(bag/10)` |
| `submit_gem_base_duration` | **0** | fixed rounds added to a tribute |
| `submit_gem_minimum_duration` | **1** | minimum tribute channel |
| `submit_gems_range` | **1** | max Chebyshev distance champion→altar to tribute (the 8 neighbours) |
| `max_rounds` | **2000** | round limit |
| `winning_gems` | **1000** | tributed gems that end the game immediately |
| `kill_gold` | **50** | gold to the killer |
| `assist_gold` | **0** | there is no assist gold |
| `passive_gold` | **0** | there is no passive income |
| `spawn_healing` | **1** (fraction) | 100 % of max health restored per round on a spawn tile (at turn end) |
| `active_wards` / `ward_charges` | **1 / 1** | one ward active, one charge |
| `ward_cooldown` | **1** | rounds to recharge the ward charge (re-place every turn) |
| `ward_lifetime` | **-1** | wards never expire |
| `ward_vision_radius` | **6** | ward sight radius |
| `ward_placement_radius` | **2** | place within Chebyshev 2 of the champion |
| `control_ward_*` | 6 / 2 | retired "control ward" settings, same values (not exposed as actions) |
| `shared_memory_bytes` | **8** | live team memory |
| `draft_memory_bytes` | **1024** | draft memory, frozen after the draft |
| `tile_reveal_rounds` | **1** | a hidden blocker that bounced an action is revealed for the current round only |
| `altar_lock_hits` | **3** | basic attacks to destroy an altar lock |
| `tp_crystal_time` | **3** | teleport-crystal channel (`tp_scroll_time` = 3 is a retired setting) |
| `passive_abilities::prism_focus_per_gem` | 0.2 | compile-time constants in the header [cambc.h] |
| `passive_abilities::varg_splash_radius` | 1 | |
| `passive_abilities::vesper_vale_bonus_gold_percent` | 20 | |
| `passive_abilities::ryth_health_strength_ratio` | 0.01 | 1 Strength per 100 **bonus** health |
| `passive_abilities::ren_retaliation_focus_ratio` | 0.2 | retaliation = 0.2 × Focus (20 at base Focus, scales with Focus) |

Fuel [probe]: first draft turn starts with 1,000,000 minus static init (979,258 left with `<iostream>`, 991,152 without);
later draft turns 100,000; each champion's first play turn 200,000 minus static init (≈179,000 with `<iostream>`,
190,754 without); every later turn 100,000 (99,979 measured after a `fuel_remaining()` + `is_draft()`). See `docs/FUEL.md`.

## 2. Winning, tiebreaks, rounds [rules]

First team to tribute 1,000 gems wins immediately. At round 2,000: tributed gems → lifetime gold earned (not gold
in hand; Tavi's sale refunds do not count) → champion kills → Team B (the team that acted second in round 1).
Carried gems count for nothing. A local `cambc run --rounds N` may stop earlier ("no result").

## 3. Turn structure and action rules

Round order [rules, "The exact turn order"]:

1. **Round start**: camp respawns → spell/ward updates → camp captures → camp actions (monsters attack).
2. **Champion turns**, teams alternating execution slots; the team that starts swaps every round. Within a team:
   higher `priority()` first, then draft order. `set_priority(p)` applies from the **next** round and persists [cambc.h].
   Dead champions still get a turn (to shop).
3. **Champion turn start**: zero timers resolve — respawn, ward recharge, buff/shield expiry, then channels.
4. **Champion turn end**: timers decrement (without triggering); spawn healing happens at turn end.

Per turn each champion may [rules, cambc.h error codes]:

| Action | Rule | Engine evidence |
|---|---|---|
| Walk | 1 tile, 8 directions, once per turn | `champion_state::can_move`, `invalid_action_move_already_moved` |
| Basic attack | once; melee range 1, ranged range 3 (Chebyshev); damage = Strength; target must be visible | `can_attack`, `invalid_action_attack_already_attacked`, `attack_range()` |
| Ranged walk/attack exclusion | a **ranged** champion's walk and basic attack consume each other's allowance; melee may do both | [rules] |
| Ability | **one successful cast per turn across both slots**; each slot has its own 8-turn-end cooldown | `can_cast`, `invalid_action_spell_cast_already_cast`, `max_cooldown=8` [probe] |
| `exclusive_with_movement` abilities | cannot share a turn with an ordinary walk, in either order | `invalid_action_spell_cast_already_moved` (126), `invalid_action_move_exclusive_spell_cast` (127) |
| Attack reset | restores the basic-attack allowance: attack → reset → attack (Marcella's also makes the next attack an enhanced stunning hit) | `ability_attack_reset::enhanced_attack` |
| Ward | one free permanent ward, placement radius 2, recharge 1 round; placing a new one replaces the old | constants above, `place_ward` |
| Consumable | `tp_crystal` / `altar_lock`, 10 gold, carry 1 of each | [probe] `consumable_info` |
| Shop | on own spawn tiles or while dead; Tavi buys shards anywhere (consumables still spawn-only) and may sell at spawn | `invalid_action_buy_item_not_on_spawn`, `invalid_action_sell_not_tavi`, `remote_shop` |
| Recall / tribute | one start and one cancel per turn; a channel blocks other actions until it finishes or is cancelled | `invalid_action_recall_already_used`, `invalid_action_is_submitting_gems`, … |

**Hidden blockers** [rules, cambc.h]: `move`/`move_absolute`/`move_towards`, ability `use(...)`, `place_ward`, consumable
`use` return **`false`** (no throw, allowance kept, tile revealed for the round) when an *unseen* unit blocks the destination;
a destination already known to be illegal **throws** `cambc::error`.

### Channels [rules; constants from probe]

| Channel | Duration (owner turn ends) | Damage | Stun / death | Manual cancel |
|---|---|---|---|---|
| Tribute (`submit_gems`) | `max(1, ceil(bag_at_start / 10))` | continues | cancels; a **newly placed enemy altar lock** also cancels | yes (`cancel_submit_gems`) |
| Recall | 5 (3 with a Home emblem, "rounded up" from 50 %) | **cancels** | cancels | yes (`cancel_recall`) |
| Teleport crystal | 3 | continues | enemy stun / death cancel | no |

Timers tick at the owner's turn end including the starting turn and complete at the owner's **next turn start** after reaching 0,
so a 1-tick tribute banks at the start of your next turn, not immediately. Tribute length uses the bag at start but deposits the
whole bag at completion (captures resolve before channels, so a capture that lands mid-channel is included). Tribute requires
Chebyshev ≤ 1 from the altar (`submit_gems_range`) and ≥ 1 gem. Recall keeps gems and arrives at turn start; stay on spawn to
the end of that turn to heal to full. Tributing at an enemy-locked altar is converted into a basic attack on the lock [rules].

## 4. Map and movement [rules, probe]

Grid 30×30 … 100×100 (actual pool: 30×30 to 100×100, see `docs/MAPS.md`). Tiles: wall (blocks move, sight and dashes),
hole (blocks move, not sight; dashes cross it), bush (walkable; blocks sight from outside; the connected bush cluster you stand
in is transparent to you), altar (not walkable; tribute from the 8 neighbours), spawn (enemy cannot enter; heals). Dashes: up to 2
legal steps incl. diagonals. Teleport crystal: 3 tiles ignoring terrain in between, landing tile must be legal (the handbook's
"TP Crystal can reach walls" cell contradicts its own landing rule; untested). Landing is never allowed on wall/hole/altar/enemy
spawn/occupied tile. All distances are Chebyshev. `map::symmetry()` is one of horizontal / vertical / rotational180 [probe:
all three occur in the pool].

## 5. Camps and capture [rules; probe for engine designs and group rewards]

Engine camp designs (`sdk_camp_design`) [probe]: all five types have `vision=8`, `move=1`, `attack_range=3`, no growth, focus 1:

| Type (`camp_type`) | HP | Str | Gold | Gems | Respawn (group) | Notes |
|---|---|---|---|---|---|---|
| `wolf` (0) | 1350 | 20 | 90 | 22 | 40 | |
| `elk` (1) | 2400 | 60 | 160 | 36 | 60 | |
| `gem_cluster` (2) | 500 | 0 | 0 | 24 | 50 | engine design says range 3 (moot, Str 0); handbook table says 0 |
| `gold_cluster` (3) | 500 | 0 | 35 | 0 | 50 | |
| `monolith` (4) | 20000 | 200 | 300 | 200 | 150 | range **3** [probe]; the playbook's "5" is wrong |

Gold/gems per type were derived from single-camp groups on the maps and confirmed additive on multi-camp groups
(e.g. monolith + 8 gem clusters = 200 + 8×24 = 392 gems; elk + gold cluster = 195 gold / 36 gems) [probe]. A group's
`max_respawn_timer()` is the **max** of its members' timers (wolf + gold cluster → 50; anything with an elk → 60) [probe].
A group is the set of cardinally connected monsters [rules]; the engine exposes it as `map::camp_group` with fixed spawn tiles.

Behaviour [rules]: monsters are asleep until damaged, then the whole group wakes and attacks the nearest champion every
round (camps act at round start, before your next turn); a group goes back to sleep after `camp_sleep_time` = 2 rounds without
a champion in range. Monsters see through bushes and do not chase. After the last member dies, stand on any of its spawn tiles,
uncontested, for 5 capture phases; leaving the region empty or an enemy entering resets progress; extra allies do not speed it
up; the reward is split among allies present (lowest ID takes the remainder). The group respawns `max_respawn_timer()`
rounds after the reward is claimed; an occupied spawn tile delays the whole group.

## 6. Champions — exact engine designs [probe `champion_designs()`]

Shared by all 12 physical champions: vision 8, move 1, both cooldowns 8, `camp_damage_multiplier` 1.0.
"excl" = `exclusive_with_movement` (cannot walk in the same turn). "targets" is the `ability_target_mask` bit set.
Spell numbers at base Focus are in brackets. `champion_type` ids in parentheses; note `champion_draft` ids differ (section 8).

| Champion (type id) | Attack | HP (+/shard) | Str (+) | Focus (+) | Passive (`passive_type`) | Slot 0 | Slot 1 |
|---|---|---|---|---|---|---|---|
| Ryth (0) | melee r1 | 1200 (+100) | 50 (+5) | 100 (+10) | `health_strength`: +1 Str per 100 bonus HP | `aoe_damage` r4, radius 1, 1.0×Focus [100] + 1-round stun, targets camps/enemies/ground, **excl** | `attack_reset` (self) |
| Vesper Vale (1) | ranged r3 | 900 (+100) | 45 (+5) | 100 (+10) | `bonus_gold`: +20 % gold | `attack_reset` | `dash` range 2 (ground) |
| Vendetta (2) | melee r1 | 800 (+100) | 70 (+5) | 100 (+10) | `reset_cooldowns_on_kill` (camp kills too) | `attack_reset` | `dash` range 2 |
| Cassi (3) | ranged r3 | 700 (+71.25) | 30 (+3.5) | 70 (+15) | none (dies with Issac) | `damage` r4, 2.0×Focus [140], camps/enemies, **excl** | `cc` r4, modifier **0** (no damage), 1-round stun, camps/enemies, walk-compatible |
| Issac (4) | melee r1 | 900 (+95) | 40 (+4.75) | 100 (+10) | none (dies with Cassi) | `attack_reset` | `dash` range 2 |
| Stellar (5) | ranged r3 | 750 (+75) | 35 (+3.5) | 100 (+10) | none | `damage` r4, 2.0×Focus [200], **excl** | `aoe_damage` r4, radius 1, 1.4×Focus [140], duration 0 (no stun), **excl** |
| Prism (6) | ranged r3 | 700 (+75) | 35 (+3.5) | 80 (+10) | `gem_focus`: +0.2 Focus per carried gem | `cc` r4, 1.5×Focus [120] + 1-round stun, **excl** | `aoe_damage` r4, radius 1, 1.0×Focus [80], **excl** |
| Marcella (7) | melee r1 | 1200 (+100) | 45 (+5) | 50 (+10) | `stun_immunity` | `attack_reset` with `enhanced_attack=1` (next basic attack stuns 1 round) | `damage` r4, 2.0×Focus [100], **excl** |
| Willow (8) | ranged r3 | 700 (+50) | 45 (+3) | 105 (+5) | none | `heal` r4, 3.0×Focus [315], targets **allies only** (not self), walk-compatible | `attack_reset` |
| Ren (9) | melee r1 | 900 (+100) | 40 (+5) | 100 (+10) | `retaliation`: 0.2×Focus [20] to each attacker per hit taken | `cc` r4, 1.25×Focus [125] + 1-round stun, **excl** | `dash` range 2 |
| Varg (10) | melee r1 | 1000 (+100) | 50 (+5) | 100 (+10) | `splash_attack`: basic attacks deal full Str to other targets within radius 1 of the target | `attack_reset` | `shield` self, 2.5×Focus [250], duration 5 (engine stores range 4; target is self) |
| Tavi (11) | ranged r3 | 750 (+75) | 35 (+5) | 100 (+10) | `remote_shop`: buys shards anywhere, sells at spawn for full price | `attack_reset` | `cc` r4, 2.2×Focus [220] + 1-round stun, **excl** |

`ability_target_mask` bits [probe]: attack resets and Varg's shield = `self` (8); dashes = `ground` (16); single-target
spells = `camps|enemies` (3); AoE spells = `camps|enemies|ground` (19); Willow heal = `allies` (4). Only Cassi's stun and
Willow's heal are castable on a turn you also walk; every damage spell and every stun that deals damage is `excl`.
Attack resets, dashes and the shield are walk-compatible.

Design-vs-live gotcha: `champion_designs[i]` (and `champion{champion_type}`) answer `base_stats()`, `passive()`,
`champion_name()`, `abilities()[k].info()` / `valid_targets()`; they **throw `api_not_in_vision`** on `position()`, `health()`,
`state()`, `abilities()[k].cooldown()`, `valid_tiles()`, `buffs()` [probe].

## 7. Items and consumables [probe `item_type::info()`, `consumable_type::info()`]

42 item types = 7 families × {strength, focus, health} × {shard, emblem}. Every shard: cost 30, purchasable, `combine_count=5`,
gives **1 point** of its stat (one point = the champion's growth value for that stat from section 6). Every emblem: cost 150
(what Tavi gets back when selling), not purchasable (auto-combined from 5 identical shards), gives 5 points of its stat and:

| Family | Emblem stats (points) | `item_passive` | `passive_magnitude` | Effect [rules] |
|---|---|---|---|---|
| Monster | 5 | `monster` | 0.1 | +10 % damage to camps |
| Power | **6** | none | 0 | one extra shard instead of a passive |
| Durability | 5 | `durability` | 0.1 | −10 % incoming damage |
| Treasure | 5 | `treasure` | 0.2 | +20 % gold (adds with Vesper: +40 %) |
| Jungle | 5 | `jungle` | 0.3 | −30 % damage from camps |
| Home | 5 | `home` | 0.5 | recall 5 → 3 turn ends |
| Bruiser | 5 + **1 of each other stat** | none | 0 | spread of stats |

Stats stack (including duplicate emblems); a family's passive does not stack with itself. Buying Health on spawn also heals the
amount gained; Tavi's field Health purchases do not [rules]. Consumables [probe]: `tp_crystal` cost 10, carry limit 1;
`altar_lock` cost 10, carry limit 1. Altar lock: placed on an altar within 2 tiles (handbook; the engine has
`invalid_action_altar_lock_out_of_range` but exposes no constant), blocks enemy tributes and cancels enemy channels at that
altar, removed by `altar_lock_hits` = 3 basic attacks (`champion::attack(const altar_lock&)`).

## 8. Draft [rules, probe]

`draft::team_size()` picks per team (3, 4 or 5 depending on the map — see MAPS.md), one pick per draft turn, simultaneous
and revealed per round (`draft::current_draft(team::enemy)` shows the enemy's previous picks [probe]). No duplicate within a
team; mirroring the enemy is allowed. A draft bot that yields, crashes or runs out of fuel without picking gets random picks
for the rest of the draft (observed when the probe's first draft turn ran out of fuel). The second `draft::pick` in one turn throws
`draft_already_picked` [probe]. `champion_draft` ids: ryth 0, vesper_vale 1, vendetta 2, **cassi_and_issac 3**, stellar 4,
prism 5, marcella 6, willow 7, ren 8, varg 9, tavi 10 — they are **not** the `champion_type` ids used by
`champion_designs` / `champion_name()` (cassi 3, issac 4, stellar 5, prism 6, marcella 7, willow 8, ren 9, varg 10, tavi 11).

What the draft can see [probe]: `map::width/height/symmetry/walls/holes/bushes/altars/spawn_region(…)`, `map::camp_groups()`
(handle, gold, gems, respawn, member types and spawn tiles), `champion_designs`, items, `draft::team_size()`, bitboard and
distance-field ops. `round_number()` returns 0. **Throws `api_not_initialized` in the draft**: `vision::visible_tiles()`,
`cambc::champions(...)`, `map::camp_group::state()`, `camp::state()/health()/is_visible()`. `draft_memory = bytes` submits
immediately; anywhere later it throws `invalid_action`. The champion bots read `cambc::draft_memory` (1024 bytes, snapshot
taken at their startup).

## 9. Vision and wards [rules, probe]

Each champion sees radius 8 by symmetric shadowcasting, plus its own ward (radius 6, placed within 2, permanent, one
active, re-placeable every round). Walls and bushes occlude; the bush cluster you stand in is transparent to you. **Vision is
not shared between teammates**; the 8 bytes of `shared_memory` are the only live channel. `vision::*` queries are from your
champion's perspective only; `cambc::champions(team)` lists handles of all champions including hidden ones, but any
`position()/health()/state()` on a hidden one throws `api_not_in_vision` (≈3,500 fuel per throw [probe]) — check
`is_visible()` first (57 fuel). Hidden champions still answer `champion_name()`, `team()`, `is_alive()`, `abilities()[k].info()`
[probe].

## 10. Death, kills, gold [rules, probe constants]

Killer gets the victim's whole carried bag and `kill_gold` = 50; `assist_gold` = 0. Camp kills destroy the bag. Dead champions
keep shopping turns and respawn after `respawn_time` = 60 owner turn-ends at their next turn start, on a free spawn tile, at full
health with cooldowns reset. Cassi and Issac die together, giving one kill reward and both bags. No passive gold
(`passive_gold` = 0): all gold comes from captures, kills and the Vesper/Treasure multipliers.

## 11. Disagreements between sources (engine wins)

| Topic | BRIEF.md / playbook / handbook says | Engine [probe / cambc.h] |
|---|---|---|
| Monolith attack range | playbook: table says 5, text says 3 | design `attack_range=3` |
| Cluster attack range | handbook table: 0 | design `attack_range=3`, Str 0 (irrelevant) |
| Camp sleep trigger | handbook: "does not take damage for 2 turns" in one place, "2 rounds without an in-range champion" in another | `camp_sleep_time=2` documented as "rounds **without an in-range champion**" |
| Draft visibility | BRIEF: "full map, camps and altars are visible during draft" | terrain, altars, spawn regions and camp-group layout: yes; `vision::*`, champions and live camp/group **state** throw `api_not_initialized` |
| First-turn fuel | BRIEF: 200k first play turn, 1M first draft turn | budgets are right, but ≈9k (SDK globals) to ≈21k (with `<iostream>`) is spent before `main()` — see FUEL.md |
| Assist gold | BRIEF lists "kill/assist gold" as settings | `assist_gold=0`, `passive_gold=0` |
| Ren retaliation | BRIEF: "20 per hit" | 0.2 × Focus (= 20 only at base Focus; grows with Focus shards) |
| Shop location | BRIEF: "shop only on spawn (Tavi anywhere)" | also while dead; Tavi's remote shop covers shards only, consumables need spawn/dead |
| Ward "1-round cooldown" | BRIEF | `ward_cooldown=1`, `ward_lifetime=-1`, `active_wards=1` — i.e. relocate every turn, never expires |
| Varg shield "range" | — | engine stores `range=4` on the shield payload but `valid_targets=self`; use `cast_untargeted`/`use()` |
| Tribute vs TP wall landing | handbook table: "TP Crystal can reach walls" vs its own landing rule | not probed; the error code `invalid_action_champion_teleport_bad_destination` exists; do not plan wall landings |
| Python fuel | playbook: homepage 20× vs handbook 25× | not measured here (C++ only) |

Everything else in BRIEF.md's "game in one page" matched the engine exactly (all base stats, growths, kit multipliers, camp
HP/Str/rewards, capture 5, sleep 2, respawn 60, recall 5, ceil(bag/10), lock 3 hits, TP 3, ward 6/2/1, 8-byte/1024-byte memory,
2000 rounds, 1000 gems).

---

## 12. C++ API cheat sheet (exact signatures from `sdk/cpp/cambc.h`)

All in `namespace cambc`. Any host failure throws `cambc::error` (`.code()` → `error_code`, `to_string(code)` gives the
name; `.what()` = "engine error N: msg"). A throw+catch costs ≈2.9k fuel [probe]: use the `can_*`/`is_*`/`has_*` queries instead
of try/catch for control flow. Bitboard-returning functions allocate (≈250–390 fuel); the `*_into(bitboard&)` variants reuse
a buffer.

### Turn plumbing

```cpp
bool  is_draft() noexcept;                   // true in the draft bot instance
champion& self();                            // throws std::bad_optional_access in the draft
std::uint64_t fuel_remaining();              // 17 fuel; first draft turn 10x, each champion's first turn 2x
void  end_turn();                            // fflush(nullptr) then yield; ALWAYS reached (keep the try/catch skeleton)
std::uint32_t round_number();                // 0 during the draft
std::uint32_t global_submitted_gems(team side);
inline constexpr shared_memory_buffer shared_memory;   // 8 bytes, live, team-private
    shared_memory.read(0, std::span{&u64, 1}); shared_memory.write(0, std::span{&u64, 1}); // ~80 fuel each
    std::uint8_t b = shared_memory[i]; shared_memory[i] = b;                                // byte proxy, throws out_of_range
inline draft_memory_buffer draft_memory;     // snapshot of 1024 bytes taken at startup; .data()/.size()/operator[]
    draft_memory = std::span<const std::byte>{...} / any contiguous trivially-copyable range  // draft only; throws invalid_action later
```

### Draft

```cpp
namespace draft {
  std::uint32_t team_size();                                  // picks per team (3..5)
  std::vector<champion_draft> current_draft(team side);       // picks so far, either team
  void pick(champion_draft pick);                             // once per draft turn; 2nd call throws draft_already_picked
}
inline const std::array<champion, 12> champion_designs;       // index = champion_type; design-only views
```

### Champion (live view; `self()` is the only one you can act with — other champions throw `std::invalid_argument("action is only available for self")`)

```cpp
champion_type champion_name() const;   // works without vision         passive_type passive() const;      // design lookup, 184 fuel
team   team() const;  bool is_ally() const;  bool is_enemy() const;    // work without vision
bool   is_alive() const;               // works for hidden champions   bool is_visible() const;            // false if hidden (never throws)
vec2   position() const;  float health(), max_health(), strength(), focus() const;  std::int32_t attack_range() const;  // THROW api_not_in_vision if hidden
champion_state state() const;          // flags: alive, recalling, submitting_gems, can_move, can_attack, can_cast, completed_turn, teleporting, ...
bool   can_move(), can_attack(), can_cast(), is_recalling(), is_submitting_gems(), is_teleporting(), has_completed_turn() const;
buff_restrictions restrictions() const; buff_collection buffs() const;  // buffs(): has_stun()/has_shield()/get<buff_shield>().value
champion_stats base_stats() const;     // {vision_radius, base_move_speed, base_attack_range, base_health, health_growth, base_strength, strength_growth, base_focus, focus_growth, camp_damage_multiplier}
std::array<ability, 2> abilities() const;                     // copies the two slot views (107 fuel)
std::uint32_t gem_count() const;       // visible champions         std::uint32_t gold_count() const;   // SELF ONLY (others throw std::invalid_argument)
enum_map<item_type> item_count() const;  const enum_map<consumable_type, consumable>& consumables() const;  // consumables()[t].count()/use(vec2)/use()
bitboard attack_tiles() const;  void attack_tiles_into(bitboard&) const;     // Chebyshev range mask around position(); not a target filter
bitboard visible_tiles() const;  bitboard visible_tiles_from(vec2 p) const;   // own vision / hypothetical vision from p
std::int32_t remaining_channel_time() const;                  // throws when not channelling
std::vector<vec2> ward_positions() const;  std::int32_t ward_cooldown() const;  bitboard ward_placement_tiles() const;   // self only
bool can_buy(item_type item, std::uint32_t count = 1) const;  bool can_buy(consumable_type) const;  bool can_sell() const;  // self only
// actions (self only; engine validates, throws cambc::error on known-illegal, returns false on hidden blocker)
bool move(vec2 delta) const;  bool move_absolute(vec2 destination) const;
bool move_towards(vec2 target) const;  // one step along a shortest path avoiding walls/holes/altars/enemy spawn/visible units; false if already there, cannot move, target blocked/unreachable. 1.7k–6.3k fuel (it builds 7 bitboards + a distance field every call)
void attack(const unit& target) const;  void attack(const altar_lock& target) const;   // champion or camp view, must be visible and in range
void recall() const;  void cancel_recall() const;  void submit_gems(vec2 altar) const;  void cancel_submit_gems() const;
bool place_ward(vec2 destination) const;
void buy(item_type type, std::uint32_t count = 1) const;  void buy(consumable_type type) const;  void sell(item_type type, std::uint32_t count = 1) const;
void set_priority(std::int32_t priority) const;  std::int32_t priority() const;  std::int32_t prev_priority() const;
```

### Ability (slot view from `self().abilities()[i]`)

```cpp
std::uint32_t slot() const;  std::int32_t cooldown() const;  bool is_on_cooldown() const;  bool is_ready() const;  // is_ready = !cooldown && owner can_cast (481 fuel: re-queries the owner)
const std::optional<ability_info>& info() const;        // cached at construction; std::variant<ability_attack_reset, ability_damage, ability_aoe_damage, ability_cc, ability_heal, ability_dash, ability_shield>
template <class T> T get() const;                        // e.g. a.get<ability_cc>().range / .modifier / .duration; throws bad_variant_access on wrong kind
bool is(ability_kind) const noexcept;  bool is_attack_reset()/is_damage()/is_aoe_damage()/is_cc()/is_heal()/is_dash()/is_shield() const noexcept;
ability_target_mask valid_targets() const;  bool can_target_camps()/can_target_enemies()/can_target_allies()/can_target_self()/can_target_ground() const;
bitboard valid_tiles() const;                             // Chebyshev cast range around the owner; throws std::invalid_argument for range-less slots (resets)
float damage_amount() const;  float damage_amount(const champion& target) const;  // predicted, includes items/passives; target must be visible
float heal_amount() const;  float heal_amount(const champion&) const;  float shield_amount() const;  float shield_amount(const champion&) const;
// casting (self only) — the "cast_on / cast_at / cast_untargeted" trio is spelled `use` in C++:
bool use() const;                      // untargeted: attack resets, Varg shield
bool use(vec2 destination) const;      // ground: dashes, Ryth/Stellar/Prism AoE
bool use(const champion& target) const;  bool use(const camp& target) const;   // unit targets: damage/cc/heal spells; lands on the target's tile
```

Common fields: `max_cooldown` (configured, always 8), `valid_targets`, `exclusive_with_movement`. Payload fields:
`ability_damage{range, modifier}`, `ability_aoe_damage{range, modifier, radius, duration}`, `ability_cc{range, modifier, duration}`,
`ability_heal{range, modifier}`, `ability_dash{range}`, `ability_shield{range, modifier, duration}`, `ability_attack_reset{enhanced_attack}`.

### Vision (perspective of `self()`; all recomputed per call)

```cpp
namespace vision {
  bitboard visible_tiles();  bitboard champion_tiles(team side);  bitboard champion_tiles();  bitboard camp_tiles();
  bitboard ward_tiles(team side); bitboard ward_tiles(); bitboard teleport_tiles(); bitboard altar_lock_tiles(team side); bitboard altar_lock_tiles();
  // every one also exists as `*_into(bitboard& out)`
  std::vector<champion> champions(team side);  std::vector<champion> champions();     // visible only; ~700 fuel per champion constructed
  std::vector<camp> camps();                                                           // visible camps (~250 fuel each)
  std::vector<map::camp_group> camp_groups();     // groups with any spawn tile in vision — 13k–17k fuel, it reloads map::camp_groups()
  champion champion_at(vec2 p);  camp camp_at(vec2 p);   // throw api_not_found / api_not_in_vision
  bool has_champion(vec2 p);  bool has_camp(vec2 p);  bool has_ward(vec2 p);  bool has_altar_lock(vec2 p);  bool has_teleport(vec2 p);   // false when hidden/absent
  std::vector<ward> wards();  std::vector<ward> wards(team);  ward ward_at(vec2);
  std::vector<altar_lock> altar_locks();  std::vector<altar_lock> altar_locks(team);  altar_lock altar_lock_at(vec2);  // .position() .team() .hits_remaining()
  std::vector<teleport> teleports();  teleport teleport_at(vec2);
}
std::vector<champion> champions(team side);  std::vector<champion> champions();   // cambc:: — ALL handles incl. hidden/dead (3.1k fuel for 5)
```

### Map (static; cached references cost ~10 fuel, `camp_groups()` does NOT cache)

```cpp
namespace map {
  std::int32_t width();  std::int32_t height();  bool is_in_bounds(vec2);  symmetry symmetry();
  const bitboard& walls();  const bitboard& bushes();  const bitboard& holes();  const bitboard& altars();  const bitboard& spawn_region(team);
  vec2 reflect_horizontal(vec2);  vec2 reflect_vertical(vec2);  vec2 rotate_180(vec2);     // ~104 fuel each (host calls)
  std::vector<camp_group> camp_groups();   // 11k–15k fuel: CACHE IT ONCE (static); also works in the draft
  camp camp(vec2 spawn_position);          // throws std::out_of_range if no camp spawns there
}
class map::camp_group { std::uint32_t gold_value(), gem_value() const; std::int32_t max_respawn_timer() const; std::span<const camp> camps() const;
  bitboard spawn_tiles() const; camp_group_state state() const; /* variant<camp_group_alive, camp_group_reward_available, camp_group_respawning{timer}>; throws api_not_in_vision unless a spawn tile is visible (4k fuel when it throws) */ };
class camp : public unit { camp_type kind() const; vec2 spawn_position() const; map::camp_group camp_group() const /* reloads, ~1k */;
  camp_state state() const /* dead / alive(=awake) / asleep; needs vision */; bool is_dead()/is_alive()/is_awake()/is_asleep() const;
  float health(), max_health(), strength(), focus() const; vec2 position() const; bool is_visible() const; camp_stats base_stats() const; std::int32_t attack_range() const; };
```

### Bitboard and distance field (engine-native; costs in FUEL.md)

```cpp
class bitboard {            // empty by default; bitboard{uninitialized} skips zeroing
  bool is_set(vec2) const;  void set(vec2, bool value = true);  void toggle(vec2);  void clear();  void fill();  bool is_empty() const;  std::uint32_t count() const;
  std::vector<vec2> positions() const;  std::uint32_t positions_into(std::span<vec2> out) const;
  bitboard operator| & ^ ~;  &= |= ^=;  operator==;  bitboard shift_right(int) / shift_up(int) / reflect(symmetry) const (+ *_into)
  void draw_circle(vec2 c, float r);  void draw_chebyshev(vec2 c, float r);                       // ADDS tiles (clear() first)
  bitboard reachable_tiles(vec2 origin, std::uint16_t distance) const;                            // *this = walls; 8-dir BFS
  bitboard reachable_tiles(vec2, std::uint16_t, bitboard& frontier, bitboard& scratch) const;      // no allocation (85 fuel on 30x30)
  bitboard shadow_cast(vec2 origin, std::uint16_t distance) const;                                // *this = opaque tiles
  std::vector<std::uint8_t> export_linear_buffer() const;  void import_linear_buffer(std::span<const std::uint8_t>);  // row-major, 1 byte per tile
};
class distance_field {      // uint16 per tile; unreachable = 0xFFFF
  void compute(const bitboard& walls, const bitboard& roots, bitboard& visited, std::uint16_t max_distance = unreachable);
  void compute_to(const bitboard& walls, const bitboard& roots, const bitboard& destinations, bitboard& visited, std::uint16_t max_distance = unreachable);  // stops when destinations reached
  std::uint16_t get(vec2) const;  void set(vec2, std::uint16_t);
  std::optional<vec2> next_move_from(vec2 from) const;  std::vector<vec2> next_moves_from(vec2 from) const;   // direction delta towards a root (9 get() calls ≈ 930 fuel)
  std::vector<std::uint16_t> export_linear_buffer() const;    // row-major; iterate this instead of calling get() per tile
};
```

### vec2 and helpers

`vec2{x,y}` with `+ - * /`, `chebyshev_distance(vec2)` (int64), `dot/cross/length_squared`, `reflect_horizontal/vertical`,
`rotate_180/cw/ccw`; constants `vec2::north` = (0,1) … compass order N, NE, E, SE, S, SW, W, NW. `to_string(x)` exists for
every enum, `vec2`, `error_code`. `debug::draw_dot(vec2, 0xRRGGBB)` / `draw_line(...)` draw in the visualiser for one round.
`debug_block({...})` runs fuel-free only in debug builds and is compiled out with `--release`.

### Gotchas (all observed in the probe or stated in cambc.h)

1. **Never let an exception skip `end_turn()`** — a suspended/unyielded turn forfeits the rest of the turn; the starter's `main()` loop shape is mandatory.
2. Hidden-target throws are expensive (3.5k) and "known-illegal action" throws too (3.8k): gate with `is_visible()`, `can_move()`, `attack_tiles().is_set(p)`, `is_ready()`.
3. `vision::champions()` / `cambc::champions()` construct full champion views (2 host calls per ability slot each): ≈700 fuel per champion. Keep the `std::vector<champion>` for the turn; do not re-query in loops.
4. `map::camp_groups()` costs 11–15k per call and `vision::camp_groups()` 13–17k: cache the groups (and their `spawn_tiles()`) in a `static` on the first turn; use `vision::visible_tiles()` + cached spawn tiles to decide which groups are observable, then call `state()` only on those.
5. `move_towards()` recomputes 7 bitboards + a distance field each call (1.7k on 30×30, 3.3k on 100×100, 6.3k on 50×50 contest): fine once per turn, not inside a search.
6. Design champions (`champion_designs[i]`) have no live state; enemy views answer name/team/alive/abilities-info but not position/health unless visible.
7. `gold_count()` is self-only (throws `std::invalid_argument` on allies); `gem_count()` works on any visible champion.
8. `draft_memory` is a snapshot taken at bot startup (fine, it is frozen after the draft); `shared_memory` reads/writes always hit the host.
9. `draw_chebyshev/draw_circle` add to the board; call `clear()` first (`attack_tiles_into` does).
10. `distance_field` default-constructed reads as zeros until `compute*`; `next_move_from` returns `nullopt` at a root or an unreached tile.
11. `camp_group::state()` throws unless one of its spawn tiles is in your vision — even for a group you cleared yourself.
12. `std::cout` costs ≈1.3k fuel per int and ≈4k per float; `printf` ≈1.4k/2.3k. Telemetry must be rare or hand-formatted (see FUEL.md).
