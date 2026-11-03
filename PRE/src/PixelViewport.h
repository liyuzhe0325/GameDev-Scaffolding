#pragma once
#include "Tilemap.h"

#include <SDL3/SDL_gpu.h>
#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pre {
struct SceneDocument;

struct SpriteBounds {
    double x;
    double y;
    int width;
    int height;
};

class PixelViewport {
public:
    explicit PixelViewport(SDL_GPUDevice* gpu);
    ~PixelViewport();
    PixelViewport(const PixelViewport&) = delete;
    PixelViewport& operator=(const PixelViewport&) = delete;

    bool resize(int width, int height);
    void prepare(const SceneDocument& scene);
    void record(SDL_GPUCommandBuffer* cmd, const std::array<float, 4>& background) const;
    std::string_view pick(double x, double y) const;
    std::optional<SpriteBounds> bounds(std::string_view id) const;
    std::optional<TileCell> tile_at(double x, double y) const;
    SpriteBounds tile_bounds(TileCell cell) const;
    void invalidate_assets();
    SDL_GPUTexture* texture() const { return target_; }
    int width() const { return width_; }
    int height() const { return height_; }
    const std::string& error() const { return error_; }
    const std::string& asset_error() const { return asset_error_; }
    static constexpr int max_size = 4096;

private:
    struct Texture {
        SDL_GPUTexture* handle = nullptr;
        int width = 0;
        int height = 0;
        std::string error;
    };
    struct DrawSprite {
        std::string id;
        SDL_GPUTexture* texture;
        SpriteBounds bounds;
    };
    struct DrawTile {
        SDL_GPUTexture* texture;
        SpriteBounds bounds;
    };
    const Texture& load_texture(const std::string& ref);

    SDL_GPUDevice* gpu_;
    SDL_GPUGraphicsPipeline* pipeline_ = nullptr;
    SDL_GPUTexture* sprite_ = nullptr;
    SDL_GPUTexture* floor_ = nullptr;
    SDL_GPUTexture* wall_ = nullptr;
    SDL_GPUTexture* target_ = nullptr;
    SDL_GPUSampler* sampler_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    double tile_origin_x_ = 0;
    double tile_origin_y_ = 0;
    std::string error_;
    std::string asset_error_;
    std::filesystem::path scene_path_;
    std::unordered_map<std::string, Texture> assets_;
    std::vector<DrawSprite> sprites_;
    std::vector<DrawTile> tiles_;
};
}
