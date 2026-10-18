# Electric Muffin v1 vs Mite (ladder #2): what Mite does differently

Two ladder games, 18 Oct 2026, decoded with `tools/replay_dump.mjs` (official engine WASM, spectator view) and
`tools/replay_analyse.py`; bundles in `replays/ladder/`. We are team A in both, Mite is team B, we lost both:
**ambush** (100×100, 5 picks) 383–1002 at round 697 and **rushbait-v2** (30×80, 4 picks) 402–1016 at round 490.
Both of our bots ran clean: 0 fuel suspensions, 0 bot errors, 0 faults (engine fuel log via `tools/telemetry.py`).
The deficit is entirely in play. The generated per-game tables follow the findings.

## Findings (the numbers behind each are in the tables below)

1. **Mite never idles; we do.** Idle fraction (alive, not channelling, no action at all in the turn): Mite 4.5% / 10%,
   us 23.8% / 27.8%. On ambush our six bodies spent **rounds 201–450 at 40–48% idle with an empty bag and zero
   captures** (g1 captured at r119, next capture r468). The routes show Issac/Ren/Ryth walking to g1 at r259–267,
   r347–370 and r449–468, standing 4–8 rounds, and leaving without fighting (attack rate 4–7% in those windows).
   Our telemetry says `squads=1` with 6 bodies on this map — a single squad that has to assemble before engaging on a
   board where the team spread stays at ~32 tiles never gets going. That stall alone is 250 rounds of a 697-round game.
2. **Mite farms the nearest rich group on its respawn clock and carries big bags.** On ambush it captured g7
   (2 elk, 45 steps from its spawn, 23 to its altar, 72 gems, 60-round respawn) **7 times** (r63, 216, 302, 377,
   452, 540, 662) plus g10 ×3 and g0 ×2, mostly solo (avg party 1.25). Average bag per tribute 77 (max 178) vs our
   43; carry time from first gem to banking averages 45 rounds vs our 19 — fewer altar trips per gem. On rushbait
   it cycled its five nearest wolves (g1/g3/g5/g7/g9, 7–26 steps) **35 times in 490 rounds** vs our 21 cycles of the
   mirror groups, and its Varg chained 5 wolves into 102- and 124-gem trips.
3. **Mite kills carriers at altars; we feed them.** Rushbait: Mite 6 kills / 0 deaths, we 0 / 7. Our 7 deaths cost
   421 champion-rounds (17% of our total body-time) and 124 gems looted straight into Mite's bags. Two patterns:
   (a) we send carriers to the altar on *Mite's* side — Ren (36 gems) and Varg died at (11,77)/(16,70) next to Mite's
   altar (15,79), and our Varg even tributed there at r268; (b) Mite's Varg raided *our* altar (14,0): altar lock placed
   at (14,0) r266, Cassi + Issac killed r264 at (10,12)/(12,2) and Stellar r299 at (9,20), all carrying gems. On ambush
   Tavi (0 gems all game, 12 wards) double-killed Cassi & Issac at r515.
4. **Mite uses the free ward and altar locks; we use neither.** Wards 84 / 11 vs 0 / 0; locks 3 / 3 vs 0 / 0. Locks
   went on the altar Mite tributes at (own-altar defence: (66,79) r107 before its first tribute; (15,79) r179 and r285)
   and on the altars we use ((62,40) r352 — our r130 and r479 tributes were at (61,39/40); (14,0) r266 during the
   raid). 84 wards is simply every champion warding on cooldown: a full vision net around its farming lane.
5. **Mite starts faster.** First capture r63 vs r119 on ambush (its five bodies went straight to g7 together, all six
   of ours wandered: mean distance from spawn 26 then 42 over the first two 50-round windows, attacking in only 5% then 18% of turns). First-100-round
   idle on rushbait 5% vs 22%. Banked by r300: 144 vs 120 (ambush, before our stall), **534 vs 234** (rushbait).
6. **Small losses on capture timing.** Clear→reward delay (minimum 6): Mite 6–9 every time; ours 6–13 — up to 7
   rounds per capture standing on cleared tiles late or stepping off.
7. **Draft/roles.** Mite drafted Varg, Stellar, Cassi & Issac, Tavi, Ren on ambush: four farmers plus Tavi (roamer,
   wards, finishes carriers) and Ren (fights camps with the squad, 0 gems). On rushbait Varg, Ryth, Cassi & Issac,
   Stellar — all four farm solo and all four kill. Its Varg is the raider in both games (3 kills on rushbait).

