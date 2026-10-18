#!/usr/bin/env python3
"""Local tournament: BOT_A vs BOT_B over the map pool, both seats, in parallel.

    arena.py BOT_A BOT_B [--maps all|name,name] [--sides both|a|b] [--jobs N] [--rounds N]
             [--out DIR] [--no-build] [--timeout SEC] [--fuel N] [--backend batch|run]

Everything is oriented to BOT_A ("the candidate"): a "win" is BOT_A winning, "side A" means
BOT_A sat in the teamA seat. Writes <out>/<map>-<side>.repl26a, <map>-<side>.json (engine
statistics), summary.json, summary.md and run-batch.log. Exit codes: 0 ok, 2 a match
errored (timeout/crash/missing stats), 3 a bot faulted (either side), 4 usage/build error.
Stdlib only; requires `cambc` on PATH.
"""
from __future__ import annotations

import argparse
import json
import math
import os
import re
import shutil
import statistics
import subprocess
import sys
import threading
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import replaylib  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
MAPS_DIR = ROOT / "maps"
STATUS_RE = re.compile(r"^\[(?P<mark>[+x|])\]\s+(?P<status>.+?)\s{2,}(?P<a>\S+) vs (?P<b>\S+)\s{2,}(?P<map>\S+)(?:\s+Round (?P<round>\d+)/\d+)?")


def die(msg: str, code: int = 4) -> None:
    print(f"arena: error: {msg}", file=sys.stderr)
    sys.exit(code)


def which_cambc() -> str:
    exe = shutil.which("cambc") or str(ROOT / ".venv" / "bin" / "cambc")
    if not os.access(exe, os.X_OK):
        die("cambc not found on PATH (export PATH=.venv/bin:$PATH)")
    return exe


def resolve_bot(spec: str) -> Path:
    p = Path(spec)
    if not p.is_absolute():
        p = (ROOT / p) if (ROOT / p).is_dir() else p.resolve()
    if not (p / "src").is_dir():
        die(f"bot '{spec}' has no src/ directory ({p})")
    return p.resolve()


def resolve_maps(spec: str) -> list[Path]:
    pool = sorted(MAPS_DIR.glob("*.map26a"))
    if spec == "all":
        if not pool:
            die(f"no maps in {MAPS_DIR}")
        return pool
    out = []
    for name in [s.strip() for s in spec.split(",") if s.strip()]:
        p = Path(name)
        if p.suffix == ".map26a" and p.exists():
            out.append(p.resolve())
            continue
        cand = MAPS_DIR / f"{name}.map26a"
        if not cand.exists():
            die(f"unknown map '{name}' (have: {', '.join(m.stem for m in pool)})")
        out.append(cand)
    return out


def build(cambc: str, bot: Path) -> None:
    t = time.time()
    r = subprocess.run([cambc, "build", str(bot), "--release"], capture_output=True, text=True)
    if r.returncode != 0:
        tail = "\n".join((r.stdout + r.stderr).strip().splitlines()[-25:])
        die(f"build failed for {bot}:\n{tail}")
    print(f"built {bot.name} in {time.time() - t:.1f}s")


class Match:
    def __init__(self, map_path: Path, side: str, bot_a: Path, bot_b: Path, out: Path):
        self.map, self.side = map_path, side          # side: 'A' => BOT_A is teamA
        self.team_a, self.team_b = (bot_a, bot_b) if side == "A" else (bot_b, bot_a)
        stem = f"{map_path.stem}-{side}"
        self.replay, self.stats = out / f"{stem}.repl26a", out / f"{stem}.json"
        self.status, self.error, self.wall = "pending", None, None

    def manifest_row(self) -> str:
        return f"{self.team_a} {self.team_b} {self.map} {self.replay} --statistics {self.stats} --json"

    def clean(self) -> None:
        for p in (self.replay, self.stats, Path(str(self.replay) + ".error.log")):
            p.unlink(missing_ok=True)


