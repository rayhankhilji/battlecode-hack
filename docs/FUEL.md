# FUEL — measured C++ fuel costs in the real engine

Method: `bots/probe` (C++, `cambc build --release`, `-O3 -DNDEBUG`, wasm exceptions on) wraps each call in
`fuel_remaining()` before/after and prints the delta; one block of calls per turn so no turn exceeds its budget (rows marked
as suspension artefacts are the few that still did). Measured on three maps: elkring (30×30), contest (50×50), ambush
(100×100). Values are deterministic (identical on repeated turns and across maps for map-independent calls). Each row includes
the ≈5 fuel of the two `fuel_remaining()` calls (`noop` row). Raw rows: `F <round> <name> <fuel>` in the replays' `team-a.stdout`.

## Budgets observed

| Turn | Budget | Measured at the top of `main()`'s loop | Where the rest went |
|---|---|---|---|
| Draft turn 1 | 1,000,000 | **979,258** (probe, includes `<iostream>`), **991,152** (minimal bot, `<cstdio>` only) | static initialisation before `main()`: SDK globals ≈ 8.8k (game settings, 4 terrain bitboards, 2 spawn bitboards, 12 champion designs with 2 ability queries each, draft-memory read, consumable info), plus ≈ 12k for libc++ iostream init |
| Draft turns 2..n | 100,000 | 99,821 (probe) / 99,994 (minimal) | loop overhead only |
| Champion first play turn | 200,000 | **179,012** on 30×30, 178,862 on 50×50, **177,530** on 100×100 (probe); **190,754** (minimal bot) | same static init; terrain bitboards cost more on bigger maps |
| Every later play turn | 100,000 | **99,979** after one `is_draft()` + one `fuel_remaining()` | — |

So a bot that includes `<iostream>` has ≈179k, not 200k, on its first turn; one that formats output by hand has ≈191k.
Unused fuel does not carry over [rules]. Running out suspends the instance mid-instruction and resumes it next round; the
probe's suspended rows came back as *negative* deltas, i.e. the code after the suspension point ran with a fresh budget and
stale state — exactly the failure mode BRIEF.md warns about.

Realistic per-turn cost of the probe's starter-like `farm()` (attack-in-range check over `vision::camps()`, nearest-camp
scan, 6 bitboard unions, one `compute_to` on the full map, `next_move_from`, `move_absolute`): **11.7k** on 30×30, **12.2k** on
50×50, **26.9k** on 100×100 (fuel left at turn end 88.2k / 87.8k / 73.1k of 99,979). The full-map BFS is almost all of the
difference between maps.

## Measured costs per call

