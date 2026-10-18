// Draft-turn dump: every engine constant, design, item and map fact the bot authors rely on.
// Every line starts with "D " so replay stdout can be grepped. Uses out.h, not iostream (fuel).
#include <cambc.h>
#include "draft.h"
#include "out.h"
#include <algorithm>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

using namespace cambc;

namespace {

std::string targets_str(ability_target_mask m) {
    std::string s;
    if (m.contains(ability_target_mask::camps)) s += "camps,";
    if (m.contains(ability_target_mask::enemies)) s += "enemies,";
    if (m.contains(ability_target_mask::allies)) s += "allies,";
    if (m.contains(ability_target_mask::self)) s += "self,";
    if (m.contains(ability_target_mask::ground)) s += "ground,";
    if (!s.empty()) s.pop_back();
    return s;
}

void print_ability(const ability& a) {
    const auto& info = a.info();
    out::s("D   slot"); out::i(a.slot());
    if (!info) { out::s(" kind=none\n"); return; }
    std::visit([&](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        out::kvs("kind", to_string(T::kind));
        out::kv("max_cooldown", v.max_cooldown);
        out::kv("targets", v.valid_targets.bits());
        out::s("["); out::s(targets_str(v.valid_targets)); out::s("]");
        out::kv("exclusive_with_movement", v.exclusive_with_movement ? 1 : 0);
        if constexpr (requires { v.range; }) out::kvf("range", v.range);
        if constexpr (requires { v.modifier; }) out::kvf("modifier", v.modifier);
        if constexpr (requires { v.radius; }) out::kvf("radius", v.radius);
        if constexpr (requires { v.duration; }) out::kv("duration", v.duration);
        if constexpr (requires { v.enhanced_attack; }) out::kv("enhanced_attack", v.enhanced_attack ? 1 : 0);
        out::nl();
    }, *info);
}

template <class F> void section(const char* name, F&& f) {
    out::s("D == "); out::s(name); out::s(" == fuel="); out::i(static_cast<long long>(fuel_remaining())); out::nl();
    try {
        f();
    } catch (const cambc::error& e) {
        out::s("D ERROR in "); out::s(name); out::s(": "); out::s(to_string(e.code())); out::s(" ("); out::s(e.message()); out::s(")\n");
    } catch (const std::exception& e) {
        out::s("D ERROR in "); out::s(name); out::s(": "); out::s(e.what()); out::nl();
    }
    out::flush();
}

void line_i(const char* key, long long value) { out::s("D "); out::s(key); out::s("="); out::i(value); out::nl(); }
void line_f(const char* key, double value) { out::s("D "); out::s(key); out::s("="); out::f(value); out::nl(); }

void dump_constants() {
    using namespace constants;
    line_i("capture_time", capture_time);
    line_i("camp_sleep_time", camp_sleep_time);
    line_i("respawn_time", respawn_time);
    line_i("recall_time", recall_time);
    line_f("submit_gem_duration_multiplier", submit_gem_duration_multiplier);
    line_i("submit_gem_base_duration", submit_gem_base_duration);
    line_i("submit_gem_minimum_duration", submit_gem_minimum_duration);
    line_i("submit_gems_range", submit_gems_range);
    line_i("max_rounds", max_rounds);
    line_i("winning_gems", winning_gems);
    line_i("kill_gold", kill_gold);
    line_i("assist_gold", assist_gold);
    line_i("passive_gold", passive_gold);
    line_f("spawn_healing", spawn_healing);
    line_i("active_wards", active_wards);
    line_i("ward_charges", detail::cached_game_settings.ward_charges);
    line_i("shared_memory_bytes", shared_memory_bytes);
    line_i("draft_memory_bytes", draft_memory_bytes);
    line_i("tile_reveal_rounds", tile_reveal_rounds);
    line_i("ward_cooldown", ward_cooldown);
    line_i("ward_lifetime", ward_lifetime);
    line_f("ward_vision_radius", ward_vision_radius);
    line_f("ward_placement_radius", ward_placement_radius);
    line_f("control_ward_vision_radius", detail::cached_game_settings.control_ward_vision_radius);
    line_f("control_ward_placement_radius", detail::cached_game_settings.control_ward_placement_radius);
    line_i("altar_lock_hits", altar_lock_hits);
    line_i("tp_scroll_time", detail::cached_game_settings.tp_scroll_time);
    line_i("tp_crystal_time", tp_crystal_time);
    line_i("reserved", detail::cached_game_settings.reserved);
    line_f("passive.prism_focus_per_gem", passive_abilities::prism_focus_per_gem);
    line_f("passive.varg_splash_radius", passive_abilities::varg_splash_radius);
    line_i("passive.vesper_vale_bonus_gold_percent", passive_abilities::vesper_vale_bonus_gold_percent);
    line_f("passive.ryth_health_strength_ratio", passive_abilities::ryth_health_strength_ratio);
    line_f("passive.ren_retaliation_focus_ratio", passive_abilities::ren_retaliation_focus_ratio);
    line_i("shared_memory_buffer.size", static_cast<long long>(shared_memory_buffer::size()));
    line_i("draft_memory.size", static_cast<long long>(draft_memory.size()));
}

void dump_champions() {
    for (std::size_t i = 0; i < champion_designs.size(); ++i) {
        const auto& c = champion_designs[i];
        const auto s = c.base_stats();
        out::s("D champion "); out::i(static_cast<long long>(i)); out::s(" "); out::s(to_string(c.champion_name()));
        out::kvs("passive", to_string(c.passive()));
        out::kv("vision", s.vision_radius); out::kv("move", s.base_move_speed); out::kv("attack_range", s.base_attack_range);
        out::kvf("hp", s.base_health); out::kvf("hp_growth", s.health_growth);
        out::kvf("str", s.base_strength); out::kvf("str_growth", s.strength_growth);
        out::kvf("focus", s.base_focus); out::kvf("focus_growth", s.focus_growth);
        out::kvf("camp_dmg_mult", s.camp_damage_multiplier);
        out::nl();
        for (const auto& a : c.abilities()) print_ability(a);
        if (i == 0) {
            try { (void)c.position(); out::s("D   design.position() OK?!\n"); }
            catch (const cambc::error& e) { out::s("D   design.position() throws "); out::s(to_string(e.code())); out::nl(); }
            try { (void)c.abilities()[0].cooldown(); out::s("D   design.cooldown() OK?!\n"); }
            catch (const cambc::error& e) { out::s("D   design.ability.cooldown() throws "); out::s(to_string(e.code())); out::nl(); }
            try { (void)c.abilities()[0].valid_tiles(); out::s("D   design.valid_tiles() OK?!\n"); }
            catch (const cambc::error& e) { out::s("D   design.ability.valid_tiles() throws "); out::s(to_string(e.code())); out::nl(); }
            catch (const std::exception& e) { out::s("D   design.ability.valid_tiles() throws std: "); out::s(e.what()); out::nl(); }
        }
    }
}

void dump_camps() {
    for (int t = 0; t < 5; ++t) {
        detail::camp_design d{};
        const auto code = detail::sdk_camp_design(t, &d);
        if (code != 0) {
            out::s("D camp_design "); out::i(t); out::s(" error "); out::s(to_string(static_cast<error_code>(code))); out::nl();
            continue;
        }
        const auto& s = d.base_stats;
        out::s("D camp "); out::i(t); out::s(" "); out::s(to_string(static_cast<camp_type>(t)));
        out::kvf("vision", s.vision_radius); out::kv("move", s.base_move_speed); out::kv("attack_range", s.base_attack_range);
        out::kvf("hp", s.base_health); out::kvf("hp_growth", s.health_growth);
        out::kvf("str", s.base_strength); out::kvf("str_growth", s.strength_growth);
        out::kvf("focus", s.base_focus); out::kvf("focus_growth", s.focus_growth);
        out::kv("def.type", d.definition.type); out::kv("vision_type", d.definition.vision_type);
        out::kv("passive", d.definition.passive_type); out::kv("attack_type", d.definition.attack_type);
        out::kv("ability0.kind", d.definition.abilities[0].kind); out::kv("ability1.kind", d.definition.abilities[1].kind);
        out::nl();
    }
}

void dump_items() {
    for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(item_type::count); ++i) {
        const item_type it{i};
        const auto info = it.info();
        out::s("D item "); out::i(i); out::s(" "); out::s(to_string(it));
        out::kv("cost", info.cost); out::kv("combine_count", info.combine_count);
        out::kvs("result", info.result ? to_string(*info.result) : std::string("none"));
        out::kv("hp", info.health_points); out::kv("str", info.strength_points); out::kv("focus", info.focus_points);
        out::kvs("passive", info.passive ? to_string(*info.passive) : std::string("none"));
        out::kvf("magnitude", info.passive_magnitude); out::kv("purchasable", info.purchasable ? 1 : 0);
        out::nl();
    }
    for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(consumable_type::count); ++i) {
        const consumable_type ct{i};
        out::s("D consumable "); out::i(i); out::s(" "); out::s(to_string(ct));
        out::kv("cost", ct.cost()); out::kv("carry_limit", ct.carry_limit()); out::nl();
    }
}