def run_batch_backend(cambc: str, matches: list[Match], args, out: Path) -> int:
    manifest = out / "manifest.txt"
    manifest.write_text("".join(m.manifest_row() + "\n" for m in matches))
    cmd = [cambc, "run-batch", str(manifest), "--jobs", str(args.jobs), "--release", "--json",
           "--timeout", str(args.timeout), "--color", "never"]
    if args.rounds:
        cmd += ["--rounds", str(args.rounds)]
    if args.fuel:
        cmd += ["--fuel-quantum", str(args.fuel)]
    overall = args.timeout * math.ceil(len(matches) / max(1, args.jobs)) + 300
    log = open(out / "run-batch.log", "w")
    log.write("$ " + " ".join(cmd) + "\n")
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, cwd=ROOT)
    by_replay = {str(m.replay): m for m in matches}
    done, t0, timer = 0, time.time(), threading.Timer(overall, lambda: proc.kill())
    timer.start()
    try:
        last = None
        for line in proc.stdout:
            log.write(line)
            s = line.rstrip()
            mo = STATUS_RE.match(s)
            if mo:
                last = mo
                continue
            if s.strip().startswith("Replay:") and last is not None:
                m = by_replay.get(s.split("Replay:", 1)[1].strip())
                if m is not None and last["mark"] != "|":
                    done += 1
                    m.status = last["status"]
                    print(f"[{done}/{len(matches)}] {time.time() - t0:6.0f}s  {m.map.stem:<12} side {m.side}  {last['status']}")
                last = None
            elif s.startswith("error:") or s.startswith("Finished:"):
                print(s)
    finally:
        timer.cancel()
        proc.wait()
        log.close()
    if proc.returncode not in (0, 1):
        print(f"arena: run-batch exited with {proc.returncode} (see {out / 'run-batch.log'})", file=sys.stderr)
    return proc.returncode


def run_one(cambc: str, m: Match, args) -> None:
    cmd = [cambc, "run", str(m.team_a), str(m.team_b), "--map", str(m.map), "--release",
           "--statistics", str(m.stats), "--json", "-o", str(m.replay.parent)]
    if args.rounds:
        cmd += ["--rounds", str(args.rounds)]
    if args.fuel:
        cmd += ["--fuel", str(args.fuel)]
    t = time.time()
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=args.timeout, cwd=ROOT)
        # cambc run names the bundle itself (<a>-vs-<b>-<ms>.repl26a); find and rename it.
        mo = re.search(r"replay bundle: (.+\.repl26a)", r.stdout)
        if mo:
            Path(mo.group(1).strip()).rename(m.replay)
        m.status = "ok" if r.returncode == 0 and m.stats.exists() else f"exit {r.returncode}"
        if r.returncode != 0:
            m.error = "\n".join((r.stdout + r.stderr).splitlines()[-5:])
    except subprocess.TimeoutExpired:
        m.status, m.error = "timeout", f"exceeded {args.timeout}s"
    m.wall = time.time() - t


def run_pool_backend(cambc: str, matches: list[Match], args, out: Path) -> int:
    t0, done, lock = time.time(), [0], threading.Lock()

    def task(m: Match) -> None:
        run_one(cambc, m, args)
        with lock:
            done[0] += 1
            print(f"[{done[0]}/{len(matches)}] {time.time() - t0:6.0f}s  {m.map.stem:<12} side {m.side}  {m.status}")

    with ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        list(pool.map(task, matches))
    return 0


# ----------------------------------------------------------------------------- analysis

