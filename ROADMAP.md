# Overall Project Progress: 67% (86 completed / 129 total tasks)

Welcome to the central progress tracking roadmap for the **HoDoKu Native C++20 High-Performance Windows Edition**. This document serves as the single source of truth for architectural objectives, completed milestones, technical definitions of done, and queued engineering tasks.

---

## Goal 1: Core Engine & AVX2 Vectorized Bitboard Operations
High-level architectural objective: Deliver an ultra-fast, zero-allocation Sudoku constraint and candidate engine utilizing SIMD bitboards and Knuth's Dancing Links exact cover solver.

### Implementation Plan 1.1: 81-Bit Vectorized Bitboard (`BitSet81`) & SIMD Primitives
- **Planned Impact:** 6%
- **Technical Approach & DoD:**
  - Encapsulate 81 cells across two 64-bit unsigned integers (`lo` and `hi`).
  - Utilize AVX2, BMI2, and POPCNT intrinsics (`_mm256_or_si256`, `_mm256_and_si256`, `_tzcnt_u64`, `_mm_popcnt_u64`) with portable C++20 standard library fallbacks.
  - **Definition of Done:** Pass full bitwise verification in `bin/test_core.exe` with sub-microsecond execution time and zero heap allocations.
- **Granular Checklist:**
  - [x] Implement 81-bit container utilizing dual `uint64_t` registers (`lo` and `hi`).
  - [x] Vectorize bitwise `AND`, `OR`, `XOR`, `ANDNOT`, and inversion with AVX2/BMI2.
  - [x] Implement `popcount()` and fast bit scanning (`countr_zero`, `first_set`, `next_set`).
  - [x] Implement cell intersection and peer masks with zero dynamic allocations.

### Implementation Plan 1.2: Sudoku Board Representation & Constraint Propagation (`BoardState`)
- **Planned Impact:** 6%
- **Technical Approach & DoD:**
  - Represent values as `uint8_t` (0 for unfilled, 1-9 for set digits) and candidate masks as `uint16_t` bitfields.
  - Precompute house lookup tables (`row_cells`, `col_cells`, `box_cells`, `peer_cells`) at compile-time/initialization.
  - **Definition of Done:** Correct candidate elimination across all 20 peers upon cell assignment; bidirectional serialization between 81-character strings and board states.
- **Granular Checklist:**
  - [x] Implement 81-cell value array and candidate bitmask tracking (`CandidateMask`).
  - [x] Precalculate lookup tables (`row_cells`, `col_cells`, `box_cells`, `peer_cells`).
  - [x] Implement constraint checking (rule violations, duplicate detection in houses).
  - [x] Implement string serialization and deserialization for 81-char string and 9-line format.

### Implementation Plan 1.3: Knuth's Dancing Links (DLX) Exact Cover Engine (`DlxSolver`)
- **Planned Impact:** 6%
- **Technical Approach & DoD:**
  - Map Sudoku constraints to a 324-column exact cover matrix (81 cell constraints, 81 row-digit, 81 col-digit, 81 box-digit).
  - Implement Algorithm X with minimal branching heuristic and multi-solution counting.
  - **Definition of Done:** Solve any valid Sudoku in under 50 microseconds; detect multi-solution or invalid puzzles accurately.
- **Granular Checklist:**
  - [x] Build 324-column exact cover matrix (cell, row-digit, col-digit, box-digit).
  - [x] Implement Algorithm X with recursive backtracking and column min-branch heuristic.
  - [x] Implement multi-solution counter (`count_solutions` with cutoff limit).
  - [x] Implement fast unique solution validator for procedural generation.

---

## Goal 2: Complete 45+ Solving Technique Hierarchy & Hint Escalation
High-level architectural objective: Replicate 100% of original HoDoKu solving algorithms with identical step naming, difficulty scoring, and elimination reasoning.

### Implementation Plan 2.1: Basic & Subset Techniques
- **Planned Impact:** 5%
- **Technical Approach & DoD:**
  - Search houses for Full House, Naked Single, Hidden Single, Locked Candidates (Pointing & Claiming), and Naked/Hidden Subsets (Pairs, Triples, Quads).
  - **Definition of Done:** Detect all basic and subset eliminations across standard regression exemplars; verified by `bin/test_core.exe`.
