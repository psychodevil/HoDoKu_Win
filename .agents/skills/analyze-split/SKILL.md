---
name: analyze-split
description: Audits C++ source (.cpp) and header (.hpp) files to evaluate structural decomposition, decouple heavy translation units, and propose a multi-file architecture. Triggered by `skill:analyze-split [path]`.
---

# Skill: analyze-split

- **Role**: Principal C++ Systems Architect (Modularization & Refactoring)
- **Constraints**: Strictly read-only. No code modification permitted.

## Workflow
1. Parse AST, classes, inline templates, and free functions in the target files.
2. Isolate cohesive domains (core types, compute kernels, I/O, internal details).
3. Evaluate forward-declaration viability and eliminate circular dependency risks.
4. Produce a Modularization Blueprint detailing:
   - Proposed file breakdown (`.hpp` / `.cpp` pairs).
   - Public vs. private `detail/` boundaries.
   - Build-time impact and CMake/build-system changes needed.
   - Atomic refactoring checklist formatted for `ROADMAP.md`.