What to change first, in order of gems: fix the squad stall (1), stop tributing at altars on the enemy's lane and
recall instead when a Varg/Ryth/Stellar is within ~10 tiles (3a), chain 2–3 captures per trip when the altar is >20
steps away (2), ward on cooldown and lock the altar we are about to use (4), capture the moment the group dies (6).


## Generated report

### Across both games

| game | map | winner | round | gems A/B | first tribute A/B | captures A/B | avg party A/B | kills A/B | idle A/B |
|---|---|---|---|---|---|---|---|---|---|
| match-e1fa1ed5.zip | ambush | Mite | 697 | 383/1002 | 130/111 | 5/12 | 2.40/1.25 | 0/1 | 24%/4% |
| match-aa70caa5.zip | rushbait-v2 | Mite | 490 | 402/1016 | 62/67 | 26/40 | 1.00/1.00 | 0/6 | 28%/10% |

## match-e1fa1ed5.zip — ambush: Electric Muffin (A) vs Mite (B)

Winner **Mite** at round 697; gems A 383 / B 1002; engine 0.0.3; map 100x100, team size 5, 38 camps in 12 groups (12 groups per docs/MAPS.md).

Draft A: Cassi & Issac, Stellar, Ren, Varg, Ryth · Draft B: Varg, Stellar, Cassi & Issac, Tavi, Ren

### Summary

| metric | Electric Muffin (A) | Mite (B) |
|---|---|---|
| gems tributed | 383 | 1002 |
| round of 1000 gems | - | 697 |
| first capture (round) | 119 | 63 |
| first tribute (round) | 130 | 111 |
| tributes (count, avg bag) | 9, 42.6 | 13, 77.1 |
| banked by round 100 | 0 | 0 |
| banked by round 200 | 120 | 72 |
| banked by round 300 | 120 | 144 |
| banked by round 400 | 120 | 442 |
| banked by round 500 | 240 | 514 |
| captures (groups cleared) | 5 (6) | 12 (12) |
| gems / gold from captures | 418 / 1315 | 1062 / 3630 |
| avg party size at capture | 2.40 | 1.25 |
| gems collected (incl. kill loot) | 418 | 1062 |
| gems lost on death | 0 | 0 |
| gems looted from kills | 0 | 0 |
| kills / deaths | 0 / 2 | 1 / 1 |
| champion-rounds dead | 122 | 60 |
| champion-rounds channelling | 143 | 260 |
| recalls | 21 | 29 |
| stationary fraction (pos+bag unchanged, not channelling) | 28.5% | 17.6% |
| idle fraction (no action in the turn) | 23.8% | 4.5% |
| distance travelled (tiles) | 3036 | 3385 |
| gold earned | 1315 | 3680 |
| altar locks / wards placed | 0 / 0 | 3 / 84 |
| fuel suspensions / bot errors / blocked moves | 0 / 0 / 26 | 0 / 0 / 31 |

### Tribute timeline

| Electric Muffin (A) round: +amount by champion → total | Mite (B) round: +amount by champion → total |
|---|---|
| r130: +60 Cassi#0 → 60 | r111: +36 Cassi#8 → 36 |
| r130: +60 Stellar#2 → 120 | r113: +36 Stellar#7 → 72 |
| r479: +30 Ryth#5 → 150 | r264: +72 Varg#6 → 144 |
| r479: +30 Issac#1 → 180 | r308: +120 Stellar#7 → 264 |
| r481: +30 Ren#3 → 210 | r313: +106 Issac#9 → 370 |
| r483: +30 Cassi#0 → 240 | r350: +72 Cassi#8 → 442 |
| r656: +36 Issac#1 → 276 | r426: +72 Stellar#7 → 514 |
| r667: +36 Cassi#0 → 312 | r503: +72 Cassi#8 → 586 |
| r671: +71 Varg#4 → 383 | r518: +106 Stellar#7 → 692 |
|  | r608: +178 Cassi#8 → 870 |
|  | r688: +36 Stellar#7 → 906 |
|  | r688: +36 Cassi#8 → 942 |
|  | r697: +60 Issac#9 → 1002 |

### Captures

Group ids are docs/MAPS.md ids (`gN`) when the map is known, else `L<live id>@x,y`. `d` = walking distance own spawn / to nearest altar (from MAPS.md).

