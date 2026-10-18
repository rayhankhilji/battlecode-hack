# MAPS — the 17-map pool, measured by the probe bot in the draft turn

Source: `bots/probe` draft dump (`D map`, `D terrain`, `D spawn_*`, `D altar_*`, `D group …` lines) on every map in
`maps/*.map26a`, run with `cambc run bots/probe bots/starter-cpp --map <m> --release --rounds 30`. Distances are engine
`distance_field` walking distances (8-directional, Chebyshev steps) computed in the draft with walls, holes and altars
impassable (and the enemy spawn impassable for us; our spawn impassable for them). "Spawn→altar" is the fewest steps from any
of our spawn tiles to a tribute tile (one of the altar's 8 neighbours). For a group, **d(our spawn)** is the distance from our spawn
to the nearest member's spawn tile (stand one step short of that to attack), **d(enemy spawn)** the same for the enemy, and
**d(→altar)** the steps from that camp tile to the nearest tribute tile. "−1" means unreachable on foot (walled/holed in: only
a dash or a teleport crystal gets there). Gems/gold per cycle = sum of all groups' rewards; respawn timers run per group after
capture (wolf 40, cluster 50, elk 60, monolith 150; a group uses the max of its members). All maps are symmetric, so the
enemy's view is the mirror image (`map::symmetry()`), and the "A" spawn is the one `spawn_region(team::ally)` returned to
team A; team B gets the mirrored numbers.

Team size = `draft::team_size()` = number of draft rounds (picks); Cassi & Issac count as one pick but two bodies.

## Summary table

| Map | Size | Symmetry | Team | Groups (camps) | Gems / cycle | Gold / cycle | wolf / elk / gem / gold / mono | Altars | Spawn A bbox | Spawn B bbox | Spawn→altar | Nearest gem group (dist, gems, →altar) | Walls / holes / bushes | Walkable |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| ambush | 100x100 | rot180 | 5 | 12 (38) | 692 | 2270 | 2 / 10 / 12 / 14 / 0 | 4: (33,20) (62,40) (37,59) (66,79) | (3,3)..(6,6) | (93,93)..(96,96) | 37 | 45, 72g, 23 | 540 / 1832 / 132 | 7624 |
| collision | 41x41 | vertical | 3 | 20 (24) | 536 | 2530 | 8 / 10 / 0 / 6 / 0 | 2: (27,20) (39,20) | (3,1)..(5,3) | (3,37)..(5,39) | 21 | 5, 22g, 16 | 84 / 60 / 222 | 1535 |
| contest | 50x50 | rot180 | 5 | 14 (22) | 400 | 1300 | 0 / 0 / 0 / 20 / 2 | 2: (25,24) (24,25) | (3,3)..(5,5) | (44,44)..(46,46) | 19 | 30, 200g, 17 | 40 / 0 / 26 | 2458 |
| corners | 71x71 | horizontal | 5 | 30 (42) | 916 | 4200 | 10 / 18 / 2 / 12 / 0 | 5: (1,1) (69,1) (35,34) (1,69) (69,69) | (13,33)..(15,35) | (55,33)..(57,35) | 19 | 6, 22g, 24 | 44 / 84 / 60 | 4908 |
| eight | 41x30 | horizontal | 5 | 8 (20) | 540 | 2210 | 6 / 10 / 2 / 2 / 0 | 1: (20,15) | (0,14)..(2,16) | (38,14)..(40,16) | 22 | 13, 22g, 10 | 36 / 22 / 24 | 1171 |
| elkring | 30x30 | rot180 | 5 | 10 (20) | 360 | 1950 | 0 / 10 / 0 / 10 / 0 | 2: (26,3) (3,26) | (0,0)..(2,2) | (27,27)..(29,29) | 23 | 4, 36g, 18 | 0 / 36 / 36 | 862 |
| expanse | 80x80 | rot180 | 3 | 24 (52) | 1012 | 3700 | 10 / 14 / 12 / 16 / 0 | 10: (14,4) (39,16) (13,26) (38,34) (67,32) (12,47) (41,45) (66,53) (40,63) (65,75) | (3,2)..(6,5) | (73,74)..(76,77) | 7 | 7, 22g, 9 | 224 / 16 / 0 | 6150 |
| fragment | 30x30 | rot180 | 3 | 6 (12) | 204 | 1000 | 6 / 2 / 0 / 4 / 0 | 2: (14,5) (15,24) | (1,18)..(3,20) | (26,9)..(28,11) | 12 | 7, 22g, 11 | 24 / 320 / 24 | 554 |
| interlock | 40x40 | rot180 | 4 | 14 (22) | 420 | 2100 | 6 / 8 / 0 / 8 / 0 | 4: (36,12) (19,16) (20,23) (3,27) | (0,0)..(2,2) | (37,37)..(39,39) | 17 | 6, 22g, 13 | 76 / 16 / 38 | 1504 |
| lanes | 90x30 | horizontal | 5 | 42 (42) | 876 | 3870 | 30 / 6 / 0 / 6 / 0 | 18: (13,4) (20,4) (29,4) (60,4) (69,4) (76,4) (12,13) (20,13) (29,13) (60,13) (69,13) (77,13) (11,25) (20,25) (29,25) (60,25) (69,25) (78,25) | (2,12)..(4,14) | (85,12)..(87,14) | 7 | 32, 22g, 6 | 0 / 76 / 144 | 2606 |
| monolith | 50x50 | rot180 | 5 | 22 (44) | 1252 | 2750 | 18 / 2 / 16 / 6 / 2 | 6: (4,1) (12,10) (19,19) (30,30) (37,39) (45,48) | (2,45)..(4,47) | (45,2)..(47,4) | 25 | 6, 22g, 19 | 98 / 0 / 66 | 2396 |
| packs | 31x31 | rot180 | 3 | 8 (20) | 416 | 1500 | 8 / 4 / 4 / 4 / 0 | 6: (2,6) (28,6) (2,15) (28,15) (2,24) (28,24) | (0,0)..(2,2) | (28,28)..(30,30) | 3 | 6, 44g, 5 | 0 / 0 / 62 | 955 |
| pilgrimage | 50x65 | horizontal | 5 | 23 (46) | 464 | 2340 | 8 / 4 / 6 / 28 / 0 | 4: (14,3) (35,3) (5,58) (44,58) | (1,1)..(3,3) | (46,1)..(48,3) | 54 | 40, 68g, 13 | 0 / 654 / 152 | 2592 |
| rows | 50x50 | rot180 | 4 | 9 (28) | 844 | 2430 | 18 / 0 / 2 / 6 / 2 | 2: (15,16) (34,33) | (2,2)..(5,5) | (44,44)..(47,47) | 10 | 13, 66g, 11 | 214 / 0 / 142 | 2284 |
| rushbait-v2 | 30x80 | rot180 | 4 | 18 (18) | 508 | 2180 | 10 / 8 / 0 / 0 / 0 | 2: (14,0) (15,79) | (11,38)..(14,41) | (15,38)..(18,41) | 37 | 7, 22g, 30 | 80 / 142 / 120 | 2176 |
| stampede | 60x50 | vertical | 5 | 13 (40) | 360 | 1940 | 0 / 6 / 6 / 28 / 0 | 2: (31,23) (31,26) | (0,0)..(2,2) | (0,47)..(2,49) | 28 | 56, 360g, 58 | 0 / 610 / 456 | 2388 |
| swirl | 45x30 | rot180 | 4 | 22 (80) | 1728 | 6630 | 24 / 24 / 14 / 18 / 0 | 2: (23,10) (21,19) | (26,6)..(28,8) | (16,21)..(18,23) | 2 | 6, 36g, 2 | 30 / 354 / 14 | 964 |

Per-altar walking distance from our spawn to a tribute tile (-1 = not reachable on foot):
- ambush: (33,20)=37 (62,40)=63 (37,59)=67 (66,79)=91
- collision: (27,20)=21 (39,20)=-1
- contest: (25,24)=19 (24,25)=19
- corners: (1,1)=31 (69,1)=53 (35,34)=19 (1,69)=33 (69,69)=53
- eight: (20,15)=22
- elkring: (26,3)=23 (3,26)=23
- expanse: (14,4)=7 (39,16)=32 (13,26)=20 (38,34)=35 (67,32)=60 (12,47)=41 (41,45)=56 (66,53)=63 (40,63)=57 (65,75)=76
- fragment: (14,5)=12 (15,24)=14
- interlock: (36,12)=33 (19,16)=17 (20,23)=22 (3,27)=33
- lanes: (13,4)=8 (20,4)=15 (29,4)=24 (60,4)=55 (69,4)=64 (76,4)=71 (12,13)=7 (20,13)=15 (29,13)=24 (60,13)=55 (69,13)=64 (77,13)=72 (11,25)=10 (20,25)=15 (29,25)=24 (60,25)=55 (69,25)=64 (78,25)=73
- monolith: (4,1)=43 (12,10)=34 (19,19)=25 (30,30)=25 (37,39)=32 (45,48)=40
- packs: (2,6)=3 (28,6)=25 (2,15)=12 (28,15)=25 (2,24)=21 (28,24)=25
- pilgrimage: (14,3)=110 (35,3)=110 (5,58)=54 (44,58)=80
- rows: (15,16)=10 (34,33)=62
- rushbait-v2: (14,0)=37 (15,79)=37
- stampede: (31,23)=28 (31,26)=28
- swirl: (23,10)=2 (21,19)=10

## Qualitative notes (derived from the tables above)

- **ambush** (100×100, rot180, 5 picks, 692 gems/cycle). Huge, 1,832 holes and 540 walls: dashes matter. Nearest gem group is an
  elk pair 45 steps away (72 gems, 23 to an altar); the two 48-gem gem-cluster pairs in the middle (g8, g9) are **unreachable on
  foot** (holed/walled islands: 2-step dash or TP crystal only). Four altars on a diagonal, nearest 37 from spawn. The centre
  gold-cluster blocks (g2–g5) are equidistant (60–72) → contested; the 120-gem elk+gem quads (g0/g1) sit nearer the enemy/our side
  respectively (64 vs 73). Expect 100+ round trips; fuel for pathing on this board is the most expensive in the pool (FUEL.md).
- **collision** (41×41, vertical, 3 picks, 536 gems). Two diagonal lanes of single wolves/gold clusters from each spawn to the
  centre altar (27,20) (21 steps). The second altar (39,20) is **not reachable on foot** from our spawn. Wolves g6/g7 next to the
  altar are exactly equidistant (19/19) → the collision point. Elk triples (108 gems) in the far corners (32 and 45 steps).
- **contest** (50×50, rot180, 5 picks, 400 gems). **Boss-only gems**: the only gem sources are two monoliths (20,000 HP, 200 Str,
  200 gems, 150-round respawn) at 30/30 from both spawns, 17 steps from the central twin altars. Everything else is gold clusters
  (1,300 gold). 1,000 gems needs five monolith captures, so the 2,000-round tiebreak (gems → gold) is realistic here: gold matters.
- **corners** (71×71, horizontal, 5 picks, 916 gems). Spawns mid-left/mid-right, 5 altars (centre at 19 steps, four corners
  31–53). Rich safe farming: solo wolves/elks 6–18 steps from spawn on our half; the centre altar is flanked by elk+wolf singles
  (g26–g29) at 18/22 → contested centre. 96-gem elk+gem triples (g4/g5) at 24/32.
- **eight** (41×30, horizontal, 5 picks, 540 gems). One central altar (22 steps). Each side owns a 5-camp mega group (3 elk +
  gold + gem = 132 gems, 515 gold) 16 steps out, an elk pair (72) and a wolf (22); wolf pairs (g2/g3) near the centre top are
  contested (21/25). Figure-of-eight lanes.
- **elkring** (30×30, rot180, 5 picks, 360 gems). No walls; ten identical elk+gold pairs (36 gems, 195 gold, 60 respawn) in a
  ring. Our three nearest (4, 7, 10 steps) are 12–18 from an altar; the two altars sit in the off-diagonal corners 23 steps from
  spawn, with the ring groups g6/g9 (ours) and g7/g8 (theirs) 5 steps from them. The 30-round "clear → capture → carry" loop
  dominates; the ladder leader averages ~25 captures/game here [BRIEF].
- **expanse** (80×80, rot180, 3 picks, 1,012 gems). Only 3 picks on an 80×80 board with **10 altars** (nearest 7 steps from
  spawn; every group is ≤ 27 from an altar). Wolves at 7/17, an elk at 19 on our side; 120-gem elk+gem quads along the left/right
  edges 53–68 away. Travel dominates; three bodies cannot cover it, so route planning in the draft is the whole game.
- **fragment** (30×30, rot180, 3 picks, 204 gems). 320 walls: a maze. Two wolf+elk pairs (58 gems) in the centre at 9 (ours) /
  14 (theirs) and mirrored → contested centre; wolf+gold pairs on each edge (7, 13 steps). Altars at 12/14 from spawn. Smallest
  gem supply in the pool (204/cycle): kills and gold tiebreaks matter.
- **interlock** (40×40, rot180, 4 picks, 420 gems). Wolves at 6/12/19 on our edge (safe, 13 to an altar); elk pairs (72 gems)
  at 26–34; g2/g3 elk pairs are equidistant (29/29) and 3 steps from an altar → contested. Four altars, nearest 17.
- **lanes** (90×30, horizontal, 5 picks, 876 gems). Three horizontal lanes (y≈4, 13, 25), **18 altars** (six per lane, nearest 7
  from spawn), 42 single-camp groups all in the central band x=36…53, 32–49 steps from both spawns and 6–13 from an altar.
  Everything is contested; delivery is trivial; whoever controls the band wins. No walls.
- **monolith** (50×50, rot180, 5 picks, 1,252 gems). Two monoliths each ringed by 8 gem clusters (392 gems, 300 gold, 150
  respawn) in the far corners, 43/46 steps from both spawns, 1 step from an altar; six altars along the diagonal. Wolves/elk at
  6–21 on our side for a safe economy. The monolith groups are the prize (and the fight).
- **packs** (31×31, rot180, 3 picks, 416 gems). Six altars, nearest **3 steps** from spawn. Wolf pairs (44 gems) at 6/14 on our
  side, elk pair (72) at 22, two mixed gold+gem quads (48 gems) in the centre equidistant (15/16) → contested. Very short loops;
  3 picks.
- **pilgrimage** (50×65, horizontal, 5 picks, 464 gems). Spawns in the top corners; the two top altars (14,3),(35,3) are walled
  off and **110 steps** away on foot — the pilgrimage goes down the side (gold clusters at 10/19/28, a 68-gem wolf+gem quad at 40)
  to the bottom altars (54 / 80 steps). The 8-camp elk+gold group (144 gems, 780 gold) at the bottom centre is equidistant
  (60/60) → contested. The central gold column (113–150 steps) is behind walls/holes (654 holes: dashes shortcut it).
- **rows** (50×50, rot180, 4 picks, 844 gems). Wolf triples (66 gems, 270 gold) in rows at 13/26/37 on our side and mirrored;
  the centre holds a **double monolith + 2 gem clusters (448 gems, 150 respawn)** equidistant 36/36. Altar (15,16) is 10 steps
  from spawn (the other one is 62). 4×4 spawns.
- **rushbait-v2** (30×80, rot180, 4 picks, 508 gems). The two 4×4 spawns are **adjacent** in the middle of the map
  ((11..14,38..41) vs (15..18,38..41)); altars at the far ends (37 steps). Camps are two columns (x=13 ours, x=16 theirs) of
  single wolves/elks, 7–28 steps out, 11–33 to an altar. Fights at round 1 are possible; the long carries to the ends are the bait.
- **stampede** (60×50, vertical, 5 picks, 360 gems). Spawns top-left/bottom-left; all gems are in **one** 12-camp group (6 elk +
  6 gem = 360 gems, 960 gold) on the far right, equidistant 56/56, 58 from the central altars (28 from spawn). Everything else
  is gold-cluster pairs along the edges. One objective, both teams stampede to it; 610 holes and 456 bushes.
- **swirl** (45×30, rot180, 4 picks, 1,728 gems — the richest map, 80 camps). Spawns near the centre, an altar **2 steps** from
  spawn. Dense mixed quads/hexes at 6–41 steps; every group is within ±2 steps of both teams (dA≈dE) → fully contested, trivial
  delivery. Kill pressure and capture denial decide it.

## Group tables per map


### ambush — groups sorted by walking distance from our spawn

| id | gems | gold | respawn | d(our spawn) | d(enemy spawn) | d(→altar) | members |
|---|---|---|---|---|---|---|---|
| 6 | 72 | 320 | 60 | 45 | 97 | 23 | elk(31,44) elk(30,44) |
| 11 | 106 | 250 | 60 | 53 | 105 | 31 | gem(21,44) elk(20,44) wolf(21,43) gem(20,43) |
| 4 | 0 | 105 | 50 | 60 | 62 | 9 | gold(41,49) gold(41,48) gold(42,48) |
| 2 | 0 | 140 | 50 | 62 | 72 | 7 | gold(28,55) gold(29,54) gold(29,55) gold(28,54) |
| 5 | 0 | 105 | 50 | 62 | 60 | 9 | gold(58,50) gold(58,51) gold(57,51) |
| 1 | 120 | 320 | 60 | 64 | 73 | 5 | elk(56,34) elk(55,34) gem(57,34) gem(54,34) |
| 3 | 0 | 140 | 50 | 72 | 62 | 7 | gold(71,44) gold(70,45) gold(70,44) gold(71,45) |
| 0 | 120 | 320 | 60 | 73 | 64 | 5 | elk(43,65) elk(44,65) gem(42,65) gem(45,65) |
| 7 | 72 | 320 | 60 | 97 | 45 | 23 | elk(68,55) elk(69,55) |
| 10 | 106 | 250 | 60 | 105 | 53 | 31 | gem(78,55) elk(79,55) wolf(78,56) gem(79,56) |
| 8 | 48 | 0 | 50 | -1 | -1 | -1 | gem(31,49) gem(32,49) |
| 9 | 48 | 0 | 50 | -1 | -1 | -1 | gem(68,50) gem(67,50) |

### collision — groups sorted by walking distance from our spawn

| id | gems | gold | respawn | d(our spawn) | d(enemy spawn) | d(→altar) | members |
|---|---|---|---|---|---|---|---|
| 0 | 22 | 90 | 40 | 5 | 32 | 16 | wolf(10,5) |
| 8 | 0 | 35 | 50 | 7 | 30 | 14 | gold(12,7) |
| 2 | 22 | 90 | 40 | 10 | 28 | 11 | wolf(15,9) |
| 10 | 0 | 35 | 50 | 12 | 26 | 9 | gold(17,11) |
| 4 | 22 | 90 | 40 | 14 | 24 | 7 | wolf(19,13) |
| 14 | 36 | 160 | 60 | 16 | 34 | 16 | elk(21,3) |
| 12 | 0 | 35 | 50 | 17 | 21 | 4 | gold(22,16) |
| 6 | 22 | 90 | 40 | 19 | 19 | 2 | wolf(24,18) |
| 7 | 22 | 90 | 40 | 19 | 19 | 2 | wolf(24,22) |
| 13 | 0 | 35 | 50 | 21 | 17 | 4 | gold(22,24) |
| 5 | 22 | 90 | 40 | 24 | 14 | 7 | wolf(19,27) |
| 16 | 36 | 160 | 60 | 24 | 37 | 19 | elk(29,3) |
| 11 | 0 | 35 | 50 | 26 | 12 | 9 | gold(17,29) |
| 3 | 22 | 90 | 40 | 28 | 10 | 11 | wolf(15,31) |
| 9 | 0 | 35 | 50 | 30 | 7 | 14 | gold(12,33) |
| 1 | 22 | 90 | 40 | 32 | 5 | 16 | wolf(10,35) |
| 18 | 108 | 480 | 60 | 32 | 45 | 27 | elk(37,3) elk(37,4) elk(37,2) |
| 15 | 36 | 160 | 60 | 34 | 16 | 16 | elk(21,37) |
| 17 | 36 | 160 | 60 | 37 | 24 | 19 | elk(29,37) |
| 19 | 108 | 480 | 60 | 45 | 32 | 27 | elk(37,37) elk(37,36) elk(37,38) |

### contest — groups sorted by walking distance from our spawn

| id | gems | gold | respawn | d(our spawn) | d(enemy spawn) | d(→altar) | members |
|---|---|---|---|---|---|---|---|
| 12 | 0 | 35 | 50 | 13 | 39 | 18 | gold(5,18) |
| 6 | 0 | 35 | 50 | 14 | 41 | 20 | gold(19,3) |
| 3 | 0 | 105 | 50 | 22 | 25 | 4 | gold(27,18) gold(27,17) gold(27,19) |
| 4 | 0 | 105 | 50 | 22 | 25 | 4 | gold(18,27) gold(17,27) gold(19,27) |
| 2 | 0 | 105 | 50 | 25 | 22 | 4 | gold(22,31) gold(22,32) gold(22,30) |
| 5 | 0 | 105 | 50 | 25 | 22 | 4 | gold(31,22) gold(32,22) gold(30,22) |
| 8 | 0 | 35 | 50 | 28 | 39 | 18 | gold(5,33) |
| 0 | 200 | 300 | 150 | 30 | 30 | 17 | MONO(17,31) |
| 1 | 200 | 300 | 150 | 30 | 30 | 17 | MONO(32,18) |
| 10 | 0 | 35 | 50 | 31 | 35 | 18 | gold(36,9) |
| 11 | 0 | 35 | 50 | 35 | 31 | 18 | gold(13,40) |
| 9 | 0 | 35 | 50 | 39 | 28 | 18 | gold(44,16) |
| 13 | 0 | 35 | 50 | 39 | 13 | 18 | gold(44,31) |
| 7 | 0 | 35 | 50 | 41 | 14 | 20 | gold(30,46) |

### corners — groups sorted by walking distance from our spawn

| id | gems | gold | respawn | d(our spawn) | d(enemy spawn) | d(→altar) | members |
|---|---|---|---|---|---|---|---|
| 6 | 22 | 90 | 40 | 6 | 45 | 24 | wolf(10,41) |
| 18 | 36 | 160 | 60 | 11 | 37 | 16 | elk(18,22) |
| 24 | 36 | 160 | 60 | 14 | 38 | 17 | elk(17,49) |
| 16 | 36 | 160 | 60 | 17 | 43 | 14 | elk(12,16) |
| 22 | 36 | 160 | 60 | 18 | 45 | 15 | elk(10,53) |
| 26 | 36 | 160 | 60 | 18 | 22 | 1 | elk(33,32) |
| 28 | 22 | 90 | 40 | 18 | 22 | 1 | wolf(33,36) |
| 2 | 0 | 70 | 50 | 19 | 21 | 17 | gold(34,15) gold(34,16) |
| 10 | 22 | 90 | 40 | 20 | 27 | 20 | wolf(28,13) |
| 3 | 0 | 70 | 50 | 21 | 19 | 17 | gold(36,15) gold(36,16) |
| 27 | 36 | 160 | 60 | 22 | 18 | 1 | elk(37,32) |
| 29 | 22 | 90 | 40 | 22 | 18 | 1 | wolf(37,36) |
| 4 | 96 | 320 | 60 | 24 | 32 | 22 | gem(25,60) elk(25,59) elk(24,60) |
| 8 | 0 | 70 | 50 | 25 | 43 | 19 | gold(21,9) gold(22,9) |
| 12 | 22 | 90 | 40 | 26 | 44 | 14 | wolf(16,7) |
| 11 | 22 | 90 | 40 | 27 | 20 | 20 | wolf(42,13) |
| 14 | 22 | 90 | 40 | 28 | 46 | 7 | wolf(9,5) |
| 0 | 72 | 320 | 60 | 31 | 43 | 9 | elk(11,66) elk(12,66) |
| 5 | 96 | 320 | 60 | 32 | 24 | 22 | gem(45,60) elk(45,59) elk(46,60) |
| 20 | 0 | 70 | 50 | 32 | 32 | 32 | gold(34,1) gold(34,0) |
| 21 | 0 | 70 | 50 | 32 | 32 | 32 | gold(36,1) gold(36,0) |
| 19 | 36 | 160 | 60 | 37 | 11 | 16 | elk(52,22) |
| 25 | 36 | 160 | 60 | 38 | 14 | 17 | elk(53,49) |
| 1 | 72 | 320 | 60 | 43 | 31 | 9 | elk(59,66) elk(58,66) |
| 9 | 0 | 70 | 50 | 43 | 25 | 19 | gold(49,9) gold(48,9) |
| 17 | 36 | 160 | 60 | 43 | 17 | 14 | elk(58,16) |
| 13 | 22 | 90 | 40 | 44 | 26 | 14 | wolf(54,7) |
| 7 | 22 | 90 | 40 | 45 | 6 | 24 | wolf(60,41) |
| 23 | 36 | 160 | 60 | 45 | 18 | 15 | elk(60,53) |
| 15 | 22 | 90 | 40 | 46 | 28 | 7 | wolf(61,5) |

### eight — groups sorted by walking distance from our spawn

| id | gems | gold | respawn | d(our spawn) | d(enemy spawn) | d(→altar) | members |
|---|---|---|---|---|---|---|---|
| 6 | 22 | 90 | 40 | 13 | 34 | 10 | wolf(9,15) |
| 0 | 72 | 320 | 60 | 16 | 30 | 14 | elk(13,3) elk(13,4) |
| 4 | 132 | 515 | 60 | 16 | 28 | 13 | elk(13,25) elk(13,26) elk(13,27) gold(14,26) gem(15,26) |
| 2 | 44 | 180 | 40 | 21 | 25 | 9 | wolf(18,10) wolf(18,11) |
| 3 | 44 | 180 | 40 | 25 | 21 | 9 | wolf(22,10) wolf(22,11) |
| 5 | 132 | 515 | 60 | 28 | 16 | 13 | elk(27,25) elk(27,26) elk(27,27) gold(26,26) gem(25,26) |
| 1 | 72 | 320 | 60 | 30 | 16 | 14 | elk(27,3) elk(27,4) |
| 7 | 22 | 90 | 40 | 34 | 13 | 10 | wolf(31,15) |

### elkring — groups sorted by walking distance from our spawn

| id | gems | gold | respawn | d(our spawn) | d(enemy spawn) | d(→altar) | members |
|---|---|---|---|---|---|---|---|
| 0 | 36 | 195 | 60 | 4 | 26 | 18 | elk(3,6) gold(3,7) |
| 2 | 36 | 195 | 60 | 7 | 25 | 15 | elk(9,2) gold(10,2) |
| 4 | 36 | 195 | 60 | 10 | 24 | 12 | elk(3,12) gold(3,13) |
| 6 | 36 | 195 | 60 | 17 | 25 | 5 | elk(19,2) gold(20,2) |
| 9 | 36 | 195 | 60 | 17 | 24 | 5 | elk(3,20) gold(3,19) |
| 5 | 36 | 195 | 60 | 24 | 10 | 12 | elk(26,17) gold(26,16) |
| 8 | 36 | 195 | 60 | 24 | 17 | 5 | elk(26,9) gold(26,10) |
| 3 | 36 | 195 | 60 | 25 | 7 | 15 | elk(20,27) gold(19,27) |
| 7 | 36 | 195 | 60 | 25 | 17 | 5 | elk(10,27) gold(9,27) |
| 1 | 36 | 195 | 60 | 26 | 4 | 18 | elk(26,23) gold(26,22) |

### expanse — groups sorted by walking distance from our spawn

| id | gems | gold | respawn | d(our spawn) | d(enemy spawn) | d(→altar) | members |
|---|---|---|---|---|---|---|---|
| 18 | 22 | 90 | 40 | 7 | 78 | 9 | wolf(5,12) |
| 20 | 22 | 90 | 40 | 17 | 68 | 7 | wolf(5,22) |
| 2 | 36 | 160 | 60 | 19 | 66 | 10 | elk(25,15) |
| 14 | 0 | 70 | 50 | 26 | 61 | 9 | gold(29,26) gold(30,26) |
| 6 | 22 | 90 | 40 | 29 | 56 | 7 | wolf(17,34) |
| 16 | 0 | 70 | 50 | 33 | 52 | 9 | gold(27,38) gold(28,38) |
| 12 | 0 | 70 | 50 | 35 | 61 | 2 | gold(41,13) gold(42,13) |
| 22 | 44 | 180 | 40 | 40 | 68 | 9 | wolf(41,6) wolf(41,5) |
| 17 | 0 | 70 | 50 | 52 | 33 | 9 | gold(52,41) gold(51,41) |
| 9 | 120 | 320 | 60 | 53 | 64 | 10 | gem(8,60) gem(8,58) elk(9,59) elk(8,59) |
| 7 | 22 | 90 | 40 | 56 | 29 | 7 | wolf(62,45) |
| 5 | 120 | 320 | 60 | 60 | 64 | 17 | gem(8,65) elk(8,66) gem(8,67) elk(9,66) |
| 13 | 0 | 70 | 50 | 61 | 35 | 2 | gold(38,66) gold(37,66) |
| 15 | 0 | 70 | 50 | 61 | 26 | 9 | gold(50,53) gold(49,53) |
| 4 | 120 | 320 | 60 | 64 | 60 | 17 | gem(71,14) elk(71,13) gem(71,12) elk(70,13) |
| 8 | 120 | 320 | 60 | 64 | 53 | 10 | gem(71,19) gem(71,21) elk(70,20) elk(71,20) |
| 0 | 120 | 320 | 60 | 65 | 68 | 25 | gem(71,4) elk(71,5) gem(71,6) elk(70,5) |
| 3 | 36 | 160 | 60 | 66 | 19 | 10 | elk(54,64) |
| 10 | 0 | 70 | 50 | 66 | 70 | 27 | gold(54,4) gold(55,4) |
| 1 | 120 | 320 | 60 | 68 | 65 | 25 | gem(8,75) elk(8,74) gem(8,73) elk(9,74) |
| 21 | 22 | 90 | 40 | 68 | 17 | 7 | wolf(74,57) |
| 23 | 44 | 180 | 40 | 68 | 40 | 9 | wolf(38,73) wolf(38,74) |
| 11 | 0 | 70 | 50 | 70 | 66 | 27 | gold(25,75) gold(24,75) |
| 19 | 22 | 90 | 40 | 78 | 7 | 9 | wolf(74,67) |

### fragment — groups sorted by walking distance from our spawn

| id | gems | gold | respawn | d(our spawn) | d(enemy spawn) | d(→altar) | members |
|---|---|---|---|---|---|---|---|
| 4 | 22 | 125 | 50 | 7 | 24 | 11 | wolf(2,11) gold(1,11) |
| 0 | 58 | 250 | 60 | 9 | 14 | 7 | wolf(12,15) elk(12,16) |
| 2 | 22 | 125 | 50 | 13 | 24 | 11 | wolf(2,5) gold(1,5) |
| 1 | 58 | 250 | 60 | 14 | 9 | 7 | wolf(17,14) elk(17,13) |
| 3 | 22 | 125 | 50 | 24 | 13 | 11 | wolf(27,24) gold(28,24) |
| 5 | 22 | 125 | 50 | 24 | 7 | 11 | wolf(27,18) gold(28,18) |

### interlock — groups sorted by walking distance from our spawn

| id | gems | gold | respawn | d(our spawn) | d(enemy spawn) | d(→altar) | members |
|---|---|---|---|---|---|---|---|
| 10 | 22 | 90 | 40 | 6 | 35 | 13 | wolf(5,8) |
| 12 | 22 | 90 | 40 | 12 | 32 | 13 | wolf(5,14) |
| 6 | 0 | 70 | 50 | 17 | 34 | 12 | gold(19,2) gold(19,3) |
| 4 | 22 | 90 | 40 | 19 | 32 | 13 | wolf(5,21) |
| 0 | 72 | 320 | 60 | 26 | 34 | 12 | elk(28,3) elk(29,3) |
| 2 | 72 | 320 | 60 | 29 | 29 | 3 | elk(31,10) elk(32,10) |
| 3 | 72 | 320 | 60 | 29 | 29 | 3 | elk(8,29) elk(7,29) |
| 5 | 22 | 90 | 40 | 32 | 19 | 13 | wolf(34,18) |
| 13 | 22 | 90 | 40 | 32 | 12 | 13 | wolf(34,25) |
| 1 | 72 | 320 | 60 | 34 | 26 | 12 | elk(11,36) elk(10,36) |
| 7 | 0 | 70 | 50 | 34 | 17 | 12 | gold(20,37) gold(20,36) |
| 8 | 0 | 70 | 50 | 35 | 35 | 8 | gold(36,2) gold(36,3) |
| 9 | 0 | 70 | 50 | 35 | 35 | 8 | gold(3,37) gold(3,36) |
| 11 | 22 | 90 | 40 | 35 | 6 | 13 | wolf(34,31) |

### lanes — groups sorted by walking distance from our spawn

| id | gems | gold | respawn | d(our spawn) | d(enemy spawn) | d(→altar) | members |
|---|---|---|---|---|---|---|---|
| 14 | 0 | 35 | 50 | 32 | 49 | 6 | gold(36,27) |
| 16 | 0 | 35 | 50 | 32 | 49 | 6 | gold(36,25) |
| 18 | 0 | 35 | 50 | 32 | 49 | 6 | gold(36,23) |
| 30 | 22 | 90 | 40 | 32 | 49 | 6 | wolf(36,6) |
| 32 | 22 | 90 | 40 | 32 | 49 | 6 | wolf(36,4) |
| 34 | 22 | 90 | 40 | 32 | 49 | 6 | wolf(36,2) |
| 36 | 22 | 90 | 40 | 32 | 49 | 6 | wolf(36,13) |
| 38 | 22 | 90 | 40 | 32 | 49 | 6 | wolf(36,15) |
| 40 | 22 | 90 | 40 | 32 | 49 | 6 | wolf(36,11) |
| 6 | 36 | 160 | 60 | 35 | 46 | 9 | elk(39,13) |
| 8 | 36 | 160 | 60 | 35 | 46 | 9 | elk(39,4) |
| 20 | 36 | 160 | 60 | 35 | 46 | 9 | elk(39,25) |
| 22 | 22 | 90 | 40 | 38 | 43 | 12 | wolf(42,27) |
| 24 | 22 | 90 | 40 | 38 | 43 | 12 | wolf(42,25) |
| 26 | 22 | 90 | 40 | 38 | 43 | 12 | wolf(42,23) |
| 0 | 22 | 90 | 40 | 39 | 42 | 13 | wolf(43,15) |
| 2 | 22 | 90 | 40 | 39 | 42 | 13 | wolf(43,13) |
| 4 | 22 | 90 | 40 | 39 | 42 | 13 | wolf(43,11) |
| 10 | 22 | 90 | 40 | 39 | 42 | 13 | wolf(43,6) |
| 12 | 22 | 90 | 40 | 39 | 42 | 13 | wolf(43,4) |
| 28 | 22 | 90 | 40 | 39 | 42 | 13 | wolf(43,2) |
| 1 | 22 | 90 | 40 | 42 | 39 | 13 | wolf(46,15) |
| 3 | 22 | 90 | 40 | 42 | 39 | 13 | wolf(46,13) |
| 5 | 22 | 90 | 40 | 42 | 39 | 13 | wolf(46,11) |
| 11 | 22 | 90 | 40 | 42 | 39 | 13 | wolf(46,6) |
| 13 | 22 | 90 | 40 | 42 | 39 | 13 | wolf(46,4) |
| 29 | 22 | 90 | 40 | 42 | 39 | 13 | wolf(46,2) |
| 23 | 22 | 90 | 40 | 43 | 38 | 12 | wolf(47,27) |
| 25 | 22 | 90 | 40 | 43 | 38 | 12 | wolf(47,25) |
| 27 | 22 | 90 | 40 | 43 | 38 | 12 | wolf(47,23) |
| 7 | 36 | 160 | 60 | 46 | 35 | 9 | elk(50,13) |
| 9 | 36 | 160 | 60 | 46 | 35 | 9 | elk(50,4) |
| 21 | 36 | 160 | 60 | 46 | 35 | 9 | elk(50,25) |
| 15 | 0 | 35 | 50 | 49 | 32 | 6 | gold(53,27) |
| 17 | 0 | 35 | 50 | 49 | 32 | 6 | gold(53,25) |
| 19 | 0 | 35 | 50 | 49 | 32 | 6 | gold(53,23) |
| 31 | 22 | 90 | 40 | 49 | 32 | 6 | wolf(53,6) |
| 33 | 22 | 90 | 40 | 49 | 32 | 6 | wolf(53,4) |
| 35 | 22 | 90 | 40 | 49 | 32 | 6 | wolf(53,2) |
| 37 | 22 | 90 | 40 | 49 | 32 | 6 | wolf(53,13) |
| 39 | 22 | 90 | 40 | 49 | 32 | 6 | wolf(53,15) |
| 41 | 22 | 90 | 40 | 49 | 32 | 6 | wolf(53,11) |

### monolith — groups sorted by walking distance from our spawn

| id | gems | gold | respawn | d(our spawn) | d(enemy spawn) | d(→altar) | members |
|---|---|---|---|---|---|---|---|
| 7 | 22 | 90 | 40 | 6 | 44 | 19 | wolf(3,39) |
| 5 | 22 | 90 | 40 | 11 | 42 | 15 | wolf(15,46) |
| 11 | 36 | 160 | 60 | 13 | 36 | 16 | elk(13,38) |
| 9 | 22 | 90 | 40 | 14 | 43 | 18 | wolf(3,31) |
| 19 | 44 | 180 | 40 | 21 | 37 | 12 | wolf(9,24) wolf(8,24) |
| 13 | 0 | 35 | 50 | 23 | 27 | 3 | gold(26,31) |
| 3 | 44 | 180 | 40 | 25 | 42 | 7 | wolf(29,46) wolf(29,47) |
| 12 | 0 | 35 | 50 | 27 | 23 | 3 | gold(23,18) |
| 15 | 0 | 35 | 50 | 27 | 32 | 5 | gold(31,36) |
| 14 | 0 | 35 | 50 | 32 | 27 | 5 | gold(18,13) |
| 17 | 0 | 35 | 50 | 32 | 38 | 2 | gold(36,42) |
| 10 | 36 | 160 | 60 | 36 | 13 | 16 | elk(36,11) |
| 20 | 44 | 180 | 40 | 36 | 41 | 7 | wolf(3,9) wolf(4,9) |
| 18 | 44 | 180 | 40 | 37 | 21 | 12 | wolf(40,25) wolf(41,25) |
| 16 | 0 | 35 | 50 | 38 | 32 | 2 | gold(13,7) |
| 21 | 44 | 180 | 40 | 41 | 36 | 7 | wolf(46,40) wolf(45,40) |
| 2 | 44 | 180 | 40 | 42 | 25 | 7 | wolf(20,3) wolf(20,2) |
| 4 | 22 | 90 | 40 | 42 | 11 | 15 | wolf(34,3) |
| 1 | 392 | 300 | 150 | 43 | 46 | 1 | MONO(48,48) gem(49,47) gem(49,48) gem(49,49) gem(48,49) gem(47,49) gem(47,48) gem(48,47) gem(47,47) |
| 8 | 22 | 90 | 40 | 43 | 14 | 18 | wolf(46,18) |
| 6 | 22 | 90 | 40 | 44 | 6 | 19 | wolf(46,10) |
| 0 | 392 | 300 | 150 | 46 | 43 | 1 | MONO(1,1) gem(0,2) gem(0,1) gem(0,0) gem(1,0) gem(2,0) gem(2,1) gem(1,2) gem(2,2) |

### packs — groups sorted by walking distance from our spawn

| id | gems | gold | respawn | d(our spawn) | d(enemy spawn) | d(→altar) | members |
|---|---|---|---|---|---|---|---|
| 0 | 44 | 180 | 40 | 6 | 25 | 5 | wolf(8,2) wolf(8,3) |
| 4 | 44 | 180 | 40 | 14 | 25 | 11 | wolf(16,2) wolf(16,3) |
| 2 | 48 | 70 | 50 | 15 | 16 | 8 | gold(12,18) gold(11,17) gem(11,18) gem(12,17) |
| 3 | 48 | 70 | 50 | 16 | 15 | 8 | gold(18,12) gold(19,13) gem(19,12) gem(18,13) |
| 6 | 72 | 320 | 60 | 22 | 25 | 3 | elk(24,3) elk(24,2) |
| 1 | 44 | 180 | 40 | 25 | 6 | 5 | wolf(22,28) wolf(22,27) |
| 5 | 44 | 180 | 40 | 25 | 14 | 11 | wolf(14,28) wolf(14,27) |
| 7 | 72 | 320 | 60 | 25 | 22 | 3 | elk(6,27) elk(6,28) |

### pilgrimage — groups sorted by walking distance from our spawn

| id | gems | gold | respawn | d(our spawn) | d(enemy spawn) | d(→altar) | members |
|---|---|---|---|---|---|---|---|
| 4 | 0 | 35 | 50 | 10 | 113 | 44 | gold(5,13) |
| 6 | 0 | 35 | 50 | 19 | 104 | 35 | gold(3,22) |
| 8 | 0 | 35 | 50 | 28 | 95 | 26 | gold(5,31) |
| 10 | 68 | 215 | 50 | 40 | 82 | 13 | gold(3,43) wolf(3,44) wolf(4,43) gem(4,44) |
| 12 | 144 | 780 | 60 | 60 | 60 | 17 | elk(24,58) elk(25,58) elk(24,57) elk(25,57) gold(23,58) gold(26,58) gold(23,57) gold(26,57) |
| 20 | 0 | 70 | 50 | 66 | 66 | 23 | gold(24,43) gold(25,43) |
| 11 | 68 | 215 | 50 | 82 | 40 | 13 | gold(46,43) wolf(46,44) wolf(45,43) gem(45,44) |
| 19 | 0 | 70 | 50 | 83 | 83 | 27 | gold(24,26) gold(25,26) |
| 9 | 0 | 35 | 50 | 95 | 28 | 26 | gold(44,31) |
| 17 | 0 | 70 | 50 | 98 | 98 | 12 | gold(24,11) gold(25,11) |
| 7 | 0 | 35 | 50 | 104 | 19 | 35 | gold(46,22) |
| 18 | 0 | 70 | 50 | 106 | 106 | 9 | gold(24,3) gold(25,3) |
| 5 | 0 | 35 | 50 | 113 | 10 | 44 | gold(44,13) |
| 21 | 0 | 35 | 50 | 113 | 113 | 4 | gold(14,8) |
| 22 | 0 | 35 | 50 | 113 | 113 | 4 | gold(35,8) |
| 13 | 0 | 35 | 50 | 121 | 121 | 12 | gold(16,16) |
| 14 | 0 | 35 | 50 | 121 | 121 | 12 | gold(33,16) |
| 2 | 0 | 35 | 50 | 131 | 131 | 22 | gold(16,26) |
| 3 | 0 | 35 | 50 | 131 | 131 | 22 | gold(33,26) |
| 0 | 0 | 35 | 50 | 142 | 142 | 33 | gold(15,37) |
| 1 | 0 | 35 | 50 | 142 | 142 | 33 | gold(34,37) |
| 15 | 92 | 180 | 50 | 150 | 150 | 41 | wolf(16,46) gem(15,46) gem(16,45) wolf(15,45) |
| 16 | 92 | 180 | 50 | 150 | 150 | 41 | wolf(33,46) gem(34,46) gem(33,45) wolf(34,45) |

### rows — groups sorted by walking distance from our spawn

| id | gems | gold | respawn | d(our spawn) | d(enemy spawn) | d(→altar) | members |
|---|---|---|---|---|---|---|---|
| 0 | 66 | 270 | 40 | 13 | 73 | 11 | wolf(18,5) wolf(18,4) wolf(18,3) |
| 2 | 66 | 270 | 40 | 26 | 86 | 24 | wolf(31,5) wolf(31,4) wolf(31,3) |
| 6 | 448 | 600 | 150 | 36 | 36 | 25 | MONO(25,24) MONO(24,25) gem(24,24) gem(25,25) |
| 4 | 66 | 270 | 40 | 37 | 97 | 35 | wolf(42,5) wolf(42,4) wolf(42,3) |
| 7 | 0 | 105 | 50 | 43 | 60 | 32 | gold(48,17) gold(48,16) gold(48,18) |
| 8 | 0 | 105 | 50 | 60 | 43 | 32 | gold(1,32) gold(1,33) gold(1,31) |
| 1 | 66 | 270 | 40 | 73 | 13 | 11 | wolf(31,44) wolf(31,45) wolf(31,46) |
| 3 | 66 | 270 | 40 | 86 | 26 | 24 | wolf(18,44) wolf(18,45) wolf(18,46) |
| 5 | 66 | 270 | 40 | 97 | 37 | 35 | wolf(7,44) wolf(7,45) wolf(7,46) |

### rushbait-v2 — groups sorted by walking distance from our spawn

| id | gems | gold | respawn | d(our spawn) | d(enemy spawn) | d(→altar) | members |
|---|---|---|---|---|---|---|---|
| 0 | 22 | 90 | 40 | 7 | 59 | 30 | wolf(13,31) |
| 2 | 22 | 90 | 40 | 12 | 54 | 25 | wolf(13,26) |
| 11 | 36 | 160 | 60 | 12 | 62 | 33 | elk(13,50) |
| 4 | 22 | 90 | 40 | 17 | 49 | 20 | wolf(13,21) |
| 13 | 36 | 160 | 60 | 18 | 57 | 28 | elk(13,55) |
| 6 | 22 | 90 | 40 | 22 | 44 | 15 | wolf(13,16) |
| 15 | 36 | 160 | 60 | 23 | 52 | 23 | elk(13,60) |
| 8 | 22 | 90 | 40 | 26 | 40 | 11 | wolf(13,12) |
| 17 | 36 | 160 | 60 | 28 | 47 | 18 | elk(13,65) |
| 9 | 22 | 90 | 40 | 40 | 26 | 11 | wolf(16,67) |
| 7 | 22 | 90 | 40 | 44 | 22 | 15 | wolf(16,63) |
| 16 | 36 | 160 | 60 | 47 | 28 | 18 | elk(16,14) |
| 5 | 22 | 90 | 40 | 49 | 17 | 20 | wolf(16,58) |
| 14 | 36 | 160 | 60 | 52 | 23 | 23 | elk(16,19) |
| 3 | 22 | 90 | 40 | 54 | 12 | 25 | wolf(16,53) |
| 12 | 36 | 160 | 60 | 57 | 18 | 28 | elk(16,24) |
| 1 | 22 | 90 | 40 | 59 | 7 | 30 | wolf(16,48) |
| 10 | 36 | 160 | 60 | 62 | 12 | 33 | elk(16,29) |

### stampede — groups sorted by walking distance from our spawn

| id | gems | gold | respawn | d(our spawn) | d(enemy spawn) | d(→altar) | members |
|---|---|---|---|---|---|---|---|
| 2 | 0 | 70 | 50 | 11 | 50 | 23 | gold(13,1) gold(13,2) |
| 0 | 0 | 70 | 50 | 22 | 51 | 24 | gold(24,1) gold(24,2) |
| 11 | 0 | 140 | 50 | 27 | 27 | 27 | gold(3,24) gold(3,25) gold(2,24) gold(2,25) |
| 12 | 0 | 140 | 50 | 27 | 27 | 11 | gold(18,25) gold(18,24) gold(19,25) gold(19,24) |
| 4 | 0 | 70 | 50 | 32 | 61 | 34 | gold(34,1) gold(34,2) |
| 6 | 0 | 70 | 50 | 40 | 69 | 42 | gold(42,1) gold(42,2) |
| 8 | 0 | 70 | 50 | 48 | 77 | 50 | gold(50,1) gold(50,2) |
| 3 | 0 | 70 | 50 | 50 | 11 | 23 | gold(13,48) gold(13,47) |
| 1 | 0 | 70 | 50 | 51 | 22 | 24 | gold(24,48) gold(24,47) |
| 10 | 360 | 960 | 60 | 56 | 56 | 58 | elk(56,25) elk(56,24) elk(57,25) elk(57,24) elk(58,25) elk(58,24) gem(56,23) gem(56,26) gem(57,23) gem(57,26) gem(58,23) gem(58,26) |
| 5 | 0 | 70 | 50 | 61 | 32 | 34 | gold(34,48) gold(34,47) |
| 7 | 0 | 70 | 50 | 69 | 40 | 42 | gold(42,48) gold(42,47) |
| 9 | 0 | 70 | 50 | 77 | 48 | 50 | gold(50,48) gold(50,47) |

### swirl — groups sorted by walking distance from our spawn

| id | gems | gold | respawn | d(our spawn) | d(enemy spawn) | d(→altar) | members |
|---|---|---|---|---|---|---|---|
| 10 | 36 | 265 | 60 | 6 | 8 | 2 | elk(20,14) gold(20,13) gold(19,14) gold(19,13) |
| 20 | 0 | 35 | 50 | 6 | 13 | 2 | gold(20,8) |
| 11 | 36 | 265 | 60 | 8 | 6 | 2 | elk(24,15) gold(24,16) gold(25,15) gold(25,16) |
| 18 | 88 | 360 | 40 | 12 | 14 | 8 | wolf(13,13) wolf(13,12) wolf(14,12) wolf(14,13) |
| 21 | 0 | 35 | 50 | 13 | 6 | 2 | gold(24,21) |
| 19 | 88 | 360 | 40 | 14 | 12 | 8 | wolf(31,16) wolf(31,17) wolf(30,17) wolf(30,16) |
| 6 | 72 | 390 | 60 | 18 | 20 | 14 | elk(8,24) elk(7,24) gold(7,23) gold(8,23) |
| 8 | 0 | 35 | 50 | 18 | 20 | 14 | gold(8,18) |
| 7 | 72 | 390 | 60 | 20 | 18 | 14 | elk(36,5) elk(37,5) gold(37,6) gold(36,6) |
| 9 | 0 | 35 | 50 | 20 | 18 | 14 | gold(36,11) |
| 13 | 96 | 320 | 60 | 21 | 23 | 17 | elk(13,27) gem(13,28) elk(14,28) |
| 12 | 96 | 320 | 60 | 23 | 21 | 17 | elk(31,2) gem(31,1) elk(30,1) |
| 0 | 44 | 215 | 50 | 27 | 29 | 23 | gold(1,19) wolf(1,20) wolf(1,18) |
| 1 | 44 | 215 | 50 | 29 | 27 | 23 | gold(43,10) wolf(43,9) wolf(43,11) |
| 15 | 120 | 320 | 60 | 31 | 33 | 27 | elk(25,28) elk(26,28) gem(26,29) gem(25,29) |
| 14 | 120 | 320 | 60 | 33 | 31 | 27 | elk(19,1) elk(18,1) gem(18,0) gem(19,0) |
| 2 | 66 | 305 | 50 | 34 | 36 | 30 | gold(3,13) wolf(2,13) wolf(2,12) wolf(3,12) |
| 3 | 66 | 305 | 50 | 36 | 34 | 30 | gold(41,16) wolf(42,16) wolf(42,17) wolf(41,17) |
| 17 | 192 | 640 | 60 | 37 | 39 | 33 | elk(31,27) elk(32,27) elk(32,26) elk(31,26) gem(32,28) gem(31,28) |
| 4 | 150 | 430 | 60 | 39 | 41 | 35 | wolf(5,8) gem(5,6) gem(6,6) wolf(6,8) elk(5,7) wolf(6,7) |
| 16 | 192 | 640 | 60 | 39 | 37 | 33 | elk(13,2) elk(12,2) elk(12,3) elk(13,3) gem(12,1) gem(13,1) |
| 5 | 150 | 430 | 60 | 41 | 39 | 35 | wolf(39,21) gem(39,23) gem(38,23) wolf(38,21) elk(39,22) wolf(38,22) |
