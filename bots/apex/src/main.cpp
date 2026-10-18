// PROBE build: measures engine call fuel costs and prints map facts. Replaced by the real bot.
#include <cambc.h>
#include <cstdio>
#include <variant>
#include <string>
#include <cstring>
#include <vector>
#include <cstdlib>
using namespace cambc;

static std::uint64_t F() { return fuel_remaining(); }
#define MEASURE(label, expr) do { auto f0 = F(); expr; auto f1 = F(); std::printf("fuel %-32s %llu\n", label, (unsigned long long)(f0 - f1)); } while (0)

static void dump_groups(const char* tag) {
    auto groups = map::camp_groups();
    std::printf("%s groups=%zu\n", tag, groups.size());
    for (std::size_t i = 0; i < groups.size(); ++i) {
        const auto& g = groups[i];
        std::printf("%s G%zu h=%u gold=%u gems=%u resp=%d n=%zu:", tag, i, (unsigned)static_cast<handle_camp_group_t>(g), g.gold_value(), g.gem_value(), g.max_respawn_timer(), g.camps().size());
        for (const auto& c : g) std::printf(" %d@(%d,%d)", (int)c.kind(), c.spawn_position().x, c.spawn_position().y);
        std::printf("\n");
    }
}

static void probe_draft() {
    static bool done = false;
    if (!done) {
        done = true;
        std::printf("fuel at draft start %llu\n", (unsigned long long)F());
        std::printf("map %dx%d sym=%d team_size=%u\n", map::width(), map::height(), (int)map::symmetry(), draft::team_size());
        std::printf("consts capture=%d sleep=%d respawn=%d recall=%d submit_range=%d max_rounds=%u win=%u ward_cd=%d ward_r=%.1f lockhits=%u tp=%d shared=%u draft=%u\n",
            constants::capture_time, constants::camp_sleep_time, constants::respawn_time, constants::recall_time, constants::submit_gems_range,
            constants::max_rounds, constants::winning_gems, constants::ward_cooldown, constants::ward_vision_radius, constants::altar_lock_hits, constants::tp_crystal_time,
            constants::shared_memory_bytes, constants::draft_memory_bytes);
        auto sp = map::spawn_region(team::ally).positions();
        std::printf("ally spawn n=%zu first=(%d,%d) last=(%d,%d)\n", sp.size(), sp.front().x, sp.front().y, sp.back().x, sp.back().y);
        auto esp = map::spawn_region(team::enemy).positions();
        std::printf("enemy spawn n=%zu first=(%d,%d)\n", esp.size(), esp.front().x, esp.front().y);
        for (auto a : map::altars().positions()) std::printf("altar (%d,%d)\n", a.x, a.y);
        std::printf("walls=%u holes=%u bushes=%u\n", map::walls().count(), map::holes().count(), map::bushes().count());
        MEASURE("camp_groups()", (void)map::camp_groups());
        dump_groups("DRAFT");
        for (int i = 0; i < 12; ++i) {
            const auto& d = champion_designs[i];
            auto s = d.base_stats();
            std::printf("champ %d hp=%.0f str=%.0f foc=%.0f range=%d speed=%d vis=%d campmul=%.2f growth h=%.2f s=%.2f f=%.2f", i, s.base_health, s.base_strength, s.base_focus, s.base_attack_range, s.base_move_speed, s.vision_radius, s.camp_damage_multiplier, s.health_growth, s.strength_growth, s.focus_growth);
            for (auto& ab : d.abilities()) {
                if (!ab.info()) { std::printf(" [none]"); continue; }
                std::visit([&](const auto& v) {
                    std::printf(" [k=%d cd=%d tg=%u exw=%d", (int)v.kind, v.max_cooldown, v.valid_targets.bits(), (int)v.exclusive_with_movement);
                    if constexpr (requires { v.range; }) std::printf(" r=%.1f", v.range);
                    if constexpr (requires { v.modifier; }) std::printf(" mod=%.2f", v.modifier);
                    if constexpr (requires { v.radius; }) std::printf(" rad=%.1f", v.radius);
                    if constexpr (requires { v.duration; }) std::printf(" dur=%d", v.duration);
                    std::printf("]");
                }, *ab.info());
            }
            std::printf("\n");
        }
        for (int i = 0; i < 5; ++i) {
            camp_stats s{};
            detail::camp_design d{};
            detail::check(detail::sdk_camp_design(i, &d));
            s = d.base_stats;
            std::printf("camp %d hp=%.0f str=%.0f range=%d vis=%.0f\n", i, s.base_health, s.base_strength, s.base_attack_range, s.vision_radius);
        }
        // Fuel measurements
        bitboard walls = map::walls() | map::holes();
        bitboard roots; roots.set(sp.front());
        bitboard visited; distance_field field;
        MEASURE("field.compute full", field.compute(walls, roots, visited));
        bitboard dest; dest.set(esp.front());
        MEASURE("field.compute_to far", field.compute_to(walls, roots, dest, visited));
        MEASURE("field.get", (void)field.get(esp.front()));
        MEASURE("bitboard ctor", bitboard b);
        MEASURE("bitboard or", (void)(walls | roots));
        MEASURE("bitboard is_set", (void)walls.is_set(sp.front()));
        MEASURE("bitboard count", (void)walls.count());
        MEASURE("bitboard positions walls", (void)walls.positions());
        MEASURE("reachable_tiles d1", (void)walls.reachable_tiles(sp.front(), 1));
        MEASURE("draw_chebyshev r8", roots.draw_chebyshev(sp.front(), 8));
        MEASURE("fuel_remaining", (void)F());
        MEASURE("field copy", distance_field c = field);
        std::vector<std::uint8_t> mem(1024, 7);
        MEASURE("draft_memory write", draft_memory = mem);
        std::printf("fuel at draft end %llu\n", (unsigned long long)F());
    }
    auto chosen = draft::current_draft(team::ally);
    auto enemy = draft::current_draft(team::enemy);
    std::printf("draft round ally=%zu enemy=%zu\n", chosen.size(), enemy.size());
    champion_draft order[] = {champion_draft::cassi_and_issac, champion_draft::stellar, champion_draft::ren, champion_draft::varg, champion_draft::ryth};
    draft::pick(order[chosen.size() % 5]);
}