void print_positions(const char* label, const std::vector<vec2>& p) {
    out::s("D "); out::s(label); out::kv("count", static_cast<long long>(p.size()));
    if (!p.empty()) {
        vec2 lo = p[0], hi = p[0];
        for (auto v : p) { lo.x = std::min(lo.x, v.x); lo.y = std::min(lo.y, v.y); hi.x = std::max(hi.x, v.x); hi.y = std::max(hi.y, v.y); }
        out::s(" bbox="); out::v(lo); out::s(".."); out::v(hi);
    }
    out::nl();
}

int d_or_neg(std::uint16_t d) { return d == distance_field::unreachable ? -1 : static_cast<int>(d); }

void dump_map() {
    out::s("D map"); out::kv("width", map::width()); out::kv("height", map::height()); out::kvs("symmetry", to_string(map::symmetry())); out::nl();
    out::s("D terrain"); out::kv("walls", map::walls().count()); out::kv("holes", map::holes().count());
    out::kv("bushes", map::bushes().count()); out::kv("altars", map::altars().count()); out::nl();
    print_positions("spawn_ally", map::spawn_region(team::ally).positions());
    print_positions("spawn_enemy", map::spawn_region(team::enemy).positions());
    out::s("D altar_positions:");
    for (auto a : map::altars().positions()) { out::s(" "); out::v(a); }
    out::nl();
    line_i("draft.team_size", draft::team_size());
    try { line_i("draft.visible_tiles", vision::visible_tiles().count()); }
    catch (const cambc::error& e) { out::s("D draft visible_tiles throws "); out::s(to_string(e.code())); out::nl(); }
    try { line_i("draft.champions_ally", static_cast<long long>(cambc::champions(team::ally).size())); }
    catch (const cambc::error& e) { out::s("D draft champions(ally) throws "); out::s(to_string(e.code())); out::nl(); }
    try { line_i("draft.round_number", round_number()); }
    catch (const cambc::error& e) { out::s("D draft round_number throws "); out::s(to_string(e.code())); out::nl(); }

    // Distance fields: from the ally spawn, from the enemy spawn, and to the altars.
    const auto f0 = fuel_remaining();
    const bitboard terrain = map::walls() | map::holes();
    bitboard visited;
    distance_field from_ally, from_enemy, to_altar;
    from_ally.compute(terrain | map::altars() | map::spawn_region(team::enemy), map::spawn_region(team::ally), visited);
    from_enemy.compute(terrain | map::altars() | map::spawn_region(team::ally), map::spawn_region(team::enemy), visited);
    // Altars are roots, so a tile adjacent to an altar reads 1 (a tribute tile).
    to_altar.compute(terrain | map::spawn_region(team::enemy), map::altars(), visited);
    line_i("draft.three_distance_fields_fuel", static_cast<long long>(f0 - fuel_remaining()));
    line_i("walkable_tiles", static_cast<long long>(map::width()) * map::height() - map::walls().count() - map::holes().count() - map::altars().count());

    std::uint16_t spawn_to_altar = distance_field::unreachable;
    for (auto p : map::spawn_region(team::ally).positions()) spawn_to_altar = std::min(spawn_to_altar, to_altar.get(p));
    line_i("spawn_to_altar_tribute_tile", spawn_to_altar == distance_field::unreachable ? -1 : spawn_to_altar - 1);
    // Per-altar distance from the ally spawn (to the altar tile; subtract 1 for the tribute tile).
    // Per-altar walking distance from the ally spawn to the nearest tribute tile (an 8-neighbour of the altar).
    out::s("D altar_dist_from_ally_to_tribute_tile:");
    for (auto a : map::altars().positions()) {
        std::uint16_t best = distance_field::unreachable;
        for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
            const vec2 p{ a.x + dx, a.y + dy };
            if ((dx || dy) && map::is_in_bounds(p)) best = std::min(best, from_ally.get(p));
        }
        out::s(" "); out::v(a); out::s("="); out::i(d_or_neg(best));
    }
    out::nl();

    const auto groups = map::camp_groups();
    line_i("camp_groups", static_cast<long long>(groups.size()));
    for (const auto& g : groups) {
        std::uint16_t da = distance_field::unreachable, de = distance_field::unreachable, dalt = distance_field::unreachable;
        for (const auto& c : g) {
            da = std::min(da, from_ally.get(c.spawn_position()));
            de = std::min(de, from_enemy.get(c.spawn_position()));
            dalt = std::min(dalt, to_altar.get(c.spawn_position()));
        }
        std::string state = "?";
        try {
            const auto st = g.state();
            state = std::holds_alternative<map::camp_group_alive>(st) ? "alive"
                : std::holds_alternative<map::camp_group_reward_available>(st) ? "reward" : "respawning";
        } catch (const cambc::error& e) { state = std::string("throws:") + to_string(e.code()); }
        out::s("D group "); out::i(static_cast<std::uint32_t>(static_cast<handle_camp_group_t>(g)));
        out::kv("gold", g.gold_value()); out::kv("gems", g.gem_value()); out::kv("respawn", g.max_respawn_timer());
        out::kv("size", static_cast<long long>(g.camps().size()));
        out::kv("d_ally", d_or_neg(da)); out::kv("d_enemy", d_or_neg(de));
        out::kv("d_altar", dalt == distance_field::unreachable ? -1 : static_cast<int>(dalt) - 1);
        out::kvs("state", state);
        out::s(" members:");
        for (const auto& c : g) {
            out::s(" "); out::s(to_string(c.kind())); out::s("@"); out::v(c.spawn_position());
            out::s("#"); out::i(static_cast<std::uint32_t>(static_cast<handle_camp_t>(c)));
        }
        out::nl();
    }
    if (!groups.empty() && !groups[0].camps().empty()) {
        const auto& c = groups[0].camps()[0];
        try {
            out::s("D draft camp.state()="); out::i(static_cast<std::uint32_t>(static_cast<camp_state::enum_type>(c.state())));
            out::kvf("health", c.health()); out::kv("visible", c.is_visible() ? 1 : 0); out::nl();
        } catch (const cambc::error& e) { out::s("D draft camp.state() throws "); out::s(to_string(e.code())); out::nl(); }
    }
}

} // namespace