- **Granular Checklist:**
  - [x] Implement Full House and Naked Single detection.
  - [x] Implement Hidden Single across rows, columns, and boxes.
  - [x] Implement Locked Candidates (Pointing and Claiming).
  - [x] Implement Naked Subsets (Pairs, Triples, Quads).
  - [x] Implement Hidden Subsets (Pairs, Triples, Quads).

### Implementation Plan 2.2: Advanced Fish & Single Digit Patterns
- **Planned Impact:** 6%
- **Technical Approach & DoD:**
  - Implement combinatorial line/column fish solvers up to size 4 (X-Wing, Swordfish, Jellyfish) with finned, sashimi, Franken, and mutant variants.
  - Implement single-digit conjugate chains (Skyscraper, 2-String Kite, Turbot Fish, Empty Rectangle, Dual Empty Rectangle).
  - **Definition of Done:** Detect all basic and finned fish and single digit patterns with exact elimination sets.
- **Granular Checklist:**
  - [x] Implement Basic Fish (X-Wing, Swordfish, Jellyfish).
  - [x] Implement Finned and Sashimi Fish variants.
  - [x] Implement Franken and Mutant Fish solvers.
  - [x] Implement Single Digit Patterns: Skyscraper, 2-String Kite, and Turbot Fish.
  - [x] Implement Empty Rectangle (ER) and Dual Empty Rectangle (Dual ER).

### Implementation Plan 2.3: Wings, Coloring, & Uniqueness Patterns
- **Planned Impact:** 6%
- **Technical Approach & DoD:**
  - Implement bivalue wings (XY-Wing, XYZ-Wing, W-Wing, WXYZ-Wing).
  - Implement Simple Colors (Rule 1 Trap & Rule 2 Wrap) and Multi-Colors (Type 1 Bridge & Type 2 Wrap).
  - Implement Unique Rectangles (Types 1-6), Avoidable Rectangles (Types 1-2), and BUG+1.
  - **Definition of Done:** Match Java HoDoKu elimination outputs across color graphs and uniqueness deadly pattern traps.
- **Granular Checklist:**
  - [x] Implement Wings suite: XY-Wing, XYZ-Wing, W-Wing, and WXYZ-Wing.
  - [x] Implement Simple Colors (Rule 1: Color Trap, Rule 2: Color Wrap).
  - [x] Implement Multi-Colors (Type 1 Bridge, Type 2 Wrap).
  - [x] Implement Unique Rectangles (Types 1 through 6).
  - [x] Implement Avoidable Rectangles (Types 1 and 2) and BUG+1.

### Implementation Plan 2.4: Chains, Loops, & Almost Locked Sets (ALS)
- **Planned Impact:** 6%
- **Technical Approach & DoD:**
  - Construct alternating inference graphs for Remote Pairs, X-Chains, XY-Chains, and Grouped AIC.
  - Implement Cell and Region Forcing Chains, ALS-XZ, ALS-XY-Wing, Sue de Coq, Death Blossom, and Template sets.
  - **Definition of Done:** Discover complex chain contradictions and ALS overlaps without recursion overflow; validated in test suite.
- **Granular Checklist:**
  - [x] Implement Remote Pairs and Simple Chains.
  - [x] Implement X-Chains and XY-Chains.
  - [x] Implement Alternating Inference Chains (AIC) and Grouped AIC.
  - [x] Implement Cell and Region Forcing Chains.
  - [x] Implement ALS-XZ (Rule 1 & 2), ALS-XY-Wing, and ALS-XY-Chain.
  - [x] Implement Sue de Coq and Death Blossom.
  - [x] Implement Template Set and Template Delete algorithms.

### Implementation Plan 2.5: FAS (Find All Steps) & Hint Escalation Workflow
- **Planned Impact:** 5%
- **Technical Approach & DoD:**
  - Aggregate all technique modules into a prioritized hierarchy (`StepFinder`).
  - Provide 4-stage progressive hint workflow: Vague Hint -> Concrete Hint -> Execute Step -> Solve Up To.
  - **Definition of Done:** Compute FAS in <10ms for standard grids; format explanations with house coordinates and eliminated candidates.
- **Granular Checklist:**
  - [x] Implement prioritized logical step finder (`StepFinder::find_next_step`).
  - [x] Implement Find All Steps (FAS) collecting all available deductions for current state.
  - [x] Implement 4-tier progressive hint system (Vague, Concrete, Execute Step, Solve Up To).
  - [x] Implement hint explanation strings with coordinates and elimination justifications.

