#include "SceneRuntime.h"
#include "ScriptReference.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <lua.hpp>
#include <stdexcept>

namespace pre {
namespace {
constexpr std::size_t source_limit = 64 * 1024;
constexpr std::size_t memory_limit = 2 * 1024 * 1024;

void* allocate(void* context, void* pointer, std::size_t old_size, std::size_t new_size) {
    auto& used = *static_cast<std::size_t*>(context);
    if (!pointer) old_size = 0; // Lua passes a type tag for a fresh allocation.
    if (new_size == 0) {
        std::free(pointer);
        used -= old_size;
        return nullptr;
    }
    if (new_size > memory_limit - (used - old_size)) return nullptr;
    void* replacement = std::realloc(pointer, new_size);
    if (replacement) used = used - old_size + new_size;
    return replacement;
}

void instruction_limit(lua_State* state, lua_Debug*) {
    luaL_error(state, "Instruction limit exceeded");
}

std::string script_source(const std::filesystem::path& scene, const std::string& ref) {
    if (!is_script_reference(ref)) throw std::runtime_error("Invalid script reference");
    const auto root = std::filesystem::canonical(scene.parent_path().empty() ? "." : scene.parent_path());
    const auto file = std::filesystem::canonical(root / ref);
    if (file.parent_path() != root / "scripts" || !std::filesystem::is_regular_file(file))
        throw std::runtime_error("Script must be a regular file inside the scene's scripts directory");
    std::ifstream input(file, std::ios::binary);
    if (!input) throw std::runtime_error("Could not open script");
    std::string source(source_limit + 1, '\0');
    input.read(source.data(), static_cast<std::streamsize>(source.size()));
    const auto size = static_cast<std::size_t>(input.gcount());
    if (input.bad() || size > source_limit) throw std::runtime_error("Could not read script or exceeds 64 KiB");
    source.resize(size);
    return source;
}
}

struct SceneRuntime::Script {
    std::size_t used = 0;
    std::unique_ptr<lua_State, decltype(&lua_close)> state{
        lua_newstate(allocate, &used, luaL_makeseed(nullptr)), lua_close};
    std::size_t entity_index = 0;
    std::string description;
    RuntimeInput input;
    std::array<double, 3> position{};
    bool position_changed = false;

    static Script& self(lua_State* lua) {
        return **static_cast<Script**>(lua_getextraspace(lua));
    }

    static int read_position(lua_State* lua) {
        for (double value : self(lua).position) lua_pushnumber(lua, value);
        return 3;
    }

    static int set_position(lua_State* lua) {
        std::array<double, 3> next{};
        for (int i = 0; i < 3; ++i) {
            const double value = luaL_checknumber(lua, i + 1);
            if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<float>::max())
                return luaL_argerror(lua, i + 1, "expected a finite scene coordinate");
            next[i] = value;
        }
        auto& script = self(lua);
        script.position = next;
        script.position_changed = true;
        return 0;
    }

    static int key_down(lua_State* lua) {
        std::size_t length = 0;
        const char* name = luaL_checklstring(lua, 1, &length);
        const auto it = std::find(RuntimeInput::keys.begin(), RuntimeInput::keys.end(), std::string_view(name, length));
        if (it == RuntimeInput::keys.end()) return luaL_argerror(lua, 1, "unknown key");
        lua_pushboolean(lua, self(lua).input.down[it - RuntimeInput::keys.begin()]);
        return 1;
    }

    void install_api() {
        auto* lua = state.get();
        *static_cast<Script**>(lua_getextraspace(lua)) = this;
        luaL_requiref(lua, "math", luaopen_math, 1);
        lua_pop(lua, 1);
        static const luaL_Reg api[] = {
            {"position", read_position}, {"set_position", set_position},
            {"key_down", key_down}, {nullptr, nullptr}
        };
        luaL_newlib(lua, api);
        lua_setglobal(lua, "pre");
    }

    // Lookup and call under the same protected call, including allocation failures.
    static int dispatch(lua_State* lua) {
        const char* callback = static_cast<const char*>(lua_touserdata(lua, 1));
        if (lua_getglobal(lua, callback) == LUA_TNIL) return 0;
        if (!lua_isfunction(lua, -1)) return luaL_error(lua, "%s must be a function", callback);
        lua_pushvalue(lua, 2);
        lua_call(lua, 1, 0);
        return 0;
    }

    bool call(const char* callback, float seconds) {
        auto* lua = state.get();
        lua_sethook(lua, instruction_limit, LUA_MASKCOUNT, 50'000);
        lua_pushcfunction(lua, dispatch);
        lua_pushlightuserdata(lua, const_cast<char*>(callback));
        lua_pushnumber(lua, seconds);
        return lua_pcall(lua, 2, 0, 0) == LUA_OK;
    }

    void commit(nlohmann::json& entity) {
        if (position_changed) entity["position"] = position;
        position_changed = false;
    }

    std::string error() const {
        const char* message = lua_tostring(state.get(), -1);
        return description + ": " + (message ? message : "Lua script failed");
    }
};

SceneRuntime::SceneRuntime() = default;
SceneRuntime::~SceneRuntime() = default;

void SceneRuntime::play(const SceneDocument& source) {
    stop();
    // The runtime snapshot carries no saved revision and cannot overwrite the editor document.
    scene_.emplace();
    scene_->data = source.data;
    scene_->path = source.path;
    auto& entities = scene_->data["entities"];
    for (std::size_t index = 0; index < entities.size(); ++index) {
        auto& entity = entities[index];
        if (!entity.contains("script")) continue;
        const auto& ref = entity["script"].get_ref<const std::string&>();
        auto script = std::make_unique<Script>();
        script->entity_index = index;
        script->description = entity["id"].get<std::string>() + " [" + ref + "]";
        script->position = entity["position"].get<std::array<double, 3>>();
        if (!script->state) {
            errors_.push_back(script->description + ": Could not initialize Lua");
            continue;
        }
        std::string source_text;
        try {
            source_text = script_source(scene_->path, ref);
        } catch (const std::exception& error) {
            errors_.push_back(script->description + ": " + error.what());
            continue;
        }
        script->install_api();
        auto* lua = script->state.get();
        lua_sethook(lua, instruction_limit, LUA_MASKCOUNT, 50'000);
        const auto chunk = "@" + ref;
        if (luaL_loadbufferx(lua, source_text.data(), source_text.size(), chunk.c_str(), "t") != LUA_OK ||
            lua_pcall(lua, 0, 0, 0) != LUA_OK || !script->call("on_start", 0.0f)) {
            errors_.push_back(script->error());
            continue;
        }
        script->commit(entity);
        scripts_.push_back(std::move(script));
    }
}

void SceneRuntime::stop() {
    scripts_.clear();
    errors_.clear();
    scene_.reset();
}

void SceneRuntime::tick(float delta_seconds, const RuntimeInput& input) {
    if (!scene_ || !std::isfinite(delta_seconds) || delta_seconds <= 0.0f) return;
    // Window stalls and debugger pauses should not create huge movement steps.
    delta_seconds = std::min(delta_seconds, 0.1f);
    for (auto& script : scripts_) {
        if (!script) continue;
        script->input = input;
        if (!script->call("on_update", delta_seconds)) {
            errors_.push_back(script->error());
            script.reset();
            continue;
        }
        script->commit(scene_->data["entities"][script->entity_index]);
    }
}
}
