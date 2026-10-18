#!/usr/bin/env python3
"""replay_analyse.py — turn replay_dump.mjs JSONL into a side-by-side team A vs team B markdown report.

    tools/replay_analyse.py DUMP.jsonl [DUMP2.jsonl ...] [-o report.md] [--names "A=Electric Muffin,B=Mite"]
                            [--map NAME] [--maps-md docs/MAPS.md] [--checkpoints 100,200,300,400,500]
    tools/replay_analyse.py REPLAY.repl26a ...          # runs replay_dump.mjs first (needs node)

Per team: time to first tribute, every tribute (round, amount, champion), captures timeline with party size,
gems banked at checkpoint rounds, deaths (round, position, gems lost, killer), kills, per-champion gems / 100 rounds,
idle fraction, distance travelled, camp groups farmed in order (matched to docs/MAPS.md group ids and spawn tiles),
recalls, channel time, fuel suspensions and bot errors. Metric definitions: docs/REPLAY.md.
"""
import argparse, json, os, re, subprocess, sys, tempfile
from collections import Counter, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
DUMP = os.path.join(HERE, 'replay_dump.mjs')
MOVE_ACTS = ('move', 'attack', 'cast', 'shop', 'ward', 'lock', 'consumable', 'recall', 'submit', 'teleport')


# ----------------------------------------------------------------------------- loading
def load_dump(path):
    if path.endswith('.jsonl'):
        rows = [json.loads(l) for l in open(path)]
    else:
        fd, tmp = tempfile.mkstemp(suffix='.jsonl'); os.close(fd)
        try:
            subprocess.run(['node', DUMP, path, '-o', tmp], check=True, capture_output=True)
            rows = [json.loads(l) for l in open(tmp)]
        finally:
            os.unlink(tmp)
    d = {'meta': rows[0], 'final': rows[-1], 'champ': defaultdict(list), 'round': {}, 'events': []}
    for r in rows[1:-1]:
        if r['type'] == 'champ': d['champ'][r['id']].append(r)
        elif r['type'] == 'round': d['round'][r['round']] = r
        elif r['type'] == 'event': d['events'].append(r)
    for rows_ in d['champ'].values(): rows_.sort(key=lambda r: r['round'])
    d['teams'] = {0: sorted({r['id'] for r in d['final']['champions'] if r['team'] == 0}), 1: sorted({r['id'] for r in d['final']['champions'] if r['team'] == 1})}
    d['names'] = {c['id']: c['name'] for c in d['final']['champions']}
    return d


# ----------------------------------------------------------------------------- docs/MAPS.md
def parse_maps_md(path):
    maps = {}
    if not path or not os.path.exists(path): return maps
    txt = open(path).read()
    for line in txt.splitlines():
        m = re.match(r'^\| ([\w-]+) \| (\d+)x(\d+) \| (\w+) \| (\d+) \| (\d+) \((\d+)\) \|', line)
        if m:
            maps[m.group(1)] = {'width': int(m.group(2)), 'height': int(m.group(3)), 'symmetry': m.group(4), 'team_size': int(m.group(5)),
                                'n_groups': int(m.group(6)), 'n_camps': int(m.group(7)), 'groups': {},
                                'altars': [(int(a), int(b)) for a, b in re.findall(r'\((\d+),(\d+)\)', line.split('|')[9])]}
    cur = None
    for line in txt.splitlines():
        m = re.match(r'^### ([\w-]+) — groups', line)
        if m: cur = maps.get(m.group(1)); continue
        if cur is None: continue
        m = re.match(r'^\| (\d+) \| (\d+) \| (\d+) \| (\d+) \| (-?\d+) \| (-?\d+) \| (-?\d+) \| (.*) \|$', line)
        if m:
            members = [(t, int(x), int(y)) for t, x, y in re.findall(r'(\w+)\((\d+),(\d+)\)', m.group(8))]
            cur['groups'][int(m.group(1))] = {'gems': int(m.group(2)), 'gold': int(m.group(3)), 'respawn': int(m.group(4)), 'd_a': int(m.group(5)), 'd_b': int(m.group(6)), 'd_altar': int(m.group(7)), 'members': members}
    return maps