def analyse(m: Match, bot_a: Path, bot_b: Path) -> dict:
    rec = {"map": m.map.stem, "side": m.side, "teamA": m.team_a.name, "teamB": m.team_b.name,
           "replay": str(m.replay), "stats": str(m.stats), "status": m.status, "error": m.error,
           "result": None, "winner": None, "rounds": None, "A": None, "B": None}
    err_log = Path(str(m.replay) + ".error.log")
    if err_log.exists():
        rec["error"] = err_log.read_text().strip()[:500]
    if not m.stats.exists():
        rec["result"] = "truncated" if "TRUNCATED" in (m.status or "") else "error"
        if rec["result"] == "error" and not rec["error"]:
            rec["error"] = f"no statistics written (status: {m.status})"
    else:
        try:
            st = replaylib.load_stats(m.stats)
        except (OSError, ValueError) as e:
            rec["result"], rec["error"] = "error", f"bad statistics JSON: {e}"
            return rec
        seat = {"A": ("teamA", "teamB"), "B": ("teamB", "teamA")}[m.side]  # (BOT_A seat, BOT_B seat)
        va, vb = replaylib.stats_team_view(st, seat[0]), replaylib.stats_team_view(st, seat[1])
        outcome = st.get("match", {}).get("outcome")
        rec["rounds"] = st.get("match", {}).get("roundsStarted", st.get("match", {}).get("roundsCompleted"))
        rec["outcome"] = outcome
        rec["winner"] = "A" if outcome == seat[0] else "B" if outcome == seat[1] else "none"
        rec["result"] = {"A": "win", "B": "loss"}.get(rec["winner"], "draw")
        rec["A"], rec["B"] = va, vb
    if m.replay.exists():
        sizes = {}
        if rec["A"]:
            sizes = {"a": len((rec["A"] if m.side == "A" else rec["B"])["draft"]) or None,
                     "b": len((rec["B"] if m.side == "A" else rec["A"])["draft"]) or None}
        rp = replaylib.inspect_replay(m.replay, sizes)
        if rp.get("ok"):
            for bot, team in (("A", "a" if m.side == "A" else "b"), ("B", "b" if m.side == "A" else "a")):
                side_info = rp[team]
                fuel = side_info.get("fuel") or {}
                extra = {"stdout_bot_errors": side_info["bot_error_lines"], "stdout_fuel_lines": side_info["fuel_lines"],
                         "stdout_error_samples": side_info["bot_error_samples"],
                         "fuel_peak_used": max((c["max_used"] for c in fuel.get("champions", [])), default=None),
                         "fuel_quantum": fuel.get("quantum"),
                         "fuel_suspended_turns": sum(c["suspended"] for c in fuel.get("champions", [])) if fuel.get("ok") else None}
                rec[bot] = {**(rec[bot] or {}), **extra}
    return rec


def summarise(records: list[dict], bot_a: Path, bot_b: Path, wall: float, args) -> dict:
    played = [r for r in records if r["result"] in ("win", "loss", "draw")]
    wins = [r for r in played if r["result"] == "win"]
    losses = [r for r in played if r["result"] == "loss"]

    def mean(xs):
        return round(statistics.fmean(xs), 1) if xs else None

    def tot(bot, key):
        return sum((r[bot] or {}).get(key) or 0 for r in records if r[bot])

    per_map, per_side = {}, {}
    for r in records:
        pm = per_map.setdefault(r["map"], {"games": 0, "wins": 0, "losses": 0, "errors": 0})
        ps = per_side.setdefault(r["side"], {"games": 0, "wins": 0, "losses": 0, "errors": 0})
        for d in (pm, ps):
            d["games"] += 1
            d["wins"] += r["result"] == "win"
            d["losses"] += r["result"] == "loss"
            d["errors"] += r["result"] == "error"
    for d in list(per_map.values()) + list(per_side.values()):
        d["win_rate"] = round(d["wins"] / d["games"], 3) if d["games"] else None
    agg = {
        "bot_a": str(bot_a), "bot_b": str(bot_b), "bot_a_name": bot_a.name, "bot_b_name": bot_b.name,
        "games": len(records), "played": len(played), "wins": len(wins), "losses": len(losses),
        "draws": len(played) - len(wins) - len(losses),
        "errors": sum(r["result"] == "error" for r in records),
        "truncated": sum(r["result"] == "truncated" for r in records),
        "win_rate": round(len(wins) / len(played), 3) if played else None,
        "mean_round_when_winning": mean([r["rounds"] for r in wins if r["rounds"]]),
        "mean_round_when_losing": mean([r["rounds"] for r in losses if r["rounds"]]),
        "mean_gems_a": mean([r["A"]["gems"] for r in played]), "mean_gems_b": mean([r["B"]["gems"] for r in played]),
        "faults_a": tot("A", "faults"), "faults_b": tot("B", "faults"),
        "suspensions_a": tot("A", "suspensions") + tot("A", "draft_suspensions"),
        "suspensions_b": tot("B", "suspensions") + tot("B", "draft_suspensions"),
        "stdout_bot_errors_a": tot("A", "stdout_bot_errors"), "stdout_bot_errors_b": tot("B", "stdout_bot_errors"),
        "kills_a": tot("A", "kills"), "kills_b": tot("B", "kills"),
        "fuel_peak_used_a": max((r["A"]["fuel_peak_used"] for r in records if r["A"] and r["A"].get("fuel_peak_used")), default=None),
        "fuel_peak_used_b": max((r["B"]["fuel_peak_used"] for r in records if r["B"] and r["B"].get("fuel_peak_used")), default=None),
        "per_map": per_map, "per_side": per_side, "wall_seconds": round(wall, 1),
        "jobs": args.jobs, "backend": args.backend, "rounds_cap": args.rounds,
    }
    return agg


