# Electric Muffin — Cambridge Battlecode 2026 Autumn

C++ bot (`bots/apex`) plus the tooling and research behind it. See `docs/STRATEGY.md` for the
why, `docs/BRIEF.md` for the one-page game summary, `docs/SPEC.md` / `docs/MAPS.md` / `docs/FUEL.md`
for engine-measured facts, and `docs/official/` for the organisers' handbook and API export.

## Quick start

```bash
tools/setup.sh                      # venv + cambc 2.2.5 + SDK + map pool + toolchain cache
source .venv/bin/activate
cambc build bots/apex --release
cambc run bots/apex bots/spar-farmer --map elkring --release --statistics s.json --json
python3 tools/arena.py bots/apex bots/spar-hunter --jobs 8 --out replays/try1   # 17 maps x 2 seats
tools/gate.sh bots/apex             # must be 34-0 vs both starters, 0 faults, 0 fuel suspensions
```

## Submit

```bash
cambc login            # browser OAuth as the team account
cambc submit bots/apex # or: cambc bundle bots/apex --output apex.zip and upload on the Submissions page
```

## Layout

| path | what |
|---|---|
| `bots/apex/` | the competition bot (C++20, WASM via wasi-sdk) |
| `bots/spar-farmer`, `spar-hunter`, `spar-locker` | sparring opponents for regression gates |
| `bots/probe/` | engine probe: dumps constants, kits, items, map groups, fuel costs |
| `bots/starter-cpp`, `starter-python` | official starters (floor gate opponents) |
| `tools/arena.py` | parallel local tournaments with per-map tables and fuel-suspension detection |
| `tools/telemetry.py` | decode a replay bundle: per-turn fuel, bot stdout |
| `tools/gate.sh` | the floor gate every version must pass before submission |
| `maps/` | the 17-map official pool (`cambc maps pull` refreshes it) |