**Electric Muffin (A)** — 5 captures, farming order: g1(r119) → g4(r244) → g11(r626) → g6(r640)

| round | group | type | gems | gold | party | d own/altar | champions |
|---|---|---|---|---|---|---|---|
| 119 | g1 | 2elk+2gem | 120 | 320 | 2 | 64/5 | Cassi#0(+60), Stellar#2(+60) |
| 244 | g4 | 3gold | 0 | 105 | 1 | 60/9 | Varg#4 |
| 468 | g1 | 2elk+2gem | 120 | 320 | 4 | 64/5 | Cassi#0(+30), Issac#1(+30), Ren#3(+30), Ryth#5(+30) |
| 626 | g11 | 2gem+elk+wolf | 106 | 250 | 3 | 53/31 | Issac#1(+36), Stellar#2(+35), Varg#4(+35) |
| 640 | g6 | 2elk | 72 | 320 | 2 | 45/23 | Cassi#0(+36), Varg#4(+36) |

Captures per group: g1×2, g4×1, g11×1, g6×1

**Mite (B)** — 12 captures, farming order: g7(r63) → g0(r245) → g10(r266)

| round | group | type | gems | gold | party | d own/altar | champions |
|---|---|---|---|---|---|---|---|
| 63 | g7 | 2elk | 72 | 320 | 2 | 45/23 | Stellar#7(+36), Cassi#8(+36) |
| 216 | g7 | 2elk | 72 | 320 | 1 | 45/23 | Varg#6(+72) |
| 245 | g0 | 2elk+2gem | 120 | 320 | 1 | 64/5 | Stellar#7(+120) |
| 266 | g10 | 2gem+elk+wolf | 106 | 250 | 1 | 53/31 | Issac#9(+106) |
| 302 | g7 | 2elk | 72 | 320 | 1 | 45/23 | Cassi#8(+72) |
| 377 | g7 | 2elk | 72 | 320 | 1 | 45/23 | Stellar#7(+72) |
| 452 | g7 | 2elk | 72 | 320 | 1 | 45/23 | Cassi#8(+72) |
| 464 | g10 | 2gem+elk+wolf | 106 | 250 | 1 | 53/31 | Stellar#7(+106) |
| 540 | g7 | 2elk | 72 | 320 | 1 | 45/23 | Cassi#8(+72) |
| 560 | g10 | 2gem+elk+wolf | 106 | 250 | 1 | 53/31 | Cassi#8(+106) |
| 662 | g7 | 2elk | 72 | 320 | 2 | 45/23 | Stellar#7(+36), Cassi#8(+36) |
| 689 | g0 | 2elk+2gem | 120 | 320 | 2 | 64/5 | Varg#6(+60), Issac#9(+60) |

Captures per group: g7×7, g10×3, g0×2

### Deaths and kills

| round | victim | team | at | gems lost | gold | killed by |
|---|---|---|---|---|---|---|
| 169 | Ren#11 | Mite | (70,56) | 0 | 0 | camp |
| 515 | Cassi#0 | Electric Muffin | (39,49) | 0 | 240 | Tavi#10 (Mite) |
| 515 | Issac#1 | Electric Muffin | (35,38) | 0 | 80 | Tavi#10 (Mite) |

### Champions

`gems/100r` = gems collected per 100 rounds of the match; `still` = alive, not channelling, position and bag unchanged since last round (includes standing and hitting a camp); `idle` = no move/attack/cast/shop/ward/lock/consumable/channel in the turn; `dist` = Chebyshev tiles walked; route = camp groups the champion stood next to (≥2 rounds), in order.

