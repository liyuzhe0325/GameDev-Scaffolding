---
name: opensource-solution-research
description: "When solving a complex game-development task, first work backwards from the desired player-facing result, check existing project capabilities, then research relevant open-source implementations and verify fit before building new infrastructure."
---

# Goal-first open-source research

1. Clarify the observable goal and the specific unresolved condition. For simple well-scoped tasks, do not add needless research.
2. Before creating a new subsystem, inspect the existing project for functionality that may already satisfy the condition.
3. Search official examples and mature open-source projects using **the unresolved condition**, plus alternate terms from relevant professional disciplines.
4. Inspect actual source locations and tests. Check documentation, releases, issues, PRs, dependencies, engine compatibility, and license. Do not trust README alone.
5. Distinguish: directly reusable, needs adaptation, technique-only reference, unsuitable. Identify preconditions and known failure modes.
6. If external evidence changes an important assumption, update the approach and cite its origin. Do not claim an optional technology is necessary.
7. Prefer a minimal compatible route to the target, verify the resulting behavior, then stop. Report what remains uncertain.

Only save discoveries that meaningfully change the decision. Do not create new tooling or a full knowledge graph merely to complete this workflow.

## Notes
This skill is a **portable candidate**, not evidence that Hermes/Codex automatically loads it. Check the specific agent's skill-discovery and installation instructions before deploying. Research references are in ../../docs/architecture/opensource-solution-navigation-r1.md.
