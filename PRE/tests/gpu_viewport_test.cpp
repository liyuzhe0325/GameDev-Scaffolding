#include "PixelViewport.h"
#include "SceneDocument.h"
#include "SpriteAsset.h"
#include <SDL3/SDL.h>
#include <array>
#include <cstdio>
#include <memory>
#include <chrono>
#include <fstream>

namespace {
using RGBA = std::array<Uint8, 4>;
constexpr RGBA blue{51, 102, 153, 255};
constexpr RGBA shirt{81, 191, 223, 255};
constexpr std::array<float, 4> background{0.2f, 0.4f, 0.6f, 1.0f};

bool check_frame(SDL_GPUDevice* gpu, pre::PixelViewport& viewport,
                 const pre::SceneDocument& scene, int sprite_x, int sprite_y, int clear_x, int clear_y,
                 const RGBA& expected = shirt) {
    viewport.prepare(scene);
    const auto width = static_cast<Uint32>(viewport.width());
    const auto height = static_cast<Uint32>(viewport.height());
    // Keep the download row pitch aligned for D3D12.
    const auto pitch = (width * 4 + 255u) & ~255u;
    SDL_GPUTransferBufferCreateInfo info{};
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    info.size = pitch * height;
    auto* download = SDL_CreateGPUTransferBuffer(gpu, &info);
    if (!download) return false;
    auto* cmd = SDL_AcquireGPUCommandBuffer(gpu);
    if (!cmd) { SDL_ReleaseGPUTransferBuffer(gpu, download); return false; }
    viewport.record(cmd, background);
    auto* copy = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTextureRegion source{};
    source.texture = viewport.texture();
    source.w = width;
    source.h = height;
    source.d = 1;
    SDL_GPUTextureTransferInfo dest{};
    dest.transfer_buffer = download;
    dest.pixels_per_row = pitch / 4;
    dest.rows_per_layer = height;
    SDL_DownloadFromGPUTexture(copy, &source, &dest);
    SDL_EndGPUCopyPass(copy);
    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    bool passed = false;
    if (fence && SDL_WaitForGPUFences(gpu, true, &fence, 1)) {
        const auto* bytes = static_cast<const Uint8*>(SDL_MapGPUTransferBuffer(gpu, download, false));
        if (bytes) {
            const auto pixel = [&](int x, int y) {
                const auto* p = bytes + y * pitch + x * 4;
                return RGBA{p[0], p[1], p[2], p[3]};
            };
            const auto observed = pixel(sprite_x, sprite_y);
            const auto clear = pixel(clear_x, clear_y);
            passed = observed == expected && clear == blue;
            if (!passed) std::fprintf(stderr, "Readback sprite RGB %u %u %u; clear RGB %u %u %u\n",
                observed[0], observed[1], observed[2], clear[0], clear[1], clear[2]);
            SDL_UnmapGPUTransferBuffer(gpu, download);
        }
    }
    if (fence) SDL_ReleaseGPUFence(gpu, fence);
    SDL_ReleaseGPUTransferBuffer(gpu, download);
    return passed;
}

int verify(SDL_GPUDevice* gpu, const std::filesystem::path& png) {
    pre::PixelViewport viewport(gpu);
    if (!viewport.resize(480, 270)) {
        std::fprintf(stderr, "Viewport initialization: %s\n", viewport.error().c_str());
        return 1;
    }
    pre::SceneDocument scene;
    scene.data = nlohmann::json::parse(R"({
        "schema_version":1,
        "render":{"clear_color":[0.2,0.4,0.6,1],"pixel_resolution":[480,270]},
        "entities":[{"id":"hero","name":"Hero","type":"sprite","position":[0,0,0]}]
    })");
    if (!check_frame(gpu, viewport, scene, 240, 134, 232, 127)) return 2;
    scene.data["entities"][0]["position"][0] = 2;
    if (!check_frame(gpu, viewport, scene, 288, 134, 240, 134)) return 3;
    if (!viewport.resize(96, 64) || viewport.width() != 96 || viewport.height() != 64) return 4;
    scene.data["entities"][0]["position"][0] = 0;
    if (!check_frame(gpu, viewport, scene, 48, 31, 0, 0)) return 5;
    std::printf("PASS %s: sprite color, transparency, movement and target resize\n", SDL_GetGPUDeviceDriver(gpu));

    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto folder = std::filesystem::path("sprite-test-" + std::to_string(stamp));
    std::filesystem::create_directory(folder);
    const auto file = folder / "scene.json";
    std::ofstream(file) << scene.data.dump(2);
    if (!scene.load(file)) return 6;
    std::string error;
    const auto invalid = folder / "bad.png";
    std::ofstream(invalid) << "Not a PNG";
    if (pre::import_sprite_png(invalid, file, error) || error.empty()) return 7;
    const auto first = pre::import_sprite_png(png, file, error);
    const auto second = pre::import_sprite_png(png, file, error);
    if (!first || !second || *first == *second) { std::fprintf(stderr, "Import: %s\n", error.c_str()); return 8; }
    scene.data["entities"][0]["sprite_image"] = *second;
    if (!scene.save()) return 9;
    pre::SceneDocument reopened;
    if (!reopened.load(file) || reopened.data["entities"][0]["sprite_image"] != *second) return 10;
    if (!check_frame(gpu, viewport, reopened, 48, 32, 44, 29, RGBA{240,72,108,255}) ||
        !viewport.asset_error().empty()) return 11;
    const auto bounds = viewport.bounds("hero");
    if (!bounds || bounds->width != 8 || bounds->height != 6 ||
        viewport.pick(48, 32) != "hero" || !viewport.pick(43, 32).empty()) return 12;
    reopened.data["entities"].push_back(reopened.data["entities"][0]);
    reopened.data["entities"][1]["id"] = "front";
    viewport.prepare(reopened);
    if (viewport.pick(48, 32) != "front") return 13;
    std::puts("PASS: invalid PNG refused, same-name imports preserved, save/reopen, PNG readback, native size and frontmost picking");

