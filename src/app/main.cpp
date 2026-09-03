#include "AppTypes.hpp"
#include "StudioModel.hpp"
#include "GridRenderer.hpp"
#include "Dialogs.hpp"
#include "UiLayout.hpp"
#include "CommandLine.hpp"
#include "Settings.hpp"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")

using namespace Gdiplus;
using namespace hodoku::core;
using namespace hodoku::ui;

static std::unique_ptr<HoDoKuStudio> g_studio;
static GridRenderer g_renderer;

// Global Keystroke Processing (Pre-Filter for 100% Reliable Shortcuts)
bool ProcessGlobalKeyShortcuts(UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    if (!g_studio) return false;
    if (msg != WM_KEYDOWN && msg != WM_SYSKEYDOWN) return false;

    bool isCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool isShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool isAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;

    // 1. Function Keys (F2..F4 Game Modes, F5..F8 Tabs, F11 Singles, F12 Hints)
    if (wParam == VK_F2) {
        g_studio->set_game_mode(GameMode::Playing);
        UpdateStatusBarText(*g_studio);
        UpdateHintBoxText(*g_studio);
        InvalidateRect(g_hwnd, NULL, FALSE);
        return true;
    }
    if (wParam == VK_F3) {
        g_studio->set_game_mode(GameMode::Learning);
        UpdateStatusBarText(*g_studio);
        UpdateHintBoxText(*g_studio);
        InvalidateRect(g_hwnd, NULL, FALSE);
        return true;
    }
    if (wParam == VK_F4) {
        g_studio->set_game_mode(GameMode::Practicing);
        UpdateStatusBarText(*g_studio);
        UpdateHintBoxText(*g_studio);
        ShowPracticingDialog(g_hwnd, *g_studio);
        InvalidateRect(g_hwnd, NULL, FALSE);
        return true;
    }
    if (wParam == VK_F5) {
        SwitchTab(TabView::ActiveCell, *g_studio);
        return true;
    }
    if (wParam == VK_F6) {
        SwitchTab(TabView::Summary, *g_studio);
        return true;
    }
    if (wParam == VK_F7) {
        SwitchTab(TabView::SolutionPath, *g_studio);
        return true;
    }
    if (wParam == VK_F8) {
        SwitchTab(TabView::AllSteps, *g_studio);
        return true;
    }
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

    if (isCtrl && isShift) {
        if (wParam == '0') {
            ToggleSudokuOnly(*g_studio);
            return true;
        }
        if (wParam == '1') {
            SwitchTab(TabView::ActiveCell, *g_studio);
            return true;
        }
        if (wParam == '2') {
            SwitchTab(TabView::Summary, *g_studio);
            return true;
        }
        if (wParam == '3') {
            SwitchTab(TabView::SolutionPath, *g_studio);
            return true;
        }
        if (wParam == '4') {
            SwitchTab(TabView::AllSteps, *g_studio);
            return true;
        }
        if (wParam == 'C') {
            g_studio->toggle_colorku_mode();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
    }

    // 2. Control Shortcuts (Ctrl+Z, Ctrl+Y, Ctrl+N, Ctrl+C, Ctrl+V, Ctrl+G, Ctrl+R, Ctrl+E, Ctrl+H, Ctrl+O, Ctrl+S, Ctrl+P)
    if (isCtrl && !isAlt) {
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
        if (wParam == 'R') {
            g_studio->reset_puzzle();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            else PopulateListView(*g_studio);
            UpdateHintBoxText(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
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
        if (wParam == 'H') {
            if (isShift) {
                g_studio->give_concrete_hint();
            } else {
                g_studio->give_vague_hint();
            }
            UpdateHintBoxText(*g_studio);
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
            ShowPreferencesDialog(g_hwnd);
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
        if (wParam == 'C') {
            SetClipboardText(g_hwnd, g_studio->export_pm_grid());
            return true;
        }
        if (wParam == 'G') {
            ShowSetGivensDialog(g_hwnd, *g_studio);
            return true;
        }
        if (wParam == 'V') {
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

        // Ctrl + 1..9: Toggle candidate in focused cell
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

        // Ctrl + Arrows: Jump to next unsolved cell
        if (wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN) {
            g_studio->jump_next_unsolved_cell();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == VK_HOME) {
            g_studio->move_to_home(true);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == VK_END) {
            g_studio->move_to_end(true);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
    }

    // 3. Arrow Keys Navigation
    if (!isCtrl && !isAlt) {
        if (wParam == VK_UP) {
            g_studio->move_selection(-1, 0);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == VK_DOWN) {
            g_studio->move_selection(1, 0);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == VK_LEFT) {
            g_studio->move_selection(0, -1);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == VK_RIGHT) {
            g_studio->move_selection(0, 1);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == VK_HOME) {
            g_studio->move_to_home(false);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == VK_END) {
            g_studio->move_to_end(false);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
    }

    // 4. Digits 1..9 or Numpad 1..9: Set Cell Value
    if (!isCtrl && !isAlt && ((wParam >= '1' && wParam <= '9') || (wParam >= VK_NUMPAD1 && wParam <= VK_NUMPAD9))) {
        int d = (wParam >= '1' && wParam <= '9') ? static_cast<int>(wParam - '0') : static_cast<int>(wParam - VK_NUMPAD1 + 1);
        g_studio->set_digit_at_selected(d);
        if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
        else PopulateListView(*g_studio);
        UpdateHintBoxText(*g_studio);
        UpdateStatusBarText(*g_studio);
        InvalidateRect(g_hwnd, NULL, FALSE);
        return true;
    }

    // 5. Digit Deletion: Backspace, Delete, '0'
    if (wParam == VK_BACK || wParam == VK_DELETE || wParam == '0' || wParam == VK_NUMPAD0) {
        g_studio->set_digit_at_selected(0);
        if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
        else PopulateListView(*g_studio);
        UpdateHintBoxText(*g_studio);
        UpdateStatusBarText(*g_studio);
        InvalidateRect(g_hwnd, NULL, FALSE);
        return true;
    }

    // 6. Cell Coloring by Keystroke: 1..9 (with Alt or Shift) or 'R' to clear
    if (wParam == 'R' && !isCtrl) {
        g_studio->clear_all_colors();
        if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(*g_studio);
        InvalidateRect(g_hwnd, NULL, FALSE);
        return true;
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
            int candDigit = g_renderer.hit_test_candidate(x, y, cell);

            if (g_studio->get_active_candidate_color() >= 0 && candDigit > 0) {
                // Apply candidate color
                g_studio->set_candidate_color(cell, candDigit, g_studio->get_active_candidate_color());
            } else if (candDigit > 0 && g_studio->get_board().is_unfilled(cell) &&
                       g_studio->get_board().has_candidate(cell, candDigit)) {
                // Clicking candidate pencilmark directly sets cell value
                g_studio->set_cell_digit(cell, candDigit);
                g_studio->set_selected_cell(cell);
            } else {
                g_studio->set_selected_cell(cell);
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
            g_studio->set_selected_cell(cell);

            int candDigit = g_renderer.hit_test_candidate(x, y, cell);
            if (candDigit > 0 && g_studio->get_board().is_unfilled(cell)) {
                // Right-click toggles individual candidate on/off
                g_studio->toggle_cell_candidate(cell, candDigit);
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
        } else if (id == IDM_FILE_EXPORT_PNG) {
            DoExportPng(hwnd, *g_studio);
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
            g_studio->set_digit_at_selected(d);
            UpdateActiveCellPanel(*g_studio);
            UpdateStatusBarText(*g_studio);
            InvalidateRect(hwnd, NULL, FALSE);
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
            g_studio->set_selected_cell_color(0);
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
            int col = (sel >= 0 && g_studio) ? g_studio->get_cell_color(sel) : 0;
            Color c = (col > 0) ? HODOKU_PALETTE[col] : Color(255, 255, 255, 255);
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
            Color c = (col >= 0) ? HODOKU_PALETTE[col] : Color(255, 255, 255, 255);
            SolidBrush b(c);
            g.FillRectangle(&b, 0, 0, w, h);
            Pen bdr(Color(255, 100, 100, 100), 1.0f);
            g.DrawRectangle(&bdr, 0, 0, w - 1, h - 1);
            return TRUE;
        }

        // 7. Clear 'R' Buttons
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
        if (!ProcessGlobalKeyShortcuts(msg.message, msg.wParam, msg.lParam)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    GdiplusShutdown(gdiplusToken);
    return static_cast<int>(msg.wParam);
}
