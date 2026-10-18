// Draft: fixed roster table + a static economy plan written into draft memory.
#include "draft.h"
#include "plan.h"
#include "model.h"

namespace apex {
namespace {

using cambc::champion_draft;

// Roster preference (current top-50 meta). Change per map profile here only.
constexpr std::array<champion_draft, 11> ROSTER{
    champion_draft::cassi_and_issac, champion_draft::stellar, champion_draft::ren, champion_draft::varg,
    champion_draft::ryth, champion_draft::marcella, champion_draft::tavi, champion_draft::vendetta,
    champion_draft::vesper_vale, champion_draft::willow, champion_draft::prism,
};

// Bodies produced by a pick, in engine handle order (Cassi before Issac, verified).
void bodies_of(champion_draft pick, std::vector<champion_type>& out) {
    switch (pick) {
    case champion_draft::cassi_and_issac: out.push_back(champion_type::cassi); out.push_back(champion_type::issac); return;
    case champion_draft::ryth: out.push_back(champion_type::ryth); return;
    case champion_draft::vesper_vale: out.push_back(champion_type::vesper_vale); return;
    case champion_draft::vendetta: out.push_back(champion_type::vendetta); return;
    case champion_draft::stellar: out.push_back(champion_type::stellar); return;
    case champion_draft::prism: out.push_back(champion_type::prism); return;
    case champion_draft::marcella: out.push_back(champion_type::marcella); return;
    case champion_draft::willow: out.push_back(champion_type::willow); return;
    case champion_draft::ren: out.push_back(champion_type::ren); return;
    case champion_draft::varg: out.push_back(champion_type::varg); return;
    case champion_draft::tavi: out.push_back(champion_type::tavi); return;
    }
}

struct GroupInfo {
    vec2 center{};
    float value{}, deliver{};
    float hp{}, danger{}, max_danger{};
    int gems{}, gold{}, resp{};
    int dist_spawn{255}, dist_altar{255}, dist_enemy{255};
    int altar{};
    bool excluded{}, home{};
    bool has_monolith{};
};

struct Squad {
    std::vector<int> bodies;
    float dps{}, poke{}, tank_hp{}, stun8{}, shield8{};
    bool melee{};
    std::vector<int> circuit;
    float rate{};
};

Squad make_squad(const std::vector<Body>& bodies, std::vector<int> idx) {
    Squad s; s.bodies = std::move(idx);
    for (int i : s.bodies) {
        const Body& b = bodies[i];
        s.dps += b.dps();
        s.poke += b.poke;
        s.stun8 += b.stun8;
        if (!b.ranged) { s.melee = true; s.tank_hp += b.hp; s.shield8 += b.shield8; }
    }
    s.stun8 = std::min(s.stun8, 4.0f);
    return s;
}

// Rounds to clear and whether a squad can take a group; returns rounds (<0 if infeasible).
// Returns the cycle cost of clearing (rounds + amortised healing), or -1 if infeasible.
float clear_rounds(const Squad& s, const GroupInfo& g) {
    if (g.excluded) return -1;
    const Fight f = estimate_fight(g.hp, g.danger, g.max_danger, s.dps, s.poke, s.melee, 0.7f * s.tank_hp, s.stun8, s.shield8);
    if (!f.feasible) return -1;
    if (f.poke_only) return 2.0f * f.rounds; // slow and it idles the melee bodies
    const float heal = s.tank_hp > 0 ? f.tank_damage / s.tank_hp * (5 + g.dist_spawn) : 0;
    return f.rounds + heal;
}

float gold_weight = 0.30f;

float group_value(const GroupInfo& g) {
    float v = g.gems + gold_weight * g.gold;
    if (g.home) v *= 1.3f;
    return v;
}

void build_circuits(std::vector<Squad>& squads, const std::vector<GroupInfo>& groups) {
    // Per-squad clear rounds are fixed per group: compute once (G x S), then the greedy tour is cheap.
    std::vector<std::vector<float>> rounds(squads.size(), std::vector<float>(groups.size(), -1.0f));
    for (std::size_t si = 0; si < squads.size(); ++si)
        for (std::size_t gi = 0; gi < groups.size(); ++gi) rounds[si][gi] = clear_rounds(squads[si], groups[gi]);
    std::vector<bool> taken(groups.size(), false);
    std::vector<vec2> pos(squads.size());
    std::vector<bool> first(squads.size(), true);
    std::vector<float> cycle(squads.size(), 0), value(squads.size(), 0), longest(squads.size(), 0), gems(squads.size(), 0);
    bool progress = true;
    while (progress && fuel() > 200000) {
        progress = false;
        for (std::size_t si = 0; si < squads.size(); ++si) {
            Squad& s = squads[si];
            if (s.circuit.size() >= MAX_CIRCUIT) continue;
            int best = -1; float best_score = 0, best_cycle = 0, best_rounds = 0;
            const int px = pos[si].x, py = pos[si].y;
            for (std::size_t gi = 0; gi < groups.size(); ++gi) {
                if (taken[gi]) continue;
                const float r = rounds[si][gi];
                if (r < 0) continue;
                const GroupInfo& g = groups[gi];
                const float travel = first[si] ? g.dist_spawn : std::max(std::abs(px - g.center.x), std::abs(py - g.center.y)) * 1.2f;
                const float cyc = travel + r + 6 + g.deliver;
                const float score = g.value / cyc;
                if (score > best_score) { best_score = score; best = static_cast<int>(gi); best_cycle = cyc; best_rounds = r; }
            }
            if (best < 0) continue;
            taken[best] = true; s.circuit.push_back(best); pos[si] = groups[best].center; first[si] = false;
            cycle[si] += best_cycle; value[si] += groups[best].value; gems[si] += groups[best].gems + 0.1f * groups[best].gold;
            longest[si] = std::max(longest[si], groups[best].resp + best_rounds + 6);
            progress = true;
        }
    }
    for (std::size_t si = 0; si < squads.size(); ++si)
        squads[si].rate = gems[si] / std::max(cycle[si], std::max(longest[si], 1.0f));
}

// Static plan computed once in the first draft turn.
Plan g_plan{};
bool g_plan_ready = false;
std::vector<champion_draft> g_picks;

std::uint8_t cap255(int v) { return static_cast<std::uint8_t>(std::clamp(v, 0, 255)); }

void compute_plan() {
    const auto groups_raw = cambc::map::camp_groups();
    const auto altars = cambc::map::altars().positions();
    const auto spawn = cambc::map::spawn_region(team::ally);
    const auto espawn = cambc::map::spawn_region(team::enemy);
    const int n_groups = static_cast<int>(std::min<std::size_t>(groups_raw.size(), MAX_GROUPS));
    const bitboard walls = cambc::map::walls() | cambc::map::holes() | cambc::map::altars() | espawn;
    const bitboard walls_home = cambc::map::walls() | cambc::map::holes() | cambc::map::altars() | spawn;

    bitboard visited;
    distance_field f_spawn, f_enemy, f_altar;
    f_spawn.compute(walls, spawn, visited);
    f_enemy.compute(walls_home, espawn, visited);
    bitboard altar_goals;
    for (auto a : altars) altar_goals.draw_chebyshev(a, 1);
    altar_goals &= ~walls;
    f_altar.compute(walls, altar_goals, visited);
    // Per-altar fields to pick the nearest altar index for each group (altars <= 18 on lanes).
    std::vector<distance_field> f_each(altars.size());
    for (std::size_t a = 0; a < altars.size(); ++a) {
        bitboard r; r.draw_chebyshev(altars[a], 1); r &= ~walls;
        f_each[a].compute(walls, r, visited);
    }

    std::vector<GroupInfo> groups(n_groups);
    for (int gi = 0; gi < n_groups; ++gi) {
        const auto& g = groups_raw[gi];
        GroupInfo& info = groups[gi];
        int sx = 0, sy = 0, n = 0;
        int ds = 65535, de = 65535, da = 65535;
        for (const auto& c : g) {
            const vec2 p = c.spawn_position();
            sx += p.x; sy += p.y; ++n;
            info.hp += camp_hp(c.kind()); info.danger += camp_dmg(c.kind()); info.max_danger = std::max(info.max_danger, camp_dmg(c.kind()));
            if (c.kind() == cambc::camp_type::monolith) info.has_monolith = true;
            ds = std::min<int>(ds, f_spawn.get(p)); de = std::min<int>(de, f_enemy.get(p)); da = std::min<int>(da, f_altar.get(p));
        }
        info.center = { sx / std::max(n, 1), sy / std::max(n, 1) };
        info.gems = static_cast<int>(g.gem_value()); info.gold = static_cast<int>(g.gold_value()); info.resp = g.max_respawn_timer();
        info.dist_spawn = ds; info.dist_enemy = de; info.dist_altar = da;
        info.home = ds <= de;
        info.excluded = ds >= 60000 || da >= 60000;
        int best_a = 0, best_d = 65535;
        for (std::size_t a = 0; a < altars.size(); ++a) {
            int d = 65535;
            for (const auto& c : g) d = std::min<int>(d, f_each[a].get(c.spawn_position()));
            if (d < best_d) { best_d = d; best_a = static_cast<int>(a); }
        }
        info.altar = best_a;
        info.value = group_value(info);
        info.deliver = 0.5f * (info.dist_altar + info.gems / 10.0f + 2);
    }

    // Monoliths are a last resort: keep them only when the reachable ordinary gems are scarce.
    int ordinary_gems = 0;
    for (const auto& g : groups) if (!g.excluded && !g.has_monolith) ordinary_gems += g.gems;
    if (ordinary_gems >= 300) for (auto& g : groups) if (g.has_monolith) g.excluded = true;

    // Bodies in handle order.
    std::vector<champion_type> types;
    for (auto p : g_picks) bodies_of(p, types);
    std::vector<Body> bodies;
    for (auto t : types) bodies.push_back(model_body(t));
    const int nb = static_cast<int>(bodies.size());

    // Candidate partitions: k squads, melee-first round robin, Cassi joins Issac.
    std::vector<int> order(nb);
    for (int i = 0; i < nb; ++i) order[i] = i;
    auto tank_key = [&](int i) { const Body& b = bodies[i]; return (b.ranged ? 0 : 1000) + (b.role == ROLE_TANK ? 500 : 0) + static_cast<int>(b.hp); };
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return tank_key(a) > tank_key(b); });
    int n_melee = 0; for (const auto& b : bodies) if (!b.ranged) ++n_melee;

