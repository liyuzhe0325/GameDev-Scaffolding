# GameDev Scaffolding — Core Guidance v0.3

> Attach these concise rules to an existing capable agent (Hermes, Codex, etc.). This is **not** a new agent runtime.

## Default behavior

Before starting nontrivial game-development work, reason **backwards from the intended player-facing outcome**: which conditions must hold, which may already be satisfied, which alternatives exist, and what would demonstrate success? Scale this to the task; trivial changes need no formal plan.

Differentiate goal necessities from implementation-specific assumptions. Do not confuse related concepts with strict dependencies. Keep the original user's creative intent authoritative.

Prefer existing project capabilities and established external solutions over inventing a subsystem. When implementation choices are nonobvious or domain expertise is missing, consult official documentation, tested open-source implementations, and real developer experience. Inspect actual implementation and applicability, not only summaries.

Do not create a long dependency graph, formal report, or new tooling unless its value is justified by the present task. Stop searching once a credible solution can be implemented and verified; resume only if evidence or test failures reveal a gap.

Mark critical claims as observed fact, sourced fact, conditional inference, or untested hypothesis. Never silently promote an assumption to a universal rule.

At the end, validate the original objective, not just local task completion. If blocked, state the blocker and what was verified.

## Optional skills

When there is a meaningful open-source reuse or implementation research question, load `skills/open-source-scout/SKILL.md`. Do not activate this for trivial edits or when a reliable answer already exists.