---

## Goal 3: Procedural Puzzle Generation & Symmetry Engine
High-level architectural objective: Generate valid, uniquely solvable Sudoku puzzles across all 5 difficulty tiers with configurable geometric symmetries and background generation.

### Implementation Plan 3.1: Symmetric Generator & Difficulty Rating
- **Planned Impact:** 6%
- **Technical Approach & DoD:**
  - Seed random full terminal boards using randomized DLX solver.
  - Dig clues symmetrically (180°, 90°, Axial, Diagonal, Anti-Diagonal, Horizontal, Vertical, None) maintaining unique solvability.
  - Run background generation thread cache keeping pre-generated puzzles ready for instant loading.
  - **Definition of Done:** Generated puzzles verify with exactly 1 solution and match score ranges for Easy, Medium, Hard, Unfair, Extreme.
- **Granular Checklist:**
  - [x] Implement random valid terminal grid generation using DLX random seeding.
  - [x] Implement symmetric clue digger supporting 180°, 90°, Axial, and Diagonal symmetries.
  - [x] Implement difficulty scorer matching HoDoKu scoring tables (Easy, Medium, Hard, Unfair, Extreme).
  - [x] Implement background thread puzzle cache with pre-generated puzzles across 5 levels.

---

## Goal 4: Native Win32 / GDI+ UI Architecture & Visual Fidelity
High-level architectural objective: Deliver a high-performance native Win32/GDI+ desktop application with zero external runtime dependencies, authentic HoDoKu styling, and complete keyboard navigation.

### Implementation Plan 4.1: Vector Grid Canvas, 3D ColorKu Spheres, & DPI Awareness
- **Planned Impact:** 6%
- **Technical Approach & DoD:**
  - Double-buffered GDI+ rendering engine with anti-aliasing and ClearType typography.
  - Render cell backgrounds, candidate pencilmarks, hint markers, and 3D ColorKu marble spheres.
  - Support Per-Monitor V2 high-DPI scaling via application manifest.
  - **Definition of Done:** Clean rendering without flicker or tearing at 4K resolution; crisp 1:1 visual match with original screenshots.
- **Granular Checklist:**
  - [x] Implement double-buffered GDI+ vector renderer (`GridRenderer`).
  - [x] Implement candidate pencilmark rendering with highlighted candidate circles.
  - [x] Implement ColorKu 3D marble rendering with specular gradients and drop shadows.
  - [x] Implement Per-Monitor V2 high-DPI manifest and vector font scaling.

### Implementation Plan 4.2: 4-Tab Multi-View Sidebar & Panels
- **Planned Impact:** 6%
- **Technical Approach & DoD:**
  - Implement native sidebar containing 4 selectable views: Summary, Solution Path, FAS / All Steps, Active Cell / Zoom.
  - **Definition of Done:** Synchronous updates across sidebar tabs upon board modifications or hint executions.
- **Granular Checklist:**
  - [x] Implement Summary Tab displaying difficulty, score breakdown, and clue stats.
  - [x] Implement Solution Path Tab with step list and single-click state jump.
  - [x] Implement All Possible Steps (FAS) Tab with interactive step execution.
  - [x] Implement Active Cell / Zoom Tab with digit pad, candidate pad, and color swatches.

### Implementation Plan 4.3: Interactive 10-Color Palette & Candidate Filter Navigation
- **Planned Impact:** 6%
- **Technical Approach & DoD:**
  - Interactive status bar palette with 10 colors mapped to keyboard shortcuts `A`-`E` and `Shift+A`-`E`, with `R` reset.
  - Replicate official HoDoKu candidate filter navigation (`F1`..`F9`, `Ctrl+F1`..`Ctrl+F9`, `<`/`,` and `>`/`.` cycling, `Space` toggle, `Enter` commit).
  - Isolate modal dialog message pump via `IsDialogMessageW` to prevent edit keystrokes from mutating the grid.
  - **Definition of Done:** 100% parity with `src/help/keyboard.html` layout; seamless keyboard filter and navigation workflow.