| team | champion | tributed (n) | collected | gems/100r | captures | kills/deaths/assists | dead rounds | channel rounds | still | idle | dist | max bag | recalls | fuel-out/err | route |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Electric Muffin | Cassi#0 | 126 (3) | 126 | 18.1 | 3 | 0/1/0 | 61 | 16 | 48% | 44% | 323 | 60 | 1 | 0/0 | g1(r114-119) → g1(r464-468) → g6(r636-640) |
| Electric Muffin | Issac#1 | 66 (2) | 66 | 9.5 | 2 | 0/1/0 | 61 | 27 | 21% | 16% | 599 | 36 | 4 | 0/0 | g1(r57-59) → g6(r154-156) → g1(r259-267) → g1(r347-349) → g1(r449-467) → g11(r620-625) |
| Electric Muffin | Stellar#2 | 60 (1) | 95 | 13.6 | 4 | 0/0/0 | 0 | 6 | 55% | 48% | 308 | 60 | 0 | 0/0 | g1(r112-119) → g1(r466-469) → g11(r620-627) → g6(r635-639) |
| Electric Muffin | Ren#3 | 30 (1) | 30 | 4.3 | 1 | 0/0/0 | 0 | 33 | 17% | 15% | 679 | 30 | 6 | 0/0 | g1(r51-53) → g6(r151-152) → g1(r260-264) → g1(r363-366) → g1(r462-467) → g6(r538-551) → g11(r604-612) |
| Electric Muffin | Varg#4 | 71 (1) | 71 | 10.2 | 3 | 0/0/0 | 0 | 28 | 17% | 12% | 552 | 71 | 4 | 0/0 | g1(r74-80) → g4(r229-244) → g2(r270-282) → g1(r423-429) → g6(r542-551) → g11(r618-626) → g6(r633-640) |
| Electric Muffin | Ryth#5 | 30 (1) | 30 | 4.3 | 1 | 0/0/0 | 0 | 33 | 12% | 8% | 575 | 30 | 6 | 0/0 | g1(r70-73) → g6(r181-183) → g1(r280-283) → g1(r365-370) → g1(r452-469) → g4(r502-508) → g6(r556-562) |
| Mite | Varg#6 | 72 (1) | 132 | 18.9 | 3 | 0/0/0 | 0 | 37 | 15% | 3% | 556 | 72 | 5 | 0/0 | g7(r47-59) → g10(r66-81) → g7(r140-150) → g7(r206-216) → g7(r286-295) → g1(r358-361) → g1(r369-371) → g7(r433-450) → g10(r457-459) → g7(r518-524) → g0(r666-689) |
| Mite | Stellar#7 | 370 (5) | 370 | 53.1 | 7 | 0/0/0 | 0 | 69 | 12% | 3% | 550 | 120 | 5 | 0/0 | g7(r58-67) → g0(r161-165) → g0(r237-245) → g7(r359-378) → g7(r449-450) → g10(r459-467) → g7(r540-541) → g7(r655-662) |
| Mite | Cassi#8 | 394 (5) | 394 | 56.5 | 6 | 0/0/0 | 0 | 72 | 22% | 5% | 486 | 178 | 6 | 0/0 | g7(r57-67) → g0(r160-177) → g7(r270-302) → g7(r373-374) → g7(r447-455) → g7(r533-540) → g10(r554-560) → g7(r629-630) → g7(r633-634) → g7(r655-662) |
| Mite | Issac#9 | 166 (2) | 166 | 23.8 | 2 | 0/0/0 | 0 | 47 | 17% | 5% | 663 | 106 | 6 | 0/0 | g7(r37-41) → g7(r134-138) → g7(r189-200) → g7(r245-246) → g10(r252-270) → g7(r347-348) → g7(r359-372) → g10(r377-386) → g7(r432-433) → g7(r435-445) → g10(r450-461) → g7(r541-542) → g10(r547-556) → g7(r562-563) → g0(r6… |
| Mite | Tavi#10 | 0 (0) | 0 | 0.0 | 0 | 1/0/0 | 0 | 15 | 23% | 6% | 520 | 0 | 3 | 0/0 | g7(r268-282) → g7(r332-333) → g7(r335-336) → g7(r346-372) → g7(r658-659) → g7(r666-667) |
| Mite | Ren#11 | 0 (0) | 0 | 0.0 | 1 | 0/1/1 | 60 | 20 | 16% | 4% | 610 | 0 | 4 | 0/0 | g7(r36-58) → g7(r60-62) → g7(r160-162) → g7(r168-228) → g7(r265-296) → g5(r327-328) → g1(r340-343) → g7(r397-398) → g10(r405-417) → g7(r515-535) |

## match-aa70caa5.zip — rushbait-v2: Electric Muffin (A) vs Mite (B)

Winner **Mite** at round 490; gems A 402 / B 1016; engine 0.0.3; map 30x80, team size 4, 18 camps in 18 groups (18 groups per docs/MAPS.md).

Draft A: Cassi & Issac, Stellar, Ren, Varg · Draft B: Varg, Ryth, Cassi & Issac, Stellar