def fmt_table(records: list[dict], a: str, b: str) -> str:
    hdr = f"{'map':<12} {'side':<4} {'result':<9} {'winner':<15} {'gems A/B':>11} {'rounds':>6} {'kills A/B':>9} {'deaths A/B':>10} {'faults A/B':>10} {'susp A/B':>9} {'err A/B':>8}"
    lines = [hdr, "-" * len(hdr)]
    for r in records:
        A, B = r["A"] or {}, r["B"] or {}
        winner = {"A": a, "B": b}.get(r["winner"], r["winner"] or "-")[:15]
        lines.append(
            f"{r['map']:<12} {r['side']:<4} {r['result']:<9} {winner:<15} "
            f"{str(A.get('gems', '-')) + '/' + str(B.get('gems', '-')):>11} {str(r['rounds'] or '-'):>6} "
            f"{str(A.get('kills', '-')) + '/' + str(B.get('kills', '-')):>9} "
            f"{str(A.get('deaths', '-')) + '/' + str(B.get('deaths', '-')):>10} "
            f"{str(A.get('faults', '-')) + '/' + str(B.get('faults', '-')):>10} "
            f"{str(A.get('suspensions', '-')) + '/' + str(B.get('suspensions', '-')):>9} "
            f"{str(A.get('stdout_bot_errors', '-')) + '/' + str(B.get('stdout_bot_errors', '-')):>8}"
            + (f"   !! {r['error'].splitlines()[0][:80]}" if r.get("error") else ""))
    return "\n".join(lines)


def fmt_aggregate(s: dict) -> str:
    a, b = s["bot_a_name"], s["bot_b_name"]
    L = [f"{a} vs {b}: {s['wins']}W {s['losses']}L {s['draws']}D of {s['played']} played"
         f" ({s['games']} scheduled, {s['errors']} errors, {s['truncated']} truncated)"
         f"  win rate {s['win_rate']}",
         f"mean finish round when winning: {s['mean_round_when_winning']}   when losing: {s['mean_round_when_losing']}",
         f"mean gems {a}: {s['mean_gems_a']}   {b}: {s['mean_gems_b']}   kills {s['kills_a']}/{s['kills_b']}",
         f"faults {a}: {s['faults_a']}   {b}: {s['faults_b']}   |  fuel suspensions {s['suspensions_a']}/{s['suspensions_b']}"
         f"   |  stdout 'bot error' lines {s['stdout_bot_errors_a']}/{s['stdout_bot_errors_b']}"
         f"   |  peak fuel used {s['fuel_peak_used_a']}/{s['fuel_peak_used_b']}",
         "per side: " + "  ".join(f"{k}: {v['wins']}/{v['games']}" for k, v in sorted(s["per_side"].items())),
         "per map:  " + "  ".join(f"{k} {v['wins']}/{v['games']}" for k, v in sorted(s["per_map"].items())),
         f"wall {s['wall_seconds']}s with --jobs {s['jobs']} ({s['backend']} backend)"]
    return "\n".join(L)