| Call (as measured) | elkring 30x30 | contest 50x50 | ambush 100x100 | Note |
|---|---|---|---|---|
| `noop` | 5 | 5 | 5 | two fuel_remaining() calls back to back: subtract from every row for the pure call cost |
| `bitboard.default_ctor` | 253 | 267 | 387 |  |
| `bitboard.uninit_ctor` | 224 | 205 | 205 |  |
| `bitboard.copy_ctor` | 252 | 266 | 386 |  |
| `bitboard.clear` | 34 | 67 | 187 |  |
| `bitboard.fill` | 34 | 67 | 187 |  |
| `bitboard.set` | 27 | 27 | 27 |  |
| `bitboard.set_false` | 27 | 27 | 27 |  |
| `bitboard.is_set` | 42 | 42 | 42 |  |
| `bitboard.is_set_x10` | 346 | 346 | 346 |  |
| `bitboard.count` | 46 | 79 | 199 |  |
| `bitboard.is_empty` | 49 | 82 | 202 |  |
| `bitboard.positions(full)` | 8438 | 23099/22904 | 90839/90644 | ~9.4 fuel per set tile; a full 100x100 board is 90k = a whole turn |
| `bitboard.positions(cheb8)` | 1076/1985 | 1709/2249 | 1949 | 289 tiles clipped at the map edge; varies with the count |
| `bitboard.positions_into(cheb8,vec400)` | 3337/3438 | 3433/3493 | 3553 | the std::array<vec2,400> zero-init costs more than the call |
| `bitboard.draw_chebyshev8` | 275 | 289/291 | 409 |  |
| `bitboard.draw_chebyshev1` | 272 | 286 | 406 |  |
| `bitboard.draw_circle8` | 275 | 289/291 | 409 |  |
| `bitboard.or` | 257 | 271 | 391 |  |
| `bitboard.or_assign` | 38 | 71 | 191 |  |
| `bitboard.and` | 257 | 271 | 391 |  |
| `bitboard.and_assign` | 38 | 71 | 191 |  |
| `bitboard.not` | 255 | 269 | 389 |  |
| `bitboard.xor` | 257 | 271 | 391 |  |
| `bitboard.equals` | 49 | 82 | 202 |  |
| `bitboard.shift_right1` | 256 | 270 | 390 |  |
| `bitboard.reflect` | 259 | 273 | 393 |  |
| `bitboard.export_linear` | 372 | 586 | 1643 |  |
| `bitboard.reachable_tiles(1)` | 267 | 281 | 401 |  |
| `bitboard.reachable_tiles(2)` | 268 | 282/286 | 402 |  |
| `bitboard.reachable_tiles(8)` | 304 | 318/338 | 438 |  |
| `bitboard.reachable_tiles(40)` | 872/792 | 1790/2042 | 1910 |  |
| `bitboard.reachable_tiles(200)` | 872/792 | 2679 | 23213 | BFS cost grows with the reached area (whole 100x100 board = 23k) |
| `bitboard.reachable_tiles_into(8,scratch)` | 85 | 118/138 | 238 | no allocation: 3x cheaper than the allocating form |
| `bitboard.shadow_cast(8)` | 332/394 | 418/453 | 658 |  |
| `bitboard.shadow_cast(6)` | 313/365 | 393/411 | 633 |  |
| `bitboard.shadow_cast(8,centre)` | 451 | 391 | 599 |  |
| `map.walls()(cached ref)` | 10 | 10 | 10 |  |
| `map.spawn_region(ally)(cached)` | 10 | 10 | 10 |  |
| `distance_field.default_ctor` | 185 | 380/243 | 185/243 |  |
| `distance_field.compute(full map,1 root)` | 811 | 3212 | 25208/25039 | full-board BFS: 0.8k / 3.2k / 25k |
| `distance_field.compute(max 10)` | 620 | 1478/1536 | 4598/4656 | max_distance caps the work |
| `distance_field.compute(altars as roots)` | 461 | 989/1047 | 2909/2967 |  |
| `vision.visible_tiles` | 281 | 326 | 446 |  |
| `vision.champion_tiles(ally)` | 281 | 326 | 446 |  |
| `vision.champion_tiles(enemy)` | 281 | 326 | 446 |  |
| `vision.champion_tiles()` | 574 | 724 | 1084 |  |
| `vision.camp_tiles` | 281 | 326 | 446 |  |
| `vision.ward_tiles` | 574 | 724 | 1084 |  |
| `vision.teleport_tiles` | 281 | 326 | 446 |  |
| `vision.altar_lock_tiles` | 574 | 724 | 1084 |  |
| `vision.champions(ally)` | 3701 | 3807 | 4167 | 5 allies: ~700 per champion view constructed |
| `vision.champions(enemy)` | 551 | 604 | 964 | no enemy visible: just the occupancy board |
| `vision.champions()` | 3971 | 3996 | 4596 |  |
| `vision.camps` | 1552/1803 | 552/918 | 912/1023 | per visible camp ~250 |
| `vision.wards` | 103 | 103 | 103 |  |
| `vision.altar_locks` | 105 | 105 | 105 |  |
| `vision.teleports` | 105 | 105 | 105 |  |
| `vision.has_champion` | 115 | 115 | 115 |  |
| `vision.has_camp` | 115 | 115 | 115 |  |
| `vision.has_ward` | 103 | 103 | 103 |  |
| `vision.has_altar_lock` | 434 | 500 | 758 |  |
| `vision.champion_at(self)` | 440 | 440 | 440 |  |
| `champions(ally)` | 3108 | 3108 | 3108 | all 5 handles incl. hidden |
| `champions(enemy)` | 3574 | 3558 | 3072 |  |
| `champions()` | 9454 | 9438 | 8781 |  |
| `round_number` | 23 | 23 | 23 |  |
| `global_submitted_gems` | 24 | 24 | 24 |  |
| `ward.visible_tiles_from` | 384 | 401 | 641 |  |
| `self.visible_tiles` | 315 | 302 | 422 |  |
| `self.visible_tiles_from` | 403 | 423 | 663 |  |
| `map.reflect_horizontal` | 104 | 104 | 104 |  |
| `map.altars().positions` | 414 | 452 | 710 |  |
| `map.walls().count` | 41 | 74 | 194 |  |
| `map.camp_groups` | 11063 | 13686 | 15085 | 10 / 14 / 12 groups: CACHE |
| `camp_group.spawn_tiles` | 340 | 685 | 631 |  |
| `camp_group.copy` | 417 | 415 | 631 |  |
| `map.camp(pos)` | 129 | 129 | 129 |  |
| `camp.camp_group()` | 983 | 782 | 1223 |  |
| `camp.base_stats` | 165 | 165 | 165 |  |
| `camp.attack_range` | 180 | 180 | 180 |  |
| `vision.camp_groups` | 13016 | 15421 | 17181 | reloads map::camp_groups() every call |
| `camp_group.state(visible)` | 449 | - | - |  |
| `camp.state(visible)` | 47 | - | - |  |
| `camp.health(visible)` | 50 | - | - |  |
| `camp.position(visible)` | 52 | - | - |  |
| `camp.is_visible(visible)` | 57 | - | - |  |
| `camp.buffs(visible)` | 137 | - | - |  |
| `camp_group.state(hidden,throws)` | 4150 | 4510 | 4552 | cost of the api_not_in_vision throw+catch |
| `camp.is_visible(hidden)` | 52 | 52 | 52 |  |
| `camp.health(hidden,throws)` | 3342 | 3486 | 3416 | same |
| `self()` | 16 | 16 | 16 |  |
| `self.position` | 81 | 81 | 81 |  |
| `self.health` | 81 | 81 | 81 |  |
| `self.max_health` | 81 | 81 | 81 |  |
| `self.strength` | 81 | 81 | 81 |  |
| `self.focus` | 81 | 81 | 81 |  |
| `self.state` | 59 | 59 | 59 |  |
| `self.can_move` | 63 | 63 | 63 |  |
| `self.is_alive` | 55 | 55 | 55 |  |
| `self.is_visible` | 76 | 76 | 76 |  |
| `self.champion_name` | 37 | 37 | 37 |  |
| `self.team` | 48 | 48 | 48 |  |
| `self.passive` | 184 | 184 | 184 | reloads the design every call |
| `self.base_stats` | 185 | 185 | 185 | reloads the design every call |
| `self.attack_range` | 81 | 81 | 81 |  |
| `self.attack_tiles` | 460 | 506 | 626 |  |
| `self.gem_count` | 48 | 48 | 48 |  |
| `self.gold_count` | 60 | 60 | 60 |  |
| `self.item_count` | 219 | 219 | 219 |  |
| `self.buffs` | 123 | 123 | 123 |  |
| `self.restrictions` | 48 | 48 | 48 |  |
| `self.abilities()` | 107 | 107 | 107 |  |
| `ability.cooldown` | 52 | 52 | 52 |  |
| `ability.is_ready` | 481 | 481 | 481 | cooldown + a fresh champion view of the owner |
| `ability.info()` | 16 | 16 | 16 |  |
| `ability.valid_targets` | 48 | 48 | 48 |  |
| `ability.valid_tiles(ranged slot)` | 426 | 505 | 745 |  |
| `ability.damage_amount` | 54 | 54 | 54 |  |
| `self.ward_cooldown` | 55 | 55 | 55 |  |
| `self.ward_positions` | 121 | 121 | 121 |  |
| `self.ward_placement_tiles` | 402 | 448 | 568 |  |
| `self.can_buy(item)` | 64 | 64 | 64 |  |
| `self.can_buy(consumable)` | 61 | 61 | 61 |  |
| `self.priority` | 50 | 50 | 50 |  |
| `consumable.count` | 54 | 54 | 54 |  |
| `shared_memory.read8` | 82 | 82 | 82 |  |
| `shared_memory.write8` | 78 | 78 | 78 |  |
| `shared_memory[0]` | 32 | 32 | 32 |  |
| `draft_memory[0]` | 17 | 17 | 17 |  |
| `champion_designs[0].base_stats` | 168 | 168 | 168 |  |
| `champion_designs[0].abilities()[0].info` | 103 | 103 | 103 |  |
| `item_type.info` | 37 | 37 | 37 |  |
| `consumable_type.cost` | 39 | 39 | 39 |  |
| `fuel_remaining` | 17 | 17 | 17 |  |
| `enemy.champion_name(hidden ok)` | 38 | 38 | 38 |  |
| `enemy.team(hidden ok)` | 46 | 46 | 46 |  |
| `enemy.is_visible` | 57 | 57 | 57 |  |
| `enemy.position(hidden,throws)` | 3603 | 3461 | 3605 | same |
| `enemy.is_alive(hidden)` | 53 | 53 | 53 |  |
| `enemy.abilities()[0].info(hidden)` | 106 | 106 | 106 |  |
| `ally.position` | 80 | 80 | 80 |  |
| `ally.gold_count(not self,throws)` | 2454 | 2459 | 2459 | std::invalid_argument throw+catch |
| `ally.gem_count` | 49 | 49 | 49 |  |
| `loop1000_add` | 2174 | 2174 | 2174 | ~2.2 fuel per iteration (mul+add+compare) |
| `loop1000_array_write` | 4539 | 4539 | 4539 | ~4.5 per store |
| `loop1000_array_sum` | 13 | 13 | 13 | vectorised (simd128): 1000 loads for 13 fuel |
| `vector_push100` | 7795 | 7743 | 7895 | ~78 per push_back (growth + libc++ overhead) |
| `vector_reserve_push100` | 5036 | 5094 | 5049 | ~50 per push_back even with reserve |
| `new_delete_64B` | 240 | 282 | 212 |  |
| `new_delete_8KB` | 188 | 188 | 188 |  |
| `out.s+i+nl(cheap)` | 62 | 62 | 62 | hand-formatted int line into a buffer (bots/probe/src/out.h) |
| `out.f(cheap float)` | 555 | 555 | 555 | hand-formatted float (2 decimals) |
| `out.flush(fwrite)` | 174 | 174 | 174 | one fwrite of the buffer |
| `printf_first_use` | 1356 | 1356 | 1356 |  |
| `printf_int` | 1356 | 1356 | 1356 |  |
| `printf_float` | 2349 | 2349 | 2349 |  |
| `puts_80chars` | 638 | 638 | 638 |  |
| `fflush` | 147 | 147 | 147 |  |
| `cout_first_use` | 1486 | 1486 | 1486 | iostream static init is paid before main() (~12k), not here |
| `cout_short_int` | 1344 | 1344 | 1344 |  |
| `cout_80chars` | 337 | 337 | 337 |  |
| `cout_float` | 4040 | 4040 | 4040 | iostream float formatting |
| `cout_endl(flush)` | 937 | 937 | 937 |  |
| `cout_to_string(vec2)` | 1241 | 1241 | 1241 |  |
| `self.move_towards(centre)` | 1703 | 6299 | 3264 | 7 bitboard ops + compute_to + reachable_tiles; contest value hit a long detour |
| `self.move_towards(again,noop)` | 1703 | 185 | 3264 |  |
| `self.place_ward` | 139 | 139 | 139 |  |
| `self.set_priority` | 36 | 36 | 36 |  |
| `self.can_sell` | 62 | 62 | 62 |  |
| `self.attack(invalid,throws)` | 3847 | 3678 | 3821 | known-illegal action: throw+catch |
| `self.move(blocked,throws)` | 3789 | 3664 | 3791 | known-illegal action: throw+catch |
| `sort100` | 23613 | 23613 | 23613 | std::sort of 100 ints: 236 per element |
| `chebyshev_x100` | 2289 | 2289 | 2289 |  |
| `grid_scan_30x30` | 9177 | 9177 | 9177 | 900 cells with a 64-bit accumulator |
| `grid_scan_50x50` | 24279 | 24279 | 24279 | 2500 cells |
| `grid_scan_50x50_u32` | 14133 | 14133 | 14133 | ~5.7 per cell with 32-bit arithmetic (64-bit ops are libcalls/slow in wasm32) |
| `float_mul_x1000` | 4912 | 4912 | 4912 | ~4.9 per float mul+add |
| `std::string_concat10` | 2369 | 2341 | 2341 |  |
| `to_string(vec2)` | 694 | 694 | 694 |  |
| `try_catch_throw(cambc::error)` | 2872 | 2876 | 2876 | string construction included |
| `try_catch_throw(int)` | 1228 | 1228 | 1228 |  |
| `try_no_throw` | 11 | 11 | 11 |  |
| `distance_field.compute_to(centre)` | 925 | 2067 | 23865 | stops when destinations reached; worst case = full compute |
| `distance_field.compute_to(3 away)` | 509 | 1136 | 3416 |  |
| `distance_field.get` | 85 | 85 | 85 |  |
| `distance_field.get_x10` | 838 | 838 | 838 | ~84 per get(): host call per tile |
| `distance_field.next_move_from` | 928 | 928 | 928 | 9 get() calls |
| `distance_field.next_moves_from` | 1263 | 1268 | 1268 |  |
| `distance_field.copy` | 461 | 989 | 2909 |  |
| `distance_field.export_linear` | 2288 | 5888 | 22763 | copy out the whole field as uint16[] |
| `distance_field.export_linear+scan` | 5490 | 14690 | -42187 | export + summing every element (~3.5 fuel/element); ambush value is a suspension artefact (expected ~58k) |
| `grid_scan_30x30_is_set` | 32842 | 32842 | 32842 | 900 bitboard.is_set() = 36 per tile: 100x100 would be 365k = 3.6 turns |
| `grid_scan_30x30_linear_byte` | 13195 | 13382 | 14439 | 900 byte reads from export_linear_buffer(): ~15 per cell incl. a modulo |
| `distance_field.compute_with_expansion_count` | 928 | 3246 | 23361 |  |
| `distance_field.compute(walls only)` | 877 | 3243 | 23358 |  |