def identify_map(meta, maps, forced=None):
    if forced: return forced, maps.get(forced)
    tiles = {(c['x'], c['y']) for c in meta.get('camps', [])}
    best = None
    for name, m in maps.items():
        if (m['width'], m['height']) != (meta['width'], meta['height']): continue
        mt = {(x, y) for g in m['groups'].values() for _, x, y in g['members']}
        score = len(tiles & mt)
        if mt and score == len(tiles) == len(mt): return name, m
        if best is None or score > best[0]: best = (score, name, m)
    if best and best[0] > 0: return best[1], best[2]
    return None, None


def group_labels(meta, mapinfo):
    """live group id -> (label, docs group id or None, docs info)"""
    out = {}
    for g in meta.get('groups', []):
        tiles = {tuple(t) for t in g['tiles']}
        hit = None
        if mapinfo:
            for gid, info in mapinfo['groups'].items():
                if {(x, y) for _, x, y in info['members']} == tiles: hit = (gid, info); break
        if hit:
            kinds = Counter(t for t, _, _ in hit[1]['members'])
            desc = '+'.join(f'{n}{k}' if n > 1 else k for k, n in kinds.items())
            out[g['id']] = (f"g{hit[0]}", hit[0], hit[1], desc)
        else:
            kinds = Counter(meta['camps'][c]['type_name'] for c in g['camps'])
            desc = '+'.join(f'{n}{k}' if n > 1 else k for k, n in kinds.items())
            out[g['id']] = (f"L{g['id']}@{g['tiles'][0][0]},{g['tiles'][0][1]}", None, None, desc)
    return out


# ----------------------------------------------------------------------------- metrics
def cheb(a, b): return max(abs(a[0] - b[0]), abs(a[1] - b[1]))


def champion_stats(d, cid, glabels):
    rows = d['champ'][cid]; meta = d['meta']
    finish = d['final']['finish_round']
    st = {'id': cid, 'name': d['names'][cid], 'team': rows[0]['team'] if rows else None, 'rounds': len(rows), 'dead_rounds': 0, 'channel_rounds': 0,
          'idle': 0, 'idle_base': 0, 'act_idle': 0, 'act_base': 0, 'dist': 0, 'jumps': 0, 'collected': 0, 'submitted': 0, 'n_submits': 0,
          'recalls': 0, 'fuel_out': 0, 'errors': 0, 'blocked': 0, 'casts': 0, 'attacks': 0, 'moves': 0, 'shops': 0, 'wards': 0, 'locks': 0,
          'deaths': 0, 'kills': 0, 'assists': 0, 'route': [], 'captures': 0, 'gems_lost': 0, 'max_bag': 0, 'rounds_carrying': 0}
    tiles_by_group = {g['id']: [tuple(t) for t in g['tiles']] for g in meta.get('groups', [])}
    prev = None; cur_group = None; cur_start = None
    for r in rows:
        a = r['acted']
        st['moves'] += a.get('move', 0); st['attacks'] += a.get('attack', 0); st['casts'] += a.get('cast', 0); st['shops'] += a.get('shop', 0)
        st['wards'] += a.get('ward', 0); st['locks'] += a.get('lock', 0); st['blocked'] += a.get('blocked', 0); st['fuel_out'] += a.get('fuel_out', 0); st['errors'] += a.get('error', 0); st['recalls'] += a.get('recall', 0)
        if not r['alive']:
            st['dead_rounds'] += 1
        else:
            if r['channel']: st['channel_rounds'] += 1
            if r['gems'] > 0: st['rounds_carrying'] += 1
            st['max_bag'] = max(st['max_bag'], r['gems'])
            if not r['channel'] and prev is not None and prev['alive']:
                st['act_base'] += 1
                if not any(a.get(k) for k in MOVE_ACTS): st['act_idle'] += 1
                st['idle_base'] += 1
                if (prev['x'], prev['y']) == (r['x'], r['y']) and prev['gems'] == r['gems']: st['idle'] += 1
            if prev is not None and prev['alive'] and prev['x'] is not None and r['x'] is not None:
                dd = cheb((prev['x'], prev['y']), (r['x'], r['y']))
                if dd <= 3: st['dist'] += dd
                else: st['jumps'] += 1
            # route: nearest group whose tiles are within 1 step
            here = None
            for gid, tiles in tiles_by_group.items():
                if any(cheb((r['x'], r['y']), t) <= 1 for t in tiles): here = gid; break
            if here != cur_group:
                if cur_group is not None and r['round'] - cur_start >= 2: st['route'].append((cur_group, cur_start, prev['round'] if prev else r['round']))
                cur_group, cur_start = here, r['round']
        if prev is not None and r['gems'] > prev['gems']: st['collected'] += r['gems'] - prev['gems']
        prev = r
    if cur_group is not None and prev is not None and prev['round'] - cur_start >= 2: st['route'].append((cur_group, cur_start, prev['round']))
    fc = next((c for c in d['final']['champions'] if c['id'] == cid), {})
    st['deaths'] = fc.get('deaths', 0); st['kills'] = fc.get('kills', 0); st['assists'] = fc.get('assists', 0); st['earned_gold'] = fc.get('earned_gold', 0); st['final_gems'] = fc.get('gems', 0)
    for e in d['events']:
        if e['kind'] == 'submission' and e['id'] == cid: st['submitted'] += e['amount']; st['n_submits'] += 1
        if e['kind'] == 'capture' and cid in e['party_ids']: st['captures'] += 1
        if e['kind'] == 'death' and e['id'] == cid: st['gems_lost'] += e['gems_lost']
    st['gems_per_100'] = st['collected'] * 100 / max(1, finish)
    st['idle_frac'] = st['idle'] / st['idle_base'] if st['idle_base'] else 0
    st['act_idle_frac'] = st['act_idle'] / st['act_base'] if st['act_base'] else 0
    return st


