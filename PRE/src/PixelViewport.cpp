#include "PixelViewport.h"
#include "SceneDocument.h"
#include "SpriteAsset.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstring>

#ifdef _WIN32
#include "SpriteVertex.h"
#include "SpritePixel.h"
#endif

namespace pre {
namespace {
constexpr auto target_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
constexpr int sprite_size = 16;
constexpr double pixels_per_unit = 24.0;

#ifdef _WIN32
SDL_GPUGraphicsPipeline* create_pipeline(SDL_GPUDevice* gpu) {
    SDL_GPUShaderCreateInfo shader{};
    shader.format = SDL_GPU_SHADERFORMAT_DXIL;
    shader.code = sprite_vertex_dxil;
    shader.code_size = sizeof(sprite_vertex_dxil);
    shader.entrypoint = "vertex_main";
    shader.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    shader.num_uniform_buffers = 1;
    auto* vertex = SDL_CreateGPUShader(gpu, &shader);
    if (!vertex) return nullptr;

    shader.code = sprite_pixel_dxil;
    shader.code_size = sizeof(sprite_pixel_dxil);
    shader.entrypoint = "pixel_main";
    shader.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    shader.num_uniform_buffers = 0;
    shader.num_samplers = 1;
    auto* fragment = SDL_CreateGPUShader(gpu, &shader);
    SDL_GPUGraphicsPipeline* pipeline = nullptr;
    if (fragment) {
        SDL_GPUColorTargetDescription color{};
        color.format = target_format;
        color.blend_state.enable_blend = true;
        color.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        color.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        color.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
        color.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        color.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        color.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        SDL_GPUGraphicsPipelineCreateInfo info{};
        info.vertex_shader = vertex;
        info.fragment_shader = fragment;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.target_info.color_target_descriptions = &color;
        info.target_info.num_color_targets = 1;
        pipeline = SDL_CreateGPUGraphicsPipeline(gpu, &info);
        SDL_ReleaseGPUShader(gpu, fragment);
    }
    SDL_ReleaseGPUShader(gpu, vertex);
    return pipeline;
}
#endif

bool upload_pixels(SDL_GPUDevice* gpu, SDL_GPUTexture* texture, Uint32 width, Uint32 height, const Uint8* rgba) {
    SDL_GPUTransferBufferCreateInfo info{};
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    const Uint32 pitch = (width * 4 + 255u) & ~255u;
    info.size = pitch * height;
    auto* transfer = SDL_CreateGPUTransferBuffer(gpu, &info);
    if (!transfer) return false;
    auto* bytes = static_cast<Uint8*>(SDL_MapGPUTransferBuffer(gpu, transfer, false));
    if (!bytes) {
        SDL_ReleaseGPUTransferBuffer(gpu, transfer);
        return false;
    }
    for (Uint32 y = 0; y < height; ++y)
        std::memcpy(bytes + y * pitch, rgba + y * width * 4, width * 4);
    SDL_UnmapGPUTransferBuffer(gpu, transfer);
    auto* cmd = SDL_AcquireGPUCommandBuffer(gpu);
    if (!cmd) {
        SDL_ReleaseGPUTransferBuffer(gpu, transfer);
        return false;
    }
    auto* pass = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTextureTransferInfo source{};
    source.transfer_buffer = transfer;
    source.pixels_per_row = pitch / 4;
    source.rows_per_layer = height;
    SDL_GPUTextureRegion destination{};
    destination.texture = texture;
    destination.w = width;
    destination.h = height;
    destination.d = 1;
    SDL_UploadToGPUTexture(pass, &source, &destination, false);
    SDL_EndGPUCopyPass(pass);
    const bool submitted = SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(gpu, transfer);
    return submitted;
}

// Fallback for scenes created before assets were supported.
bool upload_example(SDL_GPUDevice* gpu, SDL_GPUTexture* texture) {
    constexpr char art[sprite_size][sprite_size + 1] = {
        "................", ".....kkkkkk.....", "....kssssssk....", "....kskssksk....",
        "....kssssssk....", ".....kkkkkk.....", "....kbbbbbkk....", "...kbbbbbbbbk...",
        "...ksbbbbbbsk...", "....kbbbbbbk....", ".....kbbbbk.....", ".....kkbbkk.....",
        ".....kk..kk.....", "....kkk..kkk....", "................", "................"
    };
    std::array<Uint8, sprite_size * sprite_size * 4> rgba{};
    for (int y = 0; y < sprite_size; ++y) {
        for (int x = 0; x < sprite_size; ++x) {
            const char cell = art[y][x];
            const std::array<Uint8, 4> color = cell == 's' ? std::array<Uint8, 4>{251, 204, 139, 255} :
                cell == 'b' ? std::array<Uint8, 4>{81, 191, 223, 255} :
                cell == 'k' ? std::array<Uint8, 4>{26, 32, 53, 255} : std::array<Uint8, 4>{0, 0, 0, 0};
            std::copy(color.begin(), color.end(), rgba.begin() + (y * sprite_size + x) * 4);
        }
    }
    return upload_pixels(gpu, texture, sprite_size, sprite_size, rgba.data());
}

SDL_GPUTexture* make_texture(SDL_GPUDevice* gpu, Uint32 width, Uint32 height, SDL_GPUTextureUsageFlags usage) {
    SDL_GPUTextureCreateInfo info{};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = target_format;
    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    info.usage = usage;
    return SDL_CreateGPUTexture(gpu, &info);
}

#ifdef _WIN32
SDL_GPUTexture* create_tile(SDL_GPUDevice* gpu, TileKind kind) {
    std::array<Uint8, tile_pixels * tile_pixels * 4> rgba{};
    const bool wall = kind == TileKind::wall;
    const std::array<Uint8, 4> fill = wall ? std::array<Uint8, 4>{145, 104, 70, 255}
                                          : std::array<Uint8, 4>{76, 91, 107, 255};
    const std::array<Uint8, 4> edge = wall ? std::array<Uint8, 4>{96, 66, 52, 255}
                                          : std::array<Uint8, 4>{48, 61, 76, 255};
    for (int y = 0; y < tile_pixels; ++y) {
        for (int x = 0; x < tile_pixels; ++x) {
            const bool seam = x == 0 || y == 0 ||
                (wall && (y == 7 || (y < 7 ? x == 5 : x == 11)));
            const auto& color = seam ? edge : fill;
            std::copy(color.begin(), color.end(), rgba.begin() + (y * tile_pixels + x) * 4);
        }
    }
    auto* texture = make_texture(gpu, tile_pixels, tile_pixels, SDL_GPU_TEXTUREUSAGE_SAMPLER);
    if (texture && !upload_pixels(gpu, texture, tile_pixels, tile_pixels, rgba.data())) {
        SDL_ReleaseGPUTexture(gpu, texture);
        return nullptr;
    }
    return texture;
}
#endif
}

PixelViewport::PixelViewport(SDL_GPUDevice* gpu) : gpu_(gpu) {
#ifdef _WIN32
    pipeline_ = create_pipeline(gpu);
    if (!pipeline_) { error_ = SDL_GetError(); return; }
    sprite_ = make_texture(gpu, sprite_size, sprite_size, SDL_GPU_TEXTUREUSAGE_SAMPLER);
    if (!sprite_ || !upload_example(gpu, sprite_)) { error_ = SDL_GetError(); return; }
    floor_ = create_tile(gpu, TileKind::floor);
    wall_ = create_tile(gpu, TileKind::wall);
    if (!floor_ || !wall_) { error_ = SDL_GetError(); return; }
    SDL_GPUSamplerCreateInfo info{};
    info.min_filter = SDL_GPU_FILTER_NEAREST;
    info.mag_filter = SDL_GPU_FILTER_NEAREST;
    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_ = SDL_CreateGPUSampler(gpu, &info);
    if (!sampler_) error_ = SDL_GetError();
#else
    error_ = "The sprite shader is currently available for Windows D3D12";
#endif
}

PixelViewport::~PixelViewport() {
    invalidate_assets();
    if (target_) SDL_ReleaseGPUTexture(gpu_, target_);
    if (sampler_) SDL_ReleaseGPUSampler(gpu_, sampler_);
    if (sprite_) SDL_ReleaseGPUTexture(gpu_, sprite_);
    if (floor_) SDL_ReleaseGPUTexture(gpu_, floor_);
    if (wall_) SDL_ReleaseGPUTexture(gpu_, wall_);
    if (pipeline_) SDL_ReleaseGPUGraphicsPipeline(gpu_, pipeline_);
}

bool PixelViewport::resize(int width, int height) {
    if (!pipeline_ || !sprite_ || !sampler_ || !floor_ || !wall_) return false;
    if (width < 1 || height < 1 || width > max_size || height > max_size) {
        error_ = "Viewport resolution must be between 1 and 4096 per axis";
        return false;
    }
    if (width == width_ && height == height_) { error_.clear(); return true; }
    auto* next = make_texture(gpu_, static_cast<Uint32>(width), static_cast<Uint32>(height),
                              SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
    if (!next) { error_ = SDL_GetError(); return false; }
    if (target_) SDL_ReleaseGPUTexture(gpu_, target_);
    target_ = next;
    width_ = width;
    height_ = height;
    error_.clear();
    return true;
}

void PixelViewport::invalidate_assets() {
    sprites_.clear();
    tiles_.clear();
    for (const auto& [ref, asset] : assets_)
        if (asset.handle) SDL_ReleaseGPUTexture(gpu_, asset.handle);
    assets_.clear();
    asset_error_.clear();
}

const PixelViewport::Texture& PixelViewport::load_texture(const std::string& ref) {
    auto [entry, fresh] = assets_.try_emplace(ref);
    auto& result = entry->second;
    if (!fresh) return result;
    try {
        const auto path = resolve_sprite_asset(scene_path_, ref);
        auto image = load_sprite_png(path, result.error);
        if (!image) return result;
        result.width = image->width;
        result.height = image->height;
        result.handle = make_texture(gpu_, static_cast<Uint32>(image->width), static_cast<Uint32>(image->height),
                                     SDL_GPU_TEXTUREUSAGE_SAMPLER);
        if (!result.handle || !upload_pixels(gpu_, result.handle, image->width, image->height, image->rgba.data())) {
            result.error = SDL_GetError();
            if (result.handle) SDL_ReleaseGPUTexture(gpu_, result.handle);
            result.handle = nullptr;
        }
    } catch (const std::exception& failure) {
        result.error = failure.what();
    }
    return result;
}

void PixelViewport::prepare(const SceneDocument& scene) {
    if (scene.path != scene_path_) {
        invalidate_assets();
        scene_path_ = scene.path;
    }
    sprites_.clear();
    tiles_.clear();
    asset_error_.clear();
    if (!target_) return;
    const auto& entities = scene.data["entities"];
    const auto camera = std::find_if(entities.begin(), entities.end(),
                                    [](const auto& entity) { return entity["type"] == "camera"; });
    const double camera_x = camera == entities.end() ? 0.0 : (*camera)["position"][0].get<double>();
    const double camera_y = camera == entities.end() ? 0.0 : (*camera)["position"][1].get<double>();
    tile_origin_x_ = std::round(width_ * 0.5 - camera_x * pixels_per_unit);
    tile_origin_y_ = std::round(height_ * 0.5 + camera_y * pixels_per_unit);
    if (const auto map = scene.data.find("tiles"); map != scene.data.end() &&
        std::isfinite(tile_origin_x_) && std::isfinite(tile_origin_y_)) {
        for (const auto& tile : *map) {
            const auto bounds = tile_bounds({tile["x"].get<int>(), tile["y"].get<int>()});
            if (bounds.x >= width_ || bounds.y >= height_ ||
                bounds.x + bounds.width <= 0 || bounds.y + bounds.height <= 0) continue;
            const auto kind = tile_kind(tile["kind"].get_ref<const std::string&>());
            tiles_.push_back({kind == TileKind::wall ? wall_ : floor_, bounds});
        }
    }
    for (const auto& entity : entities) {
        if (entity["type"] != "sprite") continue;
        auto* texture = sprite_;
        int width = sprite_size, height = sprite_size;
        if (entity.contains("sprite_image")) {
            const auto& ref = entity["sprite_image"].get_ref<const std::string&>();
            const auto& asset = load_texture(ref);
            if (asset.handle) {
                texture = asset.handle;
                width = asset.width;
                height = asset.height;
            } else if (asset_error_.empty()) {
                asset_error_ = ref + ": " + asset.error;
            }
        }
        const auto& position = entity["position"];
        const double x = std::round(width_ * 0.5 + (position[0].get<double>() - camera_x) * pixels_per_unit) - width / 2;
        const double y = std::round(height_ * 0.5 - (position[1].get<double>() - camera_y) * pixels_per_unit) - height / 2;
        if (!std::isfinite(x) || !std::isfinite(y) || x >= width_ || y >= height_ ||
            x + width <= 0 || y + height <= 0) continue;
        sprites_.push_back({entity["id"].get<std::string>(), texture, {x, y, width, height}});
    }
}

std::string_view PixelViewport::pick(double x, double y) const {
    for (auto it = sprites_.rbegin(); it != sprites_.rend(); ++it) {
        const auto& b = it->bounds;
        if (x >= b.x && x < b.x + b.width && y >= b.y && y < b.y + b.height) return it->id;
    }
    return {};
}

std::optional<SpriteBounds> PixelViewport::bounds(std::string_view id) const {
    const auto sprite = std::find_if(sprites_.begin(), sprites_.end(),
                                    [&](const auto& item) { return item.id == id; });
    return sprite == sprites_.end() ? std::nullopt : std::optional<SpriteBounds>(sprite->bounds);
}

std::optional<TileCell> PixelViewport::tile_at(double x, double y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return std::nullopt;
    const double cell_x = std::floor((x - tile_origin_x_) / tile_pixels);
    const double cell_y = std::floor((y - tile_origin_y_) / tile_pixels);
    if (!std::isfinite(cell_x) || !std::isfinite(cell_y) ||
        std::abs(cell_x) > tile_coordinate_limit || std::abs(cell_y) > tile_coordinate_limit) return std::nullopt;
    return TileCell{static_cast<int>(cell_x), static_cast<int>(cell_y)};
}

SpriteBounds PixelViewport::tile_bounds(TileCell cell) const {
    return {tile_origin_x_ + cell.x * tile_pixels, tile_origin_y_ + cell.y * tile_pixels, tile_pixels, tile_pixels};
}

void PixelViewport::record(SDL_GPUCommandBuffer* cmd, const std::array<float, 4>& background) const {
    if (!target_) return;
    SDL_GPUColorTargetInfo color{};
    color.texture = target_;
    color.clear_color = {background[0], background[1], background[2], 1.0f};
    color.load_op = SDL_GPU_LOADOP_CLEAR;
    color.store_op = SDL_GPU_STOREOP_STORE;
    auto* pass = SDL_BeginGPURenderPass(cmd, &color, 1, nullptr);
    SDL_BindGPUGraphicsPipeline(pass, pipeline_);
    const auto clip_x = [&](double x) { return static_cast<float>(2.0 * x / width_ - 1.0); };
    const auto clip_y = [&](double y) { return static_cast<float>(1.0 - 2.0 * y / height_); };
    const auto draw = [&](SDL_GPUTexture* texture, const SpriteBounds& b) {
        const SDL_GPUTextureSamplerBinding binding{texture, sampler_};
        SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
        const float rect[] = {clip_x(b.x), clip_y(b.y), clip_x(b.x + b.width), clip_y(b.y + b.height)};
        SDL_PushGPUVertexUniformData(cmd, 0, rect, static_cast<Uint32>(sizeof(rect)));
        SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
    };
    for (const auto& tile : tiles_) draw(tile.texture, tile.bounds);
    for (const auto& sprite : sprites_) draw(sprite.texture, sprite.bounds);
    SDL_EndGPURenderPass(pass);
}
}
