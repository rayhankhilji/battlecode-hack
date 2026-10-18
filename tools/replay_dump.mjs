#!/usr/bin/env node
// replay_dump.mjs — decode a Cambridge Battlecode .repl26a replay (bundle zip or inner record)
// into per-round, per-champion JSONL by driving the official engine WASM that `cambc`
// ships inside its visualiser (~/.cache/cambc/assets/<sha>/visualiser/assets/v0_0_N-*.wasm).
//
//   node tools/replay_dump.mjs REPLAY.repl26a -o out.jsonl        # full dump
//   node tools/replay_dump.mjs REPLAY.repl26a --summary           # result / draft / finish round
//   node tools/replay_dump.mjs REPLAY.repl26a --stdout            # JSONL to stdout
//   options: --every N (sample champion rows every N rounds), --no-events, --no-groups
//
// Record types (one JSON object per line):
//   meta   : engine version, map size, symmetry, drafts, camps (spawn tile, type, group), groups (gem/gold/respawn)
//   champ  : round team id name x y hp max_hp gems gold alive respawn channel channel_timer kills deaths assists
//            earned_gold items n_items priority acted{move,attack,cast,shop,ward,consumable,blocked,fuel_out,error,recall,submit,teleport,lock}
//   round  : round submitted[a,b] carried[a,b] alive[a,b] groups[[id,state,capture_team,capture_timer,respawn_rounds]...]
//   event  : kind in {submission, submit_start, death, kill, capture, camp_kill, group_clear, recall, teleport, altar_lock, ward,
//            cancel, fuel_out, bot_error, movement_blocked, buy, sell, consumable, respawn} with round/team/id/x/y/...
//   final  : outcome, winner, finish round, submitted, per-champion kda / earned gold
//
// Layout: the visualiser's PanelChrome chunk (`Q` tables, `Sr` header, `Cr` readers) — identical for engine
// v0.0.1/2/3: the glue asserts Game_Visualiser_SnapshotSize()==184696 for all three. Spectator perspective is
// {team:2, champion:0xffffffff}. See docs/REPLAY.md.
import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import zlib from 'node:zlib';

const SNAPSHOT_SIZE = 184696;
const CHAMP_TYPES = ['Ryth', 'Vesper Vale', 'Vendetta', 'Cassi', 'Issac', 'Stellar', 'Prism', 'Marcella', 'Willow', 'Ren', 'Varg', 'Tavi', 'Heart of Ruin'];
const DRAFT_NAMES = ['Ryth', 'Vesper Vale', 'Vendetta', 'Cassi & Issac', 'Stellar', 'Prism', 'Marcella', 'Willow', 'Ren', 'Varg', 'Tavi', 'Heart of Ruin'];
const CAMP_TYPES = { 0: 'wolf', 1: 'elk', 2: 'gem_cluster', 3: 'gold_cluster', 4: 'monolith' }; // sdk/cpp/cambc.h enum class camp_type
const STEP = ['Phase', 'Respawn', 'Capture', 'Tick', 'Move', 'Attack', 'Cast ability', 'Input', 'End turn', 'Set priority', '',
  'Recall', 'Submit gems', 'Shop', 'Place ward', 'Consumable', 'Cancel channel', 'Sandbox', 'Gems submitted', 'Draft champion',
  'Place altar lock', 'Teleport channel', 'Teleport complete', 'Movement blocked', 'Resign', 'Sell item', 'Turn rollover', 'Bot error'];
const K = Object.fromEntries(STEP.map((n, i) => [n, i]));
const GROUP_STATE = ['unknown', 'has_camps', 'awaiting_claim', 'respawning'];
const PHASE = { 0: 'initial', 1: 'camp_respawns', 4: 'spells', 7: 'camp_captures', 9: 'camps', 10: 'champions', 11: 'finished', 12: 'draft', 13: 'champion_end_turn', 14: 'champion_start_turn' };
const T = { champions: { count: 120, data: 140, stride: 456 }, camps: { count: 124, data: 7436, stride: 232 },
  groups: { count: 128, data: 123436, stride: 20 }, wards: { count: 132, data: 133436, stride: 40 },
  controlWards: { count: 179568, data: 179572, stride: 36 }, spells: { count: 136, data: 138556, stride: 40 }, draft: 179516, submissionDuration: 184184, earnedGold: 184248, kda: 184504 };
const N_ITEMS = 42;

