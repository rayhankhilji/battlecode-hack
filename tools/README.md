# tools/ — local evaluation

All scripts are Python 3 stdlib / bash and need `cambc` on PATH:

```
export PATH="/home/user/battlecode-hack/.venv/bin:$PATH"
```

| file | purpose |
|---|---|
| `arena.py` | BOT_A vs BOT_B over the map pool, both seats, in parallel; per-map table, aggregate, `summary.json`/`summary.md` |
| `telemetry.py` | inspect one replay bundle: team stdout/stderr/debug, engine fuel log, digest of `key=value` telemetry lines |
| `gate.sh` | floor gate: candidate must beat both starters on every map from both seats with zero faults |
| `replaylib.py` | shared replay-zip / statistics-JSON parsing used by the two scripts |
| `setup.sh` | one-shot environment setup (cambc, SDK, maps, toolchain cache) |

## arena.py

```
tools/arena.py BOT_A BOT_B [--maps all|elkring,ambush] [--sides both|a|b] [--jobs 8] [--rounds N]
               [--out DIR] [--no-build] [--timeout 900] [--fuel N] [--backend batch|run] [--fail-on-suspend]
```

- Everything is oriented to **BOT_A (the candidate)**: `win` = BOT_A won; `side A` = BOT_A sat in the
  teamA seat; `gems A/B`, `kills A/B` etc. are BOT_A/BOT_B regardless of seat.
- Builds both bots with `cambc build <bot> --release` first (skip with `--no-build`), then runs one
  `cambc run-batch` with `--jobs N --release --json --timeout T` over a generated manifest.
- Output dir (default `replays/arena-<A>-vs-<B>-<timestamp>/`): `<map>-<side>.repl26a`,
  `<map>-<side>.json` (engine statistics), `manifest.txt`, `run-batch.log`, `summary.json`, `summary.md`.
- Table columns: `faults` = `diagnostics.botFaults` (WASM traps; a trapped champion faults *every*
  later turn, so counts are large), `susp` = fuel suspensions (`fuelExhausted` events, play turns),
  `err` = lines matching `bot error` in that team's stdout (the bot's own caught exceptions).
  `summary.json` also has per-bot `draft_suspensions`, `first_suspension_round`, `fuel_peak_used`
  (max fuel used in any non-first play turn, from the `.fuel` log) and `stdout_error_samples`.
- Exit codes: `0` all matches finished and nobody faulted; `2` a match errored (timeout, crash, missing
  statistics); `3` either bot faulted (or BOT_A suspended with `--fail-on-suspend`); `4` usage/build error.
  Winning is **not** part of the exit code — `gate.sh` checks that from `summary.json`.
- `--rounds N`: a match still unfinished at N rounds is reported `TRUNCATED` by the engine and **no
  statistics are written**, so the row shows `result=truncated` with no gems/winner (not an error).
  Use it only for smoke/fuel tests; the replay is still written and `telemetry.py` can read it.
- `--backend run` uses a thread pool of `cambc run` processes instead of `run-batch` (fallback only;
  see "Backend choice" — it is 2x slower and races on builds).
- `--analyse-only` re-parses an existing `--out` dir (same BOT_A/BOT_B/maps/sides) without playing,
  e.g. after changing the parsers; it keeps the recorded wall time.

### Runtime

Container: 4 cores. One starter-vs-starter match takes 12–20 s wall but ~2.5 cores of CPU (the engine
runs champion instances in parallel), so parallelism saturates around `--jobs 4`; `--jobs 8` is fine
(the engine just time-slices). Measured for the 34-match panel (17 maps × both sides,
`starter-cpp` vs `starter-python`, `--jobs 8`): **164 s** (see "Measured" at the bottom).
Matches against a bot that finishes faster (a real bot wins in ~400–600 rounds) take proportionally less.

### Backend choice

`cambc run-batch` is the default: it really parallelises (N independent runner processes), builds
once, applies a per-match wall-clock `--timeout`, writes `<replay>.error.log` for failures, and prints
clean one-line statuses. Its quirks (handled by arena.py): manifest paths must be absolute (bare map
names are not resolved like `cambc run --map` does, and bot paths resolve relative to the manifest),
it refuses to start if any row's replay path already exists, a bot build failure aborts the whole batch
before any match, and faulted matches still count as "saved" with exit 0 (faults must be read from the
statistics). The thread-pool fallback (`--backend run`) has to rename the auto-named bundle
(`<a>-vs-<b>-<ms>.repl26a`), gives no engine-side timeout, was 2.2x slower on the panel, and lost 2 of 34
matches to a build race (each `cambc run` rebuilds/checks the bot in `.cambc/build`); kept for debugging only.

## telemetry.py

```
tools/telemetry.py REPLAY.repl26a [--team a|b|both] [--raw] [--tail N] [--grep REGEX] [--dump DIR]
                                  [--team-size N] [--stats STATS.json] [-v]
```