## What the numbers mean

**Host calls are flat-rate, ~25–45 fuel, plus memory traffic proportional to the board for whole-board ops.**
`is_set`/`set` ≈ 27–42, `count`/`is_empty` 46→199 with map area, `|`/`&`/`~`/`^` returning a new board ≈ 255 (30×30) → 390
(100×100) of which ≈ 220 is the allocation (`bitboard{uninitialized}` = 205–224; a default-constructed board adds a `clear()`); the
in-place `|=`/`&=` forms cost only 38→191. `draw_chebyshev`/`shadow_cast(8)`/`reachable_tiles(≤8)` are all 270–660 including the
allocation, independent of radius for small radii. `vision::*_tiles()` ≈ 280–450. Reading a champion's own state (`position`,
`health`, `state`, `gem_count`…) is 50–80 each; these are *fresh* host calls every time — read once per turn into locals.

**Board-proportional work:** `distance_field::compute` is 0.8k / 3.2k / 25k for 900 / 2,500 / 10,000 tiles (≈2.5 fuel per tile)
and `compute_to` is the same in the worst case (centre target), but `max_distance` and near destinations cap it (3 steps away:
0.5k / 1.1k / 3.4k). `positions()` is ≈9.4 fuel per set tile; `export_linear_buffer` on a field is ≈2.3 fuel per tile and on a bitboard
≈0.4 per tile (1 byte each, 372 → 1,643).