function parseArgs(argv) {
  const o = { file: null, out: null, every: 1, summary: false, stdout: false, events: true, groups: true, items: false };
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (a === '-o' || a === '--out') o.out = argv[++i];
    else if (a === '--every') o.every = Math.max(1, Number(argv[++i]) || 1);
    else if (a === '--summary') o.summary = true;
    else if (a === '--stdout') o.stdout = true;
    else if (a === '--no-events') o.events = false;
    else if (a === '--no-groups') o.groups = false;
    else if (a === '--items') o.items = true;
    else if (a === '-h' || a === '--help') { console.log(fs.readFileSync(new URL(import.meta.url)).toString().split('\n').slice(1, 22).map((l) => l.replace(/^\/\/ ?/, '')).join('\n')); process.exit(0); }
    else if (!o.file) o.file = a;
    else throw new Error('unexpected argument ' + a);
  }
  if (!o.file) throw new Error('usage: replay_dump.mjs REPLAY.repl26a [-o out.jsonl] [--summary] [--every N]');
  return o;
}

function findWasm(ver) {
  const pat = new RegExp(`^v0_0_${ver}-.*\\.wasm$`);
  const roots = [process.env.CAMBC_ASSETS, path.join(os.homedir(), '.cache/cambc/assets'), '/root/.cache/cambc/assets'].filter(Boolean);
  for (const root of roots) {
    if (!fs.existsSync(root)) continue;
    for (const sha of fs.readdirSync(root)) {
      const dir = path.join(root, sha, 'visualiser/assets');
      if (!fs.existsSync(dir)) continue;
      const f = fs.readdirSync(dir).find((n) => pat.test(n));
      if (f) return path.join(dir, f);
    }
  }
  throw new Error(`visualiser wasm v0_0_${ver} not found under ~/.cache/cambc/assets (run \`cambc view\` once or set CAMBC_ASSETS)`);
}

// Minimal zip reader (stored / deflate) so the tool needs nothing but Node.
function zipEntries(buf) {
  let eocd = -1;
  for (let i = buf.length - 22; i >= Math.max(0, buf.length - 70000); i--) if (buf.readUInt32LE(i) === 0x06054b50) { eocd = i; break; }
  if (eocd < 0) throw new Error('zip: no end-of-central-directory');
  const n = buf.readUInt16LE(eocd + 10), cdOff = buf.readUInt32LE(eocd + 16);
  const out = new Map();
  let p = cdOff;
  for (let i = 0; i < n; i++) {
    if (buf.readUInt32LE(p) !== 0x02014b50) throw new Error('zip: bad central directory');
    const method = buf.readUInt16LE(p + 10), csize = buf.readUInt32LE(p + 20), nlen = buf.readUInt16LE(p + 28), elen = buf.readUInt16LE(p + 30), clen = buf.readUInt16LE(p + 32), lho = buf.readUInt32LE(p + 42);
    const name = buf.toString('utf8', p + 46, p + 46 + nlen);
    out.set(name, () => {
      const ln = buf.readUInt16LE(lho + 26), le = buf.readUInt16LE(lho + 28), start = lho + 30 + ln + le;
      const raw = buf.subarray(start, start + csize);
      if (method === 0) return raw;
      if (method === 8) return zlib.inflateRawSync(raw);
      throw new Error('zip: unsupported method ' + method);
    });
    p += 46 + nlen + elen + clen;
  }
  return out;
}

function readReplayBytes(file) {
  const buf = fs.readFileSync(file);
  if (buf[0] === 0x50 && buf[1] === 0x4b) {
    const z = zipEntries(buf);
    const e = z.get('replay.repl26a') || [...z.keys()].filter((k) => k.endsWith('.repl26a')).map((k) => z.get(k))[0];
    if (!e) throw new Error('bundle has no replay.repl26a (entries: ' + [...z.keys()].join(', ') + ')');
    return e();
  }
  if (buf.toString('latin1', 0, 6) !== 'a62per') throw new Error('not a .repl26a record (magic ' + buf.toString('latin1', 0, 6) + ')');
  return buf;
}

