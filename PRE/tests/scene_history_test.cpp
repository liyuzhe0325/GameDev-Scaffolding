#include "SceneDocument.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace {
void require(bool condition, std::string_view message) {
    if (condition) return;
    std::cerr << message << '\n';
    std::exit(1);
}
}

int main() {
    pre::SceneDocument scene;
    const auto path = std::filesystem::path("scenes/lab.json");
    require(scene.load(path), "Could not load the sample scene");
    const auto original = scene.data;
    require(scene.set_entity_field("hero_01", "name", "Test Hero"), "Name edit failed");
    require(scene.can_undo() && scene.undo() && scene.data == original, "Name undo failed");
    require(scene.redo() && scene.data["entities"][*scene.find_entity("hero_01")]["name"] == "Test Hero",
            "Name redo failed");
    require(scene.undo(), "Could not undo name edit again");

    require(scene.set_tile({0, 0}, pre::TileKind::floor), "Floor placement failed");
    const auto floor = scene.data;
    scene.begin_edit();
    require(scene.paint_tiles({0, 0}, {2, 0}, pre::TileKind::wall), "First stroke segment failed");
    require(scene.paint_tiles({2, 0}, {4, 0}, pre::TileKind::wall), "Second stroke segment failed");
    scene.end_edit();
    const auto stroke = scene.data;
    require(scene.set_render_field("pixel_resolution", {320, 180}), "Resolution edit failed");
    const auto resized = scene.data;
    scene.begin_edit();
    require(scene.set_render_field("clear_color", {0.1, 0.2, 0.3, 1.0}), "First color edit failed");
    require(scene.set_render_field("clear_color", {0.4, 0.5, 0.6, 1.0}), "Second color edit failed");
    scene.end_edit();
    const auto recolored = scene.data;

    require(scene.undo() && scene.data == resized, "Undo did not restore only the color");
    require(scene.undo() && scene.data == stroke, "Undo did not restore only the resolution");
    require(scene.undo() && scene.data == floor, "Undo did not restore the floor and remove the entire stroke");
    require(scene.redo() && scene.data == stroke, "Stroke redo failed");
    require(scene.redo() && scene.data == resized, "Resolution redo failed");
    require(scene.redo() && scene.data == recolored, "Color redo failed");

    require(scene.undo() && scene.data == resized, "Could not undo color again");
    require(!scene.set_render_field("pixel_resolution", {320, 180}), "Identical resolution created an edit");
    require(!scene.set_render_field("pixel_resolution", {4294967297ULL, 180}), "Accepted oversized resolution");
    require(!scene.set_render_field("clear_color", {0, 0, 0}), "Accepted malformed color");
    require(!scene.set_entity_field("hero_01", "position", {1, 2}), "Accepted malformed position");
    require(!scene.set_entity_field("hero_01", "id", "new-id"), "Accepted a stable ID change");
    require(scene.data == resized && scene.redo() && scene.data == recolored,
            "Rejected or unchanged edits modified the scene or cleared redo");
    std::cout << "history and property mutation tests passed\n";
}
