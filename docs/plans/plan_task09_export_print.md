# Implementation Plan - Task 09: Image Export & Print System

> **Task ID:** `TASK-09`  
> **Progress Contribution:** **+2.0%** (Brings project to **98.0%**)  
> **Target Subsystem:** `src/app/` (GDI+ Image Rendering, File Export, Printing)

---

## 1. Goal & Objectives

Provide graphical export and printing capabilities matching original HoDoKu (v2.2):
* **Export Board to Image (`File -> Export -> PNG Image...`):**
  * Render high-resolution (e.g. 1080×1080 or configurable) PNG of current grid.
  * Supports candidate view, ColorKu marbles, and active step overlays.
* **Print Engine (`File -> Print...` / `File -> Page Setup...`):**
  * Standard Win32 GDI print dialog (`PrintDlgW`).
  * Prints crisp vector grid with givens, candidates, and difficulty header.

---

## 2. Java HoDoKu Parity Reference

* `src/sudoku/PrintUtilities.java` & `src/sudoku/SudokuPanel.java` (lines 1920–2040):
  * `drawPage(...)`: Offscreen rendering to image or print context.
  * `exportImage(File f, String format)`: Saves PNG.

---

## 3. Technical Specifications

### A. GDI+ PNG Image Exporter
* New utility in `src/app/ExportManager.hpp`:
  * `static bool export_grid_to_png(const std::wstring& filePath, const HoDoKuStudio& studio, int targetResolution = 1080);`
  * Creates offscreen `Gdiplus::Bitmap`, acquires `Graphics`, invokes `GridRenderer::render_grid_canvas`.
  * Encodes to PNG via GDI+ PNG encoder (`{557cf406-1a04-11d3-9a73-0000f81ef32e}`).

### B. Windows GDI Print Handler
* `static bool print_puzzle(HWND hwnd, const HoDoKuStudio& studio);`
  * Initializes `PRINTDLGW` structure.
  * `StartDocW`, `StartPageW`, scales grid to printer DPI, `EndPageW`, `EndDocW`.

---

## 4. Verification & Testing

1. Trigger `Export -> PNG Image` -> Verify clean, high-resolution PNG generated and viewable in Windows Photos.
2. Verify ColorKu mode exports authentic 3D marbles.
3. Test print dialog integration with "Microsoft Print to PDF".