**Wasm arithmetic** is cheap but not free: ≈2.2 fuel per simple loop iteration, ≈4.5 per array store, ≈5.7 per cell for a 32-bit
grid scan (≈10 with a 64-bit accumulator: avoid `std::int64_t`/`std::size_t` arithmetic in hot loops; `vec2::chebyshev_distance`
is int64 and costs ≈23). A 100×100 scan with a trivial body is therefore ≈57k–100k fuel — most of a turn. `std::sort` of 100
ints is 23.6k; `std::vector::push_back` is 50–78 each; `new`/`delete` ≈ 200; one `throw`+`catch` is 1.2k (int) to 2.9k
(`cambc::error`) and the engine's own "invalid action / not in vision" throws cost 3.4k–4.5k including the host call.

**Output:** `std::cout << int` ≈ 1.3k, `<< float` ≈ 4k, `printf("%d")` ≈ 1.4k, `printf("%f")` ≈ 2.3k, a plain 80-char string 340–640.
Hand-formatted integers into a buffer flushed with one `fwrite` cost ≈ 60 per line (+174 per flush). The probe's draft dump
(≈150 lines, dozens of floats) cost 440k on elkring and 790k on lanes (42 groups) with hand formatting, and **exhausted 1M with iostream floats before finishing the items table** — that is how we
discovered that a fuel-starved draft turn gets random picks.

