# Strategy — how we intend to win Cambridge Battlecode 2026 Autumn

Status: 18 Oct 2026 (Sprint 1 lock day). Numbers below are measured; opinions are marked.

## 1. What the data says

**Language.** Live ladder on 18 Oct (633 teams): ranks 1–25 are 19 C++, 5 Rust, 1 OCaml; ranks 26–50 are
22 C++, 3 Rust; the best Python team is rank 69 despite Python's 25× fuel. The Python interpreter burns
~550 fuel per loop iteration against ~2.2 for compiled code, so the "fair" multiplier is not fair.
Decision: C++. Not contrarian, just correct.

**Draft meta.** In the top 50's "most picked" lists: Cassi & Issac 46/50, Stellar 45/50, Ren 24, Varg 18,
Ryth 9, Tavi 6. Two bodies for one pick (duo) plus the highest burst caster (Stellar: 200 single + 140 AoE at
range 4 every 8 rounds, outside the monsters' range 3) is the economy engine; Ren/Varg/Ryth are the
durable fronts. We use the same core (duo, Stellar, Ren, Varg, Ryth prefix by team size) until our own
A/B panels say otherwise. The draft is only a small part of the edge: the same trio appears at both ends
of the ladder.

**What top teams do differently (replay inspection, Fish vs VeryGood on elkring).** 25 vs 14 captures,
8 vs 1 kills, 5,275 vs 2,780 gold. The leaders farm faster *and* win the fights at objectives; the two
feed each other (kills transfer the whole bag + 50 gold).

**Maps decide the game type** (docs/MAPS.md). Nine maps have a safe home economy (corners, eight,
elkring, expanse, interlock, monolith, packs, pilgrimage, rows). The others are contested by
construction: swirl (every group equidistant, altar 2 steps from spawn), lanes (18 altars, every camp in a
central band 32–49 steps from both spawns), stampede (all 360 gems in one 12-camp group), rushbait-v2
(spawns adjacent), fragment (204 gems per cycle: the smallest supply), contest (gems only from two
monoliths, so the gold tiebreak decides). On those maps denial and kills beat farming speed.

**Engine facts that shape the design** (docs/SPEC.md, docs/FUEL.md, measured): monsters hit only the
nearest champion in range 3, so a ranged body at distance 4 takes zero damage; capture is exactly 5
uncontested rounds and extra bodies only split the reward; tribute lasts ceil(bag/10) turn-ends and
damage does not stop it, only stun/death/lock; a 10-gold altar lock cancels enemy tributes and needs 3
hits to clear; the draft bot has ~980k fuel and can see the whole map, so the plan is made there;
printing floats costs 4k fuel each and per-tile host calls cost ~40 fuel, which is why the bot keeps its
own grids and prints one line per 100 rounds.

## 2. The bot (bots/apex)

1. **Draft-time planning.** Pick first (a fuel-starved draft gets random picks), then compute path
   distance fields from spawn, enemy spawn and every altar, score every camp group by delivered gems per
   round of a full cycle (travel, clear, capture, delivery, respawn), decide squads (melee front pools for
   elks/monoliths, solo circuits for clusters and single wolves) and write the circuits into the 1,024-byte
   draft memory. Champions only execute.
2. **Correct state machines.** TRAVEL → CLEAR (front / ranged / poke / wait) → CAPTURE → DELIVER →
   TRIBUTE → RECOVER, with explicit preconditions each turn (dead, stunned, channelling). Full kit use:
   attack → reset → attack, spells on cooldown, Varg shield before bursts, dashes on legal paths, one cast
   per turn, exclusive-with-movement respected.
3. **Opportunity-cost banking.** Bag thresholds from the plan (short altar distance → bank often),
   immediate bank when it wins, batching when the next camp is on the way.
4. **Fuel discipline.** Peak play turn 71–79k of 100k, zero suspensions in 170 panel games; cached
   groups, own grids, hand-formatted telemetry.
5. **Combat and denial** (wave 2): threat model, carrier safety and escorts, kill-steals on submitting
   or heavy carriers with stun-first focus, fight-or-flee from a 2-turn exchange estimate, kiting,
   altar locks on the enemy's altar when a map is contested, wards on tribute approaches.

## 3. Measured results (tools/arena.py, 17 maps × both seats)

| apex v1 vs | result | mean winning round |
|---|---|---|
| starter-cpp | 34–0 | 813 |
| starter-python | 34–0 | 839 |
| spar-farmer (greedy meta farmer) | 22–12 | 912 |
| spar-hunter (farmer + 2 carrier hunters) | 23–11 | 1040 |
| spar-locker (farmer + altar locker) | 23–11 | 935 |

Zero faults and zero fuel suspensions in all 170 games. The sparring bots themselves beat starter-cpp
16/17 with mean winning rounds 757 / 783 / 851, i.e. our plain greedy farmer currently out-farms the
planned bot on packs, rows, pilgrimage, interlock and ambush. That throughput gap and the deaths on
contested maps (fragment vs hunter: 71 deaths) are the two wave-2 targets.

## 4. Where the edge is going to come from (opinion, to be tested)

- **Turn economy is the whole game.** 1,000 gems in ~450 rounds is what the leaders do on elkring. Every
  idle turn (waiting for a squad, over-sized capture parties, walking to the wrong altar) is the loss.
  Our panels report finish rounds per map; the target is sub-600 on the home-economy maps.
- **Fights are an economy decision.** A kill is worth the bag + 50 gold + 60 enemy turns; a death costs the
  same. The exchange model only commits when it wins on expectation or secures a tribute.
- **Altar locks are under-used.** 10 gold cancels a channel and costs the enemy 3 attacks; on swirl, lanes,
  packs and rushbait a permanently locked enemy altar is a large swing. Nobody in the top-50 pick data is
  visibly doing this (we cannot see their code), so it is our first real experiment.
- **Contest and fragment are tiebreak maps.** Lifetime gold decides contest; we farm gold clusters
  continuously there and avoid deaths.
- **Draft A/B.** Marcella (stun-immune carrier) against the stun-heavy meta, Willow escorts for the
  duo, Vesper on gold-rich maps: all cheap to test in the arena once the economy is at parity.

## 5. Process

- Every change is measured on a fixed panel (17 maps × 2 seats) against the three sparring bots plus
  the floor gate (34–0 vs both starters, 0 faults, 0 suspensions, peak fuel < 90k). Promote on wins.
- Use `cambc match-groups create <team_id>` (unranked, 5 maps) against top-50 teams and download the
  replays: it is the only way to see how we fare against real leaders.
- Sprint timeline: S1 lock 18 Oct (economy + basic combat), S2 lock 24 Oct (combat, denial, draft A/B),
  S3 lock 31 Oct (map-specific plans, dash/TP shortcuts on ambush/pilgrimage, monolith sieges with
  Focus-stacked casters on contest/monolith/rows).