    std::vector<Squad> best_squads; float best_total = -1;
    // Gem-scarce maps (monolith-only / one giant group) need every poker on the same target: count the
    // gems the whole team could take by fronting; below 300 everyone stays together.
    int frontable_gems = 0;
    {
        std::vector<int> all(nb); for (int i = 0; i < nb; ++i) all[i] = i;
        const Squad team = make_squad(bodies, all);
        for (const auto& g : groups) {
            if (g.excluded) continue;
            const Fight f = estimate_fight(g.hp, g.danger, g.max_danger, team.dps, team.poke, team.melee, 0.7f * team.tank_hp, team.stun8, team.shield8);
            if (f.feasible && !f.poke_only) frontable_gems += g.gems;
        }
    }
    const int max_k = frontable_gems < 300 ? 1 : std::max(1, std::min(3, n_melee));
    for (int k = 1; k <= max_k; ++k) {
        if (fuel() < 250000) break;
        std::vector<std::vector<int>> parts(k);
        int cassi = -1, issac = -1;
        for (int i = 0; i < nb; ++i) { if (bodies[i].type == champion_type::cassi) cassi = i; if (bodies[i].type == champion_type::issac) issac = i; }
        int slot = 0;
        for (int i : order) { if (i == cassi && issac >= 0) continue; parts[slot % k].push_back(i); ++slot; }
        if (cassi >= 0 && issac >= 0) {
            for (auto& p : parts) if (std::find(p.begin(), p.end(), issac) != p.end()) { p.push_back(cassi); break; }
        }
        std::vector<Squad> squads;
        for (auto& p : parts) if (!p.empty()) squads.push_back(make_squad(bodies, p));
        build_circuits(squads, groups);
        float total = 0; for (const auto& s : squads) total += s.rate;
        std::printf("draft: k=%d team_rate=%.3f", k, total);
        for (const auto& s : squads) std::printf(" [n=%zu dps=%.0f circ=%zu rate=%.3f]", s.bodies.size(), s.dps, s.circuit.size(), s.rate);
        std::printf("\n");
        if (total > best_total) { best_total = total; best_squads = squads; }
    }