async function instantiate(wasmPath) {
  let mem;
  const dv = () => new DataView(mem.buffer);
  const wasi = {
    random_get: (p, n) => { new Uint8Array(mem.buffer, p, n).fill(0); return 0; },
    clock_time_get: (_a, _b, p) => { dv().setBigUint64(p, 0n, true); return 0; },
    environ_sizes_get: (a, b) => { dv().setUint32(a, 0, true); dv().setUint32(b, 0, true); return 0; },
    environ_get: () => 0,
    args_sizes_get: (a, b) => { dv().setUint32(a, 0, true); dv().setUint32(b, 0, true); return 0; },
    args_get: () => 0, fd_close: () => 0, fd_fdstat_get: () => 8, fd_prestat_get: () => 8, fd_prestat_dir_name: () => 8,
    fd_read: () => 8, fd_seek: () => 70, proc_exit: (c) => { throw new Error('engine proc_exit ' + c); },
    fd_write: (fd, iov, n, out) => {
      let t = 0, s = '';
      for (let i = 0; i < n; i++) { const p = dv().getUint32(iov + i * 8, true), l = dv().getUint32(iov + i * 8 + 4, true); t += l; s += Buffer.from(mem.buffer, p, l).toString(); }
      if (process.env.REPLAY_DUMP_DEBUG) process.stderr.write(`[wasm fd${fd}] ${s}`);
      dv().setUint32(out, t, true); return 0;
    },
  };
  const { instance } = await WebAssembly.instantiate(fs.readFileSync(wasmPath), { wasi_snapshot_preview1: wasi, deep_wasm: { on_assert_fail_impl: () => 1 } });
  mem = instance.exports.memory;
  instance.exports._initialize?.();
  return instance.exports;
}

