# bots/apex — economy bot (C++20)

Build / run:

```
cambc build bots/apex --release
cambc run bots/apex bots/starter-cpp --map elkring --release --statistics s.json --json
python3 tools/arena.py bots/apex bots/starter-cpp --jobs 8 --out replays/<name>
```

## Architecture

| file | role |
|---|---|
| `src/main.cpp` | loop: draft or play turn inside try/catch, then `end_of_turn()` (claim write, telemetry) and `cambc::end_turn()` |
| `src/common.h` | includes, fuel constants, 32-bit `cheb()`, role/camp tables |
| `src/model.h` | static champion model from engine designs (dps, poke dps, stuns, shields) and `estimate_fight()` |
| `src/plan.h` | draft-memory `Plan` layout, live shared-memory `Claim` packing |
| `src/draft.cpp` | roster table, per-map static plan (squads + circuits), draft-memory encoding |
| `src/bot.h/.cpp` | per-champion state machine, pathing, kit use, shopping, telemetry |
| `src/combat.h` | `combat_hook()` called before economy decisions (trivial for this wave) |

Draft (first turn, 1M fuel): pick first, then plan. The roster table is `ROSTER` in `draft.cpp`
(Cassi & Issac, Stellar, Ren, Varg, Ryth, ... prefix of `draft::team_size()`); enemy mirrors are allowed.
The planner reads every camp group, computes path distances (spawn, enemy spawn, each altar) with engine
distance fields, models each body from `champion_designs`, evaluates 1..3 squads (melee-first round robin,
Cassi always with Issac) and builds a greedy circuit per squad from delivered value per cycle
(`value / (travel + clear + heal + capture + amortised delivery)`). Feasibility uses the "front pool":
monsters hit only the nearest champion, so a squad can take a group if the expected damage fits in the sum
of its melee bodies' health above a 30% reserve; groups nobody can front fall back to range-4 poking
(zero damage taken). Monolith groups are excluded unless the map has fewer than 300 ordinary gems.

## Play state machine (per champion, every turn)

```
dead -> shop, idle claim
channel (tribute / recall / teleport) -> do nothing
stunned -> do nothing
combat_hook (attack enemy in range, punish adjacent enemies, flee when lethal)
RECOVER: at spawn: shop, wait for the full heal; hp < 35% -> leave monster/enemy range, recall
DELIVER: bag >= 36 (plan), near altar with >= 20, winning amount, or round horizon -> altar neighbour,
         place own lock if enemies near, submit_gems (TRIBUTE while channelling), attack enemy locks
FARM:    leader = lowest-rank alive squad mate with a claim (tank first); followers take its group
         TRAVEL  -> approach the group's tiles (dash when far)
         CLEAR   -> FRONT (melee, hp above reserve) / RANGED_ATTACK (behind a fronting melee) /
                    POKE (range-4 spells only) / WAIT (ring 4-6); retreat when hp < reserve
         CAPTURE -> every present member stands on a cleared tile until the group state leaves
                    RewardAvailable (reward and gold split evenly), ward dropped on the tile
```

Kit use (generic by ability kind): attack -> attack-reset -> attack when stationary; AoE/CC/damage spells
on monsters (exclusive-with-movement spells are only cast when no walk happened this turn); Varg shield when
incoming >= 40; dashes shorten travel (legal dash paths only). One cast per turn is enforced locally.

Pathing: goals bitboard -> `distance_field::compute_to` from the goals to our tile (cached for 6 rounds while
the goal set is unchanged), step to the reachable neighbour with the smallest distance, retry up to three
neighbours when `move()` returns false (hidden blocker). While poking/waiting, the strike zone (range 3) of
the target group is treated as blocked so nobody walks into a woken monster.

## Draft-memory layout (`plan.h`, 428 bytes, version 2)

```
u8 version, bodies, groups, squads, gold_weight_pct, bank_threshold, reserved[2]
GroupPlan g[64]: u8 altar, dist_spawn, dist_altar, flags (1 excluded, 2 home side)
SlotPlan  s[6]:  u8 type, role (0 tank 1 dps 2 caster), squad, rank, n, circuit[24] (group indices)
```
Group index = index into `map::camp_groups()` (equals the engine handle; identical in draft and play on all
17 maps). Slot i = i-th ally handle; champions match plan slots by champion type.

