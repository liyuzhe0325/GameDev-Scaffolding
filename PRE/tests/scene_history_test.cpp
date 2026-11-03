#include "SceneDocument.h"
#include <cassert>
#include <filesystem>
#include <iostream>

int main() {
    pre::SceneDocument scene;
    const auto path = std::filesystem::path("scenes/lab.json");
    assert(scene.load(path));
    const auto original = scene.data;
    assert(scene.set_entity_field("hero_01", "name", "Test Hero"));
    assert(scene.can_undo());
    assert(scene.undo());
    assert(scene.data == original);
    assert(scene.redo());
    assert(scene.data["entities"][*scene.find_entity("hero_01")]["name"] == "Test Hero");
    assert(scene.undo());
    scene.begin_edit();
    assert(scene.set_tile({0,0}, pre::TileKind::wall));
    assert(scene.set_tile({1,0}, pre::TileKind::floor));
    scene.end_edit();
    assert(scene.undo());
    assert(scene.data == original);
    assert(!scene.set_entity_field("hero_01", "position", {1,2}));
    assert(scene.data == original);
    assert(!scene.set_entity_field("hero_01", "id", "new-id"));
    assert(scene.data == original);
    std::cout << "history and property mutation tests passed\n";
}
