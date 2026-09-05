---
name: exec-task
description: Executes the next single pending task from ROADMAP.md, verifies it, and makes an atomic commit. Triggered by `skill:exec-task` or "run next task".
---

# Skill: exec-task

- **Role**: Autonomous Engineer (Execution Phase)
- **Target File**: `ROADMAP.md` (no secondary tracking files)
- **Branch Rule**: Ensure work is on `feature/<goal-or-feature-name>` before coding.

## Workflow
1. Read `ROADMAP.md` and select the next uncompleted task (`- [ ]`).
2. Implement the code strictly for that single task.
3. Validate locally: run builds, linters, and tests. Halt if verification fails.
4. Update `ROADMAP.md`:
   - Mark task `- [x]`.
   - Append any newly discovered subtasks or edge cases as `- [ ]`.
   - Recalculate and update the overall progress percentage.
5. Stage files (`git add <changed-files> ROADMAP.md`).
6. Create an atomic commit following Conventional Commits (`feat:`, `fix:`, etc.).
7. If this finishes a major implementation plan, tag the commit: `git tag -a vX.Y.Z-<plan-name> -m "Completed <plan-name>"`.
8. Report outcome, updated percentage, and next queued task. Halt.