    // Encode.
    std::memset(&g_plan, 0, sizeof(g_plan));
    g_plan.version = PLAN_VERSION; g_plan.bodies = static_cast<std::uint8_t>(nb); g_plan.groups = static_cast<std::uint8_t>(n_groups);
    g_plan.squads = static_cast<std::uint8_t>(best_squads.size());
    g_plan.gold_weight_pct = 30; g_plan.bank_threshold = 36;
    for (int gi = 0; gi < n_groups; ++gi) {
        const GroupInfo& g = groups[gi];
        g_plan.g[gi] = { static_cast<std::uint8_t>(g.altar), cap255(g.dist_spawn), cap255(g.dist_altar),
            static_cast<std::uint8_t>((g.excluded ? GF_EXCLUDED : 0) | (g.home ? GF_HOME : 0)) };
    }
    for (int i = 0; i < nb; ++i) { g_plan.s[i].type = static_cast<std::uint8_t>(bodies[i].type); g_plan.s[i].role = bodies[i].role; g_plan.s[i].rank = bodies[i].role; }
    for (std::size_t si = 0; si < best_squads.size(); ++si) {
        const Squad& s = best_squads[si];
        for (int bi : s.bodies) {
            SlotPlan& sp = g_plan.s[bi];
            sp.squad = static_cast<std::uint8_t>(si);
            sp.n = static_cast<std::uint8_t>(std::min<std::size_t>(s.circuit.size(), MAX_CIRCUIT));
            for (int c = 0; c < sp.n; ++c) sp.circuit[c] = static_cast<std::uint8_t>(s.circuit[c]);
        }
    }
    cambc::draft_memory = encode_plan(g_plan);
    g_plan_ready = true;
    std::printf("draft: plan ready bodies=%d groups=%d squads=%zu fuel=%llu\n", nb, n_groups, best_squads.size(), (unsigned long long)fuel());
    for (std::size_t si = 0; si < best_squads.size(); ++si) {
        std::printf("draft: squad %zu:", si);
        for (int bi : best_squads[si].bodies) std::printf(" %d", static_cast<int>(bodies[bi].type));
        std::printf(" circuit:");
        for (int c : best_squads[si].circuit) std::printf(" %d", c);
        std::printf("\n");
    }
}

} // namespace

void play_draft() {
    const auto chosen = cambc::draft::current_draft(team::ally);
    const std::uint32_t size = cambc::draft::team_size();
    // Pick first (a missed pick is randomised by the engine), then plan with the remaining fuel.
    champion_draft pick = ROSTER[0];
    for (auto c : ROSTER) {
        if (std::find(chosen.begin(), chosen.end(), c) == chosen.end()) { pick = c; break; }
    }
    cambc::draft::pick(pick);
    if (g_picks.empty()) {
        for (auto c : ROSTER) { if (g_picks.size() < size) g_picks.push_back(c); }
        if (fuel() > 400000) {
            try { compute_plan(); } catch (const std::exception& e) { std::printf("draft: plan failed: %s\n", e.what()); }
        } else std::printf("draft: not enough fuel for a plan (%llu)\n", (unsigned long long)fuel());
    }
}

} // namespace apex
