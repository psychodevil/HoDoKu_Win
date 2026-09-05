# Antigravity Project Rules: HoDoKu Native (WinUI 3 / C++20)

## Official Reference Documentation
- **User Manual**: https://hodoku.sourceforge.net/en/docs.php
- **Solving Techniques**: https://hodoku.sourceforge.net/en/techniques.php
- **Solver Configuration**: https://hodoku.sourceforge.net/en/docs_solv.php
- **Sudoku Creator & Rating**: https://hodoku.sourceforge.net/en/docs_cre.php
- **Find All Steps (FAS)**: https://hodoku.sourceforge.net/en/docs_fas.php
- **Solution Paths**: https://hodoku.sourceforge.net/en/docs_sol.php

---

## Architectural Principles
- **Strict Two-Tier Separation**:
  - `src/core/` -> Pure modern C++20 engine core (zero OS/WinRT/XAML dependencies, maximum portability and performance).
  - `src/ui/` -> WinUI 3 (Windows App SDK) + C++/WinRT + Microsoft Win2D presentation layer.
  - `src/idl/` -> MIDL 3.0 interface contracts for UI data-binding.
- **Hardware-Accelerated Grid**:
  - Never instantiate 81 XAML controls for grid cells. Always use Win2D `CanvasControl` (`Microsoft.Graphics.Canvas.UI.Xaml.CanvasControl`) with DirectWrite for digit and candidate pencilmark typography.

---

## HoDoKu Engine Specifications

### 1. Difficulty Level & Score Hierarchy (HoDoKu Standard)
- **Easy (Score < 800)**:
  - Full House (4), Naked Single (4), Hidden Single (14).
- **Medium (Score 800–1800)**:
  - Locked Candidates Type 1 / Pointing (50), Locked Candidates Type 2 / Claiming (50).
  - Naked Pair (60), Hidden Pair (70), Naked Triple (80), Hidden Triple (100).
- **Hard (Score 1800–3500)**:
  - Naked Quad (120), Hidden Quad (150).
  - Basic Fish: X-Wing (140), Swordfish (150), Jellyfish (160).
  - Single Digit Patterns: Skyscraper (130), 2-String Kite (150), Turbot Fish (120), Empty Rectangle (120).
  - Wings: XY-Wing (160), XYZ-Wing (180), W-Wing (150).
  - Uniqueness: Unique Rectangles Types 1–6 (100–140), Avoidable Rectangles (100), BUG+1 (100).
  - Simple Colors (150).
- **Unfair (Score 3500–6000)**:
  - Finned/Sashimi Fish (200–250), Sue de Coq (250), Multi Colors (200), Remote Pairs (110).
  - Chains: X-Chain (160), XY-Chain (160), Nice Loops / AIC (260), Grouped Nice Loops (280).
  - ALS: ALS-XZ (250), ALS-XY-Wing (300).
- **Extreme (Score > 6000 / Incomplete)**:
  - Complex Fish (Franken/Mutant), ALS Chains (350), Death Blossom (350).
  - Forcing Chains & Forcing Nets (400–500), Templates (400), Brute Force / Tabling (500+).

### 2. Core Game Modes & Workflows
- **Play Mode**: Manual solving, real-time candidate updates, multi-level undo/redo, savepoints, progress check.
- **Training / Learning Mode**: Practice specific techniques with automatic pre-solving to the target step.
- **Finding All Steps (FAS)**: Enumerates all valid human-deduction steps available in the current grid state.
- **Solution Paths**: Complete step-by-step resolution log with difficulty rating and aggregate score.
- **Batch Generation / Generator**: Minimal-clue puzzle generation by difficulty level, symmetric patterns (rotational, diagonal, horizontal, vertical), and technique targeting.

---

## C++20 Standards & Performance
- Language Standard: C++20 (`/std:c++20` or `-std=c++20`).
- Bit Manipulation: Use standard `<bit>` (`std::popcount`, `std::countr_zero`) and compiler intrinsics (`__popcnt16`, `_BitScanForward`).
- Candidate Representation: Bitwise `uint16_t` masks for digits 1–9 (`0x001` to `0x100`, full mask `0x1FF`).
- Cell Grid Bitset: `BitSet81` (64-bit lower + 32-bit upper) for zero-allocation fast set operations.
- Asynchronous Compute: Solvers, generators, and brute-force checkers must run on background threads/coroutines without blocking UI threads.

---

## Directory Layout
- `.antigravity/` -> Agent rules, standards, and HoDoKu specs.
- `src/core/` -> Types, BitSet81, GridConstants, BoardState, DLX solver, and deduction engines.
- `src/ui/` -> WinUI 3 Views, MainWindow, App, and Win2D rendering.
- `src/idl/` -> MIDL 3.0 interfaces.
- `src/app/` -> Standalone native desktop GUI application.
- `tests/` -> Native unit and performance benchmarks.

---

## Commit & Workflow Standards
- Conventional Commits: `feat(core): ...`, `feat(ui): ...`, `perf(solver): ...`, `test(core): ...`, `fix(core): ...`.
- All core engine changes must be verified against native unit test suites.

---

## Skill: init-roadmap
- **Role**: Software Architect (Planning Phase).
- **Trigger Command**: `skill:init-roadmap`
- **Constraints**: 
  - Strictly read-only for all application/source code.
  - Single source of truth: `ROADMAP.md` at project root.
