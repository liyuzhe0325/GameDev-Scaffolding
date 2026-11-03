# PRE ↔ GameDev Scaffolding integration contract (draft v0)

## Direction

The Agent can request project information, planned edits and tests. PRE owns the authoritative scene/runtime state and validation.

## Capabilities to expose incrementally

1. `scene.inspect(scene_path)` — read snapshot, stable entity IDs, revision hash, typed properties.
2. `scene.validate(scene_path)` — parse/schema/resource checks with structured errors.
3. `scene.patch(scene_path, entity_id, property, value, expected_revision)` — validated, optimistic-concurrency guarded mutation.
4. `scene.undo()` / `scene.redo()` — editor-local history; not a substitute for Git or cross-process coordination.
5. Future `runtime.snapshot()`, `runtime.play()`, `test.run()`, `render.capture()`, `asset.inspect()`.

## Existing v0.5.0 reality

The initial Python `tools/scene_cli.py` supports inspect, validate and a subset of entity edits via scene files. It requires editor reload after changes. There is no live Agent Bridge or bidirectional subscription yet.

The v0.6.0 work-in-progress uses C++ `SceneDocument::set_entity_field` and undo/redo as a first editor-side mutation boundary, but Python and C++ still have separate validation implementations. Consolidation must be tested before claiming a unified contract.

## Security and validation

- Query defaults to read-only.
- Never assume that Lua sandboxing permits arbitrary untrusted code.
- Validate IDs, field types, finite floats, paths, asset dependencies, and resolution limits.
- Reject stale revisions; do not silently overwrite external edits.
- Batch operations require explicit transaction semantics.
- Report build, test, GPU and screenshot evidence separately.
