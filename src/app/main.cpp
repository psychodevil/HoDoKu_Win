#include "AppTypes.hpp"
#include "StudioModel.hpp"
#include "GridRenderer.hpp"
#include "Dialogs.hpp"
#include "UiLayout.hpp"
#include "CommandLine.hpp"
#include "Settings.hpp"

#if defined(_MSC_VER)
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#endif

using namespace Gdiplus;
using namespace hodoku::core;
using namespace hodoku::ui;

static std::unique_ptr<HoDoKuStudio> g_studio;
static GridRenderer g_renderer;

bool TrySetCellDigitWithTutor(HWND hwnd, HoDoKuStudio& studio, int cell, int digit) {
    if (digit != 0 && studio.get_game_mode() == GameMode::Learning) {
        auto res = studio.validate_move(cell, digit);
        if (res == MoveValidation::RuleViolation) {
            std::wstring msg = L"Tutor Alert: Digit " + std::to_wstring(digit) + 
                L" at r" + std::to_wstring(cell_row(cell) + 1) + L"c" + std::to_wstring(cell_col(cell) + 1) +
                L" violates Sudoku rules (duplicate in row, column, or box).\n\nDo you want to enter this digit anyway?";
            if (MessageBoxW(hwnd, msg.c_str(), L"Tutor Alert - HoDoKu", MB_YESNO | MB_ICONEXCLAMATION) != IDYES) {
                return false;
            }
        } else if (res == MoveValidation::SolutionDeviation) {
            std::wstring msg = L"Tutor Alert: Digit " + std::to_wstring(digit) + 
                L" at r" + std::to_wstring(cell_row(cell) + 1) + L"c" + std::to_wstring(cell_col(cell) + 1) +
                L" deviates from the unique puzzle solution.\n\nDo you want to enter this digit anyway?";
            if (MessageBoxW(hwnd, msg.c_str(), L"Tutor Alert - HoDoKu", MB_YESNO | MB_ICONWARNING) != IDYES) {
                return false;
            }
        }
    }
    studio.set_cell_digit(cell, digit);
    return true;
}

bool TrySetDigitAtSelectedWithTutor(HWND hwnd, HoDoKuStudio& studio, int digit) {
    if (digit != 0 && studio.get_game_mode() == GameMode::Learning) {
        int cell = studio.get_selected_cell();
        auto res = studio.validate_move(cell, digit);
        if (res == MoveValidation::RuleViolation) {
            std::wstring msg = L"Tutor Alert: Digit " + std::to_wstring(digit) + 
                L" at r" + std::to_wstring(cell_row(cell) + 1) + L"c" + std::to_wstring(cell_col(cell) + 1) +
                L" violates Sudoku rules (duplicate in row, column, or box).\n\nDo you want to enter this digit anyway?";
            if (MessageBoxW(hwnd, msg.c_str(), L"Tutor Alert - HoDoKu", MB_YESNO | MB_ICONEXCLAMATION) != IDYES) {
                return false;
            }
        } else if (res == MoveValidation::SolutionDeviation) {
            std::wstring msg = L"Tutor Alert: Digit " + std::to_wstring(digit) + 
                L" at r" + std::to_wstring(cell_row(cell) + 1) + L"c" + std::to_wstring(cell_col(cell) + 1) +
                L" deviates from the unique puzzle solution.\n\nDo you want to enter this digit anyway?";
            if (MessageBoxW(hwnd, msg.c_str(), L"Tutor Alert - HoDoKu", MB_YESNO | MB_ICONWARNING) != IDYES) {
                return false;
            }
        }
    }
    studio.set_digit_at_selected(digit);
    return true;
}

void DoCheckProgress(HWND hwnd, HoDoKuStudio& studio) {
    auto errors = studio.audit_progress();
    if (errors.empty()) {
        MessageBoxW(hwnd, L"Tutor: Excellent! All entries so far match the unique puzzle solution.", L"Check Progress - HoDoKu", MB_OK | MB_ICONINFORMATION);
    } else {
        std::wstring msg = L"Tutor: Found " + std::to_wstring(errors.size()) + L" incorrect cell(s).\n\n"
                           L"The incorrect cells have been selected on the board.";
        studio.clear_multi_selection();
        for (int c : errors) studio.add_to_selection(c);
        if (!errors.empty()) studio.set_selected_cell(errors[0]);
        UpdateStatusBarText(studio);
        InvalidateRect(hwnd, NULL, FALSE);
        MessageBoxW(hwnd, msg.c_str(), L"Check Progress - HoDoKu", MB_OK | MB_ICONWARNING);
    }
}

