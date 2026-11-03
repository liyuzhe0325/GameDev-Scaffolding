#include "SceneRuntime.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

namespace {
double x(const pre::SceneDocument& scene, std::size_t entity) {
    return scene.data["entities"][entity]["position"][0].get<double>();
}
}

int main(int argc, char** argv) {
    if (argc != 2) return 1;
    pre::SceneDocument source;
    if (!source.load(argv[1])) {
        std::cerr << source.error << '\n';
        return 2;
    }
    const auto initial = source.data;
    pre::SceneRuntime runtime;
    runtime.play(source);
    pre::RuntimeInput input;
    runtime.tick(0.1f, input);
    if (!runtime.errors().empty() || x(*runtime.scene(), 0) <= -3 || x(*runtime.scene(), 1) != 3)
        return 3;
    const auto d = std::find(input.keys.begin(), input.keys.end(), "d");
    input.down[d - input.keys.begin()] = true;
    runtime.tick(0.1f, input);
    if (std::abs(x(*runtime.scene(), 1) - 3.4) > 0.0001 || source.data != initial || source.dirty)
        return 4;
    runtime.stop();
    if (runtime.scene() || source.data != initial) return 5;

    auto& entities = source.data["entities"];
    std::swap(entities[0]["script"], entities[1]["script"]);
    source.dirty = true;
    const auto edited = source.data;
    runtime.play(source);
    runtime.tick(0.1f, {});
    if (!runtime.errors().empty() || x(*runtime.scene(), 0) != -3 || x(*runtime.scene(), 1) <= 3)
        return 6;
    runtime.tick(0.1f, input);
    if (std::abs(x(*runtime.scene(), 0) + 2.6) > 0.0001) return 7;
    runtime.stop();
    if (source.data != edited || !source.dirty) return 7;

    entities[0]["script"] = "scripts/../escape.lua";
    std::string reason;
    if (source.validate(reason)) return 8;
    const auto directory = std::filesystem::current_path();
    source.path = directory / "test.json";
    std::filesystem::create_directories(directory / "scripts");
    std::ofstream(directory / "scripts/fault.lua")
        << "function on_update(dt) pre.set_position(900, 0, 0) while true do end end\n";
    std::ofstream(directory / "scripts/ok.lua")
        << "function on_update(dt) local x, y, z = pre.position(); pre.set_position(x + dt, y, z) end\n";
    std::ofstream(directory / "scripts/budget.lua")
        << "local t = {} function on_update(dt) for i = 1, 8000 do t[#t + 1] = {} end end\n";
    auto extra = entities[0];
    extra["id"] = "memory_test";
    extra["script"] = "scripts/budget.lua";
    entities[0]["script"] = "scripts/fault.lua";
    entities[1]["script"] = "scripts/ok.lua";
    entities.push_back(extra);
    runtime.play(source);
    for (int i = 0; i < 12; ++i) runtime.tick(0.1f, {});
    if (runtime.errors().size() != 2 ||
        runtime.errors()[0].find("Instruction limit") == std::string::npos ||
        runtime.errors()[1].find("memory") == std::string::npos ||
        x(*runtime.scene(), 0) != -3 || x(*runtime.scene(), 1) < 4)
        return 9;
}
