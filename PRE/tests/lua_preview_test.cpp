#include "LuaPreview.h"
#include <fstream>
#include <iostream>

int main() {
    pre::LuaPreview preview;
    std::uint64_t now = 0;
    auto step = [&](const char* script, bool success, float red) {
        std::ofstream("scenes/preview.lua") << script;
        preview.poll(now);
        now += 400;
        const bool valid = preview.available() && preview.error().empty() == success;
        const bool color = red < 0 ? !preview.color() : preview.color() && (*preview.color())[0] == red;
        if (!valid || !color) std::cerr << "Unexpected result for " << script << ": " << preview.error() << '\n';
        return valid && color;
    };

    if (!step("return nil", true, -1)) return 1;
    if (!step("return {0.25, 0.25, 0.25, 1}", true, 0.25f)) return 2;
    if (!step("return {", false, 0.25f)) return 3;
    if (!step("return {2, 0, 0, 1}", false, 0.25f)) return 4;
    if (!step("while true do end", false, 0.25f)) return 5;
    if (!step("return {0.75, 0, 0, 1}", true, 0.75f)) return 6;
    if (!step("return nil", true, -1)) return 7;
    return 0;
}