- **Granular Checklist:**
  - [x] Implement 10-color interactive status bar palette with primary pairs (`a`..`e`) and secondary tints (`A`..`E`).
  - [x] Implement direct keyboard shortcuts (`A`-`E`, `Shift+A`-`E`, `R` to clear).
  - [x] Implement multi-candidate digit filtering (`F1`..`F9`, `Ctrl+F1`..`Ctrl+F9`, `<`/`,` and `>`/`.` cycling).
  - [x] Implement candidate jump navigation (`Ctrl+Shift+Arrows`), candidate toggle (`Space`), and commit single (`Enter`).
  - [x] Map view tab switching accelerators (`Ctrl+Shift+S/U/O/A/Z`) and difficulty presets (`Ctrl+Shift+1..5`).
  - [x] Isolate modal dialog message pump to prevent key events from leaking to grid.

---

## Goal 5: Native Win32 Modal Dialogs Suite & Desktop Integration
High-level architectural objective: Provide authentic, native Win32 modal dialog replicas of original HoDoKu dialogs with standard Windows typography and accessibility.

### Implementation Plan 5.1: Authentic HoDoKu Modal Dialogs Suite
- **Planned Impact:** 6%
- **Technical Approach & DoD:**
  - Construct Win32 dialogs for About, Set Givens, Savepoints/Bookmarks, Backdoors, Training Configuration, and Preferences.
  - Apply Segoe UI 9pt ClearType typography, `WS_TABSTOP` navigation, and ESC key dismissal.
  - **Definition of Done:** Modal dialogs open centered, handle `IDCANCEL`/`IDOK`, persist configuration to `hodoku.ini`, and never crash.
- **Granular Checklist:**
  - [x] Implement `AboutDialog` with author credits, GPLv3 notice, and capability inventory.
  - [x] Implement `SetGivensDialog` with clue counter, multiline edit, and instant load.
  - [x] Implement `SavepointsDialog` (Bookmark Manager) with `SysListView32` table, restore, and delete.
  - [x] Implement `BackdoorsDialog` with level-1 backdoor search and direct grid application.
  - [x] Implement `TrainingConfigDialog` with 12 technique categories and preset filters.
  - [x] Implement `PreferencesDialog` with `SysTabControl32` (General, Colors, Solver) and `hodoku.ini` persistence.
  - [x] Fine-tune all dialogs with Segoe UI 9pt ClearType, `WS_TABSTOP` navigation, and ESC key dismissal.

### Implementation Plan 5.2: Multi-Format File I/O & Clipboard Integration
- **Planned Impact:** 4%
- **Technical Approach & DoD:**
  - Support standard Sudoku exchange formats (`.sdk`, `.ss`, `.hsol`, plain 81-char text) with open/save dialogs.
  - Implement system clipboard integration for givens, candidates, and grid text.
  - **Definition of Done:** Files saved by native C++ load cleanly in Java HoDoKu and vice versa; verified by `bin/test_file_io.exe`.
- **Granular Checklist:**
  - [x] Implement `.sdk` format parser and writer.
  - [x] Implement `.ss` (Simple Sudoku with `I` and `E` records) format parser and writer.
  - [x] Implement `.hsol` (HoDoKu Solution) archive format parser and writer.
  - [x] Implement Copy Givens (`Ctrl+G`), Copy Candidates (`Ctrl+C`), and Paste Sudoku (`Ctrl+V`).

### Implementation Plan 5.3: High-Resolution Image Export & Native Vector Printing
- **Planned Impact:** 4%
- **Technical Approach & DoD:**
  - Export board images via GDI+ PNG encoder with user-selected dimensions (pixels) and DPI.
  - Print puzzle using Win32 `PrintDlgW` with formatted headers, vector grid layout, and margin control.
  - **Definition of Done:** Exported PNGs match requested resolution; print output renders cleanly on Windows printer DC.
- **Granular Checklist:**
  - [x] Implement `DoExportPng` with custom pixel dimensions, DPI scaling, and candidate toggle.
  - [x] Implement `DoPrintPuzzle` with Win32 `PrintDlgW`, header stats, vector grid scaling, and margins.

---

## Goal 6: Interactive Training, Analytical Tooling, & Extended Features
High-level architectural objective: Provide interactive tutor validation, CLI automation, manual chain drawing, and puzzle generation tooling.

### Implementation Plan 6.1: Learning Mode (Tutor) Real-Time Validation
- **Planned Impact:** 4%
- **Technical Approach & DoD:**
  - Intercept user moves in Learning Mode (`F3`), testing against house duplicate rules and unique solution moves.
  - Provide `Check Progress` (`Ctrl+T`) audit tool identifying incorrectly placed digits.
  - **Definition of Done:** Immediate modal warning upon invalid move; verified by `bin/test_tutor.exe`.
