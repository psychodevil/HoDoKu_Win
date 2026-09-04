# Implementation Plan - Task 08: Learning Mode (Tutor) Interactive Alerts & Validator

> **Task ID:** `TASK-08`  
> **Progress Contribution:** **+2.5%** (Brings project to **96.0%**)  
> **Target Subsystem:** `src/app/` & `src/core/` (Tutor Engine, Validation, Interactive Feedback)

---

## 1. Goal & Objectives

Replicate HoDoKu's interactive **Tutor / Learning Mode**:
* When the user plays in **Learning Mode (`F3`)**:
  * Real-time validation of every value entry or candidate removal.
  * If a move violates Sudoku rules (house duplicate) -> Immediate alert: *"Invalid entry: violates Sudoku rules"*.
  * If a move contradicts the puzzle's unique solution -> Immediate Tutor alert: *"This value deviates from the solution"*, with option to revert or accept.
* Provide menu command: `Mode -> Check Progress` (`Ctrl+T` or menu) to audit all current user entries against the true solution.

---

## 2. Java HoDoKu Parity Reference

* `src/sudoku/SudokuPanel.java` (lines 925–945, 1750–1765):
  * When `gameMode == GameMode.LEARNING`:
    * Checks `sudoku.isValidValue(...)`.
    * Checks `sudoku.getSolution(cellIndex) == value`.
    * Shows Tutor dialog / status text if deviation occurs.

---

## 3. Technical Specifications

### A. Core Validator in `src/app/StudioModel.hpp`
* Store unique terminal solution in `StudioModel` (computed via DLX on initial puzzle setup).
* `bool validate_user_move(int cell, int digit, MoveValidationResult& outResult);`
  * `RuleViolation`: Duplicate in row, column, or block.
  * `SolutionDeviation`: Value differs from true solution.
  * `Valid`: Legal and matches solution.
* `int audit_current_progress(std::vector<int>& outErrorCells);`

### B. UI Integration in `src/app/main.cpp`
* In Learning Mode, before committing user digit:
  * Check `validate_user_move`.
  * If violation or deviation occurs, display a gentle Tutor dialog:
    * `"Tutor Alert: Digit [X] at rYcZ deviates from the solution. Do you want to keep this value anyway?"` (Options: Yes / Revert).
* Status bar flashes warning and highlights error cells in `INVALID_CELL_COLOR` (`#ffb9b9`).

---

## 4. Verification & Testing

1. Test entering wrong values in Learning Mode -> Verify Tutor alert triggers.
2. Test entering correct values -> Verify silent acceptance without intrusive dialogs.
3. Test Playing Mode -> Verify Tutor remains silent (no alerts in Playing mode).

