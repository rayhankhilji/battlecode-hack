#!/usr/bin/env python3
"""Cross-check a replay_dump.mjs JSONL against the engine statistics JSON (`cambc run --statistics`).

    tools/replay_validate.py REPLAY.repl26a [STATS.json]      # STATS defaults to <stem>.json next to the replay
    tools/replay_validate.py DIR                               # every *.repl26a with a sibling .json

Compares (exactly): team submittedGems, finish round, per-champion kills/deaths/assists/submittedGems/gems/gold/earnedGold,
death rounds, number of gemSubmission and campClear (= group_clear) events. Exit 1 on any mismatch.
"""
import json, os, subprocess, sys, tempfile
from collections import Counter, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
DUMP = os.path.join(HERE, 'replay_dump.mjs')


def num(v):
    return int(v) if isinstance(v, str) else v


def dump(replay):
    fd, tmp = tempfile.mkstemp(suffix='.jsonl'); os.close(fd)
    try:
        subprocess.run(['node', DUMP, replay, '-o', tmp], check=True, capture_output=True)
        return [json.loads(l) for l in open(tmp)]
    finally:
        os.unlink(tmp)


def validate(replay, stats_path):
    rows = dump(replay)
    st = json.load(open(stats_path))
    final = rows[-1]; ev = [r for r in rows if r['type'] == 'event']
    errs = []
    def cmp(label, a, b):
        if a != b: errs.append(f'{label}: dump={a} stats={b}')
    teams = {t['team']: num(t['submittedGems']) for t in st['teams']}
    cmp('submitted A', final['submitted'][0], teams['teamA']); cmp('submitted B', final['submitted'][1], teams['teamB'])
    cmp('winner', final['winner'], st['match']['outcome'])
    cmp('finish round', final['finish_round'], num(st['match']['roundsStarted']))
    sub_by = Counter(); 
    for e in ev:
        if e['kind'] == 'submission' and e['id'] is not None: sub_by[e['id']] += e['amount']
    deaths_by = defaultdict(list); campkill_by = Counter(); nsub_by = Counter()
    for e in ev:
        if e['kind'] == 'death': deaths_by[e['id']].append(e['round'])
        if e['kind'] == 'group_clear' and e['id'] is not None: campkill_by[e['id']] += 1
        if e['kind'] == 'submission' and e['id'] is not None: nsub_by[e['id']] += 1
    fc = {c['id']: c for c in final['champions']}
    for c in st['champions']:
        m = c['metrics']; d = fc.get(c['id']); tag = f"#{c['id']} {c['name']} {c['team']}"
        if d is None: errs.append(f'{tag}: missing in dump'); continue
        cmp(f'{tag} name', d['name'], c['name'])
        for k, dk in [('kills', 'kills'), ('deaths', 'deaths'), ('assists', 'assists'), ('gems', 'gems'), ('gold', 'gold'), ('earnedGold', 'earned_gold')]:
            cmp(f'{tag} {k}', d[dk], num(m[k]))
        cmp(f'{tag} submittedGems', sub_by[c['id']], num(m['submittedGems']))
    for e in st['events']:
        o = e['owner']
        if o['kind'] != 'champion': continue
        rounds = [x['timestamp']['round'] for x in e['entries']]
        tag = f"#{o['id']} {o['name']} {o['team']}"
        if e['kind'] == 'death': cmp(f'{tag} death rounds', deaths_by.get(o['id'], []), rounds)
        if e['kind'] == 'gemSubmission': cmp(f'{tag} #gemSubmission', nsub_by[o['id']], len(rounds))
        if e['kind'] == 'campClear': cmp(f'{tag} #campClear', campkill_by[o['id']], len(rounds))
    # channel encoding sanity: at a submit_start the champion must be channelling type 2 at round end (unless finished same round)
    return errs, rows


def main(argv):
    if not argv: print(__doc__); return 2
    pairs = []
    if os.path.isdir(argv[0]):
        for f in sorted(os.listdir(argv[0])):
            if f.endswith('.repl26a') and os.path.exists(os.path.join(argv[0], f[:-8] + '.json')):
                pairs.append((os.path.join(argv[0], f), os.path.join(argv[0], f[:-8] + '.json')))
    else:
        pairs.append((argv[0], argv[1] if len(argv) > 1 else argv[0][:-8] + '.json'))
    bad = 0
    for rp, sp in pairs:
        errs, rows = validate(rp, sp)
        final = rows[-1]
        status = 'OK ' if not errs else 'BAD'
        print(f"{status} {os.path.basename(rp)}: {final['winner']} r{final['finish_round']} gems {final['submitted']} checks={'all exact' if not errs else len(errs)}")
        for e in errs: print('    ' + e)
        bad += bool(errs)
    print(f'{len(pairs) - bad}/{len(pairs)} replays match the statistics exactly')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
