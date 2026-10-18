#!/usr/bin/env python3
"""Inspect a cambc replay bundle: team stdout/stderr/debug, engine fuel log, bot telemetry.

    telemetry.py REPLAY.repl26a [--team a|b|both] [--raw] [--dump DIR] [--grep REGEX]
                                [--tail N] [--team-size N] [--stats STATS.json]

Default: per-team summary (stream sizes, 'bot error' lines, engine fuel log per champion
slot, and a digest of telemetry lines). Telemetry lines are any lines containing key=value
tokens, e.g. "T 300 c=2 role=farm fuel_min=41200 susp=0 gems=120". Lines are grouped by
champion: the value of a champ/c/id/name/who key if present, else the leading non-numeric
words before the first key=value token. Per group: lines, min of fuel_min/fuel/fuel_rem,
max of susp/suspensions, and the last value of every other key. Round is taken from a
round/r/t key, or the first integer on the line (so 'T 300 ...' works).
--raw prints the NUL-stripped streams; --dump DIR writes them to files.
"""
from __future__ import annotations

import argparse
import re
import sys
import zipfile
from collections import OrderedDict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import replaylib  # noqa: E402

KV_RE = re.compile(r"(?<![\w.])([A-Za-z_][\w.]*)=(-?\d+(?:\.\d+)?|[A-Za-z_][\w./-]*)")
GROUP_KEYS = ("champ", "champion", "c", "id", "name", "who", "slot")
ROUND_KEYS = ("round", "r", "t", "T")
MIN_KEYS = ("fuel_min", "fuel", "fuel_rem", "fuel_remaining", "fuelmin", "minfuel")
MAX_KEYS = ("susp", "suspensions", "suspended", "susp_total")


def parse_telemetry(text: str, grep: re.Pattern | None) -> OrderedDict:
    groups: OrderedDict[str, dict] = OrderedDict()
    for raw in text.splitlines():
        line = raw.strip()
        if not line or (grep and not grep.search(line)):
            continue
        pairs = KV_RE.findall(line)
        if not pairs:
            continue
        kv = {}
        for k, v in pairs:
            try:
                kv[k] = int(v)
            except ValueError:
                try:
                    kv[k] = float(v)
                except ValueError:
                    kv[k] = v
        gkey = next((f"{k}={kv[k]}" for k in GROUP_KEYS if k in kv), None)
        if gkey is None:
            head = line[:line.find("=")] if "=" in line else line
            words = [w for w in head.split() if not re.fullmatch(r"-?\d+(\.\d+)?", w)]
            gkey = " ".join(words[:-1] if len(words) > 1 else words) or "(no prefix)"
        rnd = next((kv[k] for k in ROUND_KEYS if k in kv and isinstance(kv[k], int)), None)
        if rnd is None:
            m = re.search(r"\b(\d+)\b", line)
            rnd = int(m.group(1)) if m else None
        g = groups.setdefault(gkey, {"lines": 0, "first_round": rnd, "last_round": rnd,
                                     "min": {}, "max": {}, "last": {}, "sample": line[:160]})
        g["lines"] += 1
        if rnd is not None:
            g["first_round"] = rnd if g["first_round"] is None else min(g["first_round"], rnd)
            g["last_round"] = rnd if g["last_round"] is None else max(g["last_round"], rnd)
        for k, v in kv.items():
            if not isinstance(v, (int, float)):
                g["last"][k] = v
                continue
            if k in MIN_KEYS:
                g["min"][k] = v if k not in g["min"] else min(g["min"][k], v)
            elif k in MAX_KEYS:
                g["max"][k] = v if k not in g["max"] else max(g["max"][k], v)
            else:
                g["last"][k] = v
    return groups


