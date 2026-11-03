#include "EditorUI.h"
#include "LuaPreview.h"
#include "PixelViewport.h"
#include "SpriteAsset.h"
#include "ScriptReference.h"
#include "ViewportCanvas.h"
#include "ReleaseInfo.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>
namespace pre {
struct EditorUI::SpriteDialog {
    std::mutex mutex;
    std::filesystem::path scene;
    std::string entity;
    std::string filename;
    std::string error;
    bool done = false;
};

EditorUI::EditorUI(SDL_Window* window, bool chinese_font_available, const std::filesystem::path& locale_directory)
    : window_(window), localization_(locale_directory),
      chinese_available_(chinese_font_available && localization_.has_chinese()) {
    std::string language;
    std::ifstream("pre_language.ini") >> language;
    chinese_ = chinese_available_ && language == "zh-CN";
}

void EditorUI::choose_sprite_png(const SceneDocument& scene) {
    if (sprite_dialog_) return;
    sprite_import_error_.clear();
    sprite_dialog_ = std::make_shared<SpriteDialog>();
    sprite_dialog_->scene = scene.path;
    sprite_dialog_->entity = selected_id_;
    static constexpr SDL_DialogFileFilter filters[] = {{"PNG", "png"}};
    // SDL may complete the dialog on another thread, so transfer only the result.
    SDL_ShowOpenFileDialog([](void* userdata, const char* const* files, int) {
        const std::unique_ptr<std::shared_ptr<SpriteDialog>> holder(
            static_cast<std::shared_ptr<SpriteDialog>*>(userdata));
        auto& result = **holder;
        std::lock_guard lock(result.mutex);
        if (!files) result.error = SDL_GetError();
        else if (files[0]) result.filename = files[0];
        result.done = true;
    }, new std::shared_ptr<SpriteDialog>(sprite_dialog_), window_, filters, 1, nullptr, false);
}

void EditorUI::finish_sprite_import(SceneDocument& scene, PixelViewport& pixels) {
    const auto result = sprite_dialog_;
    if (!result) return;
    std::lock_guard lock(result->mutex);
    if (!result->done) return;
    sprite_dialog_.reset();
    sprite_import_error_ = result->error;
    if (result->filename.empty()) return;
    const auto entity = scene.find_entity(result->entity);
    if (scene.path != result->scene || !entity || scene.data["entities"][*entity]["type"] != "sprite") {
        sprite_import_error_ = tr("sprite.selection-changed");
        return;
    }
    const auto source = std::filesystem::path(std::u8string(result->filename.begin(), result->filename.end()));
    if (auto ref = import_sprite_png(source, scene.path, sprite_import_error_)) {
        finish_edit(scene);
        scene.set_entity_field(result->entity, "sprite_image", *ref);
        pixels.invalidate_assets();
    }
}

void EditorUI::track_edit(SceneDocument& scene, EditGesture gesture, bool active) {
    if (!active) {
        if (edit_gesture_ == gesture) finish_edit(scene);
        return;
    }
    if (edit_gesture_ != gesture) {
        finish_edit(scene);
        scene.begin_edit();
        edit_gesture_ = gesture;
    }
    edit_gesture_seen_ = true;
}

void EditorUI::finish_edit(SceneDocument& scene) {
    if (edit_gesture_ == EditGesture::none) return;
    scene.end_edit();
    edit_gesture_ = EditGesture::none;
}

void EditorUI::language_menu() {
    if (!ImGui::BeginMenu(label("language").c_str())) return;
    bool changed = false;
    if (ImGui::MenuItem(tr("language.en"), nullptr, !chinese_)) {
        chinese_ = false;
        changed = true;
    }
    if (ImGui::MenuItem(tr(chinese_available_ ? "language.zh-native" : "language.zh"),
                        nullptr,
                        chinese_, chinese_available_)) {
        chinese_ = true;
        changed = true;
    }
    if (!chinese_available_) ImGui::TextDisabled("%s", tr("language.zh-unavailable"));
    if (changed) std::ofstream("pre_language.ini") << (chinese_ ? "zh-CN" : "en") << '\n';
    ImGui::EndMenu();
}

void EditorUI::release_history() {
    if (!release_history_open_) return;
    static const auto history = nlohmann::json::parse(release_history_json);
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(main_viewport->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(620.0f, 420.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(label("release.history").c_str(), &release_history_open_)) {
        ImGui::Text("%s %s", tr("release.running"), version_label);
        ImGui::SameLine();
        if (ImGui::SmallButton(label("release.copy").c_str())) ImGui::SetClipboardText(version_label);
        ImGui::Separator();

        for (const auto& release : history) {
            const auto& number = release.at("version").get_ref<const std::string&>();
            const auto& date = release.at("date").get_ref<const std::string&>();
            const auto& title = release.at("title").get_ref<const std::string&>();
            const bool current = number == version;
            std::string heading = "v" + number + "   " + date + "   " + tr(title.c_str());
            if (current) heading += std::string("   [") + tr("release.current") + "]";
            heading += "###release-" + number;
            const auto flags = current ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None;
            if (ImGui::CollapsingHeader(heading.c_str(), flags)) {
                for (const auto& item : release.at("changes")) {
                    ImGui::Bullet();
                    ImGui::SameLine();
                    ImGui::TextWrapped("%s", tr(item.get_ref<const std::string&>().c_str()));
                }
                ImGui::Spacing();
            }
        }
    }
    ImGui::End();
}

std::string EditorUI::entity_type_label(const std::string& type) const {
    const std::string key = "entity." + type;
    const char* text = tr(key.c_str());
    return text == key.c_str() ? type : text;
}

void EditorUI::hierarchy(const SceneDocument& scene) {
    ImGui::Begin(label("hierarchy").c_str());
    for (const auto& entity : scene.data["entities"]) {
        const std::string id = entity.value("id", "");
        std::string display = entity.value("name", id) + "  [" + entity_type_label(entity.value("type", "?")) + "]";
        if (ImGui::Selectable((display + "##" + id).c_str(), selected_id_ == id)) selected_id_ = id;
    }
    ImGui::End();
}
void EditorUI::inspector(SceneDocument& scene) {
    if (!ImGui::Begin(label("inspector").c_str())) { ImGui::End(); return; }
    auto index = scene.find_entity(selected_id_);
    if (!index) { ImGui::TextDisabled("%s", tr("inspector.select")); ImGui::End(); return; }
    auto& entity = scene.data["entities"][*index];
    ImGui::Text("%s %s", tr("inspector.id"), selected_id_.c_str());
    ImGui::Text("%s %s", tr("inspector.type"), entity_type_label(entity["type"].get<std::string>()).c_str());
    if (const auto* live = runtime_.scene()) {
        const auto& pos = live->data["entities"][*index]["position"];
        ImGui::Text("%s %.2f, %.2f, %.2f", tr("position"),
                    pos[0].get<double>(), pos[1].get<double>(), pos[2].get<double>());
        ImGui::TextDisabled("%s", tr("runtime.read-only"));
        ImGui::End();
        return;
    }
    ImGui::PushID(selected_id_.c_str());
    char name[128]{};
    std::snprintf(name, sizeof(name), "%s", entity.value("name", "").c_str());
    const bool name_changed = ImGui::InputText(label("name").c_str(), name, sizeof(name));
    track_edit(scene, EditGesture::name, ImGui::IsItemActive() || name_changed);
    if (name_changed) scene.set_entity_field(selected_id_, "name", name);
    if (!ImGui::IsItemActive()) track_edit(scene, EditGesture::name, false);
    float p[3] = {entity["position"][0].get<float>(),entity["position"][1].get<float>(),entity["position"][2].get<float>()};
    const bool position_changed = ImGui::DragFloat3(label("position").c_str(), p, .05f);
    track_edit(scene, EditGesture::position, ImGui::IsItemActive() || position_changed);
    if (position_changed) scene.set_entity_field(selected_id_, "position", {p[0], p[1], p[2]});
    if (!ImGui::IsItemActive()) track_edit(scene, EditGesture::position, false);
    const auto attached = entity.value("script", "");
    char script[256]{};
    std::snprintf(script, sizeof(script), "%s", attached.c_str());
    if (ImGui::InputTextWithHint(label("runtime.script").c_str(), "scripts/example.lua", script,
                                 sizeof(script), ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (*script && !is_script_reference(script)) {
            invalid_script_entity_ = selected_id_;
        } else {
            invalid_script_entity_.clear();
            if (attached != script) {
                finish_edit(scene);
                if (*script) scene.set_entity_field(selected_id_, "script", script);
                else scene.erase_entity_field(selected_id_, "script");
            }
        }
    }
    ImGui::TextDisabled("%s", tr("runtime.script-hint"));
    if (invalid_script_entity_ == selected_id_) ImGui::TextWrapped("%s", tr("runtime.script-invalid"));
    ImGui::PopID();
    if (entity["type"] == "sprite") {
        const std::string reference = entity.value("sprite_image", tr("sprite.built-in"));
        ImGui::TextWrapped("%s %s", tr("sprite.image"), reference.c_str());
        ImGui::BeginDisabled(sprite_dialog_ != nullptr);
        if (ImGui::Button(label("sprite.import").c_str())) choose_sprite_png(scene);
        ImGui::EndDisabled();
        if (entity.contains("sprite_image")) {
            ImGui::SameLine();
            if (ImGui::Button(label("sprite.reset").c_str())) {
                finish_edit(scene);
                scene.erase_entity_field(selected_id_, "sprite_image");
            }
        }
    }
    ImGui::TextDisabled("%s", tr("inspector.preview"));
    ImGui::End();
}
void EditorUI::render_settings(SceneDocument& scene) {
    if (!ImGui::Begin(label("render-settings").c_str())) { ImGui::End(); return; }
    ImGui::BeginDisabled(playing());
    auto& r=scene.data["render"];
    float bg[4] = {r["clear_color"][0].get<float>(),r["clear_color"][1].get<float>(),r["clear_color"][2].get<float>(),r["clear_color"][3].get<float>()};
    const bool color_changed = ImGui::ColorEdit4(label("clear-color").c_str(), bg);
    if (!playing()) {
        track_edit(scene, EditGesture::clear_color, ImGui::IsItemActive() || color_changed);
        if (color_changed) scene.set_render_field("clear_color", {bg[0], bg[1], bg[2], bg[3]});
        if (!ImGui::IsItemActive()) track_edit(scene, EditGesture::clear_color, false);
    }
    int res[2]={r["pixel_resolution"][0].get<int>(),r["pixel_resolution"][1].get<int>()};
    const bool resolution_changed = ImGui::InputInt2(label("pixel-resolution").c_str(), res);
    if (!playing()) {
        track_edit(scene, EditGesture::resolution, ImGui::IsItemActive() || resolution_changed);
        if (resolution_changed && res[0] > 0 && res[1] > 0 &&
            res[0] <= PixelViewport::max_size && res[1] <= PixelViewport::max_size) {
            scene.set_render_field("pixel_resolution", {res[0], res[1]});
        }
        if (!ImGui::IsItemActive()) track_edit(scene, EditGesture::resolution, false);
    }
    ImGui::Checkbox(label("lua-preview").c_str(), &lua_preview_enabled_);
    ImGui::EndDisabled();
    ImGui::TextDisabled("%s", tr("render.roadmap"));
    ImGui::End();
}
std::array<float, 4> EditorUI::viewport_background(const SceneDocument& scene, const LuaPreview& lua) const {
    if (!playing() && lua_preview_enabled_ && lua.color()) return *lua.color();
    const auto& color = scene.data["render"]["clear_color"];
    return {color[0].get<float>(), color[1].get<float>(), color[2].get<float>(), 1.0f};
}

void EditorUI::viewport(SceneDocument& scene, PixelViewport& pixels, float frame_ms) {
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        track_edit(scene, EditGesture::brush, false);
        last_brush_cell_.reset();
    }
    ImGui::SetNextWindowSize(ImVec2(640.0f, 385.0f), ImGuiCond_FirstUseEver);
    if (focus_runtime_viewport_) {
        ImGui::SetNextWindowFocus();
        focus_runtime_viewport_ = false;
    }
    const bool visible = ImGui::Begin(label("viewport").c_str());
    const auto* live = runtime_.scene();
    if (live) {
        RuntimeInput input;
        const auto& io = ImGui::GetIO();
        if (visible && ImGui::IsWindowFocused() && (SDL_GetWindowFlags(window_) & SDL_WINDOW_INPUT_FOCUS) &&
            !io.WantTextInput && !io.KeyCtrl && !io.KeyAlt && !io.KeySuper) {
            for (int i = 0; i < 26; ++i)
                input.down[i] = ImGui::IsKeyDown(static_cast<ImGuiKey>(ImGuiKey_A + i));
            constexpr ImGuiKey other_keys[] = {
                ImGuiKey_LeftArrow, ImGuiKey_RightArrow, ImGuiKey_UpArrow, ImGuiKey_DownArrow, ImGuiKey_Space
            };
            for (std::size_t i = 0; i < std::size(other_keys); ++i)
                input.down[26 + i] = ImGui::IsKeyDown(other_keys[i]);
        }
        runtime_.tick(frame_ms / 1000.0f, input);
    }
    if (visible) {
        const auto& resolution = scene.data["render"]["pixel_resolution"];
        if (!pixels.resize(resolution[0].get<int>(), resolution[1].get<int>())) {
            ImGui::TextWrapped("%s", tr("viewport.error"));
            ImGui::TextWrapped("%s", pixels.error().c_str());
        } else {
            pixels.prepare(live ? *live : scene);
            ImGui::Text("%s  %d × %d", tr("viewport.gpu"), pixels.width(), pixels.height());
            if (live) {
                ImGui::TextDisabled("%s", tr("runtime.input-hint"));
            } else {
                const auto tool = [&](const char* key, std::optional<TileKind> kind) {
                    if (ImGui::RadioButton(label(key).c_str(), brush_ == kind)) {
                        brush_ = kind;
                        last_brush_cell_.reset();
                    }
                };
                tool("tile.select", std::nullopt);
                ImGui::SameLine();
                tool("tile.floor", TileKind::floor);
                ImGui::SameLine();
                tool("tile.wall", TileKind::wall);
                ImGui::SameLine();
                tool("tile.erase", TileKind::empty);
            }
            const ImVec2 avail = ImGui::GetContentRegionAvail();
            const float width = static_cast<float>(pixels.width());
            const float height = static_cast<float>(pixels.height());
            float scale = std::min(avail.x / width, avail.y / height);
            if (scale >= 1.0f) scale = std::floor(scale);
            if (scale > 0.0f) {
                const auto canvas = viewport_canvas(ImVec2(width * scale, height * scale));
                const auto& platform = ImGui::GetPlatformIO();
                auto* draw = ImGui::GetWindowDrawList();
                if (platform.DrawCallback_SetSamplerNearest && platform.DrawCallback_SetSamplerLinear)
                    draw->AddCallback(platform.DrawCallback_SetSamplerNearest, nullptr);
                draw->AddImage(static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(pixels.texture())),
                               canvas.min, canvas.max);
                if (platform.DrawCallback_SetSamplerNearest && platform.DrawCallback_SetSamplerLinear)
                    draw->AddCallback(platform.DrawCallback_SetSamplerLinear, nullptr);
                const auto top = canvas.min;
                const auto bottom = canvas.max;
                const auto outline = [&](const SpriteBounds& b, ImU32 color) {
                    const float x = top.x + static_cast<float>(b.x) * scale;
                    const float y = top.y + static_cast<float>(b.y) * scale;
                    draw->AddRect(ImVec2(x, y), ImVec2(x + b.width * scale, y + b.height * scale), color);
                };
                draw->PushClipRect(top, bottom, true);
                if (!live && brush_) {
                    if (canvas.held && edit_gesture_ == EditGesture::brush)
                        track_edit(scene, EditGesture::brush, true);
                    std::optional<TileCell> cell;
                    if (canvas.hovered) {
                        const auto mouse = ImGui::GetMousePos();
                        cell = pixels.tile_at((mouse.x - top.x) / scale, (mouse.y - top.y) / scale);
                    }
                    if (cell) {
                        outline(pixels.tile_bounds(*cell), IM_COL32(122, 221, 246, 255));
                        if ((canvas.pressed || canvas.held) && cell != last_brush_cell_) {
                            track_edit(scene, EditGesture::brush, true);
                            if (scene.paint_tiles(last_brush_cell_.value_or(*cell), *cell, *brush_))
                                pixels.prepare(scene);
                            last_brush_cell_ = cell;
                        }
                    } else {
                        last_brush_cell_.reset();
                    }
                } else if (canvas.pressed) {
                    const auto mouse = ImGui::GetMousePos();
                    const auto picked = pixels.pick((mouse.x - top.x) / scale, (mouse.y - top.y) / scale);
                    if (!picked.empty()) selected_id_ = picked;
                }
                if (!live && !brush_) if (const auto selection = pixels.bounds(selected_id_))
                    outline(*selection, IM_COL32(255, 208, 105, 255));
                draw->PopClipRect();
            }
        }
    }
    ImGui::End();
}
void EditorUI::draw(SceneDocument& scene, const LuaPreview& lua, PixelViewport& pixels,
                    float frame_ms, const char* gpu_driver) {
    edit_gesture_seen_ = false;
    if (!playing()) finish_sprite_import(scene, pixels);
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
    bool confirm_reload = false;
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu(label("file").c_str())) {
            if (ImGui::MenuItem(tr("file.save"), "Ctrl+S", false, !playing() && scene.dirty)) scene.save();
            if (ImGui::MenuItem(tr("file.reload"), nullptr, false, !playing())) {
                finish_edit(scene);
                invalid_script_entity_.clear();
                if (scene.dirty) confirm_reload = true;
                else if (scene.load(scene.path)) pixels.invalidate_assets();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(label("edit").c_str())) {
            if (ImGui::MenuItem(tr("edit.undo"), "Ctrl+Z", false, !playing() && scene.can_undo())) {
                finish_edit(scene);
                scene.undo();
            }
            if (ImGui::MenuItem(tr("edit.redo"), "Ctrl+Y", false, !playing() && scene.can_redo())) {
                finish_edit(scene);
                scene.redo();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(label("view").c_str())) { ImGui::MenuItem(tr("view.demo"),nullptr,&demo_open_); ImGui::EndMenu(); }
        language_menu();
        if (ImGui::BeginMenu(label("help").c_str())) {
            if (ImGui::MenuItem(tr("release.history"))) release_history_open_ = true;
            ImGui::EndMenu();
        }
        if (ImGui::SmallButton(version_label)) release_history_open_ = true;
        ImGui::BeginDisabled(sprite_dialog_ != nullptr);
        if (ImGui::SmallButton(label(playing() ? "runtime.stop" : "runtime.play").c_str())) {
            if (playing()) runtime_.stop();
            else {
                finish_edit(scene);
                runtime_.play(scene);
                focus_runtime_viewport_ = true;
            }
            last_brush_cell_.reset();
        }
        ImGui::EndDisabled();
        ImGui::EndMainMenuBar();
    }
    // Shortcuts run only when text widgets are not handling keyboard input.
    const auto& io = ImGui::GetIO();
    if (!playing() && !io.WantTextInput && !ImGui::IsAnyItemActive() &&
        io.KeyCtrl && !io.KeyAlt && !io.KeySuper) {
        if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
            finish_edit(scene);
            if (io.KeyShift) scene.redo(); else scene.undo();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
            finish_edit(scene);
            scene.redo();
        }
    }
    const auto reload_dialog = label("reload.confirm");
    if (confirm_reload) ImGui::OpenPopup(reload_dialog.c_str());
    if (ImGui::BeginPopupModal(reload_dialog.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(tr("reload.unsaved"));
        if (ImGui::Button(tr("reload.cancel"))) ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button(tr("reload.discard"))) {
            if (scene.load(scene.path)) pixels.invalidate_assets();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    hierarchy(scene);
    render_settings(scene);
    viewport(scene, pixels, frame_ms);
    inspector(scene);
    if (!edit_gesture_seen_) finish_edit(scene);
    ImGui::Begin(label("diagnostics").c_str());
    ImGui::Text("%s %.2f ms | FPS: %.1f", tr("diag.frame"), frame_ms, frame_ms>0.0f?1000.0f/frame_ms:0.0f);
    ImGui::Text("%s %s", tr("diag.gpu"),gpu_driver);
    ImGui::Text("%s %s%s", tr("diag.scene"),scene.path.string().c_str(),scene.dirty?tr("diag.unsaved"):"");
    if (scene.save_conflict) {
        ImGui::TextWrapped("%s", tr("diag.save-conflict"));
    } else if (!scene.error.empty()) {
        ImGui::TextColored(ImVec4(1,.35f,.3f,1), "%s %s", tr("diag.error"), scene.error.c_str());
    }
    if (!sprite_import_error_.empty()) {
        ImGui::TextWrapped("%s %s", tr("sprite.import-error"), sprite_import_error_.c_str());
    }
    if (!pixels.asset_error().empty()) {
        ImGui::TextWrapped("%s %s", tr("sprite.asset-error"), pixels.asset_error().c_str());
    }
    ImGui::Separator();
    if (playing()) {
        ImGui::TextUnformatted(tr("runtime.running"));
        for (const auto& error : runtime_.errors())
            ImGui::TextWrapped("%s %s", tr("runtime.error"), error.c_str());
        ImGui::Separator();
    }
    ImGui::TextUnformatted("scenes/preview.lua");
    if (!lua.available()) {
        ImGui::TextDisabled("%s", tr("lua.missing"));
    } else if (!lua.error().empty()) {
        ImGui::TextWrapped("%s", tr("lua.failed"));
        ImGui::TextWrapped("%s", lua.error().c_str());
    } else {
        ImGui::TextDisabled("%s", tr("lua.ready"));
    }
    ImGui::TextDisabled("%s", tr("diag.cli"));
    ImGui::End();
    release_history();
    if (demo_open_) ImGui::ShowDemoWindow(&demo_open_);
}
}
