---
name: atomic-git-commits
description: >-
  Guides and enforces the creation of clean, atomic Git commits following Conventional Commits standards.
  Use this skill when staging changes, making git commits, structuring pull requests, splitting large diffs
  into logical milestones, or whenever the user asks for atomic commits, commit guidelines, or commit hygiene.
---

# Atomic Git Commits Skill

This skill provides standard operating procedures, validation checklists, and automation tools to structure repository history using clean, atomic Git commits that adhere to Conventional Commits standards.

---

## Core Principles of Atomic Commits

1. **Single Logical Change:**
   Each commit should encapsulate exactly one logical unit of work (one bug fix, one discrete feature, one refactoring step, or one test addition). Never bundle unrelated edits together.
2. **Always Compiling & Passing Tests (Bisectability):**
   Every individual commit must leave the repository in a fully compiling, green-test state. This ensures `git bisect` can isolate bugs without failing on broken intermediate commits.
3. **Separation of Concerns:**
   - Never mix structural refactoring with functional behavior changes.
   - Never mix formatting/whitespace cleanup with business logic changes.
   - Stage tests either alongside the feature or as an immediate follow-up commit.
4. **Revertibility:**
   If a change causes a regression, a single `git revert <hash>` must cleanly undo that feature without collateral damage to other systems.

---

## Conventional Commits Message Format

Each commit message must follow this structure:

```text
<type>(<scope>): <imperative summary>

[optional body explaining motivation, context, and non-obvious rationale]

[optional footer: Closes #123, Breaking Changes, etc.]
```

### Commit Types
| Type | Purpose | Example |
| :--- | :--- | :--- |
| **`feat`** | Introduces a new feature or capability | `feat(generator): add 90-degree rotational symmetry support` |
| **`fix`** | Resolves a bug, defect, or unexpected behavior | `fix(dlx): handle zero-candidate cells without crashing` |
| **`refactor`** | Code change that neither fixes a bug nor adds a feature | `refactor(app): separate GridRenderer from main.cpp` |
| **`perf`** | Performance improvement or algorithmic optimization | `perf(bitset): vectorize BitSet81 bitwise operations with SSE` |
| **`test`** | Adding or modifying unit, integration, or regression tests | `test(forcing-chains): add contradiction test cases` |
| **`docs`** | Documentation changes only | `docs(readme): add compilation instructions for MinGW-w64` |
| **`chore`** | Build scripts, tooling, configuration, dependency updates | `chore(git): ignore build artifacts and temporary files` |

### Subject Line Rules
- Use the **imperative, present tense**: *"implement"*, *"fix"*, *"refactor"*, not *"implemented"* or *"fixes"*.
- Do **not** capitalize the first letter after the colon.
- Do **not** end with a period (`.`).
- Keep under 72 characters.

---

## Standard Atomic Commit Workflow

### Step 1: Analyze the Working Tree
Run:
```bash
git status -s
git diff --stat
```
Identify the discrete milestones present in the working copy.

### Step 2: Use the Commit Analyzer Helper
Run the bundled analysis script to automatically identify file boundaries and group files by domain:
```bash
python .agents/skills/atomic-git-commits/scripts/commit_analyzer.py
```

### Step 3: Stage Exactly One Logical Milestone
Stage only the files belonging to the first milestone:
```bash
git add path/to/file1 path/to/file2
```
*Tip:* For partial file changes, use `git add -p` to stage specific hunks.

### Step 4: Verify the Build & Tests
Before committing, ensure the staged state compiles and tests pass:
```bash
# Example for C++ projects:
g++ -std=c++20 tests/test_core.cpp -I src -o bin/test_core.exe && ./bin/test_core.exe
```

### Step 5: Commit with Conventional Format
```bash
git commit -m "feat(module): concise descriptive imperative message"
```

### Step 6: Verify History
Check the commit:
```bash
git log -n 1 --stat
```
Repeat for remaining unstaged changes until `git status` is clean.