static void probe_play() {
    static int turns = 0;
    static unsigned prev_round = 0;
    auto& me = self();
    unsigned r = round_number();
    if (turns == 0) {
        std::printf("fuel at first play turn %llu champ=%d pos=(%d,%d) allies=%zu\n", (unsigned long long)F(), (int)me.champion_name(), me.position().x, me.position().y, champions(team::ally).size());
        auto all = champions(team::ally);
        for (std::size_t i = 0; i < all.size(); ++i) std::printf("ally %zu handle=%u type=%d\n", i, (unsigned)static_cast<handle_champion_t>(all[i]), (int)all[i].champion_name());
        std::printf("draft mem[0]=%d size=%zu\n", (int)draft_memory[0], draft_memory.size());
        if (me.champion_name() == champion_type::stellar || all.size() == 1) dump_groups("PLAY");
        MEASURE("camp_groups() play", (void)map::camp_groups());
    } else if (turns == 1) {
        // Default buffering: print 60 lines of ~40 bytes, record each line's cost.
        static std::uint32_t costs[60];
        std::uint64_t t0 = F();
        for (int i = 0; i < 60; ++i) {
            auto f0 = F();
            std::printf("L%02d 0123456789012345678901234567890123\n", i);
            auto f1 = F();
            costs[i] = (std::uint32_t)(f0 - f1);
        }
        std::uint64_t t1 = F();
        std::printf("lines total=%llu :", (unsigned long long)(t0 - t1));
        for (int i = 0; i < 60; ++i) std::printf(" %u", costs[i]);
        std::printf("\n");
        static char buf[8192];
        MEASURE("memset 4000", std::memset(buf, 1, 4000));
        MEASURE("memcpy 4000", std::memcpy(buf + 4000, buf, 4000));
        MEASURE("string(3999)", std::string s(3999, 'c'));
        MEASURE("vector<int>(1000)", std::vector<int> v(1000));
        MEASURE("loop 1000 adds", { volatile int x = 0; for (int i = 0; i < 1000; ++i) x += i; });
        MEASURE("fflush", std::fflush(stdout));
        std::printf("fuel after output test %llu\n", (unsigned long long)F());
    } else if (turns == 2) {
        setvbuf(stdout, nullptr, _IOFBF, 1 << 16);
        static std::uint32_t costs[60];
        std::uint64_t t0 = F();
        for (int i = 0; i < 60; ++i) {
            auto f0 = F();
            std::printf("M%02d 0123456789012345678901234567890123\n", i);
            auto f1 = F();
            costs[i] = (std::uint32_t)(f0 - f1);
        }
        std::uint64_t t1 = F();
        std::printf("lines64k total=%llu :", (unsigned long long)(t0 - t1));
        for (int i = 0; i < 60; ++i) std::printf(" %u", costs[i]);
        std::printf("\n");
        MEASURE("fflush 64k buffer", std::fflush(stdout));
        std::printf("fuel after output test2 %llu\n", (unsigned long long)F());
    }
    if (turns > 1 && prev_round + 1 != r) std::printf("ROUND JUMP %u -> %u\n", prev_round, r);
    prev_round = r;
    ++turns;
    if (r % 100 == 0) std::printf("T r=%u fuel=%llu\n", r, (unsigned long long)F());
}

int main() {
    while (true) {
        try {
            if (is_draft()) probe_draft(); else probe_play();
        } catch (const std::exception& e) { std::printf("bot error: %s\n", e.what()); }
        catch (...) { std::printf("bot error: unknown\n"); }
        end_turn();
    }
}
