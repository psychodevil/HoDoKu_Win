# Implementation Plan - Task 10: Advanced Solver Patterns (Empty Rectangle, Grouped AIC)

> **Task ID:** `TASK-10`  
> **Progress Contribution:** **+2.0%** (Brings project to **100.0% COMPLETE**)  
> **Target Subsystem:** `src/core/` (Solving Algorithms, Single Digit Patterns, Chaining)

---

## 1. Goal & Objectives

Implement the final remaining solving techniques from original HoDoKu:
* **Empty Rectangle (ER) & Dual Empty Rectangle:**
  * Single-digit intersection pattern where candidates in a box form an L-shape or cross, interacting with a conjugate pair to eliminate candidate at intersection.
* **Grouped Alternating Inference Chains (Grouped AIC):**
  * Extends AIC chains to allow groups of cells (e.g. mini-lines in a box) to act as a single node with strong/weak links.

---

## 2. Java HoDoKu Parity Reference

* `src/solver/SingleDigitPatternSolver.java`:
  * `findEmptyRectangle()` & `findDualEmptyRectangle()`.
* `src/solver/ChainingSolver.java`:
  * Grouped nodes in continuous/discontinuous nice loops and AIC.

---

## 3. Technical Specifications

### A. Empty Rectangle in `src/core/SingleDigitPatterns.hpp`
* For each digit $d \in 1 \dots 9$:
  * For each box $b \in 0 \dots 8$:
    * Detect if cells with candidate $d$ in box $b$ are confined to at most one row and one column within the box (Empty Rectangle structure).
    * Locate external conjugate pair in another house sharing a line with the box.
    * Identify and eliminate candidate at the intersection cell.

### B. Grouped Links in `src/core/Chains.hpp`
* Group cells within the same box and row/column as a `GroupNode`.
* Add strong links between `GroupNode` and opposite conjugate cells.

---

## 4. Verification & Testing

1. Create unit tests in `tests/test_core.cpp` with canonical Empty Rectangle and Grouped AIC puzzles.
2. Verify FAS step counts include Empty Rectangle and Grouped AIC steps when applicable.
3. Full engine performance benchmark with AVX2 SIMD candidate scans.

