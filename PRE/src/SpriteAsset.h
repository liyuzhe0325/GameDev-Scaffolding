#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
namespace pre {
inline bool is_sprite_reference(std::string_view ref) {
    if (!ref.starts_with("assets/") || !ref.ends_with(".png") || ref.size() <= 11) return false;
    for (const unsigned char c : ref.substr(7)) {
        if (c < 32 || std::string_view("/\\:*?\"<>|").find(static_cast<char>(c)) != std::string_view::npos)
            return false;
    }
    return true;
}
struct SpriteImage {
    int width;
    int height;
    std::vector<std::uint8_t> rgba;
};
std::optional<SpriteImage> load_sprite_png(const std::filesystem::path& path, std::string& error);
std::filesystem::path resolve_sprite_asset(const std::filesystem::path& scene, std::string_view ref);
std::optional<std::string> import_sprite_png(const std::filesystem::path& source,
                                             const std::filesystem::path& scene, std::string& error);
}