export async function decode(file, opts = {}) {
  const bytes = readReplayBytes(file);
  const ver = bytes[10] || 1;
  const x = await instantiate(findWasm(ver));
  const mem = x.memory;
  const dv = () => new DataView(mem.buffer);
  const check = (code, what) => { if (code !== 0) throw new Error(`engine error ${code} in ${what}`); };
  if (x.Game_Visualiser_SnapshotSize() !== SNAPSHOT_SIZE) throw new Error(`unsupported snapshot layout ${x.Game_Visualiser_SnapshotSize()} (expected ${SNAPSHOT_SIZE})`);
  const outBuf = x.Engine_Malloc(64), args = x.Engine_Malloc(256), snap = x.Engine_Malloc(SNAPSHOT_SIZE);
  const p = x.Engine_Malloc(bytes.length);
  new Uint8Array(mem.buffer, p, bytes.length).set(bytes);
  check(x.Game_FromReplayFileBytes(p, bytes.length, outBuf), 'Game_FromReplayFileBytes');
  const game = dv().getBigUint64(outBuf, true);
  const u32 = (o) => dv().getUint32(o, true), i32 = (o) => dv().getInt32(o, true);

  // --- static map data (template, unmirrored) ---
  check(x.Game_GetBitboardMetadata(game, outBuf), 'metadata');
  const width = i32(outBuf), height = i32(outBuf + 4);
  check(x.Game_GetSymmetry(game, outBuf), 'symmetry'); const symmetry = i32(outBuf);
  check(x.Game_GetNumChampsPerTeam(game, outBuf), 'champs'); const teamSize = i32(outBuf);
  check(x.Game_GetTeamASpawnArea(game, outBuf), 'spawn'); const spawnA = { minX: i32(outBuf), minY: i32(outBuf + 4), maxX: i32(outBuf + 8), maxY: i32(outBuf + 12) };
  check(x.Game_GetNumCamps(game, outBuf), 'numCamps'); const nTemplateCamps = u32(outBuf);
  check(x.Game_GetNumCampGroups(game, outBuf), 'numGroups'); const nTemplateGroups = u32(outBuf);
  const templateCamps = [], rewardByHandle = new Map();
  for (let i = 0; i < nTemplateCamps; i++) {
    check(x.Game_GetCampHandle(game, i, outBuf), 'campHandle'); const h = dv().getBigUint64(outBuf, true);
    check(x.Game_GetCamp(game, h, outBuf), 'camp');
    templateCamps.push({ handle: h, type: i32(outBuf), x: i32(outBuf + 4), y: i32(outBuf + 8) });
  }
  const templateGroups = [];
  for (let g = 0; g < nTemplateGroups; g++) {
    check(x.Game_GetCampGroupHandle(game, g, outBuf), 'groupHandle'); const gh = dv().getBigUint64(outBuf, true);
    check(x.Game_GetCampGroup(game, gh, outBuf), 'group');
    const grp = { gems: u32(outBuf), gold: u32(outBuf + 4), respawn: u32(outBuf + 8), n: u32(outBuf + 12), camps: [] };
    for (let k = 0; k < grp.n; k++) { check(x.Game_GetCampGroupCamp(game, gh, k, outBuf), 'groupCamp'); const ch = dv().getBigUint64(outBuf, true); grp.camps.push(ch); rewardByHandle.set(ch, grp); }
    templateGroups.push(grp);
  }
  // live camp i -> template camp: the visualiser mirrors each template camp by the map symmetry (physicalCamps);
  // we match by position under identity/mirror-x/mirror-y/rot180 so we need not trust the symmetry enum.
  const byPos = new Map();
  for (const c of templateCamps) {
    const cand = [[c.x, c.y], [width - 1 - c.x, c.y], [c.x, height - 1 - c.y], [width - 1 - c.x, height - 1 - c.y]];
    for (const [cx, cy] of cand) if (!byPos.has(cx + ',' + cy)) byPos.set(cx + ',' + cy, c);
  }
  const campTypeName = (t) => CAMP_TYPES[t] ?? ('type' + t);

  check(x.Game_Visualiser_GetConstants(args), 'constants');
  const constants = { recall_rounds: i32(args + 16), teleport_rounds: i32(args + 20), submit_base_rounds: i32(args + 24), submit_min_rounds: i32(args + 28), capture_rounds: i32(args + 32), altar_lock_hits: u32(args + 36), submit_gems_range: dv().getFloat32(args + 48, true), submit_gem_multiplier: dv().getFloat64(args + 56, true) };

  check(x.Game_Runner_SetCheckpointsEnabled(game, 1), 'checkpoints');
  check(x.Game_Visualiser_Timeline_GetMain(game, outBuf), 'timeline'); const timeline = dv().getBigUint64(outBuf, true);
  check(x.Game_Visualiser_Timeline_Info(game, timeline, args, args + 8, args + 16), 'timelineInfo'); let timelineSize = u32(args);
  const persp = args + 48;

  function read() {
    const d = dv();
    d.setUint32(persp, 2, true); d.setUint32(persp + 4, 0xffffffff, true);
    check(x.Game_Visualiser_ReadState(game, persp, snap), 'ReadState');
    const d2 = dv();
    const su = (o) => d2.getUint32(snap + o, true), si = (o) => d2.getInt32(snap + o, true), sf = (o) => d2.getFloat32(snap + o, true);
    if (su(0) !== 1 || su(4) !== SNAPSHOT_SIZE) throw new Error('unsupported game snapshot ABI');
    const nC = su(T.champions.count), champs = [];
    for (let i = 0; i < nC; i++) {
      const b = T.champions.data + i * T.champions.stride;
      const idx = su(b + 4), known = su(b + 16);
      const items = []; let nItems = 0;
      for (let k = 0; k < N_ITEMS; k++) { const q = su(b + 240 + k * 4); items.push(q); nItems += q; }
      champs.push({ index: idx, team: su(b + 8), type: su(b + 12), known, x: si(b + 20), y: si(b + 24), hp: sf(b + 28), maxHp: sf(b + 32),
        roundFlags: su(b + 36), buffMask: su(b + 40), alive: su(b + 224) !== 0, respawnTimer: si(b + 228), gold: su(b + 232), gems: su(b + 236), items, nItems,
        consumables: [0, 1, 2, 3].map((k) => su(b + 412 + k * 4)), channelTimer: si(b + 436), channelType: su(b + 440), priority: si(b + 444), draftOrder: su(b + 452),
        earnedGold: su(T.earnedGold + idx * 16) + su(T.earnedGold + idx * 16 + 4) * 4294967296,
        kills: su(T.kda + idx * 12), deaths: su(T.kda + idx * 12 + 4), assists: su(T.kda + idx * 12 + 8), submissionDuration: si(T.submissionDuration + idx * 4) });
    }
    const nCamps = su(T.camps.count), camps = [];
    for (let i = 0; i < nCamps; i++) {
      const b = T.camps.data + i * T.camps.stride;
      camps.push({ index: su(b + 4), team: su(b + 8), type: su(b + 12), known: su(b + 16), x: si(b + 20), y: si(b + 24), hp: sf(b + 28), maxHp: sf(b + 32), group: su(b + 224), awake: su(b + 228) !== 0 });
    }
    const nG = su(T.groups.count), groups = [];
    for (let i = 0; i < nG; i++) { const b = T.groups.data + i * T.groups.stride; groups.push({ state: su(b), rewardAvailable: su(b + 4) !== 0, captureTeam: su(b + 8), captureTimer: si(b + 12), respawnRounds: si(b + 16) }); }
    const nW = su(T.wards.count), wards = [];
    for (let i = 0; i < nW; i++) { const b = T.wards.data + i * T.wards.stride; wards.push({ owner: su(b + 12), team: su(b + 16), x: si(b + 20), y: si(b + 24), disabled: d2.getUint8(snap + b + 36) !== 0 }); }
    const nS = su(T.spells.count), spells = [];
    for (let i = 0; i < nS; i++) { const b = T.spells.data + i * T.spells.stride; spells.push({ kind: su(b), index: su(b + 4), ownerKind: su(b + 8), owner: su(b + 12) }); }
    const drafts = [0, 1].map((t) => [0, 1, 2, 3, 4].map((k) => si(T.draft + 12 + t * 20 + k * 4)));
    const stepKind = su(68);
    return { round: su(8), turn: su(12), team: su(16), awaiting: su(20) !== 0, phase: su(24), outcome: su(28),
      submitted: [Number(d2.getBigUint64(snap + 32, true)), Number(d2.getBigUint64(snap + 40, true))], inputPos: su(56), currentChampion: su(60),
      step: { phase: su(64), kind: stepKind, actorKind: su(72), actor: su(76), fromX: si(80), fromY: si(84), toX: si(88), toY: si(92), round: su(96), inputPos: su(100), substep: su(104), abilitySlot: su(108), abilityType: su(112), priority: si(116) },
      draftRounds: su(T.draft), drafts, champs, camps, groups, wards, spells };
  }

  check(x.Game_Visualiser_Timeline_Seek(game, timeline, 0, 0), 'seek');
  let s = read();
  const meta = { type: 'meta', file: path.basename(file), engine: `0.0.${ver}`, width, height, symmetry, team_size: teamSize, spawn_a: spawnA, constants, draft_rounds: s.draftRounds,
    template_camps: nTemplateCamps, template_groups: nTemplateGroups };

  const records = [], events = [];
  const emit = (r) => records.push(r);
  const ev = (e) => { if (opts.events !== false) events.push(e); };
  const champName = (c) => CHAMP_TYPES[c.type] ?? ('type' + c.type);

  // per-round action flags per champion
  let acted = new Map();
  const flag = (idx, f) => { if (!acted.has(idx)) acted.set(idx, {}); acted.get(idx)[f] = (acted.get(idx)[f] || 0) + 1; };
  let prev = s, lastEmittedRound = -1, campInfo = null, groupInfo = null, captures = [[], []];
  const liveGroupReward = new Map();

  function initLayout(st) {
    campInfo = st.camps.map((c) => {
      const t = byPos.get(c.x + ',' + c.y);
      const reward = t ? rewardByHandle.get(t.handle) : undefined;
      if (reward && !liveGroupReward.has(c.group)) liveGroupReward.set(c.group, reward);
      return { id: c.index, type: c.type, type_name: campTypeName(c.type), x: c.x, y: c.y, group: c.group, max_hp: c.maxHp };
    });
    groupInfo = st.groups.map((g, i) => {
      const members = campInfo.filter((c) => c.group === i);
      const r = liveGroupReward.get(i);
      return { id: i, gems: r?.gems ?? null, gold: r?.gold ?? null, respawn: r?.respawn ?? null, camps: members.map((m) => m.id), tiles: members.map((m) => [m.x, m.y]) };
    });
    meta.camps = campInfo; meta.groups = groupInfo;
    meta.drafts = st.drafts.map((d) => d.filter((v) => v >= 0).map((v) => ({ id: v, name: DRAFT_NAMES[v] ?? ('draft' + v) })));
  }

  function flushRound(st, round) {
    // champion rows describe the state at the END of `round` (after every step with step.round == round)
    if (opts.every && round % opts.every !== 0 && !st.outcome) { acted = new Map(); return; }
    for (const c of st.champs) {
      const a = acted.get(c.index) || {};
      const row = { type: 'champ', round, team: c.team, id: c.index, name: champName(c), x: c.alive ? c.x : null, y: c.alive ? c.y : null, hp: Math.round(c.hp * 100) / 100, max_hp: Math.round(c.maxHp * 100) / 100,
        gems: c.gems, gold: c.gold, alive: c.alive, respawn: c.alive ? 0 : c.respawnTimer, channel: c.channelType, channel_timer: c.channelTimer,
        kills: c.kills, deaths: c.deaths, assists: c.assists, earned_gold: c.earnedGold, n_items: c.nItems, priority: c.priority, acted: a };
      if (opts.items) { row.items = c.items; row.consumables = c.consumables; }
      emit(row);
    }
    const alive = [0, 1].map((t) => st.champs.filter((c) => c.team === t && c.alive).length);
    const carried = [0, 1].map((t) => st.champs.filter((c) => c.team === t).reduce((a, c) => a + c.gems, 0));
    const row = { type: 'round', round, submitted: st.submitted, carried, alive, wards: [0, 1].map((t) => st.wards.filter((w) => w.team === t && !w.disabled).length) };
    if (opts.groups !== false) row.groups = st.groups.map((g, i) => [i, g.state, g.captureTeam > 1 ? null : g.captureTeam, g.captureTimer, g.respawnRounds]);
    emit(row);
    acted = new Map();
  }

  const champById = (st, idx) => st.champs.find((c) => c.index === idx);
  let steps = 0;
  for (;;) {
    if (s.outcome) break;
    if (s.awaiting && s.inputPos >= timelineSize) {
      // the main timeline extends as it is replayed; refresh the size before concluding we are done
      check(x.Game_Visualiser_Timeline_Info(game, timeline, args, args + 8, args + 16), 'timelineInfo'); timelineSize = u32(args);
      if (s.inputPos >= timelineSize) break;
    }
    const code = x.Game_Visualiser_Timeline_ReplayNext(game, 1);
    if (code !== 0) { meta.replay_stop = `ReplayNext returned ${code} at round ${s.round}`; break; }
    s = read(); steps++;
    if (s.inputPos === prev.inputPos && s.step.substep === prev.step.substep && s.round === prev.round && s.step.kind === prev.step.kind && s.phase === prev.phase) {
      if (++meta.stalls > 50) { meta.replay_stop = 'timeline made no progress'; break; }
    } else meta.stalls = 0;
    if (!campInfo && s.phase !== 12 && s.camps.length) initLayout(s);
    const st = s.step, r = st.round || s.round;
    // ----- round boundary: flush previous round state -----
    if (campInfo && r !== prev.round && prev.round > 0 && prev.round !== lastEmittedRound && prev.phase !== 12) { flushRound(prev, prev.round); lastEmittedRound = prev.round; }
    if (s.phase === 12 || st.phase === 12) { prev = s; continue; }
    const actorChamp = st.actorKind === 1 ? st.actor : null;
    const A = actorChamp !== null ? champById(s, actorChamp) : null;
    // champion credited with damage dealt in this step: the acting champion, a spell's owner, or (a camp's step) the
    // champion the camp hit, whose retaliation (Ren) may kill the monster — this matches the engine's campClear credit.
    let C = A;
    if (!C && st.actorKind === 4) { const sp = [...s.spells, ...prev.spells].find((q) => q.index === st.actor); if (sp && sp.ownerKind === 1) C = champById(s, sp.owner) ?? null; }
    if (!C && st.actorKind === 2) C = s.champs.find((c) => c.alive && c.x === st.toX && c.y === st.toY) ?? null;
    const base = (c) => ({ round: r, team: c.team, id: c.index, name: champName(c), x: c.x, y: c.y });
    switch (st.kind) {
      case K['Move']: if (A) { flag(A.index, 'move'); } break;
      case K['Attack']: if (A) { flag(A.index, 'attack'); } break;
      case K['Cast ability']: if (A) { flag(A.index, 'cast'); ev({ type: 'event', kind: 'cast', ...base(A), slot: st.abilitySlot, ability_type: st.abilityType, to_x: st.toX, to_y: st.toY }); } break;
      case K['Recall']: if (A) { flag(A.index, 'recall'); ev({ type: 'event', kind: 'recall', ...base(A) }); } break;
      case K['Submit gems']: if (A) { flag(A.index, 'submit'); ev({ type: 'event', kind: 'submit_start', ...base(A), gems: A.gems, channel_rounds: A.channelTimer }); } break;
      case K['Shop']: if (A) { flag(A.index, 'shop'); ev({ type: 'event', kind: 'buy', ...base(A), gold: A.gold }); } break;
      case K['Sell item']: if (A) { flag(A.index, 'shop'); ev({ type: 'event', kind: 'sell', ...base(A) }); } break;
      case K['Place ward']: if (A) { flag(A.index, 'ward'); ev({ type: 'event', kind: 'ward', ...base(A), to_x: st.toX, to_y: st.toY }); } break;
      case K['Consumable']: if (A) { flag(A.index, 'consumable'); ev({ type: 'event', kind: 'consumable', ...base(A) }); } break;
      case K['Place altar lock']: if (A) { flag(A.index, 'lock'); ev({ type: 'event', kind: 'altar_lock', ...base(A), to_x: st.toX, to_y: st.toY }); } break;
      case K['Teleport channel']: if (A) { flag(A.index, 'teleport'); ev({ type: 'event', kind: 'teleport', ...base(A), to_x: st.toX, to_y: st.toY }); } break;
      case K['Cancel channel']: if (A) { ev({ type: 'event', kind: 'cancel', ...base(A), channel: prev.champs.find((c) => c.index === A.index)?.channelType }); } break;
      case K['Movement blocked']: if (A) { flag(A.index, 'blocked'); } break;
      case K['Turn rollover']: if (A) { flag(A.index, 'fuel_out'); ev({ type: 'event', kind: 'fuel_out', ...base(A) }); } break;
      case K['Bot error']: if (A) { flag(A.index, 'error'); ev({ type: 'event', kind: 'bot_error', ...base(A) }); } break;
      case K['Gems submitted']: {
        for (const t of [0, 1]) if (s.submitted[t] > prev.submitted[t]) {
          const who = A && A.team === t ? A : s.champs.find((c) => c.team === t && c.gems < (prev.champs.find((p) => p.index === c.index)?.gems ?? 0));
          ev({ type: 'event', kind: 'submission', round: r, team: t, id: who?.index ?? null, name: who ? champName(who) : null, x: who?.x ?? null, y: who?.y ?? null, amount: s.submitted[t] - prev.submitted[t], total: s.submitted[t] });
        }
        break;
      }
      case K['Capture']: {
        s.groups.forEach((g, i) => {
          const pg = prev.groups[i];
          if (pg && pg.rewardAvailable && !g.rewardAvailable && (pg.captureTeam === 0 || pg.captureTeam === 1)) {
            const gi = groupInfo[i], team = pg.captureTeam;
            const onTiles = s.champs.filter((c) => c.team === team && c.alive && gi.tiles.some(([tx, ty]) => tx === c.x && ty === c.y));
            const near = s.champs.filter((c) => c.team === team && c.alive && gi.tiles.some(([tx, ty]) => Math.max(Math.abs(tx - c.x), Math.abs(ty - c.y)) <= 2));
            const gained = s.champs.filter((c) => c.team === team).map((c) => [c.index, c.gems - (prev.champs.find((p) => p.index === c.index)?.gems ?? 0)]).filter(([, d]) => d > 0);
            const e = { type: 'event', kind: 'capture', round: r, team, group: i, gems: gi.gems, gold: gi.gold, party: onTiles.length, party_near: near.length, party_ids: near.map((c) => c.index), gem_split: gained, x: gi.tiles[0]?.[0] ?? null, y: gi.tiles[0]?.[1] ?? null };
            captures[team].push(e); ev(e);
          }
        });
        break;
      }
      default: break;
    }
    // camp kills (monster hp reaching 0) and group clears
    const clearedNow = new Set();
    if (campInfo) for (let i = 0; i < s.camps.length; i++) {
      const c = s.camps[i], pc = prev.camps[i];
      if (pc && pc.hp > 0 && c.hp <= 0 && (pc.known & 1)) {
        const who = C ? { team: C.team, id: C.index, name: champName(C) } : { team: null, id: null, name: null };
        ev({ type: 'event', kind: 'camp_kill', round: r, ...who, camp: c.index, camp_type: campTypeName(c.type), group: c.group, x: campInfo[i]?.x ?? c.x, y: campInfo[i]?.y ?? c.y });
        // group clear = the last living monster of the group died (what `--statistics` calls campClear)
        if (!clearedNow.has(c.group) && !s.camps.some((q) => q.group === c.group && q.hp > 0)) { clearedNow.add(c.group); ev({ type: 'event', kind: 'group_clear', round: r, ...who, group: c.group, gems: groupInfo[c.group]?.gems ?? null, gold: groupInfo[c.group]?.gold ?? null, x: groupInfo[c.group]?.tiles[0]?.[0] ?? null, y: groupInfo[c.group]?.tiles[0]?.[1] ?? null }); }
      }
    }
    // deaths / kills / respawns from champion transitions
    for (const c of s.champs) {
      const pc = prev.champs.find((q) => q.index === c.index);
      if (!pc) continue;
      if (pc.alive && !c.alive) {
        const killer = s.champs.find((q) => q.kills > (prev.champs.find((z) => z.index === q.index)?.kills ?? 0));
        ev({ type: 'event', kind: 'death', round: r, team: c.team, id: c.index, name: champName(c), x: pc.x, y: pc.y, gems_lost: pc.gems, gold: pc.gold, by_team: killer ? killer.team : (A ? A.team : null), by_id: killer ? killer.index : (A?.index ?? null), by_name: killer ? champName(killer) : (A ? champName(A) : (st.actorKind === 2 ? 'camp' : null)), by_kind: killer ? 'champion' : st.actorKind === 2 ? 'camp' : st.actorKind === 4 ? 'spell' : st.actorKind === 1 ? 'champion' : 'unknown' });
      }
      if (!pc.alive && c.alive) ev({ type: 'event', kind: 'respawn', round: r, team: c.team, id: c.index, name: champName(c), x: c.x, y: c.y });
      if (c.kills > pc.kills) ev({ type: 'event', kind: 'kill', round: r, team: c.team, id: c.index, name: champName(c), x: c.x, y: c.y, n: c.kills - pc.kills, gems: c.gems, gems_gained: c.gems - pc.gems });
    }
    prev = s;
  }
  if (campInfo && prev.round > 0 && prev.round !== lastEmittedRound) { flushRound(prev, prev.round); lastEmittedRound = prev.round; }
  if (!campInfo) initLayout(s);
  const final = { type: 'final', outcome: s.outcome, winner: s.outcome === 1 ? 'teamA' : s.outcome === 2 ? 'teamB' : s.outcome ? 'outcome' + s.outcome : null, finish_round: s.round, steps, submitted: s.submitted,
    champions: s.champs.map((c) => ({ id: c.index, team: c.team, name: champName(c), kills: c.kills, deaths: c.deaths, assists: c.assists, gems: c.gems, gold: c.gold, earned_gold: c.earnedGold, items: c.items, consumables: c.consumables, alive: c.alive })),
    captures: captures.map((l) => l.length) };
  x.Game_Destroy?.(game);
  return { meta, records, events, final };
}

