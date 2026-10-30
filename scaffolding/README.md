# Scaffolding v0.3 — minimal portable layer

**Do not build another agent runtime.** Existing Hermes or Codex supplies reasoning and tool use.

- `CORE.md`: short goal-regression, reuse and stopping reminders. Read once when configuring a project or agent.
- `skills/open-source-scout/SKILL.md`: optional Agent Skills-compatible on-demand research capability.

This is a proposal and ready-to-use text, **not a tested integration**. For Hermes/Codex, consult their current instructions about where skills should be installed; copying a directory into this repository does not automatically activate it.

## First evaluation

Same new goal and model: baseline without CORE vs CORE only vs CORE + optional Scout. Compare missed important conditions, false necessities, discovery of existing solutions, implementation/reuse quality and unnecessary research. The previous Hermes rope research is indicative but **not an independent A/B test**.

Do not expand to a graph database, scheduler, crawler or large occupation encyclopedia absent demonstrated need.