// Global Keystroke Processing (100% Exact Replica of HoDoKu v2.2 Specification)
bool ProcessGlobalKeyShortcuts(UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    if (!g_studio) return false;
    if (msg != WM_KEYDOWN && msg != WM_SYSKEYDOWN) return false;

    // Isolate modal dialogs so keystrokes (like typing into edit controls) do not leak to the board
    HWND hActive = GetActiveWindow();
    if (hActive && hActive != g_hwnd) {
        return false;
    }

    bool isCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool isShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool isAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;

    // 0. Escape: In main window, cancels link start/mode, ends coloring mode, or clears active hint step
    if (wParam == VK_ESCAPE) {
        if (g_studio->has_link_start()) {
            g_studio->cancel_link_start();
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (g_studio->is_link_mode()) {
            g_studio->set_link_mode(false);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (g_studio->get_active_color_index() != -1) {
            g_studio->set_active_color_index(-1);
            for (HWND b : g_hStatusColorBtns) if (b) InvalidateRect(b, NULL, TRUE);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (g_studio->get_selected_step().has_value()) {
            g_studio->cancel_hint();
            UpdateHintBoxText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        return false;
    }

    // Alt+X: Quit program (HoDoKu official shortcut)
    if (isAlt && (wParam == 'X' || wParam == 'x')) {
        PostMessageW(g_hwnd, WM_CLOSE, 0, 0);
        return true;
    }

    // 1. Digit Filters: [F1] ... [F9], [Shift][F1] ... [Shift][F9], [Ctrl][F1] ... [Ctrl][F9]
    if (wParam >= VK_F1 && wParam <= VK_F9) {
        int d = static_cast<int>(wParam - VK_F1 + 1);
        if (isCtrl) {
            // [ctrl][F1] ... [ctrl][F9]: Adds/removes candidate to/from multi-filter set
            g_studio->toggle_multi_filter_digit(d);
        } else {
            if (isShift) {
                // [shift][F1] ... [shift][F9]: Inverts allowed/forbidden mode and toggles digit
                g_studio->toggle_filter_mode();
            }
            g_studio->toggle_filter_digit(d);
        }
        for (HWND btn : g_toolbarButtons) if (btn) InvalidateRect(btn, NULL, TRUE);
        InvalidateRect(g_hwnd, NULL, FALSE);
        return true;
    }

    // 2. Filter Navigation: [>] / [.], [<] / [,]
    if (!isCtrl && !isAlt) {
        if (wParam == VK_OEM_COMMA || wParam == VK_OEM_PERIOD) {
            bool forward = (wParam == VK_OEM_PERIOD);
            g_studio->cycle_filter_digit(forward);
            for (HWND btn : g_toolbarButtons) if (btn) InvalidateRect(btn, NULL, TRUE);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
    }

    // 3. Filter Cell Actions: [Space] to toggle candidate, [Enter] to set single or candidate
    if (wParam == VK_SPACE && !isCtrl && !isAlt) {
        if (g_studio->is_filter_active()) {
            g_studio->toggle_filter_candidate_at_selected();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
    }
    if (wParam == VK_RETURN && !isCtrl && !isAlt) {
        if (g_studio->set_single_or_filtered_at_selected()) {
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
    }

    // 4. Hints & Solver Function Keys: F11 (Singles), F12 (Next Step), Alt+F12 (Vague), Ctrl+F12 (Concrete)
    if (wParam == VK_F11) {
        g_studio->set_all_singles();
        if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
        else PopulateListView(*g_studio);
        UpdateHintBoxText(*g_studio);
        UpdateStatusBarText(*g_studio);
        InvalidateRect(g_hwnd, NULL, FALSE);
        return true;
    }
    if (wParam == VK_F12) {
        if (isAlt) {
            g_studio->give_vague_hint();
        } else if (isCtrl) {
            g_studio->give_concrete_hint();
        } else {
            g_studio->show_next_step();
        }
        UpdateHintBoxText(*g_studio);
        InvalidateRect(g_hwnd, NULL, FALSE);
        return true;
    }

    // 5. Views & Tabs Switching (Ctrl+Shift+S, U, O, A, Z, P, C, 1..5)
    if (isCtrl && isShift) {
        if (wParam == 'S') {
            ToggleSudokuOnly(*g_studio);
            return true;
        }
        if (wParam == 'U') {
            SwitchTab(TabView::Summary, *g_studio);
            return true;
        }
        if (wParam == 'O') {
            SwitchTab(TabView::SolutionPath, *g_studio);
            return true;
        }
        if (wParam == 'A') {
            SwitchTab(TabView::AllSteps, *g_studio);
            return true;
        }
        if (wParam == 'Z') {
            SwitchTab(TabView::ActiveCell, *g_studio);
            return true;
        }
        if (wParam == 'P') {
            ShowPreferencesDialog(g_hwnd, *g_studio);
            return true;
        }
        if (wParam == 'C') {
            g_studio->toggle_colorku_mode();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam >= '1' && wParam <= '5') {
            int lvl = static_cast<int>(wParam - '1');
            SendMessageW(g_hLevelCombo, CB_SETCURSEL, lvl, 0);
            g_studio->new_puzzle(lvl);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
    }

    // 6. Navigation & Range Selection (Arrows, Ctrl+Shift+Arrows, Ctrl+Arrows, Shift+Arrows, Home/End)
    if (wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN) {
        int dr = (wParam == VK_DOWN) ? 1 : (wParam == VK_UP) ? -1 : 0;
        int dc = (wParam == VK_RIGHT) ? 1 : (wParam == VK_LEFT) ? -1 : 0;

        if (isCtrl && isShift) {
            // [shift][ctrl][arrows]: Next cell containing filtered candidate
            if (g_studio->is_filter_active()) {
                g_studio->jump_next_filtered_cell(dr, dc);
                if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
                UpdateStatusBarText(*g_studio);
                InvalidateRect(g_hwnd, NULL, FALSE);
                return true;
            }
        }
        if (isCtrl && !isShift && !isAlt) {
            // [ctrl][arrows]: Move cursor to next unsolved cell
            g_studio->jump_next_unsolved_cell();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (isShift && !isCtrl && !isAlt) {
            // [shift][arrows]: Expand selection region
            g_studio->extend_selection_region(dr, dc);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (!isCtrl && !isShift && !isAlt) {
            // Plain arrows: Move cursor
            g_studio->move_selection(dr, dc);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
    }

    if (wParam == VK_HOME) {
        if (isCtrl) {
            g_studio->move_to_home(true); // topmost row
        } else {
            g_studio->move_to_home(false); // leftmost column
        }
        if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
        UpdateStatusBarText(*g_studio);
        InvalidateRect(g_hwnd, NULL, FALSE);
        return true;
    }
    if (wParam == VK_END) {
        if (isCtrl) {
            g_studio->move_to_end(true); // bottommost row
        } else {
            g_studio->move_to_end(false); // rightmost column
        }
        if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
        UpdateStatusBarText(*g_studio);
        InvalidateRect(g_hwnd, NULL, FALSE);
        return true;
    }

    // 7. Control Shortcuts (Ctrl+Z, Ctrl+Y, Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+P, Ctrl+R, Ctrl+C, Ctrl+G, Ctrl+V, Ctrl+E, Ctrl+B, Ctrl+M, Ctrl+T)
    if (isCtrl && !isAlt && !isShift) {
        if (wParam == 'Z') {
            g_studio->undo();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == 'Y') {
            g_studio->redo();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == 'N') {
            int selLvl = SendMessageW(g_hLevelCombo, CB_GETCURSEL, 0, 0);
            g_studio->new_puzzle(selLvl);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == 'O') {
            DoFileOpen(g_hwnd, *g_studio);
            return true;
        }
        if (wParam == 'S') {
            DoFileSave(g_hwnd, *g_studio);
            return true;
        }
        if (wParam == 'P') {
            DoPrintPuzzle(g_hwnd, *g_studio);
            return true;
        }
        if (wParam == 'R') {
            g_studio->reset_puzzle();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == 'C') {
            // [ctrl][c]: Copy candidates (PM grid) to clipboard
            SetClipboardText(g_hwnd, g_studio->export_pm_grid());
            return true;
        }
        if (wParam == 'G') {
            // [ctrl][g]: Copy givens to clipboard
            SetClipboardText(g_hwnd, g_studio->export_givens_string());
            return true;
        }
        if (wParam == 'V') {
            // [ctrl][v]: Paste sudoku from clipboard
            std::string clip = GetClipboardText(g_hwnd);
            if (!clip.empty()) {
                g_studio->import_from_string(clip);
                if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
                else PopulateListView(*g_studio);
                UpdateHintBoxText(*g_studio);
                UpdateStatusBarText(*g_studio);
                InvalidateRect(g_hwnd, NULL, FALSE);
            }
            return true;
        }
        if (wParam == 'E') {
            g_studio->execute_hint();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == 'B') {
            ShowBackdoorsDialog(g_hwnd, *g_studio);
            return true;
        }
        if (wParam == 'M') {
            ShowSavepointsDialog(g_hwnd, *g_studio);
            return true;
        }
        if (wParam == 'T') {
            DoCheckProgress(g_hwnd, *g_studio);
            return true;
        }

        // [Ctrl][1] ... [Ctrl][9]: Toggle candidate in highlighted cell
        if (wParam >= '1' && wParam <= '9') {
            int d = static_cast<int>(wParam - '0');
            g_studio->toggle_candidate_at_selected(d);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }

        // [Ctrl][Delete]: Clear user-drawn inference links
        if (wParam == VK_DELETE) {
            g_studio->clear_user_links();
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
    }

    // 8. Digits 1..9 or Numpad 1..9: Set Cell Value
    if (!isCtrl && !isAlt && ((wParam >= '1' && wParam <= '9') || (wParam >= VK_NUMPAD1 && wParam <= VK_NUMPAD9))) {
        int d = (wParam >= '1' && wParam <= '9') ? static_cast<int>(wParam - '0') : static_cast<int>(wParam - VK_NUMPAD1 + 1);
        if (TrySetDigitAtSelectedWithTutor(g_hwnd, *g_studio, d)) {
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
        }
        return true;
    }

    // 9. Digit Deletion: Backspace, Delete, '0'
    if (!isCtrl && !isAlt && (wParam == VK_BACK || wParam == VK_DELETE || wParam == '0' || wParam == VK_NUMPAD0)) {
        g_studio->set_digit_at_selected(0);
        if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
        else PopulateListView(*g_studio);
        UpdateHintBoxText(*g_studio);
        UpdateStatusBarText(*g_studio);
        InvalidateRect(g_hwnd, NULL, FALSE);
        return true;
    }

    // 10. Cell Coloring by Keystroke: A..E (primary colors) and Shift+A..Shift+E (secondary tints), R to clear
    if (!isCtrl && !isAlt) {
        if (wParam >= 'A' && wParam <= 'E') {
            int baseCol = static_cast<int>(wParam - 'A') * 2;
            int colIdx = isShift ? (baseCol + 1) : baseCol;
            g_studio->set_selected_cell_color(colIdx);
            g_studio->set_active_color_index(colIdx);
            for (HWND b : g_hStatusColorBtns) if (b) InvalidateRect(b, NULL, TRUE);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == 'R') {
            g_studio->clear_all_colors();
            g_studio->set_active_color_index(-1);
            for (HWND b : g_hStatusColorBtns) if (b) InvalidateRect(b, NULL, TRUE);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
    }

    // 11. Manual Link Drawing: Shift+L (toggle link mode), Shift+K (toggle strong/weak link type)
    if (isShift && !isCtrl && !isAlt) {
        if (wParam == 'L') {
            g_studio->toggle_link_mode();
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == 'K') {
            g_studio->toggle_link_type();
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
    }

    return false;
}

// Main Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_studio = std::make_unique<HoDoKuStudio>();
        g_onPuzzleStateChanged = []() {
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
        };
        CreateHoDoKuUI(hwnd);
        SetMenu(hwnd, CreateHoDoKuMenuBar());
        return 0;
    }

    case WM_SIZE: {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        LayoutHoDoKuControls(hwnd, w, h);
        int toolbarH = 36;
        int statusBarH = 20;
        int hintBoxH = 86;
        int middleY = toolbarH + 2;
        int middleH = h - statusBarH - hintBoxH - 4 - middleY - 4;
        g_renderer.update_layout(6, middleY, middleH, middleH);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);
        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
        HGDIOBJ oldBitmap = SelectObject(memDC, memBitmap);

        Graphics g(memDC);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

        // Background
        SolidBrush bg(Color(255, 240, 240, 240));
        g.FillRectangle(&bg, 0, 0, width, height);

        // Render Sudoku Grid
        int toolbarH = 36;
        int statusBarH = 20;
        int hintBoxH = 86;
        int middleY = toolbarH + 2;
        int middleH = height - statusBarH - hintBoxH - 4 - middleY - 4;
        int gridSize = middleH;

        if (g_studio) {
            g_renderer.render_grid_canvas(g, *g_studio, 6, middleY, gridSize, gridSize);
        }

        BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        TRACKMOUSEEVENT tme = {};
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        TrackMouseEvent(&tme);

        if (g_studio) {
            int cell = g_renderer.hit_test_grid(x, y);
            int cand = (cell >= 0) ? g_renderer.hit_test_candidate(x, y, cell) : 0;

            if (cell != g_studio->get_hovered_cell() || cand != g_studio->get_hovered_candidate()) {
                g_studio->set_hovered_cell(cell, cand);
                UpdateStatusBarText(*g_studio);
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return 0;
    }

    case WM_MOUSELEAVE: {
        if (g_studio) {
            bool changed = (g_studio->get_hovered_cell() >= 0 || g_studio->get_hovered_step().has_value());
            g_studio->clear_hover();
            if (changed) {
                UpdateStatusBarText(*g_studio);
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        int cell = g_renderer.hit_test_grid(x, y);
        if (cell >= 0 && g_studio) {
            SetFocus(hwnd);
            bool isCtrl = (wParam & MK_CONTROL) != 0;
            bool isShift = (wParam & MK_SHIFT) != 0;

            int activeCol = g_studio->get_active_color_index();
            if (activeCol >= 0 && !isCtrl && !isShift) {
                // Interactive mouse painting mode (matching StatusColorPanel.java)
                int candDigit = g_renderer.hit_test_candidate(x, y, cell);
                if (candDigit > 0 && !g_studio->is_color_cells_mode()) {
                    g_studio->set_candidate_color(cell, candDigit, activeCol);
                } else {
                    g_studio->set_cell_color(cell, activeCol);
                    g_studio->set_selected_cell(cell);
                }
                if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
                UpdateStatusBarText(*g_studio);
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            if (g_studio->is_link_mode() && !isCtrl && !isShift) {
                int candDigit = g_renderer.hit_test_candidate(x, y, cell);
                if (candDigit > 0 && g_studio->get_board().has_candidate(cell, candDigit)) {
                    g_studio->handle_candidate_link_click(cell, candDigit);
                    UpdateStatusBarText(*g_studio);
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
            }

            if (isCtrl) {
                // Ctrl + Click: toggle cell in multi-selection (SudokuPanel.java lines 872-891)
                if (g_studio->get_selected_cells().empty()) {
                    g_studio->add_to_selection(g_studio->get_selected_cell());
                    g_studio->add_to_selection(cell);
                } else {
                    g_studio->toggle_in_selection(cell);
                }
                g_studio->set_selected_cell(cell);
            } else if (isShift) {
                // Shift + Click: rectangular region selection (SudokuPanel.java lines 892-895)
                int anchor = g_studio->get_selected_cell();
                g_studio->select_region(cell_row(anchor), cell_col(anchor), cell_row(cell), cell_col(cell));
            } else {
                // Normal Click: clear multi-selection
                g_studio->clear_multi_selection();

                int candDigit = g_renderer.hit_test_candidate(x, y, cell);
                if (g_studio->get_active_candidate_color() >= 0 && candDigit > 0) {
                    // Apply candidate color
                    g_studio->set_candidate_color(cell, candDigit, g_studio->get_active_candidate_color());
                } else if (candDigit > 0 && g_studio->get_board().is_unfilled(cell) &&
                           g_studio->get_board().has_candidate(cell, candDigit)) {
                    // Clicking candidate pencilmark directly sets cell value
                    if (TrySetCellDigitWithTutor(hwnd, *g_studio, cell, candDigit)) {
                        g_studio->set_selected_cell(cell);
                    }
                } else {
                    g_studio->set_selected_cell(cell);
                }
            }

            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_RBUTTONDOWN: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        int cell = g_renderer.hit_test_grid(x, y);
        if (cell >= 0 && g_studio) {
            SetFocus(hwnd);

            if (g_studio->is_link_mode()) {
                if (g_studio->has_link_start()) {
                    g_studio->cancel_link_start();
                } else {
                    g_studio->toggle_link_type();
                }
                UpdateStatusBarText(*g_studio);
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            g_studio->set_selected_cell(cell);

            if (g_studio->get_active_color_index() >= 0) {
                // Right-click toggles cell vs candidate coloring mode
                g_studio->toggle_color_cells_mode();
            } else {
                int candDigit = g_renderer.hit_test_candidate(x, y, cell);
                if (candDigit > 0 && g_studio->get_board().is_unfilled(cell)) {
                    // Right-click toggles individual candidate on/off
                    g_studio->toggle_cell_candidate(cell, candDigit);
                }
            }

            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);

        if (id == IDC_BTN_UNDO || id == IDM_EDIT_UNDO) {
            g_studio->undo();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_REDO || id == IDM_EDIT_REDO) {
            g_studio->redo();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_NEW_PUZZLE || id == IDM_FILE_NEW) {
            int selLvl = SendMessageW(g_hLevelCombo, CB_GETCURSEL, 0, 0);
            g_studio->new_puzzle(selLvl);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_RED_GREEN) {
            g_studio->toggle_filter_mode();
            for (HWND b : g_toolbarButtons) InvalidateRect(b, NULL, TRUE);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id >= IDC_BTN_FILTER_1 && id <= IDC_BTN_FILTER_9) {
            int d = id - IDC_BTN_FILTER_1 + 1;
            g_studio->toggle_filter_digit(d);
            for (HWND b : g_toolbarButtons) InvalidateRect(b, NULL, TRUE);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_FILTER_BIVALUE) {
            g_studio->toggle_bivalue_filter();
            for (HWND b : g_toolbarButtons) InvalidateRect(b, NULL, TRUE);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_HINT_NEXT || id == IDC_BTN_HINT_BOX_NEXT || id == IDM_PUZZLE_SHOW_NEXT_STEP) {
            g_studio->show_next_step();
            UpdateHintBoxText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_HINT_VAGUE) {
            g_studio->give_vague_hint();
            UpdateHintBoxText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_HINT_CONCRETE) {
            g_studio->give_concrete_hint();
            UpdateHintBoxText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_HINT_EXECUTE || id == IDC_BTN_HINT_BOX_EXECUTE || id == IDM_PUZZLE_EXECUTE_HINT) {
            g_studio->execute_hint();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_HINT_BOX_CANCEL) {
            g_studio->cancel_hint();
            UpdateHintBoxText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_SINGLES || id == IDM_PUZZLE_SET_SINGLES || id == IDC_BTN_HINT_BOX_SOLVE_UP_TO) {
            g_studio->set_all_singles();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_SOLVE || id == IDM_PUZZLE_SOLVE_DLX) {
            g_studio->solve_dlx();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDM_FILE_RESET) {
            g_studio->reset_puzzle();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDM_FILE_CLEAR) {
            g_studio->clear_grid();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDM_FILE_OPEN) {
            DoFileOpen(hwnd, *g_studio);
        } else if (id == IDM_FILE_SAVE) {
            DoFileSave(hwnd, *g_studio);
        } else if (id == IDM_FILE_SAVE_AS) {
            DoFileSaveAs(hwnd, *g_studio);
        } else if (id == IDM_FILE_EXPORT_PNG) {
            DoExportPng(hwnd, *g_studio);
        } else if (id == IDM_FILE_PRINT) {
            DoPrintPuzzle(hwnd, *g_studio);
        } else if (id == IDM_EDIT_ADD_SAVEPOINT) {
            g_studio->add_savepoint();
            MessageBoxW(hwnd, L"Current board state bookmarked successfully.", L"Bookmark Added - HoDoKu", MB_OK | MB_ICONINFORMATION);
        } else if (id == IDM_EDIT_RESTORE_SAVEPOINT) {
            ShowSavepointsDialog(hwnd, *g_studio);
        } else if (id == IDM_SOLVER_FIND_BACKDOORS) {
            ShowBackdoorsDialog(hwnd, *g_studio);
        } else if (id == IDM_FILE_SET_GIVENS) {
            ShowSetGivensDialog(hwnd, *g_studio);
        } else if (id == IDM_OPTIONS_PREFERENCES || id == 9201) {
            if (g_studio) ShowPreferencesDialog(hwnd, *g_studio);
        } else if (id == IDM_HELP_ABOUT) {
            ShowAboutDialog(hwnd);
        } else if (id == IDM_HELP_MANUAL) {
            ShellExecuteW(hwnd, L"open", L"https://github.com/psychodevil/HoDoKu_Win", NULL, NULL, SW_SHOWNORMAL);
        } else if (id == IDM_HELP_TECHNIQUES) {
            ShellExecuteW(hwnd, L"open", L"http://hodoku.sourceforge.net/en/techniques.php", NULL, NULL, SW_SHOWNORMAL);
        } else if (id == IDM_MODE_PLAYING) {
            g_studio->set_game_mode(GameMode::Playing);
            UpdateStatusBarText(*g_studio);
            UpdateHintBoxText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDM_MODE_LEARNING) {
            g_studio->set_game_mode(GameMode::Learning);
            UpdateStatusBarText(*g_studio);
            UpdateHintBoxText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDM_MODE_PRACTICING) {
            g_studio->set_game_mode(GameMode::Practicing);
            UpdateStatusBarText(*g_studio);
            UpdateHintBoxText(*g_studio);
            ShowPracticingDialog(hwnd, *g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDM_MODE_CONFIG_TRAINING) {
            ShowTrainingConfigDialog(hwnd, *g_studio);
            UpdateStatusBarText(*g_studio);
            UpdateHintBoxText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDM_MODE_CHECK_PROGRESS) {
            DoCheckProgress(hwnd, *g_studio);
        } else if (id == IDM_MODE_DRAW_LINKS) {
            g_studio->toggle_link_mode();
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDM_MODE_LINK_TYPE) {
            g_studio->toggle_link_type();
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDM_MODE_LINK_CLEAR) {
            g_studio->clear_user_links();
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDM_FILE_COPY_GIVENS) {
            SetClipboardText(hwnd, g_studio->export_givens_string());
        } else if (id == IDM_FILE_COPY_PM) {
            SetClipboardText(hwnd, g_studio->export_pm_grid());
        } else if (id == IDM_FILE_PASTE) {
            std::string clip = GetClipboardText(hwnd);
            if (!clip.empty()) {
                g_studio->import_from_string(clip);
                if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
                else PopulateListView(*g_studio);
                UpdateHintBoxText(*g_studio);
                UpdateStatusBarText(*g_studio);
                InvalidateRect(hwnd, NULL, FALSE);
            }
        } else if (id == IDM_EDIT_CLEAR_COLORS) {
            g_studio->clear_all_colors();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDM_VIEW_SUDOKU_ONLY) {
            ToggleSudokuOnly(*g_studio);
        } else if (id == IDM_VIEW_COLORKU) {
            g_studio->toggle_colorku_mode();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDM_VIEW_ACTIVE_CELL) {
            SwitchTab(TabView::ActiveCell, *g_studio);
        } else if (id == IDM_VIEW_SUMMARY) {
            SwitchTab(TabView::Summary, *g_studio);
        } else if (id == IDM_VIEW_SOL_PATH) {
            SwitchTab(TabView::SolutionPath, *g_studio);
        } else if (id == IDM_VIEW_ALL_STEPS) {
            SwitchTab(TabView::AllSteps, *g_studio);
        } else if (id == IDM_FILE_EXIT) {
            PostQuitMessage(0);
        }

        // Active Cell Panel: Set Value Buttons
        if (id >= IDC_ZOOM_SET_BASE + 1 && id <= IDC_ZOOM_SET_BASE + 9) {
            int d = id - IDC_ZOOM_SET_BASE;
            if (TrySetDigitAtSelectedWithTutor(hwnd, *g_studio, d)) {
                UpdateActiveCellPanel(*g_studio);
                UpdateStatusBarText(*g_studio);
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }

        // Active Cell Panel: Toggle Candidate Buttons
        if (id >= IDC_ZOOM_CAND_BASE + 1 && id <= IDC_ZOOM_CAND_BASE + 9) {
            int d = id - IDC_ZOOM_CAND_BASE;
            g_studio->toggle_candidate_at_selected(d);
            UpdateActiveCellPanel(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        }

        // Active Cell Panel: Cell Color Swatches
        if (id >= IDC_ZOOM_COLOR_BASE && id < IDC_ZOOM_COLOR_BASE + 10) {
            int colIdx = id - IDC_ZOOM_COLOR_BASE;
            int actualPaletteIdx = SWATCH_COLOR_MAP[colIdx];
            g_studio->set_selected_cell_color(actualPaletteIdx);
            UpdateActiveCellPanel(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        }

        if (id == IDC_ZOOM_CLEAR_BTN) {
            g_studio->set_selected_cell_color(COLOR_NONE);
            UpdateActiveCellPanel(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        }

        // Active Cell Panel: Candidate Color Swatches
        if (id >= IDC_ZOOM_CAND_COLOR_BASE && id < IDC_ZOOM_CAND_COLOR_BASE + 10) {
            int colIdx = id - IDC_ZOOM_CAND_COLOR_BASE;
            int actualPaletteIdx = SWATCH_COLOR_MAP[colIdx];
            if (g_studio->get_active_candidate_color() == actualPaletteIdx) {
                g_studio->set_active_candidate_color(-1);
            } else {
                g_studio->set_active_candidate_color(actualPaletteIdx);
            }
            UpdateActiveCellPanel(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        }

        if (id == IDC_ZOOM_CAND_CLEAR_BTN) {
            g_studio->set_active_candidate_color(-1);
            UpdateActiveCellPanel(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
        }

        // Status Bar Color Palette Swatches (Colors 0..9)
        if (id >= IDC_STATUS_COLOR_BASE && id < IDC_STATUS_COLOR_BASE + 10) {
            int colIdx = id - IDC_STATUS_COLOR_BASE;
            if (g_studio->get_active_color_index() == colIdx) {
                g_studio->set_active_color_index(-1);
            } else {
                g_studio->set_active_color_index(colIdx);
                if (g_studio->get_selected_cell() >= 0 && g_studio->is_color_cells_mode()) {
                    g_studio->set_selected_cell_color(colIdx);
                }
            }
            for (HWND b : g_hStatusColorBtns) if (b) InvalidateRect(b, NULL, TRUE);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (id == IDC_STATUS_COLOR_RESET) {
            g_studio->set_active_color_index(-1);
            for (HWND b : g_hStatusColorBtns) if (b) InvalidateRect(b, NULL, TRUE);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        return 0;
    }

    case WM_NOTIFY: {
        LPNMHDR pnm = (LPNMHDR)lParam;
        if (pnm->idFrom == IDC_TAB_CONTROL && pnm->code == TCN_SELCHANGE) {
            int curSel = TabCtrl_GetCurSel(g_hTab);
            if (g_studio) g_studio->clear_hover();
            SwitchTab(static_cast<TabView>(curSel), *g_studio);
            return 0;
        }
        if (pnm->idFrom == IDC_LIST_STEPS && (pnm->code == LVN_HOTTRACK || pnm->code == NM_HOVER)) {
            LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
            if (pnmv->iItem >= 0 && g_studio) {
                if (g_currentTab == TabView::SolutionPath) {
                    const auto& path = g_studio->get_solution_path();
                    if (static_cast<size_t>(pnmv->iItem) < path.size()) {
                        g_studio->set_hovered_step(path[pnmv->iItem]);
                    }
                } else if (g_currentTab == TabView::AllSteps) {
                    const auto& steps = g_studio->get_fas_steps();
                    if (static_cast<size_t>(pnmv->iItem) < steps.size()) {
                        g_studio->set_hovered_step(steps[pnmv->iItem]);
                    }
                }
            } else if (g_studio) {
                g_studio->set_hovered_step(std::nullopt);
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        if (pnm->idFrom == IDC_LIST_STEPS && (pnm->code == NM_CLICK || pnm->code == NM_DBLCLK)) {
            LPNMITEMACTIVATE pItem = (LPNMITEMACTIVATE)lParam;
            if (pItem->iItem >= 0 && g_studio) {
                if (g_currentTab == TabView::SolutionPath) {
                    g_studio->select_step_from_path(static_cast<size_t>(pItem->iItem));
                } else if (g_currentTab == TabView::AllSteps) {
                    g_studio->select_step_from_fas(static_cast<size_t>(pItem->iItem));
                }
                UpdateHintBoxText(*g_studio);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        break;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
        Graphics g(pdis->hDC);
        g.SetSmoothingMode(SmoothingModeAntiAlias);

        int id = static_cast<int>(pdis->CtlID);

        // 1. Red/Green Swatch Button in Toolbar
        if (id == IDC_BTN_RED_GREEN) {
            bool excluded = g_studio && g_studio->is_filter_excluded_mode();
            int w = pdis->rcItem.right - pdis->rcItem.left;
            int h = pdis->rcItem.bottom - pdis->rcItem.top;
            int halfW = w / 2;

            // Red half (left)
            Color redCol = excluded ? Color(255, 239, 68, 68) : Color(110, 239, 68, 68);
            SolidBrush redB(redCol);
            g.FillRectangle(&redB, 1, 1, halfW - 1, h - 2);

            // Green half (right)
            Color greenCol = (!excluded) ? Color(255, 34, 197, 94) : Color(110, 34, 197, 94);
            SolidBrush greenB(greenCol);
            g.FillRectangle(&greenB, halfW, 1, w - halfW - 1, h - 2);

            // Active mode highlight indicator
            if (excluded) {
                Pen actPen(Color(255, 153, 27, 27), 2.0f);
                g.DrawRectangle(&actPen, 1, 1, halfW - 1, h - 2);
            } else {
                Pen actPen(Color(255, 20, 83, 45), 2.0f);
                g.DrawRectangle(&actPen, halfW, 1, w - halfW - 2, h - 2);
            }

            Pen bdr(Color(255, 90, 90, 90), 1.0f);
            g.DrawRectangle(&bdr, 0, 0, w - 1, h - 1);
            return TRUE;
        }

        // 2. Toolbar Filter Buttons (1..9, Xy)
        if (id >= IDC_BTN_FILTER_1 && id <= IDC_BTN_FILTER_1 + 9) {
            int d = id - IDC_BTN_FILTER_1 + 1;
            bool active = (d <= 9) ? (g_studio && g_studio->get_active_filter() == d)
                                   : (g_studio && g_studio->is_bivalue_filter());

            Color bgColor = active ? Color(255, 185, 255, 185) : Color(255, 245, 245, 245);
            SolidBrush bgB(bgColor);
            g.FillRectangle(&bgB, 0, 0, pdis->rcItem.right - pdis->rcItem.left, pdis->rcItem.bottom - pdis->rcItem.top);

            Pen bdr(active ? Color(255, 34, 197, 94) : Color(255, 180, 180, 180), active ? 2.0f : 1.0f);
            g.DrawRectangle(&bdr, 0, 0, pdis->rcItem.right - pdis->rcItem.left - 1, pdis->rcItem.bottom - pdis->rcItem.top - 1);

            std::wstring label = (d <= 9) ? std::to_wstring(d) : L"Xy";
            FontFamily ff(L"Segoe UI");
            Font f(&ff, 12, active ? FontStyleBold : FontStyleRegular, UnitPixel);
            SolidBrush textB(active ? Color(255, 0, 100, 0) : Color(255, 50, 50, 50));
            StringFormat fmt;
            fmt.SetAlignment(StringAlignmentCenter);
            fmt.SetLineAlignment(StringAlignmentCenter);
            RectF r(0, 0, static_cast<float>(pdis->rcItem.right - pdis->rcItem.left),
                          static_cast<float>(pdis->rcItem.bottom - pdis->rcItem.top));
            g.DrawString(label.c_str(), -1, &f, r, &fmt, &textB);
            return TRUE;
        }

        // 3. Cell Color Swatches
        if (id >= IDC_ZOOM_COLOR_BASE && id < IDC_ZOOM_COLOR_BASE + 10) {
            int colIdx = id - IDC_ZOOM_COLOR_BASE;
            int actualPaletteIdx = SWATCH_COLOR_MAP[colIdx];
            SolidBrush b(HODOKU_PALETTE[actualPaletteIdx]);
            int w = pdis->rcItem.right - pdis->rcItem.left;
            int h = pdis->rcItem.bottom - pdis->rcItem.top;
            g.FillRectangle(&b, 0, 0, w, h);
            Pen bdr(Color(255, 120, 120, 120), 1.0f);
            g.DrawRectangle(&bdr, 0, 0, w - 1, h - 1);
            return TRUE;
        }

        // 4. Candidate Color Swatches
        if (id >= IDC_ZOOM_CAND_COLOR_BASE && id < IDC_ZOOM_CAND_COLOR_BASE + 10) {
            int colIdx = id - IDC_ZOOM_CAND_COLOR_BASE;
            int actualPaletteIdx = SWATCH_COLOR_MAP[colIdx];
            SolidBrush b(HODOKU_PALETTE[actualPaletteIdx]);
            int w = pdis->rcItem.right - pdis->rcItem.left;
            int h = pdis->rcItem.bottom - pdis->rcItem.top;
            g.FillRectangle(&b, 0, 0, w, h);

            bool isSelectedTool = (g_studio && g_studio->get_active_candidate_color() == actualPaletteIdx);
            Pen bdr(isSelectedTool ? Color(255, 0, 0, 255) : Color(255, 120, 120, 120), isSelectedTool ? 2.0f : 1.0f);
            g.DrawRectangle(&bdr, 0, 0, w - 1, h - 1);
            return TRUE;
        }

        // 5. Status Preview Swatch for Cell Color
        if (id == IDC_ZOOM_STATUS) {
            int w = pdis->rcItem.right - pdis->rcItem.left;
            int h = pdis->rcItem.bottom - pdis->rcItem.top;
            int sel = g_studio ? g_studio->get_selected_cell() : 0;
            int8_t col = (sel >= 0 && g_studio) ? g_studio->get_cell_color(sel) : COLOR_NONE;
            Color c = (col >= 0 && col < 10) ? HODOKU_PALETTE[col] : Color(255, 255, 255, 255);
            SolidBrush b(c);
            g.FillRectangle(&b, 0, 0, w, h);
            Pen bdr(Color(255, 100, 100, 100), 1.0f);
            g.DrawRectangle(&bdr, 0, 0, w - 1, h - 1);
            return TRUE;
        }

        // 6. Status Preview Swatch for Candidate Tool Color
        if (id == IDC_ZOOM_CAND_STATUS) {
            int w = pdis->rcItem.right - pdis->rcItem.left;
            int h = pdis->rcItem.bottom - pdis->rcItem.top;
            int col = g_studio ? g_studio->get_active_candidate_color() : -1;
            Color c = (col >= 0 && col < 10) ? HODOKU_PALETTE[col] : Color(255, 255, 255, 255);
            SolidBrush b(c);
            g.FillRectangle(&b, 0, 0, w, h);
            Pen bdr(Color(255, 100, 100, 100), 1.0f);
            g.DrawRectangle(&bdr, 0, 0, w - 1, h - 1);
            return TRUE;
        }

        // 7. Clear 'R' Buttons in Zoom Panel
        if (id == IDC_ZOOM_CLEAR_BTN || id == IDC_ZOOM_CAND_CLEAR_BTN) {
            int w = pdis->rcItem.right - pdis->rcItem.left;
            int h = pdis->rcItem.bottom - pdis->rcItem.top;
            SolidBrush bgB(Color(255, 240, 240, 240));
            g.FillRectangle(&bgB, 0, 0, w, h);
            Pen bdr(Color(255, 160, 160, 160), 1.0f);
            g.DrawRectangle(&bdr, 0, 0, w - 1, h - 1);

            FontFamily ff(L"Segoe UI");
            Font f(&ff, 11, FontStyleBold, UnitPixel);
            SolidBrush textB(Color(255, 50, 50, 50));
            StringFormat fmt;
            fmt.SetAlignment(StringAlignmentCenter);
            fmt.SetLineAlignment(StringAlignmentCenter);
            RectF r(0, 0, static_cast<float>(w), static_cast<float>(h));
            g.DrawString(L"R", -1, &f, r, &fmt, &textB);
            return TRUE;
        }

        // 8. Status Bar 10-Color Palette Swatches (Colors 0..9)
        if (id >= IDC_STATUS_COLOR_BASE && id < IDC_STATUS_COLOR_BASE + 10) {
            int colIdx = id - IDC_STATUS_COLOR_BASE;
            SolidBrush b(HODOKU_PALETTE[colIdx]);
            int w = pdis->rcItem.right - pdis->rcItem.left;
            int h = pdis->rcItem.bottom - pdis->rcItem.top;
            g.FillRectangle(&b, 0, 0, w, h);

            bool isActive = (g_studio && g_studio->get_active_color_index() == colIdx);
            Pen bdr(isActive ? Color(255, 20, 20, 20) : Color(255, 140, 140, 140), isActive ? 2.0f : 1.0f);
            g.DrawRectangle(&bdr, 0, 0, w - 1, h - 1);

            const wchar_t* letters[10] = { L"a", L"A", L"b", L"B", L"c", L"C", L"d", L"D", L"e", L"E" };
            FontFamily ff(L"Segoe UI");
            Font f(&ff, 9, isActive ? FontStyleBold : FontStyleRegular, UnitPixel);
            SolidBrush textB(Color(255, 40, 40, 40));
            StringFormat fmt;
            fmt.SetAlignment(StringAlignmentCenter);
            fmt.SetLineAlignment(StringAlignmentCenter);
            RectF r(0, 0, static_cast<float>(w), static_cast<float>(h));
            g.DrawString(letters[colIdx], -1, &f, r, &fmt, &textB);
            return TRUE;
        }

        // 9. Status Bar Reset 'R' Button
        if (id == IDC_STATUS_COLOR_RESET) {
            int w = pdis->rcItem.right - pdis->rcItem.left;
            int h = pdis->rcItem.bottom - pdis->rcItem.top;
            SolidBrush bgB(Color(255, 240, 240, 240));
            g.FillRectangle(&bgB, 0, 0, w, h);
            Pen bdr(Color(255, 150, 150, 150), 1.0f);
            g.DrawRectangle(&bdr, 0, 0, w - 1, h - 1);

            FontFamily ff(L"Segoe UI");
            Font f(&ff, 10, FontStyleBold, UnitPixel);
            SolidBrush textB(Color(255, 60, 60, 60));
            StringFormat fmt;
            fmt.SetAlignment(StringAlignmentCenter);
            fmt.SetLineAlignment(StringAlignmentCenter);
            RectF r(0, 0, static_cast<float>(w), static_cast<float>(h));
            g.DrawString(L"R", -1, &f, r, &fmt, &textB);
            return TRUE;
        }
        break;
    }

    case WM_DESTROY: {
        WINDOWPLACEMENT wp = {};
        wp.length = sizeof(WINDOWPLACEMENT);
        if (GetWindowPlacement(hwnd, &wp)) {
            AppSettings s;
            s.window_x = wp.rcNormalPosition.left;
            s.window_y = wp.rcNormalPosition.top;
            s.window_w = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
            s.window_h = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
            s.maximized = (wp.showCmd == SW_SHOWMAXIMIZED);
            if (g_studio) {
                s.game_mode = static_cast<int>(g_studio->get_game_mode());
                s.colorku_mode = g_studio->is_colorku_mode();
                s.filter_excluded = g_studio->is_filter_excluded_mode();
            }
            SettingsManager::save(s);
        }
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    // 1. Process Headless Command-Line Interface (CLI)
    if (CommandLine::process_command_line(__argc, __argv)) {
        return 0;
    }

    // High-DPI Awareness (Per-Monitor V2)
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // GDI+ Startup
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    const wchar_t CLASS_NAME[] = L"HoDoKuNativeStudioWindow";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = CLASS_NAME;

    RegisterClassExW(&wc);

    AppSettings settings = SettingsManager::load();

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"HoDoKu 2.2 - Native C++20 Edition",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        settings.window_x, settings.window_y, settings.window_w, settings.window_h,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        GdiplusShutdown(gdiplusToken);
        return 0;
    }

    if (settings.maximized) {
        nCmdShow = SW_SHOWMAXIMIZED;
    }

    if (g_studio) {
        if (settings.colorku_mode) g_studio->set_colorku_mode(true);
        if (settings.filter_excluded) g_studio->set_filter_excluded_mode(true);
        if (settings.game_mode >= 0 && settings.game_mode <= 2) {
            g_studio->set_game_mode(static_cast<GameMode>(settings.game_mode));
        }
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        HWND hActive = GetActiveWindow();
        if (hActive && hActive != hwnd) {
            HWND hTop = GetAncestor(hActive, GA_ROOT);
            if (hTop && hTop != hwnd) {
                // If Escape is pressed while a modal dialog is active, send IDCANCEL to close it
                if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
                    SendMessage(hTop, WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), 0);
                    continue;
                }
                if (IsDialogMessageW(hTop, &msg)) {
                    continue;
                }
            }
        }
        if (!ProcessGlobalKeyShortcuts(msg.message, msg.wParam, msg.lParam)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    GdiplusShutdown(gdiplusToken);
    return static_cast<int>(msg.wParam);
}