### Summary

| metric | Electric Muffin (A) | Mite (B) |
|---|---|---|
| gems tributed | 402 | 1016 |
| round of 1000 gems | - | 490 |
| first capture (round) | 21 | 17 |
| first tribute (round) | 62 | 67 |
| tributes (count, avg bag) | 10, 40.2 | 20, 50.8 |
| banked by round 100 | 44 | 66 |
| banked by round 200 | 154 | 278 |
| banked by round 300 | 234 | 534 |
| banked by round 400 | 322 | 622 |
| banked by round 500 | 402 | 1016 |
| captures (groups cleared) | 26 (28) | 40 (40) |
| gems / gold from captures | 642 / 2690 | 936 / 3880 |
| avg party size at capture | 1.00 | 1.00 |
| gems collected (incl. kill loot) | 642 | 1060 |
| gems lost on death | 124 | 0 |
| gems looted from kills | 0 | 124 |
| kills / deaths | 0 / 7 | 6 / 0 |
| champion-rounds dead | 421 | 0 |
| champion-rounds channelling | 83 | 258 |
| recalls | 9 | 29 |
| stationary fraction (pos+bag unchanged, not channelling) | 47.7% | 34.9% |
| idle fraction (no action in the turn) | 27.8% | 10.0% |
| distance travelled (tiles) | 1127 | 1470 |
| gold earned | 2690 | 4180 |
| altar locks / wards placed | 0 / 0 | 3 / 11 |
| fuel suspensions / bot errors / blocked moves | 0 / 0 / 1 | 0 / 0 / 1 |

### Tribute timeline

| Electric Muffin (A) round: +amount by champion → total | Mite (B) round: +amount by champion → total |
|---|---|
| r62: +44 Issac#1 → 44 | r67: +22 Stellar#9 → 22 |
| r119: +44 Issac#1 → 88 | r79: +22 Ryth#6 → 44 |
| r163: +44 Cassi#0 → 132 | r83: +22 Issac#8 → 66 |
| r194: +22 Issac#1 → 154 | r126: +102 Varg#5 → 168 |
| r224: +44 Stellar#2 → 198 | r186: +66 Ryth#6 → 234 |
| r268: +36 Varg#4 → 234 | r199: +44 Cassi#7 → 278 |
| r369: +44 Ren#3 → 278 | r279: +124 Varg#5 → 402 |
| r380: +44 Issac#1 → 322 | r290: +44 Stellar#9 → 446 |
| r417: +36 Issac#1 → 358 | r292: +66 Ryth#6 → 512 |
| r467: +44 Issac#1 → 402 | r297: +22 Cassi#7 → 534 |
|  | r304: +66 Issac#8 → 600 |
|  | r343: +22 Issac#8 → 622 |
|  | r402: +80 Varg#5 → 702 |
|  | r425: +80 Ryth#6 → 782 |
|  | r425: +66 Cassi#7 → 848 |
|  | r445: +22 Varg#5 → 870 |
|  | r456: +22 Stellar#9 → 892 |
|  | r464: +66 Issac#8 → 958 |
|  | r469: +36 Ryth#6 → 994 |
|  | r490: +22 Issac#8 → 1016 |

### Captures

Group ids are docs/MAPS.md ids (`gN`) when the map is known, else `L<live id>@x,y`. `d` = walking distance own spawn / to nearest altar (from MAPS.md).

**Electric Muffin (A)** — 26 captures, farming order: g0(r21) → g2(r38) → g4(r68) → g13(r85) → g8(r91) → g6(r103) → g11(r228) → g16(r399)

