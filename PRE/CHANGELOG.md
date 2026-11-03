# PRE release history

The editor displays the version and changes under **Help > Version and Release History**. Add new releases at the top of [releases.json](releases.json) and put the matching titles and change descriptions in the language catalogs. The first entry sets the compiled version. Build and test that version before creating its matching Git tag.

## v0.6.0 Preview · 2026-11-03

Adds undo and redo for tile strokes, entity fields, resolution, and background color. The editor records each continuous gesture as one step and applies changes in reverse order on undo.

## v0.5.0 · 2026-11-03

Adds Play and Stop with an isolated scene instance, optional entity Lua scripts, and a script API for frame time, keyboard state, and entity position. The release includes a two-sprite runtime example.

## v0.4.1 · 2026-11-03

The viewport canvas captures left mouse drags so a tile stroke stays in the canvas. The viewport title bar still moves the panel.

## v0.4.0 · 2026-11-03

Adds floor, wall, and erase brushes to a single saved tile layer. The GPU viewport draws tiles behind sprites and uses the same grid coordinates for brushing and rendering.

## v0.3.0 · 2026-11-03

Adds scene-relative PNG import and viewport sprite selection. Imported images keep their native size and transparency after saving and reopening the scene.

## v0.2.0 · 2026-11-03

First M1 slice. The Windows D3D12 viewport draws the built-in sprite from scene XY coordinates into an editable low-resolution target, displayed with nearest sampling. The editor detects stale saves and confirms before discarding unsaved edits during reload.

## v0.1.1 · 2026-11-03

Introduces the in-editor version and release history. The program title, toolbar, history panel, and `--version` use the same compiled version.

## v0.1.0 · 2026-11-03

First verified Windows x64 build of the M0 editor.

- Built the SDL3 GPU and ImGui editor and packaged its SDL3 DLL and sample scene alongside the executable.
- Added English and Simplified Chinese language catalogs, a saved language choice, and English fallback for missing translations.
- Embedded Lua 5.5.1. Saving the preview script updates the mock viewport background. Invalid scripts retain the last valid color.
- Verified Windows Release startup in both languages, English fallback, Lua reload and recovery, and all four scene CLI tests.

Tested dependencies were SDL 3.2.20, nlohmann/json 3.12.0, Lua 5.5.1, and Dear ImGui docking commit `d682dc41f58b98c9343ffbaa7d94f07e9024b3ca`. The current CMake configuration follows the ImGui docking branch on fresh downloads.

M0 uses an ImGui mock viewport. M1 begins with an offscreen pixel sprite render pass.
