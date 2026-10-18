#define SPAR_MAPDUMP 1
// Meta draft: Cassi & Issac, Stellar, Ren, Varg, Ryth (prefix by team size), then fallbacks.
#include <cambc.h>
#include "draft.h"
#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

#ifdef SPAR_MAPDUMP
#include <cstdio>
static void dump_map() {
    static bool done = false;
    if (done) return;
    done = true;
    const auto w = cambc::map::width(), h = cambc::map::height();
    const auto& walls = cambc::map::walls(); const auto& holes = cambc::map::holes();
    const auto& bushes = cambc::map::bushes(); const auto& altars = cambc::map::altars();
    const auto& sa = cambc::map::spawn_region(cambc::team::ally); const auto& se = cambc::map::spawn_region(cambc::team::enemy);
    std::vector<std::string> rows(h, std::string(w, '.'));
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        cambc::vec2 p{x, y}; char c = '.';
        if (bushes.is_set(p)) c = '"'; if (holes.is_set(p)) c = 'o'; if (walls.is_set(p)) c = '#';
        if (altars.is_set(p)) c = 'A'; if (sa.is_set(p)) c = 'S'; if (se.is_set(p)) c = 'E';
        rows[y][x] = c;
    }
    int gi = 0;
    for (const auto& g : cambc::map::camp_groups()) {
        for (const auto& c : g.camps()) {
            const auto p = c.spawn_position();
            const char k[] = {'w', 'e', 'g', 'G', 'M'};
            rows[p.y][p.x] = k[static_cast<int>(c.kind())];
        }
        std::printf("group %d gems=%u gold=%u n=%d at (%d,%d)\n", gi++, g.gem_value(), g.gold_value(), (int)g.camps().size(), g.camps()[0].spawn_position().x, g.camps()[0].spawn_position().y);
    }
    std::printf("MAP %dx%d symmetry=%d\n", w, h, (int)cambc::map::symmetry());
    for (int y = h - 1; y >= 0; --y) std::printf("%3d %s\n", y, rows[y].c_str());
}
#endif

void play_draft() {
    using cambc::champion_draft;
#ifdef SPAR_MAPDUMP
    dump_map();
#endif
    static constexpr std::array order{
        champion_draft::cassi_and_issac, champion_draft::stellar, champion_draft::ren,
        champion_draft::varg, champion_draft::ryth, champion_draft::tavi,
        champion_draft::marcella, champion_draft::vesper_vale, champion_draft::willow,
        champion_draft::prism, champion_draft::vendetta};
    const auto ours = cambc::draft::current_draft(cambc::team::ally);
    cambc::draft_memory = std::string_view("SPAR1");
    for (auto pick : order) {
        if (std::find(ours.begin(), ours.end(), pick) == ours.end()) {
            cambc::draft::pick(pick);
            return;
        }
    }
}
