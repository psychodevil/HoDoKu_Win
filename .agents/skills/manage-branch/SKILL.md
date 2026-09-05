---
name: manage-branch
description: Automates Git branch creation and milestone completion merges with tags. Triggered by `skill:manage-branch <start|finish> <feature-name>`.
---

# Skill: manage-branch

- **Role**: Git Release Engineer (Branch Lifecycle)

## Workflow
1. **If `start`**:
   - Verify working tree is clean (`git status --porcelain`).
   - Switch to `main` and pull latest: `git checkout main && git pull origin main`.
   - Create and switch to feature branch: `git checkout -b feature/<feature-name>`.
   - Verify or add the feature section in `ROADMAP.md`.
2. **If `finish`**:
   - Verify all tasks under `<feature-name>` in `ROADMAP.md` are marked `- [x]`.
   - Execute local pre-flight build and tests.
   - Switch to `main` and merge cleanly: `git checkout main && git merge --no-ff feature/<feature-name>`.
   - Create an annotated milestone tag: `git tag -a v<X.Y.Z>-<feature-name> -m "Release <feature-name>"`.
   - Report status and wait for push confirmation.