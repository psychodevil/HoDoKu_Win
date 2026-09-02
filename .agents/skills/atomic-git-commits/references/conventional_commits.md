# Conventional Commits Reference & Best Practices

This document provides guidelines and cheat sheets for writing Conventional Commits and maintaining clean Git history.

---

## 1. Structure of a Conventional Commit

```text
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

### Type Definitions
- **`feat`**: Introduces a new feature or user capability.
- **`fix`**: Patches a bug or fixes erroneous behavior.
- **`refactor`**: Code reorganization with zero change to external behavior or functionality.
- **`perf`**: Optimizations that measurably increase throughput or reduce memory consumption.
- **`test`**: Adding missing tests or correcting existing test suites.
- **`build`**: Changes that affect build systems, compilers, flags, or external dependencies.
- **`ci`**: Continuous integration configurations (GitHub Actions, build scripts).
- **`chore`**: Maintenance tasks, `.gitignore` updates, tooling changes.
- **`docs`**: Documentation only (README, architecture docs, inline manuals).

### Scopes
Scopes identify the module or subsystem touched:
- `core`: Core engine algorithms, board state, data structures.
- `solver`: Deduction techniques, elimination logic, search trees.
- `generator`: Puzzle generation, clue digging, symmetry transformations.
- `ui`: Window layout, canvas drawing, control event handlers.
- `skills`: Agent workspace skills, rules, and customization plugins.

---

## 2. Interactive Patch Staging (`git add -p`)

When a single file contains both refactoring and new feature logic, use interactive staging to keep commits atomic:

```bash
git add -p path/to/file.cpp
```

### Key Commands:
- `y`: Stage this hunk.
- `n`: Do not stage this hunk.
- `s`: Split the current hunk into smaller sub-hunks.
- `e`: Manually edit the hunk before staging.
- `q`: Quit interactive staging.

---

## 3. Atomic Commit Checklist

Before running `git commit`, verify:
1. **Compiles cleanly:** The staged code compiles without new compiler warnings or errors.
2. **Tests pass:** The entire automated test suite runs green.
3. **No extraneous files:** No debug logs, test binaries, or editor swap files are staged.
4. **Descriptive message:** The subject line answers the question: *"If applied, this commit will..."*
