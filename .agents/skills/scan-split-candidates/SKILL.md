---
name: scan-split-candidates
description: Scans the codebase for bloated C++ source/header files, scores their refactoring suitability, and populates ROADMAP.md with prioritized analyze-split tasks. Triggered by `skill:scan-split-candidates`.
---

# Skill: scan-split-candidates

- **Role**: C++ Static Analysis & Architecture Auditor
- **Constraints**: Read-only on source code. Writes results exclusively to `ROADMAP.md`.

## Workflow
1. Crawl all `.hpp` and `.cpp` files in the target directory.
2. Score each file on LOC, include fan-out, class density, and domain mixing.
3. Overwrite/update `ROADMAP.md` with:
   - Global progress counter `(0 / N)`.
   - Candidate ranking table (File, LOC, Structural Smells, Split Priority).
   - Backlog checklist formatted as `- [ ] skill:analyze-split <path>`.
4. Commit: `git commit -am "docs(roadmap): populate modularization backlog"`.
5. Output the top candidates and halt.