- **Granular Checklist:**
  - [x] Implement real-time entry validation intercepting house duplicates (Rule Violations).
  - [x] Implement real-time solution deviation detection against the unique solution.
  - [x] Implement `Check Progress` (`Ctrl+T`) audit tool highlighting mistakes and deviations.

### Implementation Plan 6.2: Headless CLI Engine & Automated Regression Test Suites
- **Planned Impact:** 3%
- **Technical Approach & DoD:**
  - Support command-line arguments for batch solving (`-b`), batch step extraction (`-bs`), solution checking (`-sc`), and puzzle generation (`-g`).
  - Maintain automated test suites for core engine, file I/O, and tutor validation.
  - **Definition of Done:** CLI commands run headless in terminal and exit with proper exit codes; tests pass 100%.
- **Granular Checklist:**
  - [x] Implement headless CLI mode with parent console attachment.
  - [x] Implement batch puzzle solver (`-b`), batch step logger (`-bs`), and solution checker (`-sc`).
  - [x] Implement standalone unit test suites (`test_core.exe`, `test_file_io.exe`, `test_tutor.exe`).

### Implementation Plan 6.3: Interactive Manual Chain & Link Drawing Tools
- **Planned Impact:** 3%
- **Technical Approach & DoD:**
  - Enable manual link creation mode allowing users to connect candidate digits across cells with strong or weak inference links.
  - Render vector arrows (solid for strong links, dashed for weak links) with customizable color assignments.
  - **Definition of Done:** Users can draw, edit, clear, and save custom inference chains to `.hsol` files.
- **Granular Checklist:**
  - [x] Implement manual link creation mode allowing users to draw strong and weak inference links between candidates.
  - [x] Add directed arrow vector rendering (solid for strong links, dashed for weak links) across cells with candidate snap points.
  - [x] Support custom coloring of drawn link paths and persistence into `.hsol` solution files.

### Implementation Plan 6.4: Step-by-Step Auto-Play Solving Animation
- **Planned Impact:** 3%
- **Technical Approach & DoD:**
  - Implement a Win32 timer-driven playback engine (`SetTimer` / `WM_TIMER`) stepping through the active Solution Path.
  - Provide Play, Pause, Step Forward, Step Backward controls and adjustable playback delay slider.
  - **Definition of Done:** Smooth automatic playback through all solution steps with visual hint highlighting on each step.
- **Granular Checklist:**
  - [x] Implement timed auto-playback controller (`SetTimer`) stepping through the solution path automatically.
  - [x] Add Play / Pause / Step Forward / Step Backward toolbar controls and speed slider.
  - [x] Highlight eliminated candidates and placed digits with visual step transitions during auto-play.

### Implementation Plan 6.5: Pattern-Based Clue Digging & Multi-Puzzle Sheet Printing
- **Planned Impact:** 3%
- **Technical Approach & DoD:**
  - Implement pattern generator allowing users to specify a binary 81-cell mask of desired clue positions.
  - Extend printer engine to support 2, 4, or 6 puzzles per page for printable Sudoku booklet generation.
  - **Definition of Done:** Generator produces valid unique puzzles strictly adhering to the selected clue pattern; printer DC formats multi-grid pages cleanly.
- **Granular Checklist:**
  - [x] Implement custom pattern designer enabling users to specify exact given cell positions for generator digging.
  - [x] Add multi-puzzle print layout options (2, 4, or 6 puzzles per page) for printable Sudoku booklets.

---

## Goal 7: Advanced Sudoku Variants & Extended Constraint Engine
High-level architectural objective: Extend the core exact cover matrix and candidate propagation system to support major Sudoku variants without compromising speed.

### Implementation Plan 7.1: Diagonal (X-Sudoku) & Hyper-Sudoku (Windoku) Constraints
- **Planned Impact:** 2%
- **Technical Approach & DoD:**
  - Add 2 diagonal houses (main and anti-diagonal) to exact cover column constraints.
  - Add 4 internal 3x3 window houses for Hyper-Sudoku.
  - **Definition of Done:** DLX solver and candidate engine recognize diagonal and window constraints; correctly detect conflicts and single/subset techniques across extended houses.
