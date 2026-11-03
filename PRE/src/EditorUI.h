#pragma once
#include "SceneDocument.h"
#include "Localization.h"
#include "SceneRuntime.h"
#include <array>
#include <memory>
#include <string>
struct SDL_Window;
namespace pre {
class LuaPreview;
class PixelViewport;
class EditorUI {
public:
    EditorUI(SDL_Window* window, bool chinese_font_available, const std::filesystem::path& locale_directory);
    const char* window_title() const { return tr("window.title"); }
    bool chinese() const { return chinese_; }
    bool playing() const { return runtime_.scene() != nullptr; }
    void draw(SceneDocument& scene, const LuaPreview& lua, PixelViewport& pixels,
              float frame_ms, const char* gpu_driver);
    std::array<float, 4> viewport_background(const SceneDocument& scene, const LuaPreview& lua) const;
private:
    struct SpriteDialog;
    SDL_Window* window_;
    Localization localization_;
    bool chinese_available_;
    bool chinese_ = false;
    std::string selected_id_ = "hero_01";
    bool demo_open_ = false;
    bool lua_preview_enabled_ = true;
    bool release_history_open_ = false;
    SceneRuntime runtime_;
    bool focus_runtime_viewport_ = false;
    std::string invalid_script_entity_;
    std::optional<TileKind> brush_;
    std::optional<TileCell> last_brush_cell_;
    enum class EditGesture { none, brush, name, position, clear_color, resolution };
    EditGesture edit_gesture_ = EditGesture::none;
    bool edit_gesture_seen_ = false;
    std::shared_ptr<SpriteDialog> sprite_dialog_;
    std::string sprite_import_error_;
    const char* tr(const char* key) const { return localization_.text(key, chinese_); }
    std::string label(const char* key) const { return std::string(tr(key)) + "###" + key; }
    std::string entity_type_label(const std::string& type) const;
    void language_menu();
    void choose_sprite_png(const SceneDocument& scene);
    void finish_sprite_import(SceneDocument& scene, PixelViewport& pixels);
    void track_edit(SceneDocument& scene, EditGesture gesture, bool active);
    void finish_edit(SceneDocument& scene);
    void release_history();
    void viewport(SceneDocument& scene, PixelViewport& pixels, float frame_ms);
    void hierarchy(const SceneDocument& scene);
    void inspector(SceneDocument& scene);
    void render_settings(SceneDocument& scene);
};
}
