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