def write_md(path: Path, records, summary, table: str, agg: str) -> None:
    path.write_text(f"# arena: {summary['bot_a_name']} vs {summary['bot_b_name']}\n\n```\n{agg}\n```\n\n"
                    f"## per match\n\n```\n{table}\n```\n")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("bot_a"); ap.add_argument("bot_b")
    ap.add_argument("--maps", default="all", help="all | comma-separated map names (default all)")
    ap.add_argument("--sides", default="both", choices=["both", "a", "b"], help="seat(s) BOT_A plays in")
    ap.add_argument("--jobs", type=int, default=8)
    ap.add_argument("--rounds", type=int, default=0, help="round cap (NOTE: a capped, unfinished match yields no statistics)")
    ap.add_argument("--out", default=None, help="output dir (default replays/arena-<A>-vs-<B>-<time>)")
    ap.add_argument("--no-build", action="store_true")
    ap.add_argument("--timeout", type=int, default=900, help="wall seconds per match (default 900)")
    ap.add_argument("--fuel", type=int, default=0, help="override fuel quantum per turn")
    ap.add_argument("--backend", default="batch", choices=["batch", "run"], help="cambc run-batch (default) or a thread pool of cambc run")
    ap.add_argument("--fail-on-suspend", action="store_true", help="also exit 3 if BOT_A had any fuel suspension")
    args = ap.parse_args()

    cambc = which_cambc()
    bot_a, bot_b = resolve_bot(args.bot_a), resolve_bot(args.bot_b)
    if bot_a == bot_b:
        die("BOT_A and BOT_B are the same directory")
    maps = resolve_maps(args.maps)
    sides = {"both": ["A", "B"], "a": ["A"], "b": ["B"]}[args.sides]
    out = Path(args.out) if args.out else ROOT / "replays" / f"arena-{bot_a.name}-vs-{bot_b.name}-{time.strftime('%Y%m%d-%H%M%S')}"
    out.mkdir(parents=True, exist_ok=True)
    out = out.resolve()

    if not args.no_build:
        for b in (bot_a, bot_b):
            build(cambc, b)
    matches = [Match(mp, side, bot_a, bot_b, out) for mp in maps for side in sides]
    for m in matches:
        m.clean()  # run-batch refuses to run if any output path already exists
    print(f"{len(matches)} matches ({len(maps)} maps x sides {sides}), --jobs {args.jobs}, {args.backend} backend, out {out}")

    t0 = time.time()
    backend = run_batch_backend if args.backend == "batch" else run_pool_backend
    rc = backend(cambc, matches, args, out)
    wall = time.time() - t0

    records = [analyse(m, bot_a, bot_b) for m in matches]
    summary = summarise(records, bot_a, bot_b, wall, args)
    summary["runner_exit"] = rc
    table, agg = fmt_table(records, bot_a.name, bot_b.name), fmt_aggregate(summary)
    print(); print(table); print(); print(agg)
    (out / "summary.json").write_text(json.dumps({"summary": summary, "matches": records}, indent=1))
    write_md(out / "summary.md", records, summary, table, agg)
    print(f"\nwrote {out / 'summary.json'} and summary.md")

    code = 0
    if summary["errors"] or rc not in (0,):
        print(f"arena: {summary['errors']} match(es) errored (runner exit {rc})", file=sys.stderr); code = 2
    if summary["faults_a"] or summary["faults_b"]:
        print(f"arena: bot faults: {bot_a.name}={summary['faults_a']} {bot_b.name}={summary['faults_b']}", file=sys.stderr); code = code or 3
    if args.fail_on_suspend and summary["suspensions_a"]:
        print(f"arena: {bot_a.name} had {summary['suspensions_a']} fuel suspensions", file=sys.stderr); code = code or 3
    sys.exit(code)


if __name__ == "__main__":
    main()