Prints, per team: stream sizes, `bot error` line count with samples, the **engine fuel log**
(per champion body slot: turns, suspended turns, first-turn fuel used, max/mean fuel used, min remaining) and
a digest of telemetry lines. If `<replay stem>.json` exists next to the replay (arena writes it) the
outcome, faults, suspensions and drafts are shown too and the fuel-log layout (picks / bodies) is taken from
the roster; otherwise it is guessed from the heavy first-turn signature (correct on 20/20 starter logs) or
given with `--picks N --bodies N` (bodies = picks + 1 when Cassi & Issac was drafted).

Telemetry lines are any lines containing `key=value` tokens, e.g.
`T 300 c=2 role=farm fuel_min=41200 susp=0 gems=120`. Grouping: by a `champ|champion|c|id|name|who|slot`
key if present, else by the leading non-numeric words before the first `key=value`. Per group it reports
line count, round range (`round|r|t` key or the first integer on the line), the **min** of
`fuel_min|fuel|fuel_rem|fuel_remaining`, the **max** of `susp|suspensions|suspended`, and the last value
of every other key. `--raw`/`--tail N` print the NUL-stripped streams (filtered by `--grep`).

## gate.sh

```
tools/gate.sh BOT [--jobs 8] [--maps all|a,b] [--out DIR] [--strict-fuel]
```

Builds BOT, runs `arena.py BOT bots/starter-cpp` and `arena.py BOT bots/starter-python` over all maps
both sides, prints both aggregates and a verdict per opponent. Exit 0 only if BOT **won every game**,
no match errored and **nobody faulted** (`--strict-fuel` also demands zero candidate suspensions).
Output under `replays/gate-<bot>-<time>/{starter-cpp,starter-python}/`. Expect roughly twice the panel
time above.

## Replay bundle (`*.repl26a`)

A zip: `replay.repl26a` (binary replay for the visualiser), `team-a.stdout`, `team-b.stdout`,
`team-a.debug`, `team-b.debug`, `team-a.fuel`, `team-b.fuel`. There is no stderr member in 2.2.5
(the scripts tolerate one). The stdout streams contain stray NUL bytes between writes (engine framing,
1–20 per line, no structure) — strip them. **Buffered `std::cout` output is lost when a champion
traps**, so flush (`std::endl`) before anything risky. Each champion runs in its own instance and nothing
in the stream says which champion wrote a line: put a champion id in your telemetry prefix.

`team-X.fuel` (decoded in `replaylib.parse_fuel_file`): magic `a62leuf` (7 B), 6 header bytes, 8 × u64 LE
quanta = (draft first turn, draft turn, play first turn, play turn) for native bots then the same four
for Python (×25; both blocks appear in both files — pick the one the records fit under), then one u64
per bot turn = **fuel used**: first `picks` records are the draft turns, then for each round one record per
champion **body** (picks + 1 with Cassi & Issac; dead champions still get one; order within a round follows
turn priority). A record
equal to its quantum ⇔ that turn ran out of fuel; totals match the `fuelExhausted` events exactly.
Defaults seen: draft 1,000,000 / 100,000, play 200,000 (first) / 100,000; Python ×25.

## Statistics JSON (`--statistics x.json --json`, formatVersion 1.0, engine 0.0.3)

Only written for **completed** matches (not for `--rounds`-truncated ones). ~0.5–2 MB. Integer-valued
metrics are encoded as **strings** (`"414"`), damage/healing as floats — use `replaylib.num`.

| key | meaning |
|---|---|
| `match.outcome` | `"teamA"` / `"teamB"` (also at the 2000-round cap, after tiebreaks) |
| `match.roundsCompleted`, `roundsStarted` | the winning tribute lands mid-round, so started = completed + 1 and `finalRoundPartial: true` |
| `match.width/height/mapHash/seed/engineVersion/engineBuildHash` | map + engine identity; `finalInput`, `finalSubstep` are engine cursors |
| `teams[]` | `{team, submittedGems}` — the score |
| `roster.draft[]` | pick order: `{team, champion}` alternating A,B,A,B…; `roster.champions[]` adds `id` (0..N-1 team A then team B) and `draftPick` |
| `champions[]` | per champion: `id, name, team, metrics{…}, items[], itemsBought[], consumables[], consumablesBought[]` (`{item, quantity}`) |
| `champions[].metrics` | `kills, deaths, assists, collectedGems, submittedGems, gems` (carried at end), `gold` (held at end), `earnedGold, spentGold, saleRefunds, championDamageDone/Taken, monsterDamageDone/Taken, healingDone/Received, shieldingDone` |
| `diagnostics.botFaults` | `{teamA: n, teamB: n}` — faulted turns (a trapped instance is restarted and faults every later turn) |
| `diagnostics.codes`, `detailedCaptureComplete` | fault codes (empty so far); whether the detailed event capture finished |
| `events[]` | one entry list per (owner, kind): kinds `death, kill, assist, wardPlacement, buyItem, buyConsumable, sellItem, campClear, gemSubmission, gemCollection, gemLoss, itemCombination, botFault, fuelExhausted, resignation`; owner `{kind: champion, id, name, team}` or `{kind: draft, team}` for the draft bot; each entry `{timestamp{round, phase, substep, ordinal, input}, value, amount, context, target, x, y}` |
| `championTimelines[]`, `teamTimelines[]` | run-length encoded per-round series (`runs: [{duration, value}]`, `sampleCount` = rounds+1) for every metric above / team `submittedGems` |
| `heatmaps[]` | run-length encoded 1-D grids (`kind` death/kill/wardPlacement) per champion |

