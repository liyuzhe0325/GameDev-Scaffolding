cmake_minimum_required(VERSION 3.24)
set(PRE_RELEASE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/releases.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PRE_RELEASE_FILE}")
file(READ "${PRE_RELEASE_FILE}" PRE_RELEASE_HISTORY)
string(JSON PRE_RELEASE_VERSION GET "${PRE_RELEASE_HISTORY}" 0 version)
string(JSON PRE_RELEASE_PREVIEW ERROR_VARIABLE PRE_RELEASE_PREVIEW_ERROR GET "${PRE_RELEASE_HISTORY}" 0 preview)
set(PRE_DISPLAY_VERSION "${PRE_RELEASE_VERSION}")
if(NOT PRE_RELEASE_PREVIEW_ERROR AND PRE_RELEASE_PREVIEW)
  string(APPEND PRE_DISPLAY_VERSION " Preview")
endif()
project(PRE VERSION "${PRE_RELEASE_VERSION}" LANGUAGES C CXX)
configure_file(src/ReleaseInfo.h.in generated/ReleaseInfo.h @ONLY)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
option(PRE_FETCH_DEPS "Download dependencies at configure time" ON)
if(PRE_FETCH_DEPS)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(SDL3 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-3.2.20 GIT_SHALLOW TRUE)
  FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG docking GIT_SHALLOW TRUE)
  FetchContent_Declare(json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.12.0 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(SDL3 imgui json)
else()
  find_package(SDL3 CONFIG REQUIRED)
  find_package(nlohmann_json CONFIG REQUIRED)
  if(NOT EXISTS "${CMAKE_SOURCE_DIR}/external/imgui/imgui.cpp")
    message(FATAL_ERROR "Set PRE_FETCH_DEPS=ON or put Dear ImGui docking in external/imgui")
  endif()
  set(imgui_SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/imgui")
endif()
FetchContent_Declare(lua
  URL https://www.lua.org/ftp/lua-5.5.1.tar.gz
  URL_HASH SHA256=1c4b4068d67061f2a2231ad2b5422e77acea1487ea9890f6320af614f4373dce
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(lua)
file(GLOB lua_sources "${lua_SOURCE_DIR}/src/*.c")
list(FILTER lua_sources EXCLUDE REGEX "/(lua|luac)\\.c$")
add_library(pre_lua STATIC ${lua_sources})
target_include_directories(pre_lua PUBLIC "${lua_SOURCE_DIR}/src")
add_library(pre_imgui STATIC
  "${imgui_SOURCE_DIR}/imgui.cpp" "${imgui_SOURCE_DIR}/imgui_demo.cpp" "${imgui_SOURCE_DIR}/imgui_draw.cpp"
  "${imgui_SOURCE_DIR}/imgui_tables.cpp" "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.cpp")
target_include_directories(pre_imgui PUBLIC "${imgui_SOURCE_DIR}" "${imgui_SOURCE_DIR}/backends")
target_link_libraries(pre_imgui PUBLIC SDL3::SDL3)
add_library(pre_pixel_viewport STATIC src/PixelViewport.cpp src/SpriteAsset.cpp)
target_include_directories(pre_pixel_viewport PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_include_directories(pre_pixel_viewport SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb")
target_link_libraries(pre_pixel_viewport PUBLIC SDL3::SDL3 nlohmann_json::nlohmann_json)
add_executable(pre_editor src/main.cpp src/SceneDocument.cpp src/EditorUI.cpp src/LuaPreview.cpp src/Localization.cpp)
target_sources(pre_editor PRIVATE src/SceneRuntime.cpp)
target_include_directories(pre_editor PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(pre_editor PRIVATE pre_imgui pre_lua pre_pixel_viewport)
if(WIN32)
  find_program(PRE_DXC NAMES dxc HINTS
    "C:/Program Files (x86)/Windows Kits/10/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64" REQUIRED)
  set(PRE_SPRITE_SHADER "${CMAKE_CURRENT_SOURCE_DIR}/shaders/sprite.hlsl")
  set(PRE_VERTEX_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/SpriteVertex.h")
  set(PRE_PIXEL_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/SpritePixel.h")
  add_custom_command(
    OUTPUT "${PRE_VERTEX_HEADER}" "${PRE_PIXEL_HEADER}"
    COMMAND "${PRE_DXC}" -T vs_6_0 -E vertex_main -Vn sprite_vertex_dxil -Fh "${PRE_VERTEX_HEADER}" "${PRE_SPRITE_SHADER}"
    COMMAND "${PRE_DXC}" -T ps_6_0 -E pixel_main -Vn sprite_pixel_dxil -Fh "${PRE_PIXEL_HEADER}" "${PRE_SPRITE_SHADER}"
    DEPENDS "${PRE_SPRITE_SHADER}" VERBATIM)
  target_sources(pre_pixel_viewport PRIVATE "${PRE_VERTEX_HEADER}" "${PRE_PIXEL_HEADER}")
endif()
if(MSVC)
  target_compile_options(pre_editor PRIVATE /W4 /permissive- /utf-8)
else()
  target_compile_options(pre_editor PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_custom_command(TARGET pre_editor POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:pre_editor>/scenes"
  COMMAND ${CMAKE_COMMAND}
    "-Dsource=${CMAKE_SOURCE_DIR}/scenes/lab.json"
    "-Ddestination=$<TARGET_FILE_DIR:pre_editor>/scenes/lab.json"
    -P "${CMAKE_SOURCE_DIR}/tools/seed_preview.cmake"
  VERBATIM)
add_custom_target(pre_locales
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/locales" "$<TARGET_FILE_DIR:pre_editor>/locales"
  VERBATIM)
add_dependencies(pre_editor pre_locales)
if(WIN32 AND TARGET SDL3::SDL3-shared)
  add_custom_command(TARGET pre_editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE:SDL3::SDL3-shared>" "$<TARGET_FILE_DIR:pre_editor>")
endif()
# The editable script is seeded only if it doesn't exist in the output folder.
foreach(asset IN ITEMS runtime_demo.json gameplay_demo.json scripts/oscillate.lua scripts/keyboard.lua scripts/gameplay.lua)
  add_custom_command(TARGET pre_editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:pre_editor>/scenes/scripts"
    COMMAND ${CMAKE_COMMAND}
      "-Dsource=${CMAKE_SOURCE_DIR}/scenes/${asset}"
      "-Ddestination=$<TARGET_FILE_DIR:pre_editor>/scenes/${asset}"
      -P "${CMAKE_SOURCE_DIR}/tools/seed_preview.cmake"
    VERBATIM)
endforeach()
add_custom_command(TARGET pre_editor POST_BUILD
  COMMAND ${CMAKE_COMMAND}
    "-Dsource=${CMAKE_SOURCE_DIR}/scenes/preview.lua"
    "-Ddestination=$<TARGET_FILE_DIR:pre_editor>/scenes/preview.lua"
    -P "${CMAKE_SOURCE_DIR}/tools/seed_preview.cmake"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${lua_SOURCE_DIR}/doc/readme.html" "$<TARGET_FILE_DIR:pre_editor>/Lua-license.html"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb/LICENSE.txt" "$<TARGET_FILE_DIR:pre_editor>/stb_image-LICENSE.txt"
  VERBATIM)

include(CTest)
if(BUILD_TESTING)
  add_executable(pre_viewport_input_test tests/viewport_input_test.cpp)
  target_include_directories(pre_viewport_input_test PRIVATE src)
  target_link_libraries(pre_viewport_input_test PRIVATE pre_imgui)
  add_test(NAME viewport_drag_ownership COMMAND pre_viewport_input_test)
  if(WIN32)
    add_dependencies(pre_viewport_input_test pre_editor)
  endif()
  add_executable(pre_gameplay_api_test tests/gameplay_api_test.cpp src/SceneRuntime.cpp src/SceneDocument.cpp)
  target_include_directories(pre_gameplay_api_test PRIVATE src)
  target_link_libraries(pre_gameplay_api_test PRIVATE pre_lua nlohmann_json::nlohmann_json)
  add_test(NAME gameplay_api COMMAND pre_gameplay_api_test)
  add_executable(pre_scene_history_test tests/scene_history_test.cpp src/SceneDocument.cpp)
  target_include_directories(pre_scene_history_test PRIVATE src)
  target_link_libraries(pre_scene_history_test PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME scene_history COMMAND pre_scene_history_test)
  set_tests_properties(scene_history PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(pre_scene_save_test tests/scene_save_test.cpp src/SceneDocument.cpp)
  target_include_directories(pre_scene_save_test PRIVATE src)
  target_link_libraries(pre_scene_save_test PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME scene_save_conflict COMMAND pre_scene_save_test "${CMAKE_CURRENT_SOURCE_DIR}/scenes/lab.json")
  if(WIN32)
    add_executable(pre_gpu_viewport_test tests/gpu_viewport_test.cpp src/SceneDocument.cpp)
    target_include_directories(pre_gpu_viewport_test PRIVATE src)
    target_link_libraries(pre_gpu_viewport_test PRIVATE pre_pixel_viewport)
    add_dependencies(pre_gpu_viewport_test pre_editor)
    add_test(NAME gpu_sprite_readback COMMAND pre_gpu_viewport_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/import-sprite.png")
    set_tests_properties(gpu_sprite_readback PROPERTIES LABELS gpu SKIP_RETURN_CODE 77 TIMEOUT 20)
  endif()
  add_executable(pre_lua_preview_test tests/lua_preview_test.cpp src/LuaPreview.cpp)
  target_include_directories(pre_lua_preview_test PRIVATE src)
  target_link_libraries(pre_lua_preview_test PRIVATE pre_lua)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lua-preview-test/scenes")
  add_test(NAME lua_preview_reload COMMAND pre_lua_preview_test)
  set_tests_properties(lua_preview_reload PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lua-preview-test" TIMEOUT 10)
  add_executable(pre_scene_runtime_test tests/scene_runtime_test.cpp src/SceneRuntime.cpp src/SceneDocument.cpp)
  target_include_directories(pre_scene_runtime_test PRIVATE src)
  target_link_libraries(pre_scene_runtime_test PRIVATE pre_lua nlohmann_json::nlohmann_json)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/scene-runtime-test")
  add_test(NAME scene_runtime COMMAND pre_scene_runtime_test "${CMAKE_CURRENT_SOURCE_DIR}/scenes/runtime_demo.json")
  set_tests_properties(scene_runtime PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/scene-runtime-test" TIMEOUT 10)
endif()
