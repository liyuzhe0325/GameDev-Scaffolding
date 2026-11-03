# PRE AI Contract v1

This contract is both documentation for agents and a boundary against unsafe edits.

## Truth about current stage

- M1 has an SDL3 GPU sprite pass on Windows D3D12. The viewport draws from scene XY positions into the configured pixel target. The editor can import a PNG and select sprites by their bounds.
- The Tilemap editor paints floor and wall cells in one saved layer. Custom tilesets, collision, depth, lighting, shadow maps, HDR, GPU particles, and scene-runtime hot reload are planned.
- Play instantiates the current scene in memory and runs attached Lua scripts. Stop discards runtime state. The scripting API is described in the README.
- Console/Diagnostics frame timing measures CPU elapsed frame time, not GPU elapsed time.

## Scene structure

- `schema_version`: fixed integer 1
- `scene_id`: string identifier
- `render.pixel_resolution`: positive int2
- `render.clear_color`: finite float4
- `entities[]`: objects with unique string `id`, display `name`, semantic `type`, and `position` float3
- `entities[].sprite_image`: optional `assets/<filename>.png` reference for a sprite. The directory is relative to the scene JSON. The CLI validates this reference and preserves it during edits.
- `entities[].script`: optional `scripts/<name>.lua` reference relative to the scene JSON. File names use ASCII letters, digits, underscores, or hyphens. The CLI validates the reference and preserves it during edits. Script loading happens on Play.
- `tiles[]`: optional sparse layer with up to 8192 unique cells. Each cell has integer `x` and `y` in `[-4096,4096]` and a `kind` of `floor` or `wall`. The grid starts at the scene origin, with positive X right and positive Y down. Omitted cells are empty. The CLI validates the layer and preserves it during edits.
- `id` is immutable through `scene_cli.py`; use unique stable IDs; `name` is mutable.

## AI workflow

1. `python tools/scene_cli.py inspect scenes/lab.json`
2. Read `sha256` from the response.
3. `python tools/scene_cli.py set scenes/lab.json light_key position '[2,3,1]' --expected-sha256 <hash>`
4. `python tools/scene_cli.py validate scenes/lab.json`
5. Editor: File -> Reload from Disk; an unsaved scene must be saved or discarded deliberately.
6. Run tests: `python -m unittest discover -s tests -v`

The CLI permits only vetted fields. Do not make arbitrary shell substitutions in scene JSON. Add new actions with schemas and tests, not ad hoc string parsing.

## Human/AI consistency

- Both human Inspector and CLI work on `scenes/lab.json` with the same entity IDs.
- The editor compares disk contents with the loaded revision before saving. The CLI uses its own optimistic check. Both use atomic replacement, with a remaining check-to-replace window for simultaneous writers. Use reload after an external edit.
- Diagnostics and source are authoritative; do not claim rendered effects based on metadata alone.

## Agent-friendly practices for future milestones

- Every render pass gets a unique machine-readable name and declared inputs/outputs.
- Structured diagnostics: `code`, `severity`, `resource_id`, `source_location`, `suggested_action`.
- Every user-editable property gets a stable machine key, type, unit, range, and help text.
- Add a replayable command journal with undo/redo and optimistic revisions before multi-agent collaboration.
- Add actual GPU timings and render-target readback after M1.
