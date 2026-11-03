#include "SceneDocument.h"
#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) return 1;
    const auto file = std::filesystem::path("scene-save-test.json");
    std::filesystem::copy_file(argv[1], file, std::filesystem::copy_options::overwrite_existing);
    pre::SceneDocument editor;
    pre::SceneDocument external;
    if (!editor.load(file) || !external.load(file)) return 2;
    const auto index = editor.find_entity("hero_01");
    if (!index) return 3;
    external.data["entities"][*index]["name"] = "External edit";
    if (!external.save()) return 4;
    editor.data["entities"][*index]["position"] = {4, 2, 0};
    editor.dirty = true;
    if (editor.save() || !editor.save_conflict || !editor.dirty) return 5;
    if (!external.load(file) || external.data["entities"][*index]["name"] != "External edit") return 6;
    if (!editor.load(file) || editor.dirty || editor.save_conflict) return 7;
    editor.data["entities"][*index]["position"] = {4, 2, 0};
    if (!editor.save() || !editor.save() || !external.load(file)) return 8;
    if (external.data["entities"][*index]["position"] != nlohmann::json({4, 2, 0}) ||
        external.data["entities"][*index]["name"] != "External edit") return 9;
    std::cout << "PASS: stale save refused, both revisions preserved, reload and retry persisted\n";
    return 0;
}
