#include "SpriteAsset.h"

#include <fstream>
#include <memory>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_MAX_DIMENSIONS 2048
#include "stb_image.h"

namespace pre {
namespace {
constexpr auto max_file_bytes = 16 * 1024 * 1024;

std::filesystem::path asset_directory(const std::filesystem::path& scene) {
    if (scene.empty()) throw std::runtime_error("Save or open a scene before importing");
    const auto root = std::filesystem::canonical(std::filesystem::absolute(scene).parent_path());
    const auto directory = root / "assets";
    if (std::filesystem::weakly_canonical(directory) != directory)
        throw std::runtime_error("The scene asset folder cannot redirect outside the scene");
    return directory;
}

std::vector<std::uint8_t> read_png_bytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    const auto length = file.tellg();
    if (!file || length <= 0 || length > max_file_bytes)
        throw std::runtime_error("PNG file is missing, empty, or larger than 16 MiB");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!file) throw std::runtime_error("Cannot read PNG file");
    return bytes;
}

SpriteImage decode_png(const std::vector<std::uint8_t>& bytes) {
    int width = 0, height = 0, channels = 0;
    std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
        stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &width, &height, &channels, 4),
        stbi_image_free);
    if (!pixels) throw std::runtime_error("Invalid PNG or dimensions exceed 2048 pixels per axis");
    const auto count = static_cast<std::size_t>(width) * height * 4;
    return {width, height, {pixels.get(), pixels.get() + count}};
}
}

std::optional<SpriteImage> load_sprite_png(const std::filesystem::path& path, std::string& error) {
    try {
        auto image = decode_png(read_png_bytes(path));
        error.clear();
        return image;
    } catch (const std::exception& failure) {
        error = failure.what();
        return std::nullopt;
    }
}

std::filesystem::path resolve_sprite_asset(const std::filesystem::path& scene, std::string_view ref) {
    if (!is_sprite_reference(ref)) throw std::runtime_error("Invalid sprite asset reference");
    const auto directory = asset_directory(scene);
    const auto name = ref.substr(7);
    const auto path = directory / std::filesystem::path(std::u8string(name.begin(), name.end()));
    if (std::filesystem::weakly_canonical(path).parent_path() != directory)
        throw std::runtime_error("Sprite asset points outside the scene asset folder");
    return path;
}

std::optional<std::string> import_sprite_png(const std::filesystem::path& source,
                                             const std::filesystem::path& scene, std::string& error) {
    try {
        (void)decode_png(read_png_bytes(source));
        const auto directory = asset_directory(scene);
        std::filesystem::create_directory(directory);
        const auto stem = source.stem().u8string();
        auto name = stem + u8".png";
        for (int suffix = 2; suffix < 10000; ++suffix) {
            const auto destination = directory / std::filesystem::path(name);
            const std::string reference = "assets/" + std::string(name.begin(), name.end());
            if (!is_sprite_reference(reference)) throw std::runtime_error("Invalid PNG file name");
            std::error_code copy_error;
            if (std::filesystem::copy_file(source, destination, std::filesystem::copy_options::none, copy_error)) {
                error.clear();
                return reference;
            }
            if (copy_error != std::errc::file_exists)
                throw std::filesystem::filesystem_error("Cannot copy sprite asset", destination, copy_error);
            if (std::filesystem::equivalent(source, destination)) {
                resolve_sprite_asset(scene, reference);
                error.clear();
                return reference;
            }
            const auto number = std::to_string(suffix);
            name = stem + u8"-" + std::u8string(number.begin(), number.end()) + u8".png";
        }
        throw std::runtime_error("Too many assets with the same name");
    } catch (const std::exception& failure) {
        error = failure.what();
        return std::nullopt;
    }
}
}
