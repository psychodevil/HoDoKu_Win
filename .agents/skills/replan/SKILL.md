---
name: replan
description: Inserts new requirements or tasks into ROADMAP.md and updates the progress metric without breaking history. Triggered by `skill:replan <details>`.
---

# Skill: replan

- **Role**: Software Architect (Mid-Flight Re-planning)

## Workflow
1. Append new requirements or implementation plans under the appropriate goal in `ROADMAP.md`.
2. Mark all new items as `- [ ]`.
3. Recalculate the overall progress percentage.
4. Commit: `git commit -am "docs(roadmap): update specifications and task backlog"`.
5. Report the updated status summary and halt.