---
name: init-roadmap
description: Audits the workspace and sets up the central ROADMAP.md tracking file. Use when the user types `skill:init-roadmap` or asks to initialize project roadmap planning.
---

# Skill: init-roadmap

- **Role**: Software Architect (Planning Phase)
- **Constraints**: 
  - Strictly read-only for application/source code.
  - Single source of truth: `ROADMAP.md` at project root.

## Workflow
1. Inspect the codebase, tests, and configurations to determine the baseline state.
2. Create/format `ROADMAP.md`:
   - Header: Overall Project Progress: `X%` (`[X] completed / [Y] total tasks`).
   - High-level goals subdivided into implementation plans.
   - Checklists using `- [ ]` (pending) and `- [x]` (completed).
3. Report current progress score and the top 3 queued tasks.
4. Stop and wait for execution approval.