## Shared-memory layout (8 bytes)

Byte i (< 6) = slot i's claim: `phase << 6 | group` (group 63 = none; phase 0 idle, 1 clearing,
2 capturing, 3 delivering). Read at turn start, own byte written at turn end. Bytes 6-7 reserved.

## Telemetry

One line per champion every 100 rounds:
`T r=<round> slot=<i> role=<x> fmin=<min fuel at turn end> susp=<round jumps > 1> gems=<bag> banked=<team>
mode=<Mode> tgt=<group> prof=<max fuel: pre/snapshot/known/decide> pick=.. step=.. eng=.. del=..`
plus `init ...` on the first turn and `draft: ...` lines from the draft bot.

## Measured engine facts (release, cambc 2.2.5)

printf of a 40-byte line ~2,000 fuel; plain loop iteration ~2-9 fuel; `cheb` via vec2 ~23 (64-bit);
`map::camp_groups()` 11-25k (cached once); `distance_field::compute` ~0.8k (30x30) .. 25k (100x100);
`compute_to` scales with distance; `vision::champions()` ~700 per champion; thrown engine errors 1-4k each.
First play turn starts at ~179k fuel, draft turn at ~980k. Group handle == index. Spawn heal is full per
round. Monsters hit only the nearest champion in range 3; spells from range 4 are free.

## Known gaps (for the combat wave)

- `combat_hook` is trivial: attack in range, melee chase within 3 tiles when healthy, hurt ranged bodies
  step away, flee under 35% HP. No threat model, no focus fire, no kiting: contest (monolith-only) is won
  but with ~50 deaths per game because idle pokers get picked off.
