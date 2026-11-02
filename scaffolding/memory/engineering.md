# Engineering Memory: v0.1 design

The repository, Unity assets/settings and test outputs are the source of truth. A prior LLM explanation is only an index/hypothesis.

## First read
1. Locate the correct project/worktree and branch.
2. Inspect `ProjectSettings/ProjectVersion.txt`, `Packages/manifest.json`, relevant source symbols and asset GUIDs as applicable.
3. Find existing implementation before creating another manager or state owner. Prefer repo-map / LSP / existing code search where useful.
4. Observe actual tests, compile, Editor logs and scene behavior. Never report playtest based on build alone.
5. Include file paths, commit hash/project version and evidence for material claims.

## Cache semantics
A memory note may include `locator`, `summary`, `checked_at`, `revision`, `trust=derived`. On any material modification or unknown revision, re-read source; treat invalidation as default, not a best-effort background guarantee.

## Safety
Do not make Unity scene/prefab changes by hand-writing binary assets. Preserve .meta/GUID. Never edit Library. Separate Unity playmode observations from web-game `game_use`; current OhMyGame docs do not grant native Unity direct game_use.
