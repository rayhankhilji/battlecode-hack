# REPLAY — decoding `.repl26a` replays into per-round state

`tools/replay_dump.mjs` replays a match through the **official engine** (the WASM the `cambc` visualiser ships) and
writes one JSON line per round per champion, plus events. `tools/replay_analyse.py` turns one or more dumps into a
side-by-side team A vs team B markdown report. `tools/replay_validate.py` proves the dump against the engine's own
`--statistics` JSON. Works on local bundles *and* on ladder bundles downloaded from the platform (same engine 0.0.3).

```
export PATH="/home/user/battlecode-hack/.venv/bin:$PATH"          # node 22 is at /opt/node22/bin (already on PATH)
node tools/replay_dump.mjs REPLAY.repl26a -o out.jsonl              # ~1.5-3.5 s per match
node tools/replay_dump.mjs REPLAY.repl26a --summary                 # result, draft, finish round, per-champion K/D/A, first tribute
python3 tools/replay_analyse.py out.jsonl [more.jsonl] -o report.md --names "A=apex,B=Mite"
python3 tools/replay_analyse.py replays/ladder/match-*.zip -o report.md     # runs the dump for you
python3 tools/replay_validate.py replays/apex-final-spar-farmer             # every replay with a sibling stats .json
```

`replay_dump.mjs` accepts the bundle zip (`.repl26a` or `.zip`, member `replay.repl26a`) or the inner record itself.
Options: `--every N` sample champion rows every N rounds (round rows and events are always complete), `--items` add the
42-slot item vector and 4 consumable counts to champion rows, `--no-events`, `--no-groups`, `--stdout`.

## How it works (and the layout assumptions)

- Engine: `~/.cache/cambc/assets/<sha>/visualiser/assets/v0_0_<N>-*.wasm` where N = byte 10 of the inner record
  (`a62per` magic; local and ladder replays from cambc 2.2.5 are v3). Set `CAMBC_ASSETS` if the cache lives elsewhere.
  The module only imports `wasi_snapshot_preview1` (stubbed) and is driven from Node 22 (`WebAssembly.instantiate`).
- Exports used: `Engine_Malloc`, `Game_FromReplayFileBytes(ptr,len,out)` → game handle (u64),
  `Game_GetBitboardMetadata/GetSymmetry/GetNumChampsPerTeam/GetTeamASpawnArea`, `Game_GetNumCamps/GetCampHandle/GetCamp`,
  `Game_GetNumCampGroups/GetCampGroupHandle/GetCampGroup/GetCampGroupCamp` (unmirrored *template* layout; group
  gem/gold/respawn values), `Game_Visualiser_GetConstants`, `Game_Runner_SetCheckpointsEnabled(game,1)`,
  `Game_Visualiser_Timeline_GetMain/Info/Seek(game,tl,0,0)`, `Game_Visualiser_Timeline_ReplayNext(game,1)` (one engine
  step), `Game_Visualiser_ReadState(game, perspective, snapshot)` with the spectator perspective `{team:2, champion:0xffffffff}`.
- Snapshot layout (184,696 bytes; the visualiser's `PanelChrome-*.js` asserts this size for v0.0.1/2/3 and uses one
  reader, so the layout is version-independent): header `round@8 phase@24 outcome@28 submitted u64@32/@40 inputPos@56`,
  step `kind@68 actorKind@72 actor@76 from@80/84 to@88/92 round@96 substep@104 abilitySlot@108`; champions
  `count@120 data@140 stride 456` with `index@4 team@8 type@12 known@16 x@20 y@24 hp f32@28 maxHp@32 buffs@40 alive@224
  respawn@228 gold@232 gems@236 items@240(42×u32) consumables@412(4) channelTimer@436 channelType@440 priority@444`;
  extensions `earnedGold u64@184248+16·idx`, `K/D/A u32×3@184504+12·idx`; camps `count@124 data@7436 stride 232`
  (`x@20 y@24 hp@28 group@224 awake@228`); groups `count@128 data@123436 stride 20` (`state rewardAvailable captureTeam
  captureTimer respawnRounds`); wards `count@132 data@133436 stride 40`; spells `count@136 data@138556 stride 40`
  (owner@12); drafts `@179516+12+20·team+4·k`. Step kinds: 2 Capture, 4 Move, 5 Attack, 6 Cast, 11 Recall, 12 Submit
  gems, 13 Shop, 14 Ward, 15 Consumable, 16 Cancel, 18 Gems submitted, 20 Altar lock, 21 Teleport, 23 Movement
  blocked, 25 Sell, 26 Turn rollover (= fuel exhausted), 27 Bot error. Champion types: 0 Ryth 1 Vesper Vale 2 Vendetta
  3 Cassi 4 Issac 5 Stellar 6 Prism 7 Marcella 8 Willow 9 Ren 10 Varg 11 Tavi. Camp types (`camp_type` enum):
  0 wolf 1 elk 2 gem_cluster 3 gold_cluster 4 monolith. Channel types: 0 none, 1 recall, 2 submit gems.
