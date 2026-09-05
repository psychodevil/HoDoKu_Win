---
name: preflight-check
description: Runs a zero-tolerance build, lint, and test gate to ensure local code will pass remote CI. Triggered by `skill:preflight-check`.
---

# Skill: preflight-check

- **Role**: Quality Assurance & Gatekeeper

## Workflow
1. Verify working directory is clean of untracked build artifacts (`git clean -xfd --dry-run`).
2. Run project linters and static analysis.
3. Perform a clean compilation with warnings treated as errors.
4. Run full test suite with verbose output.
5. Check `git status` for unstaged file leaks or desynced roadmap metrics.
6. Return binary PASS/FAIL status. If FAIL, list failing logs and block commits.