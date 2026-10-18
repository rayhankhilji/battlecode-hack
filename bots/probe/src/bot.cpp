// Play turns: measure the fuel cost of SDK calls on the first rounds, then farm like the starter.
// Every measurement line is "F <round> <name> <fuel>" where fuel = fuel_remaining() before minus after,
// which includes the two fuel_remaining() calls themselves (see the "noop" row). Printing uses out.h.
#include <cambc.h>
#include "bot.h"
#include "out.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <span>
#include <string>
#include <variant>
#include <vector>

using namespace cambc;

namespace {

volatile std::uint64_t sink;
volatile unsigned vol_1000 = 1000;
volatile unsigned vol_100 = 100;
volatile unsigned vol_30 = 30;

struct Row { const char* name; long long fuel; };
Row rows[96];
int nrows = 0;
bool skipped = false;

void add(const char* name, long long fuel) {
    if (nrows < 96) rows[nrows++] = { name, fuel };
}

template <class F> void measure(const char* name, F&& f) {
    if (fuel_remaining() < 30000) { skipped = true; return; }
    const auto before = fuel_remaining();
    f();
    const auto after = fuel_remaining();
    add(name, static_cast<long long>(before) - static_cast<long long>(after));
}

void flush_rows(unsigned round) {
    for (int i = 0; i < nrows; ++i) { out::s("F "); out::i(round); out::s(" "); out::s(rows[i].name); out::s(" "); out::i(rows[i].fuel); out::nl(); }
    if (skipped) { out::s("F "); out::i(round); out::s(" SKIPPED_rest_low_fuel 0\n"); }
    nrows = 0;
    skipped = false;
    out::flush();
}

vec2 clamp_tile(vec2 p) {
    return { std::clamp(p.x, 0, map::width() - 1), std::clamp(p.y, 0, map::height() - 1) };
}

void measure_bitboards() {
    auto& me = self();
    const vec2 origin = me.position();
    const vec2 far = clamp_tile({ map::width() / 2, map::height() / 2 });
    measure("noop", [] {});
    measure("noop", [] {});
    bitboard a;
    measure("bitboard.default_ctor", [] { bitboard b; sink = reinterpret_cast<std::uintptr_t>(b.ptr()); });
    measure("bitboard.uninit_ctor", [] { bitboard b{ uninitialized }; sink = reinterpret_cast<std::uintptr_t>(b.ptr()); });
    measure("bitboard.copy_ctor", [&] { bitboard b{ map::walls() }; sink = reinterpret_cast<std::uintptr_t>(b.ptr()); });
    measure("bitboard.clear", [&] { a.clear(); });
    measure("bitboard.fill", [&] { a.fill(); });
    measure("bitboard.set", [&] { a.set(origin); });
    measure("bitboard.set_false", [&] { a.set(origin, false); });
    measure("bitboard.is_set", [&] { sink = a.is_set(origin); });
    measure("bitboard.is_set_x10", [&] { for (int i = 0; i < 10; ++i) sink = a.is_set(origin); });
    measure("bitboard.count", [&] { sink = a.count(); });
    measure("bitboard.is_empty", [&] { sink = a.is_empty(); });
    measure("bitboard.positions(full)", [&] { sink = a.positions().size(); });
    a.clear();
    a.draw_chebyshev(origin, 8);
    measure("bitboard.positions(cheb8)", [&] { sink = a.positions().size(); });
    measure("bitboard.positions_into(cheb8,vec400)", [&] { std::array<vec2, 400> buf; sink = a.positions_into(buf); });
    measure("bitboard.draw_chebyshev8", [&] { bitboard b; b.draw_chebyshev(origin, 8); });
    measure("bitboard.draw_chebyshev1", [&] { bitboard b; b.draw_chebyshev(origin, 1); });
    measure("bitboard.draw_circle8", [&] { bitboard b; b.draw_circle(origin, 8); });
    measure("bitboard.or", [&] { bitboard b = map::walls() | map::holes(); sink = reinterpret_cast<std::uintptr_t>(b.ptr()); });
    measure("bitboard.or_assign", [&] { a |= map::walls(); });
    measure("bitboard.and", [&] { bitboard b = map::walls() & map::holes(); sink = reinterpret_cast<std::uintptr_t>(b.ptr()); });
    measure("bitboard.and_assign", [&] { a &= map::walls(); });
    measure("bitboard.not", [&] { bitboard b = ~map::walls(); sink = reinterpret_cast<std::uintptr_t>(b.ptr()); });
    measure("bitboard.xor", [&] { bitboard b = map::walls() ^ map::holes(); sink = reinterpret_cast<std::uintptr_t>(b.ptr()); });
    measure("bitboard.equals", [&] { sink = (map::walls() == map::holes()); });
    measure("bitboard.shift_right1", [&] { bitboard b = map::walls().shift_right(1); sink = reinterpret_cast<std::uintptr_t>(b.ptr()); });
    measure("bitboard.reflect", [&] { bitboard b = map::walls().reflect(map::symmetry()); sink = reinterpret_cast<std::uintptr_t>(b.ptr()); });
    measure("bitboard.export_linear", [&] { sink = map::walls().export_linear_buffer().size(); });
    const bitboard blocked = map::walls() | map::holes() | map::altars();
    measure("bitboard.reachable_tiles(1)", [&] { bitboard b = blocked.reachable_tiles(origin, 1); sink = reinterpret_cast<std::uintptr_t>(b.ptr()); });
    measure("bitboard.reachable_tiles(2)", [&] { bitboard b = blocked.reachable_tiles(origin, 2); sink = reinterpret_cast<std::uintptr_t>(b.ptr()); });
    measure("bitboard.reachable_tiles(8)", [&] { bitboard b = blocked.reachable_tiles(origin, 8); sink = reinterpret_cast<std::uintptr_t>(b.ptr()); });
    measure("bitboard.reachable_tiles(40)", [&] { bitboard b = blocked.reachable_tiles(origin, 40); sink = reinterpret_cast<std::uintptr_t>(b.ptr()); });
    measure("bitboard.reachable_tiles(200)", [&] { bitboard b = blocked.reachable_tiles(origin, 200); sink = reinterpret_cast<std::uintptr_t>(b.ptr()); });
    {
        bitboard frontier{ uninitialized }, scratch{ uninitialized }, out_b{ uninitialized };
        measure("bitboard.reachable_tiles_into(8,scratch)", [&] { blocked.reachable_tiles_into(origin, 8, frontier, scratch, out_b); });
    }
    const bitboard opaque = map::walls() | map::bushes();
    measure("bitboard.shadow_cast(8)", [&] { bitboard b = opaque.shadow_cast(origin, 8); sink = reinterpret_cast<std::uintptr_t>(b.ptr()); });
    measure("bitboard.shadow_cast(6)", [&] { bitboard b = opaque.shadow_cast(origin, 6); sink = reinterpret_cast<std::uintptr_t>(b.ptr()); });
    measure("bitboard.shadow_cast(8,centre)", [&] { bitboard b = opaque.shadow_cast(far, 8); sink = reinterpret_cast<std::uintptr_t>(b.ptr()); });
    measure("map.walls()(cached ref)", [&] { sink = reinterpret_cast<std::uintptr_t>(map::walls().ptr()); });
    measure("map.spawn_region(ally)(cached)", [&] { sink = reinterpret_cast<std::uintptr_t>(map::spawn_region(team::ally).ptr()); });
}

void measure_distance_fields() {
    auto& me = self();
    const vec2 origin = me.position();
    const bitboard blocked = map::walls() | map::holes() | map::altars() | map::spawn_region(team::enemy);
    bitboard roots;
    roots.set(origin);
    bitboard visited;
    distance_field field;
    measure("distance_field.default_ctor", [] { distance_field f; sink = reinterpret_cast<std::uintptr_t>(f.ptr()); });
    measure("distance_field.compute(full map,1 root)", [&] { field.compute(blocked, roots, visited); });
    measure("distance_field.compute(max 10)", [&] { distance_field f; f.compute(blocked, roots, visited, 10); });
    measure("distance_field.compute(altars as roots)", [&] { distance_field f; f.compute(blocked, map::altars(), visited); });
}

void measure_distance_fields2() {
    auto& me = self();
    const vec2 origin = me.position();
    const bitboard blocked = map::walls() | map::holes() | map::altars() | map::spawn_region(team::enemy);
    bitboard roots;
    roots.set(origin);
    bitboard visited;
    distance_field field;
    field.compute(blocked, roots, visited);
    bitboard dest;
    dest.set(clamp_tile({ map::width() / 2, map::height() / 2 }));
    measure("distance_field.compute_to(centre)", [&] { distance_field f; f.compute_to(blocked, roots, dest, visited); });
    bitboard near;
    near.set(clamp_tile(origin + vec2{ 3, 3 }));
    measure("distance_field.compute_to(3 away)", [&] { distance_field f; f.compute_to(blocked, roots, near, visited); });
    measure("distance_field.get", [&] { sink = field.get(clamp_tile(origin + vec2{ 5, 5 })); });
    measure("distance_field.get_x10", [&] { for (int i = 0; i < 10; ++i) sink = field.get(clamp_tile(origin + vec2{ i, i })); });
    measure("distance_field.next_move_from", [&] { auto m = field.next_move_from(clamp_tile(origin + vec2{ 5, 5 })); sink = m ? m->x : 99; });
    measure("distance_field.next_moves_from", [&] { sink = field.next_moves_from(clamp_tile(origin + vec2{ 5, 5 })).size(); });
    measure("distance_field.copy", [&] { distance_field f{ field }; sink = reinterpret_cast<std::uintptr_t>(f.ptr()); });
}

void measure_distance_fields3() {
    auto& me = self();
    const vec2 origin = me.position();
    const bitboard blocked = map::walls() | map::holes() | map::altars() | map::spawn_region(team::enemy);
    bitboard roots;
    roots.set(origin);
    bitboard visited;
    distance_field field;
    field.compute(blocked, roots, visited);
    measure("distance_field.export_linear", [&] { sink = field.export_linear_buffer().size(); });
    measure("distance_field.export_linear+scan", [&] { auto v = field.export_linear_buffer(); std::uint32_t s = 0; for (auto d : v) s += d; sink = s; });
}

void measure_distance_fields4() {
    auto& me = self();
    const vec2 origin = me.position();
    const bitboard blocked = map::walls() | map::holes() | map::altars() | map::spawn_region(team::enemy);
    bitboard roots;
    roots.set(origin);
    bitboard visited;
    measure("distance_field.compute_with_expansion_count", [&] { distance_field f; sink = f.compute_with_expansion_count(blocked, roots, visited); });
    measure("distance_field.compute(walls only)", [&] { distance_field f; f.compute(map::walls(), roots, visited); });
}

void measure_vision() {
    measure("vision.visible_tiles", [] { sink = reinterpret_cast<std::uintptr_t>(vision::visible_tiles().ptr()); });
    measure("vision.champion_tiles(ally)", [] { sink = reinterpret_cast<std::uintptr_t>(vision::champion_tiles(team::ally).ptr()); });
    measure("vision.champion_tiles(enemy)", [] { sink = reinterpret_cast<std::uintptr_t>(vision::champion_tiles(team::enemy).ptr()); });
    measure("vision.champion_tiles()", [] { sink = reinterpret_cast<std::uintptr_t>(vision::champion_tiles().ptr()); });
    measure("vision.camp_tiles", [] { sink = reinterpret_cast<std::uintptr_t>(vision::camp_tiles().ptr()); });
    measure("vision.ward_tiles", [] { sink = reinterpret_cast<std::uintptr_t>(vision::ward_tiles().ptr()); });
    measure("vision.teleport_tiles", [] { sink = reinterpret_cast<std::uintptr_t>(vision::teleport_tiles().ptr()); });
    measure("vision.altar_lock_tiles", [] { sink = reinterpret_cast<std::uintptr_t>(vision::altar_lock_tiles().ptr()); });
    measure("vision.champions(ally)", [] { sink = vision::champions(team::ally).size(); });
    measure("vision.champions(enemy)", [] { sink = vision::champions(team::enemy).size(); });
    measure("vision.champions()", [] { sink = vision::champions().size(); });
    measure("vision.camps", [] { sink = vision::camps().size(); });
    measure("vision.wards", [] { sink = vision::wards().size(); });
    measure("vision.altar_locks", [] { sink = vision::altar_locks().size(); });
    measure("vision.teleports", [] { sink = vision::teleports().size(); });
    measure("vision.has_champion", [] { sink = vision::has_champion(self().position()); });
    measure("vision.has_camp", [] { sink = vision::has_camp(self().position()); });
    measure("vision.has_ward", [] { sink = vision::has_ward(self().position()); });
    measure("vision.has_altar_lock", [] { sink = vision::has_altar_lock(map::altars().positions()[0]); });
    measure("vision.champion_at(self)", [] { sink = static_cast<std::uint32_t>(static_cast<handle_champion_t>(vision::champion_at(self().position()))); });
    measure("champions(ally)", [] { sink = cambc::champions(team::ally).size(); });
    measure("champions(enemy)", [] { sink = cambc::champions(team::enemy).size(); });
    measure("champions()", [] { sink = cambc::champions().size(); });
    measure("round_number", [] { sink = round_number(); });
    measure("global_submitted_gems", [] { sink = global_submitted_gems(team::ally); });
    measure("ward.visible_tiles_from", [] { sink = reinterpret_cast<std::uintptr_t>(ward::visible_tiles_from(self().position()).ptr()); });
    measure("self.visible_tiles", [] { sink = reinterpret_cast<std::uintptr_t>(self().visible_tiles().ptr()); });
    measure("self.visible_tiles_from", [] { sink = reinterpret_cast<std::uintptr_t>(self().visible_tiles_from(self().position()).ptr()); });
    measure("map.reflect_horizontal", [] { sink = map::reflect_horizontal(self().position()).x; });
    measure("map.altars().positions", [] { sink = map::altars().positions().size(); });
    measure("map.walls().count", [] { sink = map::walls().count(); });
}

void measure_camps() {
    measure("map.camp_groups", [] { sink = map::camp_groups().size(); });
    const auto groups = map::camp_groups();
    measure("camp_group.spawn_tiles", [&] { sink = reinterpret_cast<std::uintptr_t>(groups[0].spawn_tiles().ptr()); });
    measure("camp_group.copy", [&] { auto g = groups[0]; sink = g.gem_value(); });
    measure("map.camp(pos)", [&] { sink = static_cast<std::uint32_t>(static_cast<handle_camp_t>(map::camp(groups[0].camps()[0].spawn_position()))); });
    measure("camp.camp_group()", [&] { sink = groups[0].camps()[0].camp_group().gem_value(); });
    measure("camp.base_stats", [&] { sink = static_cast<std::uint64_t>(groups[0].camps()[0].base_stats().base_health); });
    measure("camp.attack_range", [&] { sink = groups[0].camps()[0].attack_range(); });
    measure("vision.camp_groups", [] { sink = vision::camp_groups().size(); });
    measure("vision.camps", [] { sink = vision::camps().size(); });
    const auto visible = vision::visible_tiles();
    for (const auto& g : groups) {
        bool seen = false;
        for (const auto& c : g) seen = seen || visible.is_set(c.spawn_position());
        if (!seen) continue;
        measure("camp_group.state(visible)", [&] { sink = g.state().index(); });
        measure("camp.state(visible)", [&] { sink = static_cast<std::uint32_t>(static_cast<camp_state::enum_type>(g.camps()[0].state())); });
        measure("camp.health(visible)", [&] { sink = static_cast<std::uint64_t>(g.camps()[0].health()); });
        measure("camp.position(visible)", [&] { sink = g.camps()[0].position().x; });
        measure("camp.is_visible(visible)", [&] { sink = g.camps()[0].is_visible(); });
        measure("camp.buffs(visible)", [&] { sink = g.camps()[0].buffs().size(); });
        break;
    }
    measure("camp_group.state(hidden,throws)", [&] { try { sink = groups.back().state().index(); } catch (const cambc::error&) { sink = 1; } });
    measure("camp.is_visible(hidden)", [&] { sink = groups.back().camps()[0].is_visible(); });
    measure("camp.health(hidden,throws)", [&] { try { sink = static_cast<std::uint64_t>(groups.back().camps()[0].health()); } catch (const cambc::error&) { sink = 1; } });
}

void measure_champion() {
    auto& me = self();
    measure("self()", [] { sink = reinterpret_cast<std::uintptr_t>(&self()); });
    measure("self.position", [&] { sink = me.position().x; });
    measure("self.health", [&] { sink = static_cast<std::uint64_t>(me.health()); });
    measure("self.max_health", [&] { sink = static_cast<std::uint64_t>(me.max_health()); });
    measure("self.strength", [&] { sink = static_cast<std::uint64_t>(me.strength()); });
    measure("self.focus", [&] { sink = static_cast<std::uint64_t>(me.focus()); });
    measure("self.state", [&] { sink = me.state().bits(); });
    measure("self.can_move", [&] { sink = me.can_move(); });
    measure("self.is_alive", [&] { sink = me.is_alive(); });
    measure("self.is_visible", [&] { sink = me.is_visible(); });
    measure("self.champion_name", [&] { sink = static_cast<int>(me.champion_name()); });
    measure("self.team", [&] { sink = static_cast<int>(me.team()); });
    measure("self.passive", [&] { sink = static_cast<int>(me.passive()); });
    measure("self.base_stats", [&] { sink = static_cast<std::uint64_t>(me.base_stats().base_health); });
    measure("self.attack_range", [&] { sink = me.attack_range(); });
    measure("self.attack_tiles", [&] { sink = reinterpret_cast<std::uintptr_t>(me.attack_tiles().ptr()); });
    measure("self.gem_count", [&] { sink = me.gem_count(); });
    measure("self.gold_count", [&] { sink = me.gold_count(); });
    measure("self.item_count", [&] { sink = me.item_count()[item_type::power_shard_of_strength]; });
    measure("self.buffs", [&] { sink = me.buffs().size(); });
    measure("self.restrictions", [&] { sink = me.restrictions().bits(); });
    measure("self.abilities()", [&] { sink = me.abilities()[0].slot(); });
    const auto abilities = me.abilities();
    measure("ability.cooldown", [&] { sink = abilities[0].cooldown(); });
    measure("ability.is_ready", [&] { sink = abilities[0].is_ready(); });
    measure("ability.info()", [&] { sink = abilities[0].info().has_value(); });
    measure("ability.valid_targets", [&] { sink = abilities[0].valid_targets().bits(); });
    measure("ability.valid_tiles(ranged slot)", [&] { try { sink = abilities[1].valid_tiles().count(); } catch (const std::exception&) { sink = 0; } });
    measure("ability.damage_amount", [&] { try { sink = static_cast<std::uint64_t>(abilities[1].damage_amount()); } catch (const cambc::error&) { sink = 0; } });
    measure("self.ward_cooldown", [&] { sink = me.ward_cooldown(); });
    measure("self.ward_positions", [&] { sink = me.ward_positions().size(); });
    measure("self.ward_placement_tiles", [&] { sink = reinterpret_cast<std::uintptr_t>(me.ward_placement_tiles().ptr()); });
    measure("self.can_buy(item)", [&] { sink = me.can_buy(item_type::power_shard_of_strength); });
    measure("self.can_buy(consumable)", [&] { sink = me.can_buy(consumable_type::altar_lock); });
    measure("self.priority", [&] { sink = me.priority(); });
    measure("consumable.count", [&] { sink = me.consumables()[consumable_type::altar_lock].count(); });
    measure("shared_memory.read8", [] { std::uint64_t v{}; shared_memory.read(0, std::span{ &v, 1 }); sink = v; });
    measure("shared_memory.write8", [] { std::uint64_t v = 42; shared_memory.write(0, std::span{ &v, 1 }); });
    measure("shared_memory[0]", [] { sink = static_cast<std::uint8_t>(shared_memory[0]); });
    measure("draft_memory[0]", [] { sink = static_cast<std::uint8_t>(draft_memory[0]); });
    measure("champion_designs[0].base_stats", [] { sink = static_cast<std::uint64_t>(champion_designs[0].base_stats().base_health); });
    measure("champion_designs[0].abilities()[0].info", [] { sink = champion_designs[0].abilities()[0].info().has_value(); });
    measure("item_type.info", [] { sink = item_type::power_shard_of_strength.info().cost; });
    measure("consumable_type.cost", [] { sink = consumable_type::altar_lock.cost(); });
    measure("fuel_remaining", [] { sink = fuel_remaining(); });
    const auto enemies = cambc::champions(team::enemy);
    if (!enemies.empty()) {
        measure("enemy.champion_name(hidden ok)", [&] { sink = static_cast<int>(enemies[0].champion_name()); });
        measure("enemy.team(hidden ok)", [&] { sink = static_cast<int>(enemies[0].team()); });
        measure("enemy.is_visible", [&] { sink = enemies[0].is_visible(); });
        measure("enemy.position(hidden,throws)", [&] { try { sink = enemies[0].position().x; } catch (const cambc::error&) { sink = 1; } });
        measure("enemy.is_alive(hidden)", [&] { try { sink = enemies[0].is_alive(); } catch (const cambc::error&) { sink = 1; } });
        measure("enemy.abilities()[0].info(hidden)", [&] { try { sink = enemies[0].abilities()[0].info().has_value(); } catch (const cambc::error&) { sink = 1; } });
    }
    const auto allies = cambc::champions(team::ally);
    if (allies.size() > 1) {
        const auto& other = allies[0] == me ? allies[1] : allies[0];
        measure("ally.position", [&] { try { sink = other.position().x; } catch (const cambc::error&) { sink = 1; } });
        measure("ally.gold_count(not self,throws)", [&] { try { sink = other.gold_count(); } catch (const std::exception&) { sink = 1; } });
        measure("ally.gem_count", [&] { try { sink = other.gem_count(); } catch (const cambc::error&) { sink = 1; } });
    }
}

void measure_baselines() {
    measure("loop1000_add", [] { const unsigned n = vol_1000; unsigned acc = 1; for (unsigned i = 0; i < n; ++i) acc = acc * 3 + i; sink = acc; });
    measure("loop1000_add", [] { const unsigned n = vol_1000; unsigned acc = 1; for (unsigned i = 0; i < n; ++i) acc = acc * 3 + i; sink = acc; });
    measure("loop1000_array_write", [] { const unsigned n = vol_1000; std::array<int, 1000> a; for (unsigned i = 0; i < n; ++i) a[i] = static_cast<int>(i * 7); sink = a[999]; });
    measure("loop1000_array_sum", [] { const unsigned n = vol_1000; static int a[1000]; std::uint64_t s = 0; for (unsigned i = 0; i < n; ++i) s += a[i]; sink = s; });
    measure("vector_push100", [] { const unsigned n = vol_100; std::vector<int> v; for (unsigned i = 0; i < n; ++i) v.push_back(i); sink = v.size(); });
    measure("vector_reserve_push100", [] { const unsigned n = vol_100; std::vector<int> v; v.reserve(n); for (unsigned i = 0; i < n; ++i) v.push_back(i); sink = v.size(); });
    measure("new_delete_64B", [] { auto* p = new char[64]; sink = reinterpret_cast<std::uintptr_t>(p); delete[] p; });
    measure("new_delete_8KB", [] { auto* p = new char[8192]; sink = reinterpret_cast<std::uintptr_t>(p); delete[] p; });
}

void measure_baselines2() {
    measure("sort100", [] { const unsigned n = vol_100; std::array<int, 100> a; for (unsigned i = 0; i < n; ++i) a[i] = static_cast<int>((i * 7919) % 101); std::sort(a.begin(), a.begin() + n); sink = a[0]; });
    measure("chebyshev_x100", [] { const unsigned n = vol_100; vec2 a{ 3, 4 }; std::uint64_t s = 0; for (unsigned i = 0; i < n; ++i) s += a.chebyshev_distance({ static_cast<int>(i), static_cast<int>(i / 2) }); sink = s; });
    measure("grid_scan_30x30", [] { const unsigned n = vol_30; std::uint64_t s = 0; for (unsigned y = 0; y < n; ++y) for (unsigned x = 0; x < n; ++x) s += (x * y) & 7; sink = s; });
    measure("grid_scan_50x50", [] { const unsigned n = vol_100 / 2; std::uint64_t s = 0; for (unsigned y = 0; y < n; ++y) for (unsigned x = 0; x < n; ++x) s += (x * y) & 7; sink = s; });
    measure("grid_scan_50x50_u32", [] { const unsigned n = vol_100 / 2; std::uint32_t s = 0; for (unsigned y = 0; y < n; ++y) for (unsigned x = 0; x < n; ++x) s += (x * y) & 7; sink = s; });
}

void measure_baselines4() {
    measure("grid_scan_30x30_is_set", [] { const unsigned n = vol_30; const auto& w = map::walls(); std::uint32_t s = 0; for (unsigned y = 0; y < n; ++y) for (unsigned x = 0; x < n; ++x) s += w.is_set({ static_cast<int>(x % 30), static_cast<int>(y % 30) }); sink = s; });
    measure("grid_scan_30x30_linear_byte", [] { const unsigned n = vol_30; static const auto lin = map::walls().export_linear_buffer(); std::uint32_t s = 0; for (unsigned y = 0; y < n; ++y) for (unsigned x = 0; x < n; ++x) s += lin[(y * x) % lin.size()]; sink = s; });
}

void measure_baselines3() {
    measure("float_mul_x1000", [] { const unsigned n = vol_1000; float f = 1.0001f; for (unsigned i = 0; i < n; ++i) f = f * 1.0001f + 0.5f; sink = static_cast<std::uint64_t>(f); });
    measure("std::string_concat10", [] { std::string s = "abc"; for (int i = 0; i < 10; ++i) s += std::to_string(i); sink = s.size(); });
    measure("to_string(vec2)", [] { sink = to_string(vec2{ 12, 34 }).size(); });
    measure("try_catch_throw(cambc::error)", [] { try { throw cambc::error{ error_code::api_not_found, "x" }; } catch (const cambc::error&) { sink = 1; } });
    measure("try_catch_throw(int)", [] { try { throw 42; } catch (int) { sink = 1; } });
    measure("try_no_throw", [] { try { sink = 2; } catch (int) { sink = 1; } });
}

void measure_printing() {
    measure("out.s+i+nl(cheap)", [] { out::s("x "); out::i(12345); out::nl(); });
    measure("out.f(cheap float)", [] { out::f(1234.5678); out::nl(); });
    measure("out.flush(fwrite)", [] { out::flush(); });
    measure("printf_first_use", [] { std::printf("x %d\n", 12345); });
    measure("printf_int", [] { std::printf("x %d\n", 12345); });
    measure("printf_float", [] { std::printf("%f\n", 1234.5678); });
    measure("puts_80chars", [] { std::puts("0123456789012345678901234567890123456789012345678901234567890123456789012345678"); });
    measure("fflush", [] { std::fflush(stdout); });
    measure("cout_first_use", [] { std::cout << "x " << 12345 << "\n"; });
    measure("cout_short_int", [] { std::cout << "x " << 12345 << "\n"; });
    measure("cout_80chars", [] { std::cout << "0123456789012345678901234567890123456789012345678901234567890123456789012345678\n"; });
    measure("cout_float", [] { std::cout << 1234.5678f << "\n"; });
    measure("cout_endl(flush)", [] { std::cout << "y" << std::endl; });
    measure("cout_to_string(vec2)", [] { std::cout << to_string(vec2{ 1, 2 }) << "\n"; });
}

void measure_actions() {
    auto& me = self();
    const vec2 target = clamp_tile({ map::width() / 2, map::height() / 2 });
    measure("self.move_towards(centre)", [&] { sink = me.move_towards(target); });
    measure("self.move_towards(again,noop)", [&] { sink = me.move_towards(target); });
    measure("self.place_ward", [&] { try { sink = me.place_ward(me.position()); } catch (const cambc::error&) { sink = 9; } });
    measure("self.set_priority", [&] { me.set_priority(5); });
    measure("self.can_sell", [&] { sink = me.can_sell(); });
    measure("self.attack(invalid,throws)", [&] { try { me.attack(me); sink = 0; } catch (const cambc::error&) { sink = 1; } });
    measure("self.move(blocked,throws)", [&] { try { sink = me.move({ 0, 0 }); } catch (const cambc::error&) { sink = 1; } });
}

// Minimal legal play after the measurement rounds: attack a camp in range or walk to the nearest camp tile.
void farm() {
    auto& me = self();
    if (!me.is_alive() || me.is_recalling() || me.is_submitting_gems() || me.is_teleporting()) return;
    const bool at_spawn = map::spawn_region(team::ally).is_set(me.position());
    if (!at_spawn && me.health() < me.max_health() * 0.3f) { me.recall(); return; }
    if (me.can_attack()) {
        const auto tiles = me.attack_tiles();
        for (const auto& c : vision::camps())
            if (c.is_visible() && tiles.is_set(c.position()) && c.health() > 0) { me.attack(c); return; }
    }
    if (me.gem_count() > 0) {
        for (auto altar : map::altars().positions())
            if (me.position().chebyshev_distance(altar) <= constants::submit_gems_range) { me.submit_gems(altar); return; }
    }
    static const auto spawns = [] {
        std::vector<vec2> all;
        for (const auto& g : map::camp_groups()) for (const auto& c : g) all.push_back(c.spawn_position());
        return all;
    }();
    vec2 best{}; std::int64_t best_d = 1 << 30;
    for (auto p : spawns) { const auto d = me.position().chebyshev_distance(p); if (d < best_d) { best_d = d; best = p; } }
    if (best_d > 1 && me.can_move()) {
        const bitboard blocked = map::walls() | map::holes() | map::altars() | map::spawn_region(team::enemy)
            | vision::champion_tiles() | vision::camp_tiles();
        bitboard roots; roots.set(best);
        bitboard dest; dest.set(me.position());
        bitboard visited; distance_field f;
        bitboard walls = blocked; walls.set(best, false); walls.set(me.position(), false);
        f.compute_to(walls, roots, dest, visited);
        if (auto step = f.next_move_from(me.position())) {
            const vec2 to = me.position() + *step;
            if (!blocked.is_set(to)) me.move_absolute(to);
        }
    }
}

} // namespace

