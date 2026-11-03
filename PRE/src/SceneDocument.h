#pragma once
#include "Tilemap.h"
#include <nlohmann/json.hpp>
#include <string>
#include <filesystem>
#include <optional>
#include <vector>

namespace pre {
struct SceneDocument {
    nlohmann::json data;
    std::filesystem::path path;
    std::string error;
    bool dirty = false;
    bool save_conflict = false;
    bool load(const std::filesystem::path& from);
    bool save();
    bool validate(std::string& reason) const;
    bool set_tile(TileCell cell, TileKind kind);
    bool paint_tiles(TileCell from, TileCell to, TileKind kind);
    std::optional<std::size_t> find_entity(const std::string& id) const;
    // Shared, validated mutation API for editor tools and future agent bridge.
    bool set_entity_field(const std::string& id, const std::string& field, const nlohmann::json& value);
    bool erase_entity_field(const std::string& id, const std::string& field);
    bool set_render_field(const std::string& field, const nlohmann::json& value);
    void begin_edit();
    void end_edit();
    bool undo();
    bool redo();
    bool can_undo() const { return !undo_.empty(); }
    bool can_redo() const { return !redo_.empty(); }
    void clear_history();
private:
    std::optional<std::string> loaded_revision_;
    static constexpr std::size_t history_limit_ = 64;
    std::vector<nlohmann::json> undo_, redo_;
    std::optional<nlohmann::json> edit_start_;
    int edit_depth_ = 0;
    void push_undo(nlohmann::json snapshot);
    void record_change(nlohmann::json before);
    bool set_field(nlohmann::json& object, const std::string& field, const nlohmann::json& value);
};
}