| round | group | type | gems | gold | party | d own/altar | champions |
|---|---|---|---|---|---|---|---|
| 21 | g0 | wolf | 22 | 90 | 1 | 7/30 | Issac#1(+22) |
| 38 | g2 | wolf | 22 | 90 | 1 | 12/25 | Issac#1(+22) |
| 68 | g4 | wolf | 22 | 90 | 1 | 17/20 | Cassi#0(+22) |
| 85 | g13 | elk | 36 | 160 | 1 | 18/28 | Ren#3(+36) |
| 91 | g8 | wolf | 22 | 90 | 1 | 26/11 | Issac#1(+22) |
| 103 | g6 | wolf | 22 | 90 | 1 | 22/15 | Issac#1(+22) |
| 138 | g4 | wolf | 22 | 90 | 1 | 17/20 | Cassi#0(+22) |
| 167 | g0 | wolf | 22 | 90 | 1 | 7/30 | Issac#1(+22) |
| 177 | g8 | wolf | 22 | 90 | 1 | 26/11 | Stellar#2(+22) |
| 197 | g2 | wolf | 22 | 90 | 1 | 12/25 | Ren#3(+22) |
| 204 | g6 | wolf | 22 | 90 | 1 | 22/15 | Stellar#2(+22) |
| 228 | g11 | elk | 36 | 160 | 1 | 12/33 | Varg#4(+36) |
| 239 | g4 | wolf | 22 | 90 | 1 | 17/20 | Issac#1(+22) |
| 255 | g8 | wolf | 22 | 90 | 1 | 26/11 | Issac#1(+22) |
| 289 | g6 | wolf | 22 | 90 | 1 | 22/15 | Stellar#2(+22) |
| 295 | g0 | wolf | 22 | 90 | 1 | 7/30 | Ren#3(+22) |
| 345 | g4 | wolf | 22 | 90 | 1 | 17/20 | Ren#3(+22) |
| 352 | g6 | wolf | 22 | 90 | 1 | 22/15 | Issac#1(+22) |
| 365 | g8 | wolf | 22 | 90 | 1 | 26/11 | Issac#1(+22) |
| 399 | g16 | elk | 36 | 160 | 1 | 47/18 | Issac#1(+36) |
| 420 | g6 | wolf | 22 | 90 | 1 | 22/15 | Cassi#0(+22) |
| 433 | g8 | wolf | 22 | 90 | 1 | 26/11 | Issac#1(+22) |
| 446 | g4 | wolf | 22 | 90 | 1 | 17/20 | Issac#1(+22) |
| 467 | g2 | wolf | 22 | 90 | 1 | 12/25 | Cassi#0(+22) |
| 471 | g11 | elk | 36 | 160 | 1 | 12/33 | Ren#3(+36) |
| 490 | g13 | elk | 36 | 160 | 1 | 18/28 | Varg#4(+36) |

Captures per group: g4×5, g8×5, g6×5, g0×3, g2×3, g13×2, g11×2, g16×1

**Mite (B)** — 40 captures, farming order: g1(r17) → g3(r40) → g5(r42) → g7(r53) → g9(r66) → g10(r215) → g2(r319) → g12(r374) → g17(r448)