def team_stats(d, team, glabels, checkpoints):
    ev = d['events']; finish = d['final']['finish_round']
    subs = [e for e in ev if e['kind'] == 'submission' and e['team'] == team]
    caps = [e for e in ev if e['kind'] == 'capture' and e['team'] == team]
    deaths = [e for e in ev if e['kind'] == 'death' and e['team'] == team]
    kills = [e for e in ev if e['kind'] == 'kill' and e['team'] == team]
    clears = [e for e in ev if e['kind'] == 'group_clear' and e['team'] == team]
    rounds = d['round']
    banked = {}
    for cp in checkpoints:
        r = rounds.get(cp) or (rounds[max(rounds)] if rounds and cp > max(rounds) else None)
        banked[cp] = r['submitted'][team] if r else None
    party = [len(c['gem_split']) if c['gems'] else c['party_near'] for c in caps]
    champs = [champion_stats(d, cid, glabels) for cid in d['teams'][team]]
    gl = lambda g: glabels.get(g, (f'L{g}',))[0]
    farm_order = []
    seen = set()
    for c in caps:
        if c['group'] not in seen: seen.add(c['group']); farm_order.append((c['group'], c['round']))
    ts = {'team': team, 'submitted': d['final']['submitted'][team], 'first_tribute': subs[0]['round'] if subs else None, 'tributes': subs,
          'captures': caps, 'n_captures': len(caps), 'avg_party': sum(party) / len(party) if party else 0, 'gems_from_captures': sum(c['gems'] for c in caps),
          'gold_from_captures': sum(c['gold'] for c in caps), 'deaths': deaths, 'kills': kills, 'banked': banked, 'champs': champs, 'farm_order': farm_order,
          'clears': len(clears), 'group_counts': Counter(c['group'] for c in caps),
          'recalls': sum(c['recalls'] for c in champs), 'fuel_out': sum(c['fuel_out'] for c in champs), 'errors': sum(c['errors'] for c in champs),
          'blocked': sum(c['blocked'] for c in champs), 'dead_rounds': sum(c['dead_rounds'] for c in champs), 'channel_rounds': sum(c['channel_rounds'] for c in champs),
          'dist': sum(c['dist'] for c in champs), 'collected': sum(c['collected'] for c in champs), 'gems_lost': sum(e['gems_lost'] for e in deaths),
          'lost_kill_loot': sum(e['gems_gained'] for e in kills), 'avg_bag_at_submit': (sum(e['amount'] for e in subs) / len(subs)) if subs else 0,
          'alive_rounds': sum(c['rounds'] - c['dead_rounds'] for c in champs), 'idle': sum(c['idle'] for c in champs), 'idle_base': sum(c['idle_base'] for c in champs),
          'act_idle': sum(c['act_idle'] for c in champs), 'act_base': sum(c['act_base'] for c in champs), 'locks': sum(c['locks'] for c in champs), 'wards': sum(c['wards'] for c in champs),
          'earned_gold': sum(c['earned_gold'] for c in champs)}
    ts['idle_frac'] = ts['idle'] / ts['idle_base'] if ts['idle_base'] else 0
    ts['act_idle_frac'] = ts['act_idle'] / ts['act_base'] if ts['act_base'] else 0
    # rounds from first capture to first tribute, and tribute cadence
    ts['first_capture'] = caps[0]['round'] if caps else None
    ts['rounds_to_1000'] = next((e['round'] for e in subs if e['total'] >= 1000), None)
    return ts


