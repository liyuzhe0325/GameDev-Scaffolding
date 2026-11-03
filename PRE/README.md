# PRE — Pixel Rendering Engine

The Windows editor renders scene sprites into a low-resolution SDL3 GPU target. Built around two design promises:

1. **Machine-readable inside:** stable entity IDs, JSON scene data, validated AI CLI, optimistic edit checks, executable tests, and explicit implementation status.
2. **Artist-controlled outside:** dark dockable Dear ImGui editor with Hierarchy, Inspector, Render Settings, Viewport and Diagnostics.

## Quick start (Windows 11)

Requirements: Visual Studio 2022 (Desktop development with C++), Windows SDK with `dxc`, a D3D12-capable GPU, CMake >= 3.24, and Git. The first configure downloads dependencies. Set `PRE_DXC` to the absolute path to `dxc.exe` if CMake cannot find it.

From PowerShell in this directory:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
cd build\Release
.\pre_editor.exe
```

The application loads scene and preference files relative to its working directory. Run it from `build/Release` to use the shipped examples, Lua preview, and language catalogs.

### Scene runtime

From `build/Release`, run `.\pre_editor.exe --scene scenes/runtime_demo.json`. Click **Play**. The upper sprite moves automatically and the lower sprite responds to **WASD** when the viewport has focus. Click **Stop** to restore their edit positions. Select each sprite in **Hierarchy**, exchange its **Lua Script** reference in **Inspector**, and press **Enter** to apply. The behaviors follow the scripts on the next Play.

Place Lua source files in `scripts/` next to the scene JSON. Assign `scripts/<name>.lua` to any entity in the Inspector. Clear the field and press Enter to detach. Script names use letters, digits, underscores, or hyphens. The runtime reads source on each Play and preserves unsaved editor state across a Play / Stop cycle.

- Define an optional `on_start()` for initialization and `on_update(dt)` for frame logic. Delta time is in seconds and capped at 0.1 per frame.
- `pre.position()` returns the entity's X, Y, and Z. `pre.set_position(x, y, z)` updates the live instance.
- `pre.key_down("a")` accepts lowercase letters, `left`, `right`, `up`, `down`, and `space`. Keyboard state is released when the viewport loses focus.
- Each entity has its own Lua state with the math library and `pre` API. Scripts have source, memory, and instruction limits. Diagnostics identify the entity and file when a script stops on error.

### Gameplay-first Lua API (v0.7.0 development slice)

This is a **scripted game runtime**, not only a drawing canvas. Open `scenes/gameplay_demo.json` or run:
```powershell
.\pre_editor.exe --scene scenes/gameplay_demo.json
```
Press Play, use WASD, and press Space to move the spawned marker.

Supported gameplay APIs (scripts run in isolated Play/Stop scenes):

| API | Behavior |
| --- | --- |
| `pre.position()`, `pre.set_position(x,y,z)` | Read/write the script owner's position |
| `pre.key_down("space")`, `pre.key_pressed("space")` | Key state and rising edge |
| `pre.action_down("move_left")` etc. | Built-in WASD/jump action aliases |
| `pre.entity_exists(id)` | Check an entity ID |
| `pre.entity_position(id)` | Read coordinates, or nil if absent |
| `pre.spawn(id,type,x,y,z)` | Create a data-only entity at callback boundary |
| `pre.set_entity_position(id,x,y,z)` | Change another entity's position |
| `pre.destroy(id)` | Remove an entity after callback processing |

`spawn`, `destroy`, and cross-entity moves are **deferred until scripts complete their callbacks**; spawned entities do not automatically run Lua scripts. These calls only modify the Play snapshot, never the saved scene. Runtime supports an initial 4,096-entity safety cap. IDs are limited to 1-96 ASCII bytes using letters, digits, underscore and hyphen.

**Not yet supported:** custom input bindings, arbitrary components, physics, collision events, script attachment to spawned entities, a general-purpose game event bus, scene switching, visual scripting, or standalone game export. These are future Gameplay milestones.

### Language and Lua preview

- **Language / 语言** switches between English and Simplified Chinese. Edit the source catalogs in `locales/` and run the build to copy them into the release directory. Restart PRE to read the updated catalogs. Missing Chinese entries use the English value.
- While PRE is running, edit `build/Release/scenes/preview.lua` and return four RGBA numbers between zero and one to preview a different background. Save to reload. Return `nil` to use the scene's color. A script error keeps the last valid result.

### GPU viewport

Select **Pixel Hero** in the hierarchy or click its bounds in the viewport. The viewport uses sprite XY for placement and the first camera's XY for framing, at 24 pixels per scene unit. **Pixel Resolution** reallocates the target texture.

Click **Import PNG** in the sprite inspector to assign an image. PRE copies it into `assets/` next to the open scene JSON and renders it at its native pixel size. Save with **Ctrl+S** and reopen to restore its position and image. A scene can be moved together with its `assets/` folder. Existing scenes without an image reference use the built-in sprite. PNG inputs are limited to 2048 pixels per axis and 16 MiB.

Choose **Floor**, **Wall**, or **Erase** above the viewport and drag to edit tiles. Each cell covers 16 by 16 render pixels. The single tile layer is saved in the scene and draws behind sprites. Use **Select** to resume sprite picking. The built-in floor and wall tiles provide visuals; collision and custom tilesets are planned.

The save check compares the disk contents with the last loaded or saved revision. A conflict leaves the disk file and the editor's unsaved changes intact. Reloading a dirty scene requires an explicit discard confirmation.

### AI / CLI integration (no C++ or GPU required)

```powershell
python tools\scene_cli.py inspect scenes\lab.json
python tools\scene_cli.py set scenes\lab.json light_key position "[2,3,1]"
python -m unittest discover -s tests -v
```

Scene changes made by the CLI do not instantly appear in the open editor: choose **File > Reload from Disk**. Do not overwrite unsaved editor changes.

## Actual implementation vs roadmap

- **Done:** Windows D3D12 sprite and tile rendering, PNG assets, viewport selection and tile brushes, nearest sampling, editable offscreen resolution, scene hierarchy/inspector, guarded scene saves, CLI inspection and edits, GPU readback tests, and an in-editor Lua scene runtime.
- **Planned:** custom tilesets, tile collision, depth, lighting, shadows, and a standalone runtime build.

## External dependencies

The CMake configure process downloads SDL3 (zlib), Dear ImGui docking (MIT), nlohmann/json (MIT), and Lua (MIT). Preserve upstream license notices when distributing builds. The Chinese UI uses the installed Windows Microsoft YaHei font.

PNG decoding uses the unmodified [stb_image 2.30 header](https://github.com/nothings/stb/blob/d14d46f6b44456eb88beac58cba833a4135d4c89/stb_image.h), compiled in PNG-only mode. Its MIT and public-domain license options are in [third_party/stb/LICENSE.txt](third_party/stb/LICENSE.txt).

## Release and validation

See [CHANGELOG.md](CHANGELOG.md) for milestones and tested dependency versions. The current release was built with Visual Studio 2022 Build Tools and Windows SDK 10.0.26100.0. From the repository root, run `ctest --test-dir build -C Release --output-on-failure` for the Lua reload test and `python -m unittest discover -s tests -v` for the scene CLI tests.