function summaryText(d) {
  const m = d.meta, f = d.final;
  const lines = [];
  lines.push(`${m.file}: engine ${m.engine}, map ${m.width}x${m.height}, team size ${m.team_size}, ${m.camps?.length ?? 0} camps in ${m.groups?.length ?? 0} groups`);
  lines.push(`result: ${f.winner ?? 'unfinished'} at round ${f.finish_round}; submitted A=${f.submitted[0]} B=${f.submitted[1]}; captures A=${f.captures[0]} B=${f.captures[1]}; ${f.steps} replay steps`);
  for (const t of [0, 1]) {
    lines.push(`team ${'AB'[t]} draft: ${(m.drafts?.[t] ?? []).map((d) => d.name).join(', ')}`);
    for (const c of f.champions.filter((c) => c.team === t)) lines.push(`  #${c.id} ${c.name.padEnd(12)} K/D/A ${c.kills}/${c.deaths}/${c.assists}  carried ${c.gems}  gold ${c.gold}  earned ${c.earned_gold}  ${c.alive ? 'alive' : 'dead'}`);
  }
  const subs = d.events.filter((e) => e.kind === 'submission');
  for (const t of [0, 1]) { const l = subs.filter((e) => e.team === t); lines.push(`team ${'AB'[t]} tributes: ${l.length}${l.length ? `, first at round ${l[0].round} (${l[0].amount})` : ''}`); }
  return lines.join('\n');
}

const isMain = process.argv[1] && path.resolve(process.argv[1]) === path.resolve(new URL(import.meta.url).pathname);
if (isMain) {
  (async () => {
    const o = parseArgs(process.argv.slice(2));
    const t0 = Date.now();
    const d = await decode(o.file, o);
    if (o.summary && !o.out && !o.stdout) { console.log(summaryText(d)); return; }
    const out = [JSON.stringify(d.meta), ...d.records.map((r) => JSON.stringify(r)), ...d.events.map((e) => JSON.stringify(e)), JSON.stringify(d.final)].join('\n') + '\n';
    if (o.out) { fs.writeFileSync(o.out, out); console.error(`wrote ${o.out}: ${d.records.length} state rows, ${d.events.length} events, ${d.final.steps} steps in ${Date.now() - t0} ms`); }
    else process.stdout.write(out);
    if (o.summary) console.log(summaryText(d));
  })().catch((e) => { console.error(e.stack || String(e)); process.exit(1); });
}