void Bot::play_turn() {
    const auto fuel_start = fuel_remaining();
    const auto round = round_number();
    static int my_index = -1;
    static unsigned turns = 0;
    ++turns;
    if (my_index < 0) {
        const auto f0 = fuel_remaining();
        const auto allies = cambc::champions(team::ally);
        for (std::size_t i = 0; i < allies.size(); ++i) if (allies[i] == self()) my_index = static_cast<int>(i);
        const auto f1 = fuel_remaining();
        out::s("P index="); out::i(my_index); out::kvs("type", to_string(self().champion_name()));
        out::kv("allies", static_cast<long long>(allies.size())); out::kv("enemies", static_cast<long long>(cambc::champions(team::enemy).size()));
        out::s(" draft_memory=\""); out::raw(reinterpret_cast<const char*>(draft_memory.data()), 8); out::s("\" pos="); out::v(self().position());
        out::kv("champions_ally_fuel", static_cast<long long>(f0 - f1)); out::nl();
    }
    if (turns <= 15 || round % 100 == 0) {
        out::s("T "); out::i(round); out::kv("turn", turns); out::kv("idx", my_index); out::kv("fuel_at_start", static_cast<long long>(fuel_start)); out::nl();
    }

    if (my_index == 0 && turns <= 15) {
        switch (turns) {
        case 1: measure_bitboards(); break;
        case 2: measure_distance_fields(); break;
        case 3: measure_vision(); break;
        case 4: measure_camps(); break;
        case 5: measure_champion(); break;
        case 6: measure_baselines(); break;
        case 7: measure_printing(); break;
        case 8: measure_actions(); break;
        case 9: measure_bitboards(); break; // repeat on a normal 100k turn for consistency
        case 10: measure_baselines2(); break;
        case 11: measure_baselines3(); break;
        case 12: measure_distance_fields2(); break;
        case 13: measure_distance_fields3(); break;
        case 14: measure_baselines4(); break;
        case 15: measure_distance_fields4(); break;
        }
        flush_rows(round);
        out::s("T "); out::i(round); out::kv("fuel_after_measure", static_cast<long long>(fuel_remaining())); out::nl();
        if (turns == 8) { out::flush(); return; }
    } else if (my_index == 1 && turns == 1) {
        measure_distance_fields();
        flush_rows(round);
    }
    farm();
    if (turns <= 15 || round % 100 == 0) { out::s("T "); out::i(round); out::kv("fuel_at_end", static_cast<long long>(fuel_remaining())); out::nl(); }
    out::flush();
}