## Guidance for the bot

Cheap enough to do **per tile per turn** (budget: a 100×100 scan must stay under ~5 fuel/tile to leave room for everything else):

- Read a bitboard through `export_linear_buffer()` once (0.4 fuel/tile) and index the byte array, or better, keep your own
  `std::uint8_t`/`std::uint16_t` grids — **never** call `is_set()` per tile (36–42/tile: a 100×100 pass is 3.6 turns).
- Read a distance field through `export_linear_buffer()` (2.3/tile) when you need many lookups; `get()` (84) is fine for a handful.
- Per-tile arithmetic with 32-bit ints, no allocation, no virtual calls.

Cheap enough to do **a few times per turn** (hundreds of fuel): any bitboard union/mask/`draw_chebyshev`/`shadow_cast`/
`reachable_tiles(≤8)`, `vision::*_tiles()`, `self().visible_tiles()`, one `distance_field.compute_to` with a close destination or a
`max_distance`, `next_move_from`, every `self()` getter, `shared_memory` read/write, `attack_tiles()`, `valid_tiles()`,
`damage_amount()`.

Do **once per turn** and keep the result (1k–5k): `vision::champions()` (≈700 per visible champion), `vision::camps()` (≈250 per
camp), `cambc::champions(team)` (≈600 per handle), `self().abilities()` (107, but `is_ready()` is 481 because it rebuilds a
champion view: call `cooldown()` + `can_cast()` yourself), a full-map `distance_field::compute` on small/medium maps (0.8–3.2k),
`move_towards` (1.7k–6.3k; it rebuilds 7 boards and a field).

