# World-Lore Memory: v0.1 design

## Retrieval model
- **Entities:** characters, factions, places, rules, artifacts, realms; relationships carry type, time range, cited source.
- **Events:** narrated event ID, in-world time or partial order, actors and consequences; conflicting accounts may coexist.
- **Perspectives:** `world_truth` / `character_belief(subject)` / `player_knowledge(permission, progress)` / `author_proposal`.
- **Creative provenance:** original intent, inspiration and rejected alternatives; not all notions are canonical.
- **Lifecycle:** brainstorm → proposed → approved → revised/superseded. Explicit review needed to approve.
- **Permissions:** player information visibility is a gameplay design rule and should not expose spoiler truths in player-facing tools.
- **Contradiction check:** flag when two current approved claims about the same predicate/time/perspective disagree; do not mark intentional lies, unreliable narrators, or alternate branches as errors.

## Proposed compact entry
```yaml
id: lore:character:<stable-id>
name: <display name>
status: proposed
scope: <work/project>
source: <notion page URL>
version: <version or revision>
author_intent: <why this character exists>
facts:
  - claim: <fact>
    perspective: world_truth
    timeline: <optional in-world date>
    source: <specific page/document>
beliefs:
  - holder: <character id>
    claim: <what they believe>
discoverability:
  - claim_ref: <fact id>
    gate: <player permission or story progress>
relations:
  - predicate: <relation>
    target: <other stable id>
open_questions: []
```
This is a **schema example**, not an instruction to populate facts from memory. Existing Notion pages remain canonical unless the user explicitly approves migration.
