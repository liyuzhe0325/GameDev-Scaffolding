#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "EditorUI.h"
#include "LuaPreview.h"
#include "PixelViewport.h"
#include "ReleaseInfo.h"
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

static bool load_chinese_font() {
#ifdef _WIN32
    const char* windows_dir = SDL_getenv("SystemRoot");
    if (windows_dir) {
        const std::string path = std::string(windows_dir) + "/Fonts/msyh.ttc";
        if (std::filesystem::exists(path)) {
            auto& io = ImGui::GetIO();
            ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), 17.0f);
            if (font && font->IsGlyphInFont(0x4E2D)) {
                io.FontDefault = font;
                return true;
            }
        }
    }
#endif
    return false;
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--version") {
        std::puts(pre::version_label);
        return 0;
    }
    std::filesystem::path scene_path = "scenes/lab.json";
    if (argc == 3 && std::string_view(argv[1]) == "--scene") {
        const std::string_view utf8 = argv[2];
        scene_path = std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
    } else if (argc != 1) {
        std::fprintf(stderr, "Usage: pre_editor [--version | --scene <scene.json>]\n");
        return 1;
    }
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) { std::fprintf(stderr,"SDL: %s\n",SDL_GetError()); return 1; }
    SDL_Window* window=SDL_CreateWindow(pre::version_label,1440,900,SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) { std::fprintf(stderr,"Window: %s\n",SDL_GetError()); SDL_Quit(); return 1; }
#ifdef _WIN32
    SDL_GPUDevice* gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL, true, "direct3d12");
#else
    SDL_GPUDevice* gpu=SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV|SDL_GPU_SHADERFORMAT_MSL|SDL_GPU_SHADERFORMAT_METALLIB,true,nullptr);
#endif
    if (!gpu || !SDL_ClaimWindowForGPUDevice(gpu,window)) {std::fprintf(stderr,"GPU: %s\n",SDL_GetError()); if (gpu) SDL_DestroyGPUDevice(gpu); SDL_DestroyWindow(window); SDL_Quit(); return 2;}
    SDL_SetGPUSwapchainParameters(gpu,window,SDL_GPU_SWAPCHAINCOMPOSITION_SDR,SDL_GPU_PRESENTMODE_VSYNC);
    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    const bool chinese_available = load_chinese_font();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable|ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    auto& style=ImGui::GetStyle(); style.WindowRounding=5.0f;style.FrameRounding=4.0f;
    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo info{}; info.Device=gpu;
    info.ColorTargetFormat=SDL_GetGPUSwapchainTextureFormat(gpu,window);
    info.MSAASamples=SDL_GPU_SAMPLECOUNT_1;
    info.SwapchainComposition=SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
    info.PresentMode=SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&info);
    pre::SceneDocument scene;
    if (!scene.load(scene_path)) std::fprintf(stderr,"Scene: %s\n",scene.error.c_str());
    if (!scene.data.is_object()) { // No silent invalid-state continuation.
        ImGui_ImplSDLGPU3_Shutdown(); ImGui_ImplSDL3_Shutdown(); ImGui::DestroyContext();
        SDL_ReleaseWindowFromGPUDevice(gpu,window);SDL_DestroyGPUDevice(gpu);SDL_DestroyWindow(window);SDL_Quit();return 3;
    }
    const char* base = SDL_GetBasePath();
    const std::string_view base_utf8 = base ? base : ".";
    const auto locale_directory = std::filesystem::path(std::u8string(base_utf8.begin(), base_utf8.end())) / "locales";
    pre::EditorUI editor(window, chinese_available, locale_directory);
    pre::LuaPreview lua_preview;
    auto pixels = std::make_unique<pre::PixelViewport>(gpu);
    auto update_title = [&] {
        const std::string title = std::string(editor.window_title()) + " · v" + pre::version;
        SDL_SetWindowTitle(window, title.c_str());
    };
    bool title_is_chinese = editor.chinese();
    update_title();
    const char* backend=SDL_GetGPUDeviceDriver(gpu);
    bool done=false; std::uint64_t previous=SDL_GetPerformanceCounter();
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type==SDL_EVENT_QUIT || (event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID==SDL_GetWindowID(window))) done=true;
        }
        if (done) break;
        if (SDL_GetWindowFlags(window)&SDL_WINDOW_MINIMIZED) { SDL_Delay(10);continue; }
        const auto now=SDL_GetPerformanceCounter();
        const float frame_ms=1000.0f*float(now-previous)/float(SDL_GetPerformanceFrequency());previous=now;
        ImGui_ImplSDLGPU3_NewFrame();ImGui_ImplSDL3_NewFrame();ImGui::NewFrame();
        if (!editor.playing() && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) scene.save();
        lua_preview.poll(SDL_GetTicks());
        editor.draw(scene, lua_preview, *pixels, frame_ms, backend ? backend : "unknown");
        if (title_is_chinese != editor.chinese()) {
            title_is_chinese = editor.chinese();
            update_title();
        }
        ImGui::Render();
        ImDrawData* data=ImGui::GetDrawData();
        SDL_GPUCommandBuffer* cmd=SDL_AcquireGPUCommandBuffer(gpu);
        if (!cmd) { std::fprintf(stderr,"GPU command: %s\n",SDL_GetError());break; }
        SDL_GPUTexture* target=nullptr;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd,window,&target,nullptr,nullptr)) {
            std::fprintf(stderr,"Swapchain: %s\n",SDL_GetError());SDL_CancelGPUCommandBuffer(cmd);break;
        }
        if (target && data->DisplaySize.x>0 && data->DisplaySize.y>0) {
            pixels->record(cmd, editor.viewport_background(scene, lua_preview));
            ImGui_ImplSDLGPU3_PrepareDrawData(data,cmd);
            SDL_GPUColorTargetInfo target_info{};
            target_info.texture=target;target_info.clear_color=SDL_FColor{.055f,.069f,.095f,1.0f};
            target_info.load_op=SDL_GPU_LOADOP_CLEAR;target_info.store_op=SDL_GPU_STOREOP_STORE;
            auto* pass=SDL_BeginGPURenderPass(cmd,&target_info,1,nullptr);
            ImGui_ImplSDLGPU3_RenderDrawData(data,cmd,pass);
            SDL_EndGPURenderPass(pass);
        }
        SDL_SubmitGPUCommandBuffer(cmd);
    }
    SDL_WaitForGPUIdle(gpu);
    pixels.reset();
    ImGui_ImplSDLGPU3_Shutdown(); ImGui_ImplSDL3_Shutdown(); ImGui::DestroyContext();
    SDL_ReleaseWindowFromGPUDevice(gpu,window);SDL_DestroyGPUDevice(gpu);SDL_DestroyWindow(window);SDL_Quit();
    return 0;
}