- **Granular Checklist:**
  - [x] Implement `DiagonalBitboards` mapping main and anti-diagonal houses.
  - [x] Implement `HyperSudokuWindows` mapping the four 3x3 interior window regions.
  - [x] Integrate diagonal and window constraint columns into `DlxSolver` exact cover matrix.
  - [ ] Support Diagonal and Windoku mode toggling in UI and generator.

### Implementation Plan 7.2: Killer Sudoku Arithmetic & Cage Elimination
- **Planned Impact:** 3%
- **Technical Approach & DoD:**
  - Define arbitrary cell cages with target sums and unique digit constraints.
  - Implement cage candidate filters using precomputed integer partition lookup tables.
  - **Definition of Done:** Solve Killer Sudoku puzzles using cage arithmetic reductions and exact cover modeling.
- **Granular Checklist:**
  - [ ] Implement `Cage` structure with cell bitmask, target sum, and precomputed integer partitions.
  - [ ] Implement cage candidate propagation eliminating candidates incompatible with target sum combinations.
  - [ ] Add Killer 45-rule house sum deduction technique.
  - [ ] Add Killer Sudoku interactive cage rendering in `GridRenderer`.

### Implementation Plan 7.3: Jigsaw (Irregular / Nonomino) Region Support
- **Planned Impact:** 2%
- **Technical Approach & DoD:**
  - Support arbitrary 9-cell nonomino regions replacing standard 3x3 boxes.
  - Adapt all house-based solving techniques (Locked Candidates, Fish, Subsets) to nonomino houses.
  - **Definition of Done:** Load and solve irregular Sudoku puzzles with custom house definitions.
- **Granular Checklist:**
  - [ ] Support custom 9-cell irregular region maps in `BoardState`.
  - [ ] Generalize box techniques (Pointing/Claiming, Subsets) to operate on arbitrary disjoint shapes.
  - [ ] Implement thick border rendering around irregular boundaries in `GridRenderer`.

---

## Goal 8: Ultra-Scale SIMD Acceleration & Multi-Core Parallelism
High-level architectural objective: Scale solving and generation throughput to millions of puzzles per second using advanced vector instruction sets and lock-free thread pooling.

### Implementation Plan 8.1: AVX-512 & ARM64 NEON Vector Intrinsics
- **Planned Impact:** 2%
- **Technical Approach & DoD:**
  - Implement AVX-512 (512-bit registers) candidate propagation evaluating 4 boards simultaneously.
  - Implement ARM64 NEON intrinsics for Snapdragon Windows on ARM devices.
  - **Definition of Done:** Automatic runtime CPU feature detection (`CPUID`) choosing scalar, AVX2, AVX-512, or NEON code paths.
- **Granular Checklist:**
  - [ ] Implement runtime CPU feature dispatch for AVX-512F / AVX-512BW and ARM64 NEON.
  - [ ] Vectorize 81-cell candidate updates using 512-bit ZMM registers for quad-puzzle batch solving.
  - [ ] Implement ARM64 NEON SIMD bitset operations with zero performance regression.

### Implementation Plan 8.2: Lock-Free Work-Stealing Batch Processing Thread Pool
- **Planned Impact:** 2%
- **Technical Approach & DoD:**
  - Implement a lock-free work-stealing queue thread pool for multi-core scaling across 16+ logical threads.
  - Parallelize batch puzzle generation, brute-force backdoor scanning, and benchmark datasets.
  - **Definition of Done:** Linear scaling up to CPU thread limit without lock contention or thread starvation.
- **Granular Checklist:**
  - [ ] Implement lock-free work-stealing task queue (`WorkStealingPool`).
  - [ ] Parallelize batch generator to produce 10,000+ puzzles/sec across available CPU cores.
  - [ ] Parallelize comprehensive FAS and backdoor search across solution paths.

---

## Goal 9: Modern UI/UX, Native Dark Mode & Vector Fluid Animations
High-level architectural objective: Deliver a modern Windows 11 Fluent aesthetic with seamless Dark Mode support, smooth candidate animations, and touch/pen interaction.

### Implementation Plan 9.1: Win32 Native Dark Mode & Desktop Window Manager (DWM) Styling
- **Planned Impact:** 2%
- **Technical Approach & DoD:**
  - Hook Windows 10/11 dark mode APIs (`DwmSetWindowAttribute`, `SetWindowTheme`, undocumented uxtheme ordinal 135).
  - Provide auto-system theme following or explicit Light / Dark / OLED Black toggle.
  - **Definition of Done:** Dialogs, menus, title bars, status bar, and canvas cleanly render dark themes with zero white flashing.
