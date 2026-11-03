#include "SceneRuntime.h"
#include <filesystem>
#include <fstream>
#include <string>

int main() {
    const auto dir = std::filesystem::temp_directory_path() / "pre_gameplay_api_test";
    std::filesystem::create_directories(dir / "scripts");
    std::ofstream(dir / "scripts" / "game.lua") << R"(
function on_start()
    pre.spawn("bullet", "sprite", 2, 4, 0)
end
local fired = false
function on_update(dt)
    if pre.key_pressed("space") and not fired then
        fired = true
        pre.set_entity_position("bullet", 5, 6, 0)
    end
    if pre.action_down("move_right") then
        local x, y, z = pre.position()
        pre.set_position(x + dt * 10, y, z)
    end
    if fired and not pre.key_down("space") then
        pre.destroy("bullet")
    end
end
)";
    pre::SceneDocument src;
    src.path = dir / "lab.json";
    src.data = {{"schema_version", 1}, {"render", {{"clear_color", {0.1, 0.1, 0.1, 1.0}},
                 {"pixel_resolution", {320, 180}}}},
                {"entities", {{{"id", "hero"}, {"name", "Hero"},
                 {"type", "sprite"}, {"position", {0, 0, 0}},
                 {"script", "scripts/game.lua"}}}}};
    const auto original = src.data;
    pre::SceneRuntime runtime;
    runtime.play(src);
    if (!runtime.errors().empty() || !runtime.scene()->find_entity("bullet")) return 1;
    pre::RuntimeInput buttons;
    const auto set_button = [&](std::string_view name, bool down) {
        for (std::size_t i = 0; i < buttons.keys.size(); ++i)
            if (buttons.keys[i] == name) buttons.down[i] = down;
    };
    set_button("space", true);
    set_button("d", true);
    runtime.tick(0.1f, buttons);
    if (!runtime.errors().empty() ||
        runtime.scene()->data["entities"][1]["position"][0] != 5 ||
        runtime.scene()->data["entities"][0]["position"][0] != 1) return 2;
    runtime.tick(0.1f, buttons);
    if (!runtime.errors().empty() ||
        runtime.scene()->data["entities"][0]["position"][0] != 2) return 3;
    set_button("space", false);
    runtime.tick(0.1f, buttons);
    if (!runtime.errors().empty() || runtime.scene()->find_entity("bullet")) return 4;
    runtime.stop();
    if (src.data != original || src.dirty) return 5;
    std::filesystem::remove_all(dir);
}
