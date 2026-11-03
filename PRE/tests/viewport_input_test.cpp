#include "ViewportCanvas.h"
#include <cstdio>

int main() {
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(800, 600);
    io.ConfigWindowsMoveFromTitleBarOnly = false;
    unsigned char* atlas = nullptr;
    int width = 0, height = 0;
    io.Fonts->GetTexDataAsRGBA32(&atlas, &width, &height);
    pre::ViewportCanvas canvas{};
    ImVec2 window{};
    const auto frame = [&](float x, float y, bool down) {
        io.AddMousePosEvent(x, y);
        io.AddMouseButtonEvent(ImGuiMouseButton_Left, down);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(40, 60), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(400, 330), ImGuiCond_Once);
        ImGui::Begin("Viewport test");
        window = ImGui::GetWindowPos();
        canvas = pre::viewport_canvas(ImVec2(320, 240));
        ImGui::End();
        ImGui::EndFrame();
    };
    const auto unchanged = [&] { return window.x == 40 && window.y == 60; };
    frame(120, 150, false);
    frame(120, 150, true);
    const bool claimed = canvas.pressed && canvas.held;
    frame(180, 190, true);
    const bool drag_inside = canvas.held && canvas.hovered && unchanged();
    frame(500, 200, true);
    const bool drag_outside = canvas.held && !canvas.hovered && unchanged();
    frame(500, 200, false);
    const bool released = !canvas.held && unchanged();
    frame(130, 68, false);
    frame(130, 68, true);
    frame(195, 103, true);
    const bool title_bar_moves = !unchanged();
    frame(195, 103, false);
    ImGui::DestroyContext();
    if (!claimed || !drag_inside || !drag_outside || !released || !title_bar_moves) {
        std::fprintf(stderr, "Input check: claim=%d inside=%d outside=%d release=%d title=%d\n",
                     claimed, drag_inside, drag_outside, released, title_bar_moves);
        return 1;
    }
    std::puts("PASS: canvas captures drag, window stays fixed, title bar still moves");
    return 0;
}
