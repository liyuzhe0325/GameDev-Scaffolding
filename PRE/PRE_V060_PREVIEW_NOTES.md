# PRE v0.6.0-preview — local implementation notes

Base: PRE v0.5.0, source commit ec9f88a.

## Changes implemented

- `SceneDocument` provides `set_entity_field`, `erase_entity_field`, `begin_edit`, `end_edit`, `undo`, `redo`, `clear_history`.
- Undo/redo uses bounded (64-state) JSON scene snapshots, which is easy to audit but is not suitable for enormous tilemaps; replace with diffs once functional tests succeed.
- Tile painting can group a continuous mouse stroke into a single history transaction.
- Inspector `name`, `position`, `script` and sprite reset/import use validated `SceneDocument` mutations.
- Edit > Undo/Redo and Ctrl+Z/Ctrl+Y shortcuts added. Shift+Ctrl+Z redoes.
- Scene load resets history. Maximum pixel resolution is consistently checked against 4096 in C++ and Python CLI.
- Added `scene_history_test` (C++) and pixel-resolution rejection case (Python); C++ test registered in CMake.

## Verification

- `python -m unittest discover -s tests -v`: **7/7 passed** in the current Linux environment.
- Windows C++ compilation, CTest, GPU readback and editor interaction **not executed**. This is an unverified source preview, not a release.

## Known remaining work and caveats

- The Python CLI still writes its own scene file and requires editor reload; no live PRE-Agent bridge.
- Pixel Studio, neural rendering, advanced lights and rigs are not part of this patch.
- Render Settings currently bypass the new history and do not participate in Undo/Redo.
- `set_tile` remains linear over the tile JSON array. A chunk/indexed tile store is deferred.
- Whole-scene snapshots can be expensive; history is limited to 64 entries, and a long active gesture may still do some JSON copying.
- Multi-process atomic conditional saves are not implemented. Keep optimistic scene revision guards.
- The first ImGui edit frame can start an item gesture after its initial value change; grouping behavior requires Windows UI manual testing.
- Existing asset files imported to a scene are not automatically deleted by Undo.

## Suggested manual validation on Windows

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
python -m unittest discover -s tests -v
```

In the editor: change a name, drag position, paint a multi-cell stroke, undo/redo, save, reload, and play/stop. Check that the GPU viewport and Lua demo behave as in v0.5.0.

## GitHub organization

Target repository: `liyuzhe0325/GameDev-Scaffolding`.
Draft integration PR: https://github.com/liyuzhe0325/GameDev-Scaffolding/pull/4

The PR currently contains **documentation and a protocol draft only**, not this source ZIP. Import this archive as `PRE/` to the PR branch and run Windows build/tests before requesting merge. Do not overwrite the existing agent README.
