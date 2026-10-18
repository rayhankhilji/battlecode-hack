"""Helpers for reading cambc replay bundles (*.repl26a) and match statistics JSON.

A replay bundle is a zip with: replay.repl26a (binary), team-a.stdout, team-b.stdout,
team-a.debug, team-b.debug, team-a.fuel, team-b.fuel. The stdout streams contain stray NUL
bytes between writes (engine framing); strip them before parsing. The .fuel files are a
small binary log of fuel used per bot turn (see parse_fuel_file). Stdlib only.
"""
from __future__ import annotations

import json
import re
import struct
import zipfile
from pathlib import Path

FUEL_MAGIC = b"a62leuf"
STREAMS = ("stdout", "stderr", "debug")

# Lines our bots (and the starters) print on caught exceptions / engine rejections.
BOT_ERROR_RE = re.compile(r"bot error", re.I)
# Anything a bot might print about running low on / out of fuel.
FUEL_LINE_RE = re.compile(r"suspend|suspension|out of fuel|fuel exhausted|fuel_exhausted", re.I)


def read_stream(bundle: zipfile.ZipFile, team: str, stream: str) -> str:
    """Return team ('a'/'b') stream text with engine NUL framing removed ('' if absent)."""
    name = f"team-{team}.{stream}"
    if name not in bundle.namelist():
        return ""
    return bundle.read(name).replace(b"\x00", b"").decode("utf-8", "replace")


def parse_fuel_file(data: bytes) -> dict:
    """Decode team-X.fuel.

    Layout (observed, engine 0.0.3 / cambc 2.2.5): 7-byte magic 'a62leuf', 6 header bytes,
    then 8 little-endian u64 quanta: (draft first turn, draft turn, play first turn, play
    turn) for native bots, then the same four for Python bots (x25); both blocks appear in
    both teams' files, so the block is chosen by which one the records fit under. Then
    one u64 per bot turn = fuel USED that turn. Turn records are: one per draft pick first
    (team_size of them), then for every round one per champion in champion order. A record
    equal to its quantum means the turn ran out of fuel (suspended).
    """
    if not data.startswith(FUEL_MAGIC) or len(data) < 77:
        return {"ok": False, "quanta": (), "records": ()}
    quanta = struct.unpack("<8Q", data[13:77])
    n = (len(data) - 77) // 8
    records = struct.unpack(f"<{n}Q", data[77:77 + n * 8])
    # Both blocks appear in both teams' files; block 1 is the native (C++/Rust) quantum set,
    # block 2 the Python set (x25). Pick the first block no record exceeds.
    blocks = [quanta[0:4], quanta[4:8]]
    peak = max(records) if records else 0
    block = next((b for b in blocks if peak <= max(b)), blocks[-1])
    return {"ok": True, "quanta": quanta, "records": records,
            "draft_first": block[0], "draft": block[1],
            "play_first": block[2], "play": block[3]}