- **Granular Checklist:**
  - [ ] Hook DWM dark mode title bar and system theme change notifications (`WM_SETTINGCHANGE`).
  - [ ] Create dark mode color schemes for UI canvas, givens, user digits, and pencilmarks.
  - [ ] Owner-draw menus, tab controls, and dialog backgrounds with dark brushes.

### Implementation Plan 9.2: Smooth Vector Animations & Elimination Transitions
- **Planned Impact:** 1%
- **Technical Approach & DoD:**
  - Implement high-frequency timer animation pipeline (60/120 Hz) for candidate fade-out and digit pop-in.
  - Add ripple/pulse effects when highlighting inference link chains.
  - **Definition of Done:** Smooth 60+ FPS visual feedback on step execution and auto-play without CPU overhead.
- **Granular Checklist:**
  - [ ] Implement 60 FPS interpolation animation controller for candidate elimination fades.
  - [ ] Add visual pulse glow along active strong/weak inference link paths.
  - [ ] Provide user preference toggle to disable animations for instant accessibility mode.

---

## Goal 10: Advanced Export, Native Vector PDF & Interoperability
High-level architectural objective: Enhance sharing, publishing, and puzzle exchange through standalone vector PDF generation, QR code scanning, and modern format support.

### Implementation Plan 10.1: Native Vector PDF Document Writer
- **Planned Impact:** 2%
- **Technical Approach & DoD:**
  - Implement lightweight zero-dependency vector PDF writer generating crisp multi-page printable booklets.
  - Include embedded vector outlines, table of contents, customizable booklet margins, and answer keys.
  - **Definition of Done:** Produce valid standalone PDF files viewable in any PDF reader without external libraries.
- **Granular Checklist:**
  - [ ] Implement minimal zero-dependency PDF document builder (`PdfWriter`).
  - [ ] Render vector Sudoku grids, candidate numbers, and headers directly into PDF streams.
  - [ ] Support booklet layout options (1, 2, 4, 6 per page) with auto-generated solution pages in PDF.

### Implementation Plan 10.2: QR Code Puzzle Encoding & Visual Sharing
- **Planned Impact:** 1%
- **Technical Approach & DoD:**
  - Generate QR codes containing puzzle strings, difficulty rating, and solution path URLs.
  - Support pasting clipboard images containing QR codes to automatically decode and load puzzles.
  - **Definition of Done:** Encode any active puzzle into a QR code displayed in GUI or exported to PNG/PDF; decode puzzle from clipboard screenshot.
- **Granular Checklist:**
  - [ ] Implement compact QR code matrix generator for Sudoku strings and metadata.
  - [ ] Display interactive QR Code dialog allowing users to scan with phone cameras.
  - [ ] Support automatic puzzle extraction from clipboard images or screen captures.

---

## Goal 11: Embedded Scripting & Technique Plugin Architecture
High-level architectural objective: Enable advanced users and researchers to develop, test, and register custom Sudoku solving techniques and elimination rules via embedded scripting and a C-ABI plugin interface.

### Implementation Plan 11.1: C-ABI Technique Plugin Interface (`HoDoKuPluginAPI`)
- **Planned Impact:** 2%
- **Technical Approach & DoD:**
  - Define a stable C-ABI header interface (`hodoku_plugin.h`) for dynamically loaded shared libraries (`.dll`).
  - Provide callbacks for inspecting `BoardState`, querying candidate masks, registering custom step names, scores, and candidate eliminations.
  - **Definition of Done:** Third-party `.dll` plugins can be loaded at runtime from a `plugins/` directory and integrated into `StepFinder` and FAS.
- **Granular Checklist:**
  - [ ] Define stable C-ABI plugin header (`hodoku_plugin.h`) exposing board query and elimination APIs.
  - [ ] Implement runtime dynamic library loader (`PluginManager`) scanning `plugins/*.dll` with signature validation.
  - [ ] Integrate plugin-registered techniques into `StepFinder` priority escalation and FAS enumeration.

### Implementation Plan 11.2: Embedded Lua Scripting Engine
- **Planned Impact:** 2%
- **Technical Approach & DoD:**
  - Embed lightweight Lua 5.4 runtime enabling rapid prototyping of novel Sudoku solving heuristics without C++ recompilation.
  - Expose `BoardState`, `BitSet81`, and candidate elimination APIs to Lua scripts.
  - **Definition of Done:** Execute `.lua` technique scripts from GUI or CLI and log deductions in Solution Path.