- Live camps are matched to template camps by position under identity / mirror-x / mirror-y / rot180, which gives
  each live group its gem/gold/respawn values; `replay_analyse.py` then matches live groups to `docs/MAPS.md` ids by
  member tiles (so group labels are the same `gN` ids as MAPS.md and STRATEGY.md).
- Validation (`replay_validate.py`, 34/34 replays of `replays/apex-final-spar-farmer`, cambc 2.2.5 / engine 0.0.3):
  team submittedGems, winner, finish round, per-champion kills / deaths / assists / submittedGems / final gems / gold /
  earnedGold, every death round, number of gemSubmission events and number of campClear events (credited champion
  included — the engine credits the champion whose action killed the last monster of a group, a spell's owner, or the
  champion a monster hit when retaliation killed it) all match **exactly**. `fuel_out` events equal `fuelExhausted`
  events on the replays that have them.

## Dump records

| type | fields |
|---|---|
| `meta` | `engine width height symmetry team_size spawn_a constants draft_rounds drafts[team][{id,name}] camps[{id,type,type_name,x,y,group,max_hp}] groups[{id,gems,gold,respawn,camps,tiles}]` |
| `champ` (per round, per body) | `round team id name x y hp max_hp gems gold alive respawn channel channel_timer kills deaths assists earned_gold n_items priority acted{move,attack,cast,shop,ward,consumable,lock,recall,submit,teleport,blocked,fuel_out,error}` — state at the **end** of the round; `x,y` null while dead |
| `round` | `round submitted[A,B] carried[A,B] alive[A,B] wards[A,B] groups[[id,state,capture_team,capture_timer,respawn_rounds]…]` (state 1 has camps, 2 cleared/awaiting claim, 3 respawning) |
| `event` | `kind` ∈ submission (team, champion, amount, total), submit_start (gems, channel_rounds), recall, capture (group, gems, gold, party = team champions on the group tiles, party_near ≤2 tiles, party_ids, gem_split [[id,+gems]]), camp_kill, group_clear, death (gems_lost, gold, by_team/by_id/by_name/by_kind), kill (gems_gained = loot), respawn, cast (slot, ability_type), buy, sell, ward, altar_lock, teleport, consumable, cancel, fuel_out, bot_error — all with `round` and the actor's `x,y` |
| `final` | `outcome winner finish_round steps submitted champions[{id,team,name,kills,deaths,assists,gems,gold,earned_gold,items,consumables,alive}] captures[A,B]` |

Team 0 = team A, 1 = team B. Champion ids are the engine's (0..N-1 team A, then team B; Cassi & Issac are two ids).

## Report metrics (`replay_analyse.py`)

| metric | definition |
|---|---|
| first capture / first tribute | round of the team's first `capture` / first `submission` event |
| tributes (count, avg bag) | number of successful gem submissions and mean gems per submission |
| banked by round R | team `submitted` in the round row R (last round if the match ended earlier) |
| captures (groups cleared) | completed reward captures (a multi-camp group counts once; respawned groups count again) and `group_clear` events credited to the team |
| avg party size at capture | champions of the team that received gems at the capture (`gem_split`), or those within 2 tiles for gold-only groups |
| gems collected | sum of positive changes of a champion's bag between consecutive rounds (captures + kill loot) |
| gems lost on death / looted from kills | bag of the victim when it died / `gems_gained` of the killer |
| champion-rounds dead / channelling | rounds in which a body was dead / was in a recall or tribute channel |
| stationary fraction | alive, not channelling, and position **and** bag unchanged since the previous round — the brief's "idle" definition; it includes standing still hitting a camp |
| idle fraction | alive, not channelling, and no move / attack / cast / shop / ward / lock / consumable / recall / submit / teleport in the turn — turns that did nothing |
| distance travelled | sum of Chebyshev steps between consecutive alive positions (≤3 tiles; bigger jumps are respawns and are not counted) |
| gems/100r | gems collected × 100 / finish round |
| route | sequence of camp groups a champion stood within 1 tile of for ≥2 consecutive rounds, with the round span |
| farming order | the order in which groups were first captured by the team; `d own/altar` = walking distance from the team's own spawn and from the group to the nearest altar (docs/MAPS.md; for team B the mirrored column is used) |
| clear→capture delay | (printed by the deeper per-round scripts) rounds between the group's last monster dying and the reward; the minimum is 6 |

Caveats: positions of dead champions are unknown (null); kill attribution uses the engine's K/D/A counters (exact),
death "killed by" falls back to the step's actor when no counter moved (camp kills); the route heuristic can miss a
ranged champion attacking from 3 tiles away. Replays truncated with `--rounds` have no outcome (`winner` null) and
the dump stops at the last recorded step.