# ----------------------------------------------------------------------------- report
def fmt(v, nd=1):
    if v is None: return '-'
    if isinstance(v, float): return f'{v:.{nd}f}'
    return str(v)


def report(d, names, mapname, mapinfo, glabels, checkpoints):
    meta, final = d['meta'], d['final']
    A, B = (team_stats(d, 0, glabels, checkpoints), team_stats(d, 1, glabels, checkpoints))
    gl = lambda g: glabels.get(g, (f'L{g}',))[0]
    gdesc = lambda g: glabels.get(g, (None, None, None, '?'))[3]
    def gdist(g, team):
        info = glabels.get(g, (None, None, None))[2]
        if not info: return ''
        return f"{info['d_a'] if team == 0 else info['d_b']}/{info['d_altar']}"
    L = []
    nm = lambda t: names.get(t, 'Team ' + 'AB'[t])
    winner = final['winner']
    L.append(f"## {meta['file']} — {mapname or f'{meta['width']}x{meta['height']}'}: {nm(0)} (A) vs {nm(1)} (B)")
    L.append('')
    L.append(f"Winner **{nm(0) if winner == 'teamA' else nm(1) if winner == 'teamB' else winner}** at round {final['finish_round']}; gems A {final['submitted'][0]} / B {final['submitted'][1]}; "
             f"engine {meta['engine']}; map {meta['width']}x{meta['height']}, team size {meta['team_size']}, {len(meta.get('camps', []))} camps in {len(meta.get('groups', []))} groups"
             + (f" ({mapinfo['n_groups']} groups per docs/MAPS.md)" if mapinfo else '') + '.')
    L.append('')
    L.append(f"Draft A: {', '.join(x['name'] for x in meta['drafts'][0])} · Draft B: {', '.join(x['name'] for x in meta['drafts'][1])}")
    L.append('')
    L.append('### Summary')
    L.append('')
    L.append(f'| metric | {nm(0)} (A) | {nm(1)} (B) |'); L.append('|---|---|---|')
    def row(label, fa, fb): L.append(f'| {label} | {fa} | {fb} |')
    row('gems tributed', A['submitted'], B['submitted'])
    row('round of 1000 gems', fmt(A['rounds_to_1000']), fmt(B['rounds_to_1000']))
    row('first capture (round)', fmt(A['first_capture']), fmt(B['first_capture']))
    row('first tribute (round)', fmt(A['first_tribute']), fmt(B['first_tribute']))
    row('tributes (count, avg bag)', f"{len(A['tributes'])}, {fmt(A['avg_bag_at_submit'])}", f"{len(B['tributes'])}, {fmt(B['avg_bag_at_submit'])}")
    for cp in checkpoints: row(f'banked by round {cp}', fmt(A['banked'][cp]), fmt(B['banked'][cp]))
    row('captures (groups cleared)', f"{A['n_captures']} ({A['clears']})", f"{B['n_captures']} ({B['clears']})")
    row('gems / gold from captures', f"{A['gems_from_captures']} / {A['gold_from_captures']}", f"{B['gems_from_captures']} / {B['gold_from_captures']}")
    row('avg party size at capture', fmt(A['avg_party'], 2), fmt(B['avg_party'], 2))
    row('gems collected (incl. kill loot)', A['collected'], B['collected'])
    row('gems lost on death', A['gems_lost'], B['gems_lost'])
    row('gems looted from kills', A['lost_kill_loot'], B['lost_kill_loot'])
    row('kills / deaths', f"{len(A['kills'])} / {len(A['deaths'])}", f"{len(B['kills'])} / {len(B['deaths'])}")
    row('champion-rounds dead', A['dead_rounds'], B['dead_rounds'])
    row('champion-rounds channelling', A['channel_rounds'], B['channel_rounds'])
    row('recalls', A['recalls'], B['recalls'])
    row('stationary fraction (pos+bag unchanged, not channelling)', f"{A['idle_frac']:.1%}", f"{B['idle_frac']:.1%}")
    row('idle fraction (no action in the turn)', f"{A['act_idle_frac']:.1%}", f"{B['act_idle_frac']:.1%}")
    row('distance travelled (tiles)', A['dist'], B['dist'])
    row('gold earned', A['earned_gold'], B['earned_gold'])
    row('altar locks / wards placed', f"{A['locks']} / {A['wards']}", f"{B['locks']} / {B['wards']}")
    row('fuel suspensions / bot errors / blocked moves', f"{A['fuel_out']} / {A['errors']} / {A['blocked']}", f"{B['fuel_out']} / {B['errors']} / {B['blocked']}")
    L.append('')
    # tribute timeline
    L.append('### Tribute timeline')
    L.append('')
    L.append(f'| {nm(0)} (A) round: +amount by champion → total | {nm(1)} (B) round: +amount by champion → total |'); L.append('|---|---|')
    ta = [f"r{e['round']}: +{e['amount']} {e['name']}#{e['id']} → {e['total']}" for e in A['tributes']]
    tb = [f"r{e['round']}: +{e['amount']} {e['name']}#{e['id']} → {e['total']}" for e in B['tributes']]
    for i in range(max(len(ta), len(tb))): L.append(f"| {ta[i] if i < len(ta) else ''} | {tb[i] if i < len(tb) else ''} |")
    L.append('')
    # captures
    L.append('### Captures')
    L.append('')
    L.append('Group ids are docs/MAPS.md ids (`gN`) when the map is known, else `L<live id>@x,y`. `d` = walking distance own spawn / to nearest altar (from MAPS.md).')
    L.append('')
    for T in (A, B):
        t = T['team']
        L.append(f"**{nm(t)} ({'AB'[t]})** — {T['n_captures']} captures, farming order: " + ' → '.join(f"{gl(g)}(r{r})" for g, r in T['farm_order']))
        L.append('')
        L.append('| round | group | type | gems | gold | party | d own/altar | champions |'); L.append('|---|---|---|---|---|---|---|---|')
        for c in T['captures']:
            party = len(c['gem_split']) if c['gems'] else c['party_near']
            who = ', '.join(f"{d['names'][i]}#{i}" + (f"(+{g})" if c['gems'] else '') for i, g in c['gem_split']) if c['gems'] else ', '.join(f"{d['names'][i]}#{i}" for i in c['party_ids'])
            L.append(f"| {c['round']} | {gl(c['group'])} | {gdesc(c['group'])} | {c['gems']} | {c['gold']} | {party} | {gdist(c['group'], t)} | {who} |")
        L.append('')
        if T['group_counts']:
            L.append('Captures per group: ' + ', '.join(f"{gl(g)}×{n}" for g, n in T['group_counts'].most_common()))
            L.append('')
    # deaths & kills
    L.append('### Deaths and kills')
    L.append('')
    L.append('| round | victim | team | at | gems lost | gold | killed by |'); L.append('|---|---|---|---|---|---|---|')
    for e in sorted([x for x in d['events'] if x['kind'] == 'death'], key=lambda x: x['round']):
        by = f"{e['by_name']}#{e['by_id']} ({nm(e['by_team'])})" if e['by_kind'] == 'champion' and e['by_id'] is not None else e['by_kind']
        L.append(f"| {e['round']} | {e['name']}#{e['id']} | {nm(e['team'])} | ({e['x']},{e['y']}) | {e['gems_lost']} | {e['gold']} | {by} |")
    L.append('')
    # per champion
    L.append('### Champions')
    L.append('')
    L.append('`gems/100r` = gems collected per 100 rounds of the match; `still` = alive, not channelling, position and bag unchanged since last round (includes standing and hitting a camp); '
             '`idle` = no move/attack/cast/shop/ward/lock/consumable/channel in the turn; `dist` = Chebyshev tiles walked; route = camp groups the champion stood next to (≥2 rounds), in order.')
    L.append('')
    L.append('| team | champion | tributed (n) | collected | gems/100r | captures | kills/deaths/assists | dead rounds | channel rounds | still | idle | dist | max bag | recalls | fuel-out/err | route |')
    L.append('|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|')
    for T in (A, B):
        for c in T['champs']:
            route = ' → '.join(f"{gl(g)}(r{a}-{b})" for g, a, b in c['route'])
            if len(route) > 220: route = route[:217] + '…'
            L.append(f"| {nm(T['team'])} | {c['name']}#{c['id']} | {c['submitted']} ({c['n_submits']}) | {c['collected']} | {c['gems_per_100']:.1f} | {c['captures']} | {c['kills']}/{c['deaths']}/{c['assists']} | {c['dead_rounds']} | {c['channel_rounds']} | {c['idle_frac']:.0%} | {c['act_idle_frac']:.0%} | {c['dist']} | {c['max_bag']} | {c['recalls']} | {c['fuel_out']}/{c['errors']} | {route} |")
    L.append('')
    return '\n'.join(L), A, B


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('dumps', nargs='+'); ap.add_argument('-o', '--out'); ap.add_argument('--names', default='')
    ap.add_argument('--map'); ap.add_argument('--maps-md', default=os.path.join(ROOT, 'docs', 'MAPS.md')); ap.add_argument('--checkpoints', default='100,200,300,400,500')
    ap.add_argument('--title', default='Replay analysis')
    a = ap.parse_args(argv)
    names = {}
    for part in a.names.split(','):
        if '=' in part: k, v = part.split('=', 1); names[{'A': 0, 'B': 1}[k.strip().upper()]] = v.strip()
    checkpoints = [int(x) for x in a.checkpoints.split(',') if x]
    maps = parse_maps_md(a.maps_md)
    out = [f'# {a.title}', '']
    agg = []
    for path in a.dumps:
        d = load_dump(path)
        mapname, mapinfo = identify_map(d['meta'], maps, a.map)
        glabels = group_labels(d['meta'], mapinfo)
        text, A, B = report(d, names, mapname, mapinfo, glabels, checkpoints)
        out.append(text); agg.append((d['meta']['file'], mapname, d['final'], A, B))
    if len(agg) > 1:
        nm = lambda t: names.get(t, 'Team ' + 'AB'[t])
        head = ['## Across all games', '', f'| game | map | winner | round | gems A/B | first tribute A/B | captures A/B | avg party A/B | kills A/B | idle A/B |', '|---|---|---|---|---|---|---|---|---|---|']
        for f, m, fin, A, B in agg:
            head.append(f"| {f} | {m or '?'} | {nm(0) if fin['winner']=='teamA' else nm(1) if fin['winner']=='teamB' else fin['winner']} | {fin['finish_round']} | {fin['submitted'][0]}/{fin['submitted'][1]} | {fmt(A['first_tribute'])}/{fmt(B['first_tribute'])} | {A['n_captures']}/{B['n_captures']} | {A['avg_party']:.2f}/{B['avg_party']:.2f} | {len(A['kills'])}/{len(B['kills'])} | {A['act_idle_frac']:.0%}/{B['act_idle_frac']:.0%} |")
        out.insert(2, '\n'.join(head) + '\n')
    text = '\n'.join(out) + '\n'
    if a.out: open(a.out, 'w').write(text); print(f'wrote {a.out}', file=sys.stderr)
    else: sys.stdout.write(text)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