def fuel_summary(fuel: dict, team_size: int | None) -> dict:
    """Per-champion fuel usage. team_size=None tries 5,4,3 and picks the best fit."""
    recs = fuel.get("records", ())
    if not fuel.get("ok") or not recs:
        return {"ok": False}
    if team_size is None:
        # Draft records come first (team_size of them); a partial final round may leave
        # up to team_size-1 extra records, so pick the size with the smallest remainder.
        team_size = min((5, 4, 3), key=lambda k: ((len(recs) - k) % k, -k))
    draft, play = recs[:team_size], recs[team_size:]
    q_play, q_first = fuel["play"], fuel["play_first"]
    champs = []
    for c in range(team_size):
        turns = play[c::team_size]
        if not turns:
            continue
        suspended = sum(1 for i, u in enumerate(turns) if u >= (q_first if i == 0 else q_play))
        later = turns[1:] or turns
        champs.append({
            "slot": c, "turns": len(turns), "suspended": suspended,
            "first_turn_used": turns[0], "max_used": max(later), "mean_used": sum(later) / len(later),
            "min_remaining": q_play - max(later),
        })
    return {"ok": True, "team_size": team_size, "quantum": q_play, "quantum_first": q_first,
            "draft_used": list(draft), "draft_quantum": fuel["draft_first"],
            "draft_suspended": sum(1 for i, u in enumerate(draft)
                                   if u >= (fuel["draft_first"] if i == 0 else fuel["draft"])),
            "champions": champs, "rounds": len(play) // team_size}


def scan_stdout(text: str) -> dict:
    """Count bot-error and fuel-related lines in a NUL-stripped stream."""
    lines = text.splitlines()
    errors = [l for l in lines if BOT_ERROR_RE.search(l)]
    fuel = [l for l in lines if FUEL_LINE_RE.search(l)]
    return {"lines": len(lines), "bot_error_lines": len(errors), "fuel_lines": len(fuel),
            "bot_error_samples": sorted(set(l.strip() for l in errors))[:5]}


def inspect_replay(path: str | Path, team_sizes: dict | None = None) -> dict:
    """Summarise both teams' streams and fuel logs. team_sizes: {'a': n, 'b': n} or None."""
    out = {"path": str(path), "ok": True}
    try:
        with zipfile.ZipFile(path) as bundle:
            for team in ("a", "b"):
                info = scan_stdout(read_stream(bundle, team, "stdout"))
                info["stderr_lines"] = len(read_stream(bundle, team, "stderr").splitlines())
                info["debug_lines"] = len(read_stream(bundle, team, "debug").splitlines())
                raw = bundle.read(f"team-{team}.fuel") if f"team-{team}.fuel" in bundle.namelist() else b""
                info["fuel"] = fuel_summary(parse_fuel_file(raw), (team_sizes or {}).get(team))
                out[team] = info
    except (zipfile.BadZipFile, OSError) as e:
        out["ok"] = False
        out["error"] = f"{type(e).__name__}: {e}"
    return out


# ---------------------------------------------------------------- statistics JSON helpers

def num(v):
    """Statistics JSON encodes integer metrics as strings ('414'); floats stay floats."""
    if isinstance(v, str):
        try:
            return int(v)
        except ValueError:
            return float(v)
    return v


def load_stats(path: str | Path) -> dict:
    with open(path, "rb") as f:
        return json.load(f)


def stats_team_view(stats: dict, team: str) -> dict:
    """Aggregate one team's ('teamA'/'teamB') numbers from a statistics JSON document."""
    champs = [c for c in stats.get("champions", []) if c.get("team") == team]
    agg = {k: 0 for k in ("kills", "deaths", "assists", "collectedGems", "submittedGems",
                          "earnedGold", "spentGold", "championDamageDone", "monsterDamageDone")}
    for c in champs:
        m = c.get("metrics", {})
        for k in agg:
            agg[k] += num(m.get(k, 0))
    team_row = next((t for t in stats.get("teams", []) if t.get("team") == team), {})
    susp, susp_rounds, draft_susp = 0, set(), 0
    for e in stats.get("events", []):
        owner = e.get("owner")
        if not isinstance(owner, dict) or owner.get("team") != team or e.get("kind") != "fuelExhausted":
            continue
        n = len(e.get("entries", []))
        if owner.get("kind") == "draft":
            draft_susp += n
        else:
            susp += n
            susp_rounds.update(en["timestamp"]["round"] for en in e["entries"] if "timestamp" in en)
    draft = [d["champion"] for d in stats.get("roster", {}).get("draft", []) if d.get("team") == team]
    return {
        "gems": num(team_row.get("submittedGems", agg["submittedGems"])),
        "faults": num(stats.get("diagnostics", {}).get("botFaults", {}).get(team, 0)),
        "suspensions": susp, "draft_suspensions": draft_susp,
        "first_suspension_round": min(susp_rounds) if susp_rounds else None,
        "draft": draft, **agg,
    }