void play_draft() {
    static bool dumped = false;
    static int draft_turn = 0;
    ++draft_turn;
    out::s("D draft_turn="); out::i(draft_turn); out::s(" fuel_at_start="); out::i(static_cast<long long>(fuel_remaining())); out::nl();
    if (!dumped) {
        dumped = true;
        section("constants", dump_constants);
        section("champions", dump_champions);
        section("camps", dump_camps);
        section("items", dump_items);
        section("map", dump_map);
        line_i("fuel_after_dump", static_cast<long long>(fuel_remaining()));
    }
    out::s("D picks ally:");
    for (auto p : draft::current_draft(team::ally)) { out::s(" "); out::s(to_string(p)); }
    out::s(" | enemy:");
    for (auto p : draft::current_draft(team::enemy)) { out::s(" "); out::s(to_string(p)); }
    out::nl();

    const auto chosen = draft::current_draft(team::ally);
    const champion_draft order[] = { champion_draft::marcella, champion_draft::tavi, champion_draft::varg,
        champion_draft::stellar, champion_draft::ryth, champion_draft::vendetta, champion_draft::cassi_and_issac,
        champion_draft::ren, champion_draft::willow, champion_draft::vesper_vale, champion_draft::prism };
    for (auto pick : order) {
        if (std::find(chosen.begin(), chosen.end(), pick) == chosen.end()) {
            draft_memory = std::string_view("probe-v1");
            draft::pick(pick);
            out::s("D picked "); out::s(to_string(pick)); out::s(" fuel_at_end="); out::i(static_cast<long long>(fuel_remaining())); out::nl();
            break;
        }
    }
    // Probe: a second pick in the same turn should throw draft_already_picked.
    if (draft_turn == 1) {
        try { draft::pick(champion_draft::prism); out::s("D second pick OK?!\n"); }
        catch (const cambc::error& e) { out::s("D second pick throws "); out::s(to_string(e.code())); out::nl(); }
    }
    out::flush();
}