def print_team(team: str, bundle: zipfile.ZipFile, args, team_size: int | None) -> None:
    stdout = replaylib.read_stream(bundle, team, "stdout")
    stderr = replaylib.read_stream(bundle, team, "stderr")
    debug = replaylib.read_stream(bundle, team, "debug")
    print(f"=== team {team.upper()}  stdout {len(stdout.splitlines())} lines, stderr {len(stderr.splitlines())}, debug {len(debug.splitlines())}")
    scan = replaylib.scan_stdout(stdout)
    print(f"  'bot error' lines: {scan['bot_error_lines']}   fuel/suspend lines: {scan['fuel_lines']}")
    for s in scan["bot_error_samples"]:
        print(f"    e.g. {s[:120]}")

    name = f"team-{team}.fuel"
    fuel = replaylib.fuel_summary(replaylib.parse_fuel_file(bundle.read(name)) if name in bundle.namelist() else {}, team_size)
    if fuel.get("ok"):
        print(f"  engine fuel log: quantum {fuel['quantum']} (first turn {fuel['quantum_first']}), team size {fuel['team_size']}, "
              f"{fuel['rounds']} rounds; draft turns used {fuel['draft_used']} (quantum {fuel['draft_quantum']}, suspended {fuel['draft_suspended']})")
        print(f"  {'slot':<5}{'turns':>6}{'suspended':>10}{'first_used':>11}{'max_used':>10}{'mean_used':>10}{'min_remaining':>14}")
        for c in fuel["champions"]:
            print(f"  {c['slot']:<5}{c['turns']:>6}{c['suspended']:>10}{c['first_turn_used']:>11}{c['max_used']:>10}{c['mean_used']:>10.0f}{c['min_remaining']:>14}")
    else:
        print("  engine fuel log: absent/undecodable")

    groups = parse_telemetry(stdout + "\n" + stderr, args.grep)
    if groups:
        print(f"  telemetry groups ({len(groups)}):")
        for key, g in groups.items():
            mins = " ".join(f"{k}={v}" for k, v in g["min"].items())
            maxs = " ".join(f"{k}={v}" for k, v in g["max"].items())
            last = " ".join(f"{k}={v}" for k, v in list(g["last"].items())[:8])
            print(f"    [{key}] {g['lines']} lines, rounds {g['first_round']}..{g['last_round']}"
                  + (f"  min: {mins}" if mins else "") + (f"  max: {maxs}" if maxs else "")
                  + (f"  last: {last}" if last else ""))
            if args.verbose:
                print(f"       sample: {g['sample']}")
    else:
        print("  telemetry: no key=value lines" + (" matching --grep" if args.grep else ""))

    if args.raw or args.tail:
        for sname, text in (("stdout", stdout), ("stderr", stderr), ("debug", debug)):
            if not text:
                continue
            lines = text.splitlines()
            if args.grep:
                lines = [l for l in lines if args.grep.search(l)]
            if args.tail:
                lines = lines[-args.tail:]
            print(f"--- team {team} {sname} ({len(lines)} lines) ---")
            print("\n".join(lines))


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("replay")
    ap.add_argument("--team", default="both", choices=["a", "b", "both"])
    ap.add_argument("--raw", action="store_true", help="print the full NUL-stripped streams")
    ap.add_argument("--tail", type=int, default=0, help="print only the last N lines of each stream")
    ap.add_argument("--grep", default=None, help="regex filter for telemetry lines and --raw/--tail output")
    ap.add_argument("--dump", default=None, help="write team-X.{stdout,stderr,debug} (NUL-stripped) to this dir")
    ap.add_argument("--team-size", type=int, default=None, help="champions per team for the fuel log (default: auto, or from --stats)")
    ap.add_argument("--stats", default=None, help="matching statistics JSON (default: <replay stem>.json if it exists)")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()
    args.grep = re.compile(args.grep, re.I) if args.grep else None

    path = Path(args.replay)
    if not path.exists():
        sys.exit(f"telemetry: no such file {path}")
    try:
        bundle = zipfile.ZipFile(path)
    except zipfile.BadZipFile as e:
        sys.exit(f"telemetry: {path} is not a replay bundle: {e}")

    sizes = {"a": args.team_size, "b": args.team_size}
    stats_path = Path(args.stats) if args.stats else path.with_suffix(".json")
    if args.team_size is None and stats_path.exists():
        try:
            st = replaylib.load_stats(stats_path)
            for team, key in (("a", "teamA"), ("b", "teamB")):
                n = sum(1 for d in st.get("roster", {}).get("draft", []) if d.get("team") == key)
                sizes[team] = n or None
            view = {k: replaylib.stats_team_view(st, v) for k, v in (("A", "teamA"), ("B", "teamB"))}
            print(f"stats {stats_path.name}: outcome {st['match'].get('outcome')} after {st['match'].get('roundsStarted')} rounds; "
                  + "; ".join(f"{k}: gems {v['gems']} faults {v['faults']} suspensions {v['suspensions']}"
                              f"{' (draft ' + str(v['draft_suspensions']) + ')' if v['draft_suspensions'] else ''}"
                              f" draft {','.join(v['draft'])}" for k, v in view.items()))
        except (OSError, ValueError, KeyError) as e:
            print(f"(could not read stats {stats_path}: {e})")

    print(f"{path}: " + ", ".join(f"{i.filename} {i.file_size}B" for i in bundle.infolist()))
    with bundle:
        if args.dump:
            d = Path(args.dump); d.mkdir(parents=True, exist_ok=True)
            for team in ("a", "b"):
                for s in replaylib.STREAMS:
                    (d / f"team-{team}.{s}").write_text(replaylib.read_stream(bundle, team, s))
            print(f"dumped streams to {d}")
        for team in (["a", "b"] if args.team == "both" else [args.team]):
            print_team(team, bundle, args, sizes.get(team))


if __name__ == "__main__":
    main()
