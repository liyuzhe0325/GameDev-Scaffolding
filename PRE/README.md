# PRE Engine — GitHub integration workspace

PRE is an independent C++20 / SDL3 GPU / Dear ImGui / Lua pixel-oriented game engine. The GameDev Scaffolding agent remains a separate research and orchestration system; the two should cooperate through an explicit versioned interface.

## Repository layout (planned)

```text
GameDev-Scaffolding/
  README.md                       # Agent vision (unchanged)
  PRE/                            # Independent engine source (pending source import)
  integrations/pre-agent/         # Protocol, adapters, tests
```

## Source status

**This branch contains an integration specification, not the full PRE engine source yet.** The current engine source is based on user-provided PRE v0.5.0 (ec9f88a) and is being upgraded toward v0.6.0. The complete edited source archive must be imported under `PRE/` before this branch is merged.

Do not treat the `README.md` here as evidence that the engine is already present in GitHub.

## Integration principles

- PRE continues to build and run without an Agent.
- The Agent never directly modifies live renderer internals.
- Prefer typed property operations with schema validation and scene revision guards.
- All writes should be attributable, reversible and testable; expose dry-run and confirmation policy.
- Keep GameDev-Scaffolding's existing professional maps and experience-research workflow independent.
- Test Python scene CLI before attempting Windows GPU tests; the latter require Windows and a D3D12-capable environment.
- Neural lighting and art workflows are optional experimental subsystems and must not become runtime dependencies.