| round | group | type | gems | gold | party | d own/altar | champions |
|---|---|---|---|---|---|---|---|
| 17 | g1 | wolf | 22 | 90 | 1 | 7/30 | Varg#5(+22) |
| 40 | g3 | wolf | 22 | 90 | 1 | 12/25 | Stellar#9(+22) |
| 42 | g5 | wolf | 22 | 90 | 1 | 17/20 | Varg#5(+22) |
| 53 | g7 | wolf | 22 | 90 | 1 | 22/15 | Issac#8(+22) |
| 66 | g9 | wolf | 22 | 90 | 1 | 26/11 | Ryth#6(+22) |
| 79 | g1 | wolf | 22 | 90 | 1 | 7/30 | Varg#5(+22) |
| 101 | g3 | wolf | 22 | 90 | 1 | 12/25 | Cassi#7(+22) |
| 110 | g5 | wolf | 22 | 90 | 1 | 17/20 | Ryth#6(+22) |
| 121 | g7 | wolf | 22 | 90 | 1 | 22/15 | Issac#8(+22) |
| 131 | g9 | wolf | 22 | 90 | 1 | 26/11 | Ryth#6(+22) |
| 148 | g1 | wolf | 22 | 90 | 1 | 7/30 | Varg#5(+22) |
| 155 | g3 | wolf | 22 | 90 | 1 | 12/25 | Ryth#6(+22) |
| 164 | g5 | wolf | 22 | 90 | 1 | 17/20 | Varg#5(+22) |
| 180 | g7 | wolf | 22 | 90 | 1 | 22/15 | Cassi#7(+22) |
| 201 | g9 | wolf | 22 | 90 | 1 | 26/11 | Stellar#9(+22) |
| 213 | g1 | wolf | 22 | 90 | 1 | 7/30 | Issac#8(+22) |
| 215 | g10 | elk | 36 | 160 | 1 | 12/33 | Varg#5(+36) |
| 220 | g5 | wolf | 22 | 90 | 1 | 17/20 | Ryth#6(+22) |
| 239 | g3 | wolf | 22 | 90 | 1 | 12/25 | Cassi#7(+22) |
| 254 | g7 | wolf | 22 | 90 | 1 | 22/15 | Ryth#6(+22) |
| 264 | g9 | wolf | 22 | 90 | 1 | 26/11 | Ryth#6(+22) |
| 274 | g1 | wolf | 22 | 90 | 1 | 7/30 | Issac#8(+22) |
| 316 | g3 | wolf | 22 | 90 | 1 | 12/25 | Ryth#6(+22) |
| 319 | g2 | wolf | 22 | 90 | 1 | 54/25 | Varg#5(+22) |
| 326 | g5 | wolf | 22 | 90 | 1 | 17/20 | Ryth#6(+22) |
| 327 | g1 | wolf | 22 | 90 | 1 | 7/30 | Cassi#7(+22) |
| 332 | g9 | wolf | 22 | 90 | 1 | 26/11 | Issac#8(+22) |
| 349 | g7 | wolf | 22 | 90 | 1 | 22/15 | Cassi#7(+22) |
| 352 | g10 | elk | 36 | 160 | 1 | 12/33 | Varg#5(+36) |
| 369 | g3 | wolf | 22 | 90 | 1 | 12/25 | Issac#8(+22) |
| 374 | g12 | elk | 36 | 160 | 1 | 18/28 | Ryth#6(+36) |
| 389 | g1 | wolf | 22 | 90 | 1 | 7/30 | Cassi#7(+22) |
| 390 | g5 | wolf | 22 | 90 | 1 | 17/20 | Issac#8(+22) |
| 407 | g7 | wolf | 22 | 90 | 1 | 22/15 | Stellar#9(+22) |
| 418 | g9 | wolf | 22 | 90 | 1 | 26/11 | Varg#5(+22) |
| 429 | g3 | wolf | 22 | 90 | 1 | 12/25 | Issac#8(+22) |
| 448 | g17 | elk | 36 | 160 | 1 | 47/18 | Ryth#6(+36) |
| 466 | g1 | wolf | 22 | 90 | 1 | 7/30 | Varg#5(+22) |
| 479 | g9 | wolf | 22 | 90 | 1 | 26/11 | Issac#8(+22) |
| 489 | g7 | wolf | 22 | 90 | 1 | 22/15 | Ryth#6(+22) |

Captures per group: g1×8, g3×7, g7×7, g9×7, g5×6, g10×2, g2×1, g12×1, g17×1

### Deaths and kills

| round | victim | team | at | gems lost | gold | killed by |
|---|---|---|---|---|---|---|
| 111 | Ren#3 | Electric Muffin | (11,77) | 36 | 160 | Varg#5 (Mite) |
| 122 | Varg#4 | Electric Muffin | (16,70) | 0 | 0 | Ryth#6 (Mite) |
| 219 | Ren#3 | Electric Muffin | (13,50) | 22 | 90 | Stellar#9 (Mite) |
| 264 | Cassi#0 | Electric Muffin | (10,12) | 0 | 180 | Varg#5 (Mite) |
| 264 | Issac#1 | Electric Muffin | (12,2) | 44 | 200 | Varg#5 (Mite) |
| 279 | Varg#4 | Electric Muffin | (7,68) | 0 | 160 | Stellar#9 (Mite) |
| 299 | Stellar#2 | Electric Muffin | (9,20) | 22 | 270 | Varg#5 (Mite) |

### Champions

`gems/100r` = gems collected per 100 rounds of the match; `still` = alive, not channelling, position and bag unchanged since last round (includes standing and hitting a camp); `idle` = no move/attack/cast/shop/ward/lock/consumable/channel in the turn; `dist` = Chebyshev tiles walked; route = camp groups the champion stood next to (≥2 rounds), in order.

