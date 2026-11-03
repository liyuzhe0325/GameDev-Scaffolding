#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace pre {
class LuaPreview {
public:
    void poll(std::uint64_t now_ms);

    const std::optional<std::array<float, 4>>& color() const { return color_; }
    const std::string& error() const { return error_; }
    bool available() const { return available_; }

private:
    std::uint64_t next_poll_ms_ = 0;
    bool available_ = false;
    std::optional<std::string> last_source_;
    std::optional<std::array<float, 4>> color_;
    std::string error_;
};
}
