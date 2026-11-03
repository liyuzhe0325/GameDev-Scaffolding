#include "LuaPreview.h"
#include <cmath>
#include <fstream>
#include <memory>
#include <lua.hpp>

namespace pre {
namespace {
constexpr char script_path[] = "scenes/preview.lua";
constexpr std::size_t max_source_size = 64 * 1024;

void stop_long_script(lua_State* state, lua_Debug*) {
    luaL_error(state, "Instruction limit exceeded");
}
}

void LuaPreview::poll(std::uint64_t now_ms) {
    if (now_ms < next_poll_ms_) return;
    next_poll_ms_ = now_ms + 400;

    std::ifstream input(script_path, std::ios::binary);
    available_ = input.is_open();
    if (!available_) return;

    std::string source(max_source_size + 1, '\0');
    input.read(source.data(), static_cast<std::streamsize>(source.size()));
    if (input.bad() || static_cast<std::size_t>(input.gcount()) > max_source_size) {
        error_ = "Script could not be read or exceeds 64 KiB";
        last_source_.reset();
        return;
    }
    source.resize(static_cast<std::size_t>(input.gcount()));
    if (last_source_ == source) return;
    last_source_ = source;

    // Evaluate in a fresh state with only Lua language primitives.
    // Commit the preview only after the complete result has been validated.
    std::unique_ptr<lua_State, decltype(&lua_close)> state(luaL_newstate(), lua_close);
    if (!state) {
        error_ = "Could not initialize Lua";
        return;
    }
    auto* lua = state.get();
    lua_sethook(lua, stop_long_script, LUA_MASKCOUNT, 50'000);
    if (luaL_loadbufferx(lua, source.data(), source.size(), "@scenes/preview.lua", "t") != LUA_OK ||
        lua_pcall(lua, 0, 1, 0) != LUA_OK) {
        const char* message = lua_tostring(lua, -1);
        error_ = message ? message : "Lua script failed";
        return;
    }

    if (lua_isnil(lua, -1)) {
        color_.reset();
        error_.clear();
        return;
    }
    std::array<float, 4> candidate{};
    if (lua_type(lua, -1) != LUA_TTABLE || lua_rawlen(lua, -1) != candidate.size()) {
        error_ = "Return a table with four RGBA numbers";
        return;
    }
    for (std::size_t i = 0; i < candidate.size(); ++i) {
        const int type = lua_rawgeti(lua, -1, static_cast<lua_Integer>(i + 1));
        const double value = lua_tonumber(lua, -1);
        lua_pop(lua, 1);
        if (type != LUA_TNUMBER || !std::isfinite(value) || value < 0.0 || value > 1.0) {
            error_ = "RGBA values must be numbers from 0 to 1";
            return;
        }
        candidate[i] = static_cast<float>(value);
    }
    color_ = candidate;
    error_.clear();
}
}