- **Workflow**:
  1. Inspect the codebase, tests, and configuration to identify current state.
  2. Create/format `ROADMAP.md` with:
     - Header: Overall Project Progress: `X%` (`[X] completed / [Y] total tasks`).
     - Major goals, subdivided into concrete implementation plans.
     - Checklists using `- [ ]` (pending) and `- [x]` (completed).
  3. Report current progress score and the top 3 queued tasks.
  4. Stop and await feature execution triggers.

---

## Skill: exec-task
- **Role**: Autonomous Engineer (Execution Phase).
- **Trigger Command**: `skill:exec-task`
- **Target File**: `ROADMAP.md` (no secondary tracking files).
- **Git & Branch Constraints**:
  - Check active branch before starting. If working on a distinct feature/milestone, ensure work is on `feature/<goal-or-feature-name>`.
- **Workflow**:
  1. Read `ROADMAP.md` and select the next uncompleted task (`- [ ]`).
  2. Implement code strictly for that single task.
  3. Validate locally: run builds, linters, and tests. Do not proceed if verification fails.
  4. Update `ROADMAP.md`:
     - Mark task `- [x]`.
     - Append any newly discovered subtasks or edge cases as `- [ ]`.
     - Recalculate and update the overall progress percentage.
  5. Stage files (`git add <changed-files> ROADMAP.md`).
  6. Create an atomic commit following Conventional Commits (e.g., `feat: ...`, `fix: ...`).
  7. If this task completes a major implementation plan or milestone, tag the commit: `git tag -a vX.Y.Z-<plan-name> -m "Completed <plan-name>"`.
  8. Report task outcome, updated percentage, and next pending item. Halt.

---

## Skill: replan
- **Role**: Software Architect (Mid-Flight Re-planning).
- **Trigger Command**: `skill:replan <new details>`
- **Workflow**:
  1. Append new requirements or implementation plans under the appropriate goal in `ROADMAP.md`.
  2. Mark all new items as `- [ ]`.
  3. Recalculate the overall progress percentage.
  4. Commit: `git commit -am "docs(roadmap): update specifications and task backlog"`.
  5. Report the updated status summary and halt.

---

## Skill: manage-branch
- **Role**: Git Release Engineer (Branch Lifecycle & Automation).
- **Trigger Command**: `skill:manage-branch <start|finish>`
- **Workflow**:
  1. **If `start`**:
     - Inspect `ROADMAP.md` to find the first implementation plan or goal containing unchecked tasks (`- [ ]`).
     - Automatically derive a clean, kebab-case slug from the plan/goal heading (e.g., `Plan 1: SIMD Vectorization` -> `simd-vectorization`).
     - Check working tree cleanliness (`git status --porcelain`). Halt if uncommitted changes exist.
     - Checkout `main` and pull latest: `git checkout main && git pull origin main`.
     - Create and switch to the branch: `git checkout -b feature/<derived-slug>`.
     - Announce the selected plan and created branch name, then halt.

  2. **If `finish`**:
     - Detect current branch name (`git branch --show-current`). Ensure it matches `feature/<slug>`.
     - Cross-reference the active feature with `ROADMAP.md` and verify all tasks under this section are marked `- [x]`. If any remain `- [ ]`, halt and list the incomplete tasks.
     - Execute local pre-flight build and tests.
     - Switch to `main` and merge cleanly: `git checkout main && git merge --no-ff feature/<slug>`.
     - Generate an annotated release tag automatically using the slug and current plan version:
       `git tag -a v<X.Y.Z>-<slug> -m "Completed <plan-heading>"`.
     - Report the merge, tag creation, and readiness to push.
---

## Skill: fix-cicd
- **Role**: DevOps & Build Systems Engineer.
- **Trigger Command**: `skill:fix-cicd <error-log-or-description>`
- **Constraints**:
  - Focus strictly on build toolchains, CI workflows, dependency manifests, and build config files (e.g., `.github/workflows/`, `CMakeLists.txt`, `Makefile`, `package.json`, `Cargo.toml`).
  - Do not introduce unrelated application code changes.
- **Workflow**:
  1. Isolate the failure mode:
     - Dependency mismatch or missing toolchain binaries.
     - Strict compiler errors / linker flags (`-Werror`, ABI mismatch, missing headers).
     - Dirty runner workspace or missing recursive submodules.
  2. Patch the CI configuration or build manifests to match local and remote environments.
  3. Ensure deterministic dependencies (pin exact toolchain versions and package revisions).
  4. Add or update local verification commands so `skill:exec-task` tests against identical CI flags.
  5. Validate build locally using the clean container or build command.
  6. Commit atomically: `git commit -am "ci: fix build pipeline and toolchain dependencies"`.
  7. Provide a root-cause summary and fix verification.

---

## Skill: preflight-check
- **Role**: Quality Assurance & Gatekeeper.
- **Trigger Command**: `skill:preflight-check`
- **Workflow**:
  1. Clean untracked/ephemeral build artifacts (`git clean -xfd --dry-run` to inspect).
  2. Run full static analysis / linters.
  3. Execute clean compilation from scratch with warnings treated as errors.
  4. Run full automated test suite with verbose failure output.
  5. Check `git status` to ensure no unstaged leaks, un-ignored generated files, or tracking desync.
  6. Output binary PASS/FAIL status. If FAIL, list the exact offending logs and block further commits.