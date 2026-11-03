#include "SceneDocument.h"
#include "ScriptReference.h"
#include "SpriteAsset.h"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <chrono>
#include <cmath>
#include <iterator>
#include <unordered_set>

namespace pre {
namespace {
std::string read_scene_bytes(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open " + path.string());
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

// Stage the replacement on the same filesystem and isolate parallel temp files.
struct SaveTemp {
    std::filesystem::path directory;
    std::filesystem::path file;

    explicit SaveTemp(const std::filesystem::path& target) {
        auto serial = std::chrono::steady_clock::now().time_since_epoch().count();
        do {
            directory = target.parent_path() / (".pre-save-" + std::to_string(serial++));
        } while (!std::filesystem::create_directory(directory));
        file = directory / "scene.json";
    }
    ~SaveTemp() {
        std::error_code ignored;
        std::filesystem::remove(file, ignored);
        std::filesystem::remove(directory, ignored);
    }
};
}

static bool finite_vec(const nlohmann::json& v, int count) {
    if (!v.is_array() || v.size() != static_cast<std::size_t>(count)) return false;
    for (auto& n : v) if (!n.is_number() || !std::isfinite(n.get<double>())) return false;
    return true;
}
bool SceneDocument::validate(std::string& reason) const {
    const auto& d = data;
    if (!d.is_object() || d.value("schema_version", 0) != 1) { reason = "schema_version must equal 1"; return false; }
    if (!d.contains("entities") || !d["entities"].is_array()) { reason = "entities must be an array"; return false; }
    if (!d.contains("render") || !d["render"].is_object()) { reason = "render must be an object"; return false; }
    const auto& r = d["render"];
    if (!r.contains("clear_color") || !finite_vec(r["clear_color"], 4)) { reason = "render.clear_color must be vec4"; return false; }
    if (!r.contains("pixel_resolution") || !r["pixel_resolution"].is_array() || r["pixel_resolution"].size()!=2 ||
        !r["pixel_resolution"][0].is_number_integer() || !r["pixel_resolution"][1].is_number_integer() ||
        r["pixel_resolution"][0].get<double>() <= 0 || r["pixel_resolution"][1].get<double>() <= 0 ||
        r["pixel_resolution"][0].get<double>() > 4096 || r["pixel_resolution"][1].get<double>() > 4096) {
        reason = "render.pixel_resolution requires two integers from 1 to 4096"; return false;
    }
    std::unordered_set<std::string> ids;
    const std::unordered_set<std::string> allowed_types = {"camera", "occluder", "light", "sprite", "mesh", "effect"};
    for (auto& e : d["entities"]) {
        if (!e.is_object() || !e.contains("id") || !e["id"].is_string() ||
            !e.contains("name") || !e["name"].is_string() ||
            !e.contains("type") || !e["type"].is_string() ||
            !e.contains("position") || !finite_vec(e["position"], 3)) {
            reason = "each entity needs id, name, type and vec3 position"; return false;
        }
        const auto id = e["id"].get<std::string>();
        if (!allowed_types.contains(e["type"].get<std::string>())) { reason = "unknown entity type"; return false; }
        if (e.contains("sprite_image") &&
            (e["type"] != "sprite" || !e["sprite_image"].is_string() ||
             !is_sprite_reference(e["sprite_image"].get_ref<const std::string&>()))) {
            reason = "sprite_image must reference assets/<name>.png on a sprite entity";
            return false;
        }
        if (e.contains("script") &&
            (!e["script"].is_string() || !is_script_reference(e["script"].get_ref<const std::string&>()))) {
            reason = "script must reference scripts/<name>.lua";
            return false;
        }
        if (id.empty() || !ids.insert(id).second) { reason = "entity ids must be unique and nonempty"; return false; }
    }
    if (d.contains("tiles")) {
        const auto& tiles = d["tiles"];
        if (!tiles.is_array() || tiles.size() > tile_count_limit) {
            reason = "tiles must be an array of at most 8192 cells";
            return false;
        }
        std::unordered_set<std::int64_t> occupied;
        for (const auto& tile : tiles) {
            if (!tile.is_object() || !tile.contains("x") || !tile["x"].is_number_integer() ||
                !tile.contains("y") || !tile["y"].is_number_integer() ||
                !tile.contains("kind") || !tile["kind"].is_string() ||
                tile_kind(tile["kind"].get_ref<const std::string&>()) == TileKind::empty) {
                reason = "Each tile needs integer x/y and a floor or wall kind";
                return false;
            }
            const double x = tile["x"].get<double>(), y = tile["y"].get<double>();
            if (std::abs(x) > tile_coordinate_limit || std::abs(y) > tile_coordinate_limit) {
                reason = "Tile coordinates must be between -4096 and 4096";
                return false;
            }
            const auto key = static_cast<std::int64_t>(x) * (2 * tile_coordinate_limit + 1) +
                             static_cast<std::int64_t>(y);
            if (!occupied.insert(key).second) {
                reason = "Tile coordinates must be unique";
                return false;
            }
        }
    }
    return true;
}

bool SceneDocument::set_tile(TileCell cell, TileKind kind) {
    if (!valid_cell(cell) || !data.is_object()) return false;
    auto before = edit_depth_ == 0 ? data : nlohmann::json();
    if (!data.contains("tiles")) {
        if (kind == TileKind::empty) return false;
        data["tiles"] = nlohmann::json::array();
    }
    auto& tiles = data["tiles"];
    const auto found = std::find_if(tiles.begin(), tiles.end(), [&](const auto& tile) {
        return tile["x"] == cell.x && tile["y"] == cell.y;
    });
    if (kind == TileKind::empty) {
        if (found == tiles.end()) return false;
        tiles.erase(found);
    } else {
        const char* name = kind == TileKind::floor ? "floor" : "wall";
        if (found == tiles.end()) {
            if (tiles.size() >= tile_count_limit) return false;
            tiles.push_back({{"x", cell.x}, {"y", cell.y}, {"kind", name}});
        } else {
            if ((*found)["kind"] == name) return false;
            (*found)["kind"] = name;
        }
    }
    dirty = true;
    record_change(std::move(before));
    return true;
}

bool SceneDocument::paint_tiles(TileCell from, TileCell to, TileKind kind) {
    if (!valid_cell(from) || !valid_cell(to)) return false;
    begin_edit();
    const int dx = std::abs(to.x - from.x);
    const int dy = -std::abs(to.y - from.y);
    const int step_x = from.x < to.x ? 1 : -1;
    const int step_y = from.y < to.y ? 1 : -1;
    int balance = dx + dy;
    bool changed = false;
    while (true) {
        changed |= set_tile(from, kind);
        if (from == to) { end_edit(); return changed; }
        const int twice = 2 * balance;
        if (twice >= dy) { balance += dy; from.x += step_x; }
        if (twice <= dx) { balance += dx; from.y += step_y; }
    }
}
bool SceneDocument::load(const std::filesystem::path& from) {
    try {
        auto bytes = read_scene_bytes(from);
        auto next = nlohmann::json::parse(bytes);
        SceneDocument candidate; candidate.data = std::move(next);
        std::string reason;
        if (!candidate.validate(reason)) { error = reason; return false; }
        data = std::move(candidate.data);
        path = from;
        loaded_revision_ = std::move(bytes);
        dirty = false;
        save_conflict = false;
        error.clear();
        clear_history();
        return true;
    } catch (const std::exception& ex) { error=ex.what(); return false; }
}
bool SceneDocument::save() {
    save_conflict = false;
    try {
        std::string why;
        if (!validate(why)) { error=why; return false; }
        auto payload = data.dump(2) + '\n';
        SaveTemp temp(path);
        std::ofstream out(temp.file, std::ios::binary | std::ios::trunc);
        out << payload;
        out.close();
        if (!out) { error = "Cannot write temporary scene"; return false; }
        // Optimistic check immediately before replacing the on-disk document.
        if (!loaded_revision_ || !std::filesystem::is_regular_file(path) ||
            read_scene_bytes(path) != *loaded_revision_) {
            save_conflict = true;
            error = "Scene changed on disk since load or save";
            return false;
        }
        std::filesystem::rename(temp.file, path);
        loaded_revision_ = std::move(payload);
        dirty = false;
        error.clear();
        return true;
    } catch (const std::exception& ex) {error=ex.what(); return false;}
}
std::optional<std::size_t> SceneDocument::find_entity(const std::string& id) const {
    if (!data.is_object() || !data.contains("entities")) return {};
    for (std::size_t i=0; i<data["entities"].size(); ++i)
        if (data["entities"][i].value("id", "") == id) return i;
    return {};
}
}

namespace pre {
void SceneDocument::clear_history() {
    undo_.clear(); redo_.clear(); edit_start_.reset(); edit_depth_ = 0;
}
void SceneDocument::push_undo(nlohmann::json snapshot) {
    if (undo_.size() == history_limit_) undo_.erase(undo_.begin());
    undo_.push_back(std::move(snapshot));
    redo_.clear();
}
void SceneDocument::record_change(nlohmann::json before) {
    if (edit_depth_ == 0) push_undo(std::move(before));
}
void SceneDocument::begin_edit() {
    if (edit_depth_++ == 0) edit_start_ = data;
}
void SceneDocument::end_edit() {
    if (edit_depth_ == 0) return;
    if (--edit_depth_ == 0) {
        if (edit_start_ && *edit_start_ != data) push_undo(std::move(*edit_start_));
        edit_start_.reset();
    }
}
bool SceneDocument::undo() {
    if (edit_depth_ || undo_.empty()) return false;
    auto previous = std::move(undo_.back()); undo_.pop_back();
    redo_.push_back(std::move(data));
    data = std::move(previous);
    dirty = true; // conservative: disk revision may have changed after this edit
    return true;
}
bool SceneDocument::redo() {
    if (edit_depth_ || redo_.empty()) return false;
    auto next = std::move(redo_.back()); redo_.pop_back();
    if (undo_.size() == history_limit_) undo_.erase(undo_.begin());
    undo_.push_back(std::move(data));
    data = std::move(next);
    dirty = true;
    return true;
}
bool SceneDocument::set_entity_field(const std::string& id, const std::string& field, const nlohmann::json& value) {
    const auto index = find_entity(id);
    if (!index) { error = "Entity not found"; return false; }
    if (field != "name" && field != "position" && field != "script" && field != "sprite_image") {
        error = "Unsupported entity field"; return false;
    }
    if ((field == "name" && !value.is_string()) ||
        (field == "position" && !finite_vec(value, 3)) ||
        ((field == "script" || field == "sprite_image") && !value.is_string())) {
        error = "Invalid entity field value"; return false;
    }
    return set_field(data["entities"][*index], field, value);
}
bool SceneDocument::set_render_field(const std::string& field, const nlohmann::json& value) {
    if (field != "clear_color" && field != "pixel_resolution") {
        error = "Unsupported render field";
        return false;
    }
    const auto render = data.find("render");
    if (render == data.end() || !render->is_object()) return false;
    return set_field(*render, field, value);
}
bool SceneDocument::set_field(nlohmann::json& object, const std::string& field, const nlohmann::json& value) {
    const auto existing = object.find(field);
    if (existing != object.end() && *existing == value) return false;
    const auto previous = existing != object.end() ? std::optional<nlohmann::json>(*existing) : std::nullopt;
    auto before = edit_depth_ == 0 ? data : nlohmann::json();
    object[field] = value;
    std::string reason;
    if (!validate(reason)) {
        if (previous) object[field] = *previous;
        else object.erase(field);
        error = reason;
        return false;
    }
    dirty = true;
    error.clear();
    record_change(std::move(before));
    return true;
}
bool SceneDocument::erase_entity_field(const std::string& id, const std::string& field) {
    if (field != "script" && field != "sprite_image") { error = "Field cannot be removed"; return false; }
    auto index = find_entity(id);
    if (!index || !data["entities"][*index].contains(field)) return false;
    auto before = edit_depth_ == 0 ? data : nlohmann::json();
    data["entities"][*index].erase(field); dirty = true; error.clear();
    record_change(std::move(before)); return true;
}
}
