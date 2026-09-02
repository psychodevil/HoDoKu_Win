# HoDoKu Native (C++20 Edition)

[![Language](https://img.shields.io/badge/Language-C%2B%2B20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![Platform](https://img.shields.io/badge/Platform-Windows-0078D6.svg)](https://microsoft.com/windows)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-green.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![CI](https://github.com/psychodevil/HoDoKu_Win/actions/workflows/ci.yml/badge.svg)](https://github.com/psychodevil/HoDoKu_Win/actions)

A modern, high-performance native Windows C++20 port and recreation of the classic **HoDoKu** Sudoku Studio by Bernhard Hobiger. Re-engineered from the ground up with zero runtime dependencies beyond native Windows Win32 and GDI+.

---

## Highlights & Features

### Exact HoDoKu GUI Replication
- **Faithful Window Layout:** Custom toolbar, digit filter buttons (`1`–`9`, `Xy`), full-width bottom Hint Panel, and tabbed side panels.
- **Side Panel Tabs:**
  - **Summary (`F6`):** Statistical breakdown of techniques required, counts, and score weights.
  - **Solution Path (`F7`):** Step-by-step resolution path from given clues to complete solution.
  - **All Possible Steps (`F8`):** Complete Find-All-Steps (FAS) listing of every valid deduction in the current position.
  - **Active Cell (`F5`):** 3x3 keypad for setting cell values, candidate toggles, and cell/candidate palette swatches.
- **Coloring Palette:** Full 10-color HoDoKu palette (`a`–`e`, `A`–`E`) for cell highlights and candidate marking.
- **Interactive Board Canvas:** GDI+ double-buffered canvas with candidate pencilmarks, smooth antialiasing, and mouse/keyboard navigation.

### Comprehensive Technique Hierarchy
Implements the complete logical solving hierarchy:
1. **Singles:** Naked Single, Hidden Single, Full House.
2. **Subsets:** Naked Pairs/Triples/Quads, Hidden Pairs/Triples/Quads.
3. **Intersections:** Pointing and Claiming (Box-Line Reduction).
4. **Wings:** XY-Wing, XYZ-Wing, W-Wing.
5. **Fish:** X-Wing, Swordfish, Jellyfish (basic, sashimi, finned).
6. **Single Digit Patterns / Coloring:** Simple Colors, Multi-Colors, Turbot Fish, Skyscraper, Two-String Kite.
7. **Chains:** X-Chains, XY-Chains, Remote Pairs.
8. **Almost Locked Sets (ALS):** ALS-XZ, ALS-XY-Wing, ALS-Chain.
9. **Forcing Chains & Templates:** Contradiction Chains, Cell Forcing Chains, Region Forcing Chains, Template Delete/Set.
10. **Exact Cover Solver:** Donald Knuth's Dancing Links (DLX) algorithm for instantaneous solver checks and uniqueness verification.

### Procedural Puzzle Generator & Symmetries
- Built-in generator supporting 6 geometric symmetries:
  - **180° Rotational** (`Rotational180`)
  - **90° 4-fold Rotational** (`Rotational90`)
  - **Full 360° Symmetrical** (`Full360`)
  - **Horizontal Mirror** (`Horizontal`)
  - **Vertical Mirror** (`Vertical`)
  - **Diagonal Mirror** (`Diagonal`)
- Minimal clue digging ensuring a mathematically unique solution with target difficulty evaluation (`Easy`, `Medium`, `Hard`, `Unfair`, `Extreme`).

### Game Modes
- **Playing Mode (`F2`):** Standard manual solving with full undo/redo and progress tracking.
- **Learning Mode (`F3`):** Interactive tutor mode recommending the next logical step with explanations directly in the hint box.
- **Practicing Mode (`F4`):** Generates puzzles specifically targeting advanced solving techniques (Subsets, Fish, Wings, Chains).

---

## Architectural Layout

The project adheres to a modular design separating the core solver engine from the Win32/GDI+ presentation layer:

```text
HoDoKu_Win/
├── .agents/skills/             # Custom workspace skills
│   ├── atomic-git-commits/     # Automated commit staging planner & Conventional Commits guidelines
│   └── simd-scanner/           # Automated C++ SIMD vectorization analyzer & AVX2 optimization guide
├── .github/workflows/          # GitHub Actions CI workflow (Windows C++20 build & test)
├── src/
│   ├── app/                    # Native Windows UI Layer
│   │   ├── AppTypes.hpp        # Enums, snapshot structures, palettes, control IDs
│   │   ├── StudioModel.hpp     # Decoupled HoDoKuStudio business logic & state machine
│   │   ├── GridRenderer.hpp    # Pure GDI+ canvas rendering & coordinate hit testing
│   │   ├── Dialogs.hpp         # Modal Set Givens, Preferences, and File Open/Save
│   │   ├── UiLayout.hpp        # Control creation, tabs, active cell layout, status bar
│   │   └── main.cpp            # WinMain entry point, message loop, shortcut dispatcher
│   └── core/                   # Header-Only Sudoku Engine
│       ├── Types.hpp           # Core coordinate helpers and types
│       ├── BitSet81.hpp        # High-performance 81-bit bitboard
│       ├── GridConstants.hpp   # Precomputed lookup tables (peers, houses, units)
│       ├── BoardState.hpp      # Fast candidate propagation and constraint board
│       ├── DlxSolver.hpp       # Dancing Links (DLX) exact cover solver
│       ├── Step.hpp            # Deduction step, assignment, and elimination structures
│       ├── StepFinder.hpp      # Logical technique dispatcher and FAS engine
│       ├── Generator.hpp       # Procedural generator with geometric symmetries
│       └── [Techniques]...     # SimpleTechniques, Subsets, Intersections, Wings, Fish,
│                               # Coloring, Chains, ALS, ForcingChains, Templates
└── tests/
    └── test_core.cpp           # Comprehensive C++20 unit test suite
```

---

## Building and Running

### Prerequisites
- **Compiler:** Any modern C++20 compiler (GCC 13+ / Clang 16+ / MSVC 2022).
- **Platform:** Windows 10/11 (64-bit).
- **Libraries:** GDI+, ComCtl32, GDI32, User32, ComDlg32 (included in Windows SDK / MinGW-w64).

### Option 1: Direct Build (MinGW-w64 GCC)

```powershell
# Build and run the test suite
g++ -std=c++20 -O3 -Wall -Wextra tests/test_core.cpp -I src -o bin/test_core.exe
.\bin\test_core.exe

# Build the native Windows GUI application
g++ -std=c++20 -O3 -Wall -Wextra src/app/main.cpp -I src -lgdiplus -lcomctl32 -lgdi32 -luser32 -lcomdlg32 -o bin/hodoku_native.exe
.\bin\hodoku_native.exe
```

### Option 2: CMake Build

```powershell
cmake -B build -S .
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

---

## Keyboard Shortcuts

| Shortcut | Action |
| :--- | :--- |
| **`F2`** | Switch to **Playing Mode** |
| **`F3`** | Switch to **Learning Mode** (Tutor Guidance Active) |
| **`F4`** | Switch to **Practicing Mode** |
| **`F5`** | Switch to **Active Cell** tab |
| **`F6`** | Switch to **Summary** tab |
| **`F7`** | Switch to **Solution Path** tab |
| **`F8`** | Switch to **All Possible Steps** tab |
| **`F11`** | Set All Singles |
| **`F12`** | Show Next Step |
| **`Alt + F12`** | Vague Hint |
| **`Ctrl + F12`** | Concrete Hint |
| **`Ctrl + E`** | Execute Current Hint |
| **`Ctrl + Z` / `Ctrl + Y`** | Undo / Redo |
| **`Ctrl + N`** | Generate New Random Sudoku |
| **`Ctrl + R`** | Restart / Reset Current Puzzle |
| **`Ctrl + G`** | Open **Set Givens** Modal Dialog |
| **`Ctrl + O` / `Ctrl + S`** | Open / Save Sudoku File |
| **`Ctrl + C`** | Copy Pencilmark Grid to Clipboard |
| **`Ctrl + V`** | Paste Sudoku String from Clipboard |
| **`Ctrl + P`** | Open Preferences Dialog |
| **`1` – `9`** | Set Value in Selected Cell |
| **`Ctrl + 1` – `9`** | Toggle Candidate in Selected Cell |
| **`Backspace` / `Del`** | Clear Selected Cell |
| **`R`** | Clear All Cell and Candidate Colors |

---

## License

This project is licensed under the **GNU General Public License v3.0** (GPL-3.0), consistent with the original HoDoKu codebase. See the [COPYING](COPYING) file for full details.