Do **once per game** and cache in `static`s: `map::camp_groups()` (11k–15k per call; 13k–17k for `vision::camp_groups()`),
each group's `spawn_tiles()` (340–685), `champion_designs[*]`/`base_stats()`/`passive()` (170–185 each: they reload the design),
`item_type::info()`, your own terrain-derived grids (walkability, altar-adjacent tiles, per-altar and per-group distance fields).
On 100×100 maps a full-board `compute` is 25k — a quarter of a turn — so precompute static fields in the **draft** (the draft bot has
979k and can write 1,024 bytes of draft memory; it cannot ship a 20 KB field, so ship *routes/decisions*, not grids) or
in the champion's first turn (179k) and refresh at most one dynamic field per turn, with `max_distance` or `compute_to`.

Avoid entirely: `std::cout`/`printf` with floats except in draft; exceptions as control flow (gate with `is_visible()`,
`can_move()`, `attack_tiles().is_set()`, `cooldown()==0 && can_cast()`); `positions()` on large boards (use `positions_into` with a
small buffer or iterate a linear export); `std::sort` on more than a few dozen elements; `std::vector` growth in hot paths (use
`std::array` or reserve-once statics); 64-bit arithmetic in loops; re-querying champion views in nested loops.

Safety margin: the probe's measurement turns show a turn can quietly burn 100k with ~15 host calls of the wrong kind
(`positions(full)` on ambush = 91k by itself). Check `fuel_remaining()` before each optional phase (planning, telemetry) and end the
turn with ≥ 10k spare; print the per-champion `fuel_min` telemetry line only every 100 rounds (≈ 60 fuel hand-formatted).