| team | champion | tributed (n) | collected | gems/100r | captures | kills/deaths/assists | dead rounds | channel rounds | still | idle | dist | max bag | recalls | fuel-out/err | route |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Electric Muffin | Cassi#0 | 44 (1) | 88 | 18.0 | 16 | 0/1/0 | 60 | 5 | 64% | 44% | 152 | 44 | 0 | 0/0 | g4(r61-70) → g4(r131-138) → g8(r365-390) → g6(r413-425) → g2(r460-467) |
| Electric Muffin | Issac#1 | 234 (6) | 278 | 56.7 | 13 | 0/1/0 | 60 | 38 | 38% | 21% | 308 | 44 | 3 | 0/0 | g0(r7-20) → g2(r22-37) → g8(r76-91) → g6(r92-102) → g0(r149-166) → g0(r210-218) → g4(r228-238) → g8(r244-254) → g6(r340-352) → g8(r353-364) → g16(r392-398) → g8(r425-432) → g4(r438-445) → g8(r485-490) |
| Electric Muffin | Stellar#2 | 44 (1) | 66 | 13.5 | 14 | 0/1/0 | 60 | 5 | 66% | 45% | 144 | 44 | 0 | 0/0 | g4(r61-68) → g4(r131-137) → g8(r170-195) → g6(r197-204) → g6(r282-295) → g6(r413-424) → g2(r460-466) |
| Electric Muffin | Ren#3 | 44 (1) | 138 | 28.2 | 5 | 0/2/0 | 120 | 16 | 36% | 12% | 280 | 44 | 3 | 0/0 | g11(r13-31) → g13(r58-84) → g2(r179-196) → g11(r216-278) → g0(r288-294) → g2(r296-312) → g4(r333-344) → g13(r414-434) → g11(r451-470) |
| Electric Muffin | Varg#4 | 36 (1) | 72 | 14.7 | 5 | 0/2/0 | 121 | 19 | 29% | 11% | 243 | 36 | 3 | 0/0 | g11(r17-18) → g11(r22-30) → g13(r61-84) → g9(r115-119) → g11(r221-228) → g11(r371-381) → g13(r411-429) → g11(r450-470) → g13(r483-490) |
| Mite | Varg#5 | 328 (4) | 350 | 71.4 | 12 | 3/0/0 | 0 | 65 | 28% | 13% | 303 | 124 | 6 | 0/0 | g1(r6-17) → g3(r20-21) → g3(r26-28) → g5(r34-47) → g1(r59-79) → g5(r88-89) → g7(r92-94) → g9(r96-98) → g1(r138-148) → g3(r152-153) → g5(r156-169) → g10(r183-190) → g10(r197-215) → g8(r289-292) → g6(r294-297) → g4(r306… |
| Mite | Ryth#6 | 270 (5) | 292 | 59.6 | 15 | 1/0/4 | 0 | 59 | 28% | 14% | 310 | 80 | 6 | 0/0 | g1(r7-13) → g3(r16-21) → g5(r27-28) → g5(r30-35) → g7(r40-48) → g9(r50-66) → g1(r90-92) → g3(r95-97) → g5(r100-110) → g7(r113-116) → g9(r118-120) → g9(r123-136) → g1(r144-145) → g3(r148-155) → g5(r159-160) → g7(r164-1… |
| Mite | Cassi#7 | 132 (3) | 132 | 26.9 | 13 | 0/0/3 | 0 | 50 | 34% | 5% | 287 | 66 | 7 | 0/0 | g5(r38-39) → g7(r49-61) → g7(r67-72) → g3(r94-101) → g7(r110-112) → g1(r156-158) → g3(r161-163) → g5(r166-167) → g7(r173-180) → g9(r183-184) → g1(r212-213) → g3(r232-239) → g5(r242-245) → g7(r249-257) → g7(r259-261) →… |
| Mite | Issac#8 | 198 (5) | 198 | 40.4 | 9 | 0/0/0 | 0 | 53 | 42% | 12% | 311 | 66 | 6 | 0/0 | g1(r7-11) → g3(r14-21) → g5(r27-35) → g7(r37-52) → g17(r53-64) → g5(r100-104) → g7(r107-121) → g9(r123-124) → g7(r125-126) → g5(r129-131) → g3(r133-134) → g1(r137-141) → g3(r142-149) → g5(r151-162) → g1(r191-213) → g3… |
| Mite | Stellar#9 | 88 (3) | 88 | 18.0 | 16 | 2/0/4 | 0 | 31 | 43% | 6% | 259 | 44 | 4 | 0/0 | g3(r33-40) → g5(r43-44) → g5(r105-107) → g7(r118-120) → g3(r135-137) → g3(r150-151) → g7(r175-192) → g9(r194-206) → g1(r218-219) → g3(r232-234) → g5(r237-239) → g9(r258-267) → g1(r301-303) → g3(r324-326) → g5(r329-330… |

