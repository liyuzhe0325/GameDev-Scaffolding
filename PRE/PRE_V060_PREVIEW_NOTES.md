# PRE v0.6.0-preview — local implementation notes

Base: PRE v0.5.0, source commit ec9f88a.

## Changes implemented

- `SceneDocument` provides `set_entity_field`, `erase_entity_field`, `begin_edit`, `end_edit`, `undo`, `redo`, `clear_history`.
- Undo/redo uses bounded (64-state) JSON scene snapshots, which is easy to audit but is not suitable for enormous tilemaps; replace with diffs once functional tests succeed.
- Tile painting can group a continuous mouse stroke into a single history transaction.
- Inspector `name`, `position`, `script` and sprite reset/import use validated `SceneDocument` mutations.
- Edit > Undo/Redo and Ctrl+Z/Ctrl+Y shortcuts added. Shift+Ctrl+Z redoes. Both menu entries are localized.
- Resolution and background color use the same validated edit history as tiles and entity fields.
- Scene load resets history. Maximum pixel resolution is consistently checked against 4096 in C++ and Python CLI.
- Added `scene_history_test` (C++) and a pixel-resolution rejection case (Python). The C++ checks run in Release and cover the order of tile, resolution, and color edits.

## Verification

- Windows Release built successfully. All six CTest cases passed, including D3D12 GPU readback. All seven Python CLI tests passed.
- Use the manual steps below to verify grouping and shortcuts in the editor.

## Known remaining work and caveats

- The Python CLI still writes its own scene file and requires editor reload; no live PRE-Agent bridge.
- Pixel Studio, neural rendering, advanced lights and rigs are not part of this patch.
- `set_tile` remains linear over the tile JSON array. A chunk/indexed tile store is deferred.
- Whole-scene snapshots can be expensive; history is limited to 64 entries, and a long active gesture may still do some JSON copying.
- Multi-process atomic conditional saves are not implemented. Keep optimistic scene revision guards.
- Existing asset files imported to a scene are not automatically deleted by Undo.

## Suggested manual validation on Windows

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
python -m unittest discover -s tests -v
```

In the editor, paint a multi-cell stroke and change the resolution. Click the viewport title to leave the input field. Undo once to restore only the resolution, then undo again to remove the whole stroke. Redo twice to restore the stroke and resolution in that order. Also check a name change, a position drag, save and reload, and Play / Stop.

## GitHub organization

The source lives under `PRE/` on `main` in `liyuzhe0325/GameDev-Scaffolding`. The agent contract is at `integrations/pre-agent/CONTRACT.md`.