- Stall handling is heuristic: 15 rounds without path progress marks a group unavailable for 60 rounds.
- Poke-only clears (stampede's 12-camp group, contest/monolith) are slow (~90 dps).
- No altar denial, no ward strategy beyond dropping wards on captured camps, no teleport crystals.
- Bank threshold is a global constant (36); no per-map tuning.

## Measured results (18 Oct, `tools/arena.py`, 17 maps x both seats, full 2,000-round games)

| opponent | result | mean finish round (wins) | apex faults / fuel suspensions / uncaught errors |
|---|---|---|---|
| starter-cpp | 34W 0L | 813 | 0 / 0 / 0 |
| starter-python | 34W 0L | 839 | 0 / 0 / 0 |
| spar-farmer | 22W 12L | 912 | 0 / 0 / 0 |
| spar-hunter | 23W 11L | 1040 | 0 / 0 / 0 |
| spar-locker | 23W 11L | 935 | 0 / 0 / 0 |

Peak fuel used in any non-first play turn: 71k of 100k (lanes); first play turn ~95-110k of ~179k;
first draft turn 118k-779k of ~980k (lanes is the heaviest, bounded by guards).
Per match (apex gems - opponent gems, rounds):

| map | seat | vs starter-cpp | vs starter-python | vs spar-farmer | vs spar-hunter | vs spar-locker |
|---|---|---|---|---|---|---|
| ambush | A | W 1018-394 r946 | W 1018-570 r946 | L 813-1057 r869 | L 788-1092 r879 | L 919-1023 r912 |
| ambush | B | W 1062-322 r866 | W 1062-216 r866 | L 924-1021 r827 | W 1022-986 r987 | W 1062-914 r869 |
| collision | A | W 1016-44 r719 | W 1008-44 r709 | W 628-176 r2000 | W 1028-176 r815 | L 416-1024 r831 |
| collision | B | W 1006-44 r646 | W 1008-66 r670 | W 1006-0 r841 | W 1018-220 r821 | L 854-1074 r980 |
| contest | A | W 600-200 r2000 | W 800-400 r2000 | L 200-200 r2000 | W 400-0 r2000 | W 1000-0 r1805 |
| contest | B | W 1000-0 r1520 | W 1000-0 r1944 | W 800-0 r2000 | W 400-0 r2000 | W 600-0 r2000 |
| corners | A | W 1014-44 r573 | W 1024-116 r559 | W 1016-440 r631 | W 1008-366 r814 | W 1000-608 r579 |
| corners | B | W 1022-36 r601 | W 1030-58 r562 | W 1016-616 r651 | W 1008-396 r643 | W 1002-694 r739 |
| eight | A | W 1009-88 r528 | W 1038-66 r504 | W 1045-342 r1443 | W 755-182 r2000 | L 770-1024 r784 |
| eight | B | W 1020-44 r467 | W 1028-22 r483 | W 1014-226 r571 | W 1017-0 r1490 | W 1014-460 r673 |
| elkring | A | W 1008-108 r515 | W 1008-72 r571 | W 1008-432 r522 | W 1008-288 r772 | W 1008-252 r594 |
| elkring | B | W 1008-72 r479 | W 1008-144 r524 | W 1026-504 r594 | W 1008-198 r603 | W 1008-864 r804 |
| expanse | A | W 1042-726 r1103 | W 1042-792 r1103 | W 1000-732 r1050 | W 1014-900 r1119 | W 1012-922 r1134 |
| expanse | B | W 1004-440 r977 | W 1004-418 r977 | W 1012-790 r1181 | L 868-1040 r1101 | W 1000-884 r1198 |
| fragment | A | W 1000-182 r885 | W 1016-99 r757 | L 522-1023 r1649 | L 661-1159 r1954 | W 1009-201 r868 |
| fragment | B | W 1020-146 r780 | W 1001-132 r755 | W 1009-176 r831 | W 1020-0 r1259 | W 1002-190 r826 |
| interlock | A | W 1012-452 r734 | W 1004-446 r788 | L 952-1034 r1032 | L 966-1018 r1006 | L 966-1018 r1004 |
| interlock | B | W 1004-374 r735 | W 1012-402 r812 | W 1010-770 r970 | W 1014-374 r1171 | L 464-1034 r904 |
| lanes | A | W 1028-322 r491 | W 1014-336 r496 | W 1022-968 r544 | W 1004-946 r662 | W 1000-814 r536 |
| lanes | B | W 1014-402 r559 | W 1028-468 r575 | L 590-1026 r458 | W 1012-974 r790 | W 1014-770 r588 |
| monolith | A | W 1032-88 r435 | W 1010-154 r500 | W 1002-432 r561 | L 828-1002 r738 | W 1018-658 r665 |
| monolith | B | W 1032-256 r548 | W 1010-176 r488 | W 1010-850 r649 | W 1004-740 r748 | W 1018-732 r687 |
| packs | A | W 1032-844 r709 | W 1042-556 r744 | L 576-1040 r568 | W 1030-920 r714 | L 760-1040 r739 |
| packs | B | W 1020-292 r545 | W 1006-384 r546 | L 752-1040 r579 | W 1068-832 r641 | W 1028-736 r588 |
| pilgrimage | A | W 1028-0 r1721 | W 1028-0 r1721 | W 1016-570 r837 | L 518-1006 r887 | L 490-1078 r1100 |
| pilgrimage | B | W 1012-0 r1319 | W 1012-0 r1319 | L 466-1064 r1048 | W 1094-978 r1087 | W 1018-914 r1172 |
| rows | A | W 1034-572 r725 | W 1056-594 r788 | L 660-1056 r610 | L 759-1056 r703 | L 594-1023 r646 |
| rows | B | W 1012-594 r859 | W 1056-528 r833 | L 814-1056 r695 | L 792-1056 r695 | L 693-1056 r699 |
| rushbait-v2 | A | W 1000-154 r834 | W 1008-308 r736 | W 1022-848 r867 | W 1074-906 r1051 | W 1036-968 r1077 |
| rushbait-v2 | B | W 1008-132 r707 | W 1000-308 r674 | W 1008-862 r1113 | W 1014-718 r1039 | W 1016-878 r1007 |
| stampede | A | W 1080-0 r925 | W 1080-0 r819 | W 1008-0 r714 | L 0-1080 r733 | L 180-1260 r1052 |
| stampede | B | W 1044-180 r1102 | W 1020-540 r1304 | W 1008-360 r887 | L 0-1080 r738 | W 1080-720 r1153 |
| swirl | A | W 1010-144 r513 | W 1016-90 r690 | W 1002-272 r611 | W 1009-371 r682 | W 1002-252 r797 |
| swirl | B | W 1012-45 r589 | W 1023-108 r750 | L 692-1012 r769 | L 164-1042 r699 | W 1041-760 r1136 |