- **Granular Checklist:**
  - [ ] Embed Lua 5.4 zero-dependency core and bind `BoardState` and `BitSet81` metatables.
  - [ ] Implement Lua technique handler registering custom callbacks into `StepFinder`.
  - [ ] Add interactive Lua console / script runner dialog in GUI for live technique experimentation.

---

## Goal 12: WebAssembly (Wasm / Emscripten) Cross-Platform Web Target
High-level architectural objective: Compile the pure C++20 core engine (`src/core/`) to high-speed WebAssembly, allowing web browsers to run HoDoKu's solver and generator client-side.

### Implementation Plan 12.1: Emscripten Toolchain & Core Engine Wasm Bindings
- **Planned Impact:** 2%
- **Technical Approach & DoD:**
  - Configure CMake/Emscripten build target (`emcc`) isolating `src/core/` (zero Win32 dependencies).
  - Export JavaScript / TypeScript bindings via Embind (`dlx_solve`, `generate_puzzle`, `find_all_steps`, `rate_puzzle`).
  - **Definition of Done:** Produce optimized `hodoku.wasm` and `hodoku.js` (< 250 KB) passing all unit tests in headless Node.js.
- **Granular Checklist:**
  - [ ] Create Emscripten CMake build configuration for pure C++20 core engine.
  - [ ] Implement Embind interface exposing `BoardState`, `DlxSolver`, `StepFinder`, and `Generator`.
  - [ ] Validate headless Wasm execution and benchmark parity against native x86_64 in Node.js test runner.

### Implementation Plan 12.2: Web Canvas Demo & TypeScript Type Declarations
- **Planned Impact:** 1%
- **Technical Approach & DoD:**
  - Author complete TypeScript type definitions (`hodoku.d.ts`) and modern ES module packaging.
  - Provide a zero-dependency HTML5 Canvas web demo showcasing instant solving, step walkthroughs, and generator.
  - **Definition of Done:** Web demo loads in Chrome/Firefox/Safari and solves/generates puzzles at 60 FPS purely on the client.
- **Granular Checklist:**
  - [ ] Generate comprehensive TypeScript declarations (`hodoku.d.ts`) for all exported core structures.
  - [ ] Build minimal zero-dependency HTML5 Canvas web demo (`web/index.html`) demonstrating Wasm solving and generation.

---

## Goal 13: Daily Challenges, Archive Synchronization & Online Puzzle Packs
High-level architectural objective: Provide automated daily puzzle seeding, curated puzzle pack downloading, and local progress tracking without requiring account registration.

### Implementation Plan 13.1: Deterministic Daily Puzzle Generator & Calendar Archive
- **Planned Impact:** 1%
- **Technical Approach & DoD:**
  - Implement deterministic PRNG seeding based on UTC date (`YYYY-MM-DD`) and difficulty level.
  - Build Calendar Archive dialog in Win32 displaying past daily puzzles with completion checkmarks and solve timers.
  - **Definition of Done:** Same daily puzzle is generated across all machines for any given date; solve times stored locally in `hodoku.ini`.
- **Granular Checklist:**
  - [ ] Implement deterministic daily puzzle seeding using SHA-256 / MurmurHash of calendar dates.
  - [ ] Build interactive Daily Calendar dialog (`ShowDailyCalendarDialog`) with date navigation and solve records.
  - [ ] Track completion streaks and personal best times in local persistent settings.

### Implementation Plan 13.2: Online Puzzle Pack Downloader (WinHTTP)
- **Planned Impact:** 1%
- **Technical Approach & DoD:**
  - Use native Windows HTTP Services (`WinHTTP` / `winhttp.dll`, zero third-party dependencies) to fetch curated public puzzle archives.
  - Support downloading top-tier benchmark collections (e.g. AI Escargot, Arto Inkala extreme sets, Gordon Royle 17-clue collection).
  - **Definition of Done:** Download and unpack curated collections directly into HoDoKu library with single click.
- **Granular Checklist:**
  - [ ] Implement native WinHTTP asynchronous client for downloading puzzle catalog manifests.
  - [ ] Build Puzzle Pack Browser dialog allowing one-click download and import into studio library.
  - [ ] Bundle classic benchmark datasets (17-clue collection, hardest historical puzzles) in local cache.