`replaylib.stats_team_view(stats, "teamA")` folds this into `{gems, faults, suspensions,
draft_suspensions, first_suspension_round, draft[], kills, deaths, …}`.

## Measured (18 Oct 2026, this 4-core container, starter-cpp vs starter-python, 17 maps x both seats = 34 matches)

| backend | `--jobs` | wall (matches only) | end-to-end (`time arena.py`, incl. builds + analysis) | result |
|---|---|---|---|---|
| `run-batch` (default) | 8 | **164.4 s** | 2m53s real, 8m52s user | 34/34 completed, 0 errors, 0 faults |
| thread pool of `cambc run` | 8 | 357.9 s | 5m59s real | 32/34: **2 matches errored** with corrupted-module reads (`unexpected end-of-file`, `expected at least one module field`) because every `cambc run` process re-checks/builds the bot in `.cambc/build` concurrently |

A single starter match: 10–20 s wall, ~2.5 cores. 4 matches at `--jobs 4`: 25 s. So the panel is CPU-bound
at ~5 matches/minute here; `gate.sh` (two panels) takes ~6 minutes. `cambc build --release`: C++ ~6 s
(incremental: instant), Python ~8 s.

Engine quirks observed (cambc 2.2.5 / engine 0.0.3):

- `run-batch` manifest: bot and map columns must be absolute paths (bare map names are rejected with
  "map does not exist"; relative bot paths resolve against the manifest's directory, not cwd).
- `run-batch` refuses to start if any row's replay path exists ("output already exists"), validates every
  row and aborts the whole batch (exit 1) before any match on a bad row or a failed build.
- `run-batch` status lines: `[|] STARTING`, `[+] TEAM A WIN (name)` / `[+] TEAM B WIN (name)` /
  `[+] TRUNCATED` with `Round NNNN/2000`, `[x] FAILED: timeout (log: <replay>.error.log)`, each followed by an
  indented `Replay: <path>` line, then `Finished: N replay bundles saved, M failed` and exit 1 + `error: M matches failed`.
  Faulted or suspended bots do **not** make a match "failed" — read `diagnostics.botFaults` / `fuelExhausted` events.
- `cambc run` names its bundle `<botA>-vs-<botB>-<unix ms>.repl26a` in `--out-dir`; `run-batch` uses exactly the
  manifest's path. Statistics JSON is only written for finished matches (not under a `--rounds` cap).
- `--statistics` integer metrics are strings; `teams[].submittedGems` is the authoritative score.
- A WASM trap restarts that champion's instance, which then faults again every turn (so `botFaults` is in the
  thousands for one bug), and unflushed `std::cout` text is lost on the trap.
- Team stdout inside the bundle has stray NUL bytes; there is no stderr member and `.debug` is empty in release.
- First play turn gets 200,000 fuel (draft first turn 1,000,000); the starters use 75–120k of it, so a bot that
  does heavy init on turn 1 is fine up to 2x the normal quantum but not beyond.

## replay_dump.mjs / replay_analyse.py / replay_validate.py

Full per-round, per-champion decoding of any `.repl26a` (local or ladder bundle) through the official engine WASM
that the `cambc` visualiser caches under `~/.cache/cambc/assets/`. See `docs/REPLAY.md` for the record format, the
snapshot layout assumptions and every metric definition.

```
node tools/replay_dump.mjs REPLAY.repl26a -o out.jsonl [--every N] [--items]     # JSONL: meta, champ rows, round rows, events, final
node tools/replay_dump.mjs REPLAY.repl26a --summary                              # winner, finish round, drafts, K/D/A, first tribute
tools/replay_analyse.py out.jsonl [more.jsonl|REPLAY.repl26a ...] -o report.md --names "A=apex,B=Mite"   # A vs B markdown report
tools/replay_validate.py DIR|REPLAY.repl26a [STATS.json]                         # exact cross-check against --statistics JSON
```

Validated 34/34 on `replays/apex-final-spar-farmer` (gems, winner, finish round, per-champion K/D/A, gems, gold, earned
gold, death rounds, submission and campClear counts all exact). ~1.5–3.5 s per match. `docs/MITE-ANALYSIS.md` is the
report for the two ladder games against Mite.
