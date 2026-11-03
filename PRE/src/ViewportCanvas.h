#pragma once

#include "imgui.h"

namespace pre {
struct ViewportCanvas {
    ImVec2 min;
    ImVec2 max;
    bool pressed;
    bool held;
    bool hovered;
};

inline ViewportCanvas viewport_canvas(ImVec2 size) {
    // Images don't claim an active mouse ID. Give the canvas its own input item.
    ImGui::InvisibleButton("##viewport-canvas", size, ImGuiButtonFlags_MouseButtonLeft);
    return {ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
            ImGui::IsItemClicked(ImGuiMouseButton_Left), ImGui::IsItemActive(),
            ImGui::IsItemHovered()};
}
}