    const auto cell = viewport.tile_at(24, 24);
    if (cell != pre::TileCell{-2, -1} || viewport.tile_at(-1, 0)) return 14;
    if (!reopened.paint_tiles(*cell, {0, -1}, pre::TileKind::floor) ||
        reopened.data["tiles"].size() != 3 ||
        !reopened.set_tile(*cell, pre::TileKind::wall) ||
        !reopened.set_tile({-1, -1}, pre::TileKind::empty) ||
        !reopened.set_tile({0, 0}, pre::TileKind::floor)) return 15;
    if (reopened.set_tile({0, 0}, pre::TileKind::floor) ||
        reopened.set_tile({5000, 0}, pre::TileKind::wall)) return 16;
    if (!reopened.save() || !scene.load(file) || scene.data["tiles"].size() != 3) return 17;
    if (!check_frame(gpu, viewport, scene, 24, 24, 40, 24, RGBA{145,104,70,255}) ||
        !check_frame(gpu, viewport, scene, 56, 24, 40, 24, RGBA{76,91,107,255}) ||
        !check_frame(gpu, viewport, scene, 48, 32, 0, 0, RGBA{240,72,108,255})) return 18;
    scene.data["entities"].push_back({
        {"id", "camera"}, {"name", "Camera"}, {"type", "camera"}, {"position", {1,-1,8}}
    });
    viewport.prepare(scene);
    if (viewport.tile_at(24.5, 8.5) != pre::TileCell{0,0}) return 19;
    std::puts("PASS: tile stroke, replace, erase, save/reopen, GPU colors, layering and camera-relative picking");
    return 0;
}
}

int main(int argc, char** argv) {
    if (argc != 2) return 1;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL init: %s\n", SDL_GetError());
        return 77;
    }
    auto* gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL, true, "direct3d12");
    if (!gpu) {
        std::fprintf(stderr, "D3D12 unavailable: %s\n", SDL_GetError());
        SDL_Quit();
        return 77;
    }
    const int code = verify(gpu, argv[1]);
    SDL_WaitForGPUIdle(gpu);
    SDL_DestroyGPUDevice(gpu);
    SDL_Quit();
    return code;
}
