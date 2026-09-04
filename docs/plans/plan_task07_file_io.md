# Implementation Plan - Task 07: Native File I/O & Formats (.sdk, .ss, .hsol)

> **Task ID:** `TASK-07`  
> **Progress Contribution:** **+3.5%** (Brings project to **93.5%**)  
> **Target Subsystem:** `src/app/` (File Management, Windows Dialogs, Serialization)

---

## 1. Goal & Objectives

Provide native file loading and saving capabilities matching original HoDoKu (v2.2):
* Support standard Sudoku file formats:
  * **`.sdk` / `.txt`:** Standard 81-character single line (or 9×9 text grid).
  * **`.ss`:** Simple Sudoku format with candidate pencilmarks.
  * **`.hsol`:** HoDoKu Solution archive containing givens, user moves, and difficulty metadata.
* Native Windows file picker integration via `GetOpenFileNameW` and `GetSaveFileNameW`.
* Seamless integration into `File -> Open...` (`Ctrl+O`), `File -> Save...` (`Ctrl+S`), and `File -> Save As...`.

---

## 2. Java HoDoKu Parity Reference

* `src/sudoku/FileManager.java`:
  * `readPuzzle(File f)`: Detects format by content and file extension.
  * `writePuzzle(File f, Sudoku sudoku, ...)`: Exports current board, givens, and history.
* Supported formats in `FileManager`:
  * SDK (`.sdk`)
  * SS (`.ss`)
  * HSOL (`.hsol`)
  * Standard 81-char string (`.txt`)

---

## 3. Technical Specifications

### A. New File: `src/app/FileManager.hpp`
* `enum class FileFormat { Sdk, SimpleSudoku, HSol, PlainText, Auto };`
* `struct PuzzleFileData { std::string givens; std::string current_state; std::vector<std::string> steps; };`
* `static bool load_puzzle_file(const std::wstring& path, HoDoKuStudio& studio);`
* `static bool save_puzzle_file(const std::wstring& path, const HoDoKuStudio& studio, FileFormat format);`

### B. Windows Common Dialogs in `src/app/main.cpp`
* **Open File Dialog:**
  * Filter: `All Supported Sudoku Files (*.sdk;*.ss;*.hsol;*.txt)\0*.sdk;*.ss;*.hsol;*.txt\0Sudoku (*.sdk)\0*.sdk\0Simple Sudoku (*.ss)\0*.ss\0HoDoKu Solution (*.hsol)\0*.hsol\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0`
* **Save / Save As File Dialog:**
  * Default extension based on selected filter.
* **Keyboard Shortcuts:**
  * `Ctrl+O`: Open File
  * `Ctrl+S`: Save File
  * `Ctrl+Shift+S`: Save As...

---

## 4. Verification & Testing

1. Automated tests loading sample `.sdk`, `.ss`, `.hsol`, and `.txt` files from disk.
2. Verified round-trip saving and re-loading identical givens and board state.
3. Clean error handling on corrupted/truncated files without application crash.

