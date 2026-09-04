#pragma once

#include "AppTypes.hpp"
#include "StudioModel.hpp"
#include "Dialogs.hpp"

namespace hodoku::ui {

inline HWND g_hwnd = NULL;
inline HWND g_hTab = NULL;
inline HWND g_hListView = NULL;
inline HWND g_hStatusBar = NULL;
inline HWND g_hLevelCombo = NULL;
inline std::vector<HWND> g_toolbarButtons;

// Bottom Hint Panel Controls
inline HWND g_hHintEdit = NULL;
inline HWND g_hHintNextBtn = NULL;
inline HWND g_hHintExecBtn = NULL;
inline HWND g_hHintSolveUpBtn = NULL;
inline HWND g_hHintCancelBtn = NULL;

// Active Cell (CellZoomPanel) Controls
inline HWND g_hZoomTitle = NULL;
inline HWND g_hZoomSetLabel = NULL;
inline HWND g_hZoomSetBtns[9] = {NULL};
inline HWND g_hZoomCandLabel = NULL;
inline HWND g_hZoomCandBtns[9] = {NULL};
inline HWND g_hZoomColorLabel = NULL;
inline HWND g_hZoomStatus = NULL;
inline HWND g_hZoomColorBtns[10] = {NULL};
inline HWND g_hZoomClearBtn = NULL;
inline HWND g_hZoomDetailsLabel = NULL;
inline HWND g_hZoomCandStatus = NULL;
inline HWND g_hZoomCandColorBtns[10] = {NULL};
inline HWND g_hZoomCandClearBtn = NULL;

inline TabView g_currentTab = TabView::ActiveCell;
inline bool g_sudokuOnly = false;

inline void ShowActiveCellControls(BOOL show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    if (g_hZoomTitle) ShowWindow(g_hZoomTitle, cmd);
    if (g_hZoomStatus) ShowWindow(g_hZoomStatus, cmd);
    if (g_hZoomClearBtn) ShowWindow(g_hZoomClearBtn, cmd);
    if (g_hZoomSetLabel) ShowWindow(g_hZoomSetLabel, cmd);
    for (int i = 0; i < 9; ++i) {
        if (g_hZoomSetBtns[i]) ShowWindow(g_hZoomSetBtns[i], cmd);
    }
    if (g_hZoomCandLabel) ShowWindow(g_hZoomCandLabel, cmd);
    for (int i = 0; i < 9; ++i) {
        if (g_hZoomCandBtns[i]) ShowWindow(g_hZoomCandBtns[i], cmd);
    }
    if (g_hZoomColorLabel) ShowWindow(g_hZoomColorLabel, cmd);
    for (int i = 0; i < 10; ++i) {
        if (g_hZoomColorBtns[i]) ShowWindow(g_hZoomColorBtns[i], cmd);
    }
    if (g_hZoomDetailsLabel) ShowWindow(g_hZoomDetailsLabel, cmd);
    if (g_hZoomCandStatus) ShowWindow(g_hZoomCandStatus, cmd);
    for (int i = 0; i < 10; ++i) {
        if (g_hZoomCandColorBtns[i]) ShowWindow(g_hZoomCandColorBtns[i], cmd);
    }
    if (g_hZoomCandClearBtn) ShowWindow(g_hZoomCandClearBtn, cmd);
}

inline void LayoutActiveCellControls(int x, int y, int w, int h) {
    (void)h;
    int y_cur = y + 2;

    // 1. Blue Header Banner: "Active Cell"
    if (g_hZoomTitle) MoveWindow(g_hZoomTitle, x, y_cur, w, 22, TRUE);
    y_cur += 28;

    int centerX = x + w / 2;
    int btnSize = 44;
    int pad = 2;
    int kWidth = 3 * btnSize + 2 * pad;
    int startX = centerX - kWidth / 2;

    // 2. "Set Value:" Header & Centered 3x3 Keypad
    if (g_hZoomSetLabel) MoveWindow(g_hZoomSetLabel, centerX - 80, y_cur, 160, 16, TRUE);
    y_cur += 20;

    for (int i = 0; i < 9; ++i) {
        int row = i / 3;
        int col = i % 3;
        int bx = startX + col * (btnSize + pad);
        int by = y_cur + row * (btnSize + pad);
        if (g_hZoomSetBtns[i]) MoveWindow(g_hZoomSetBtns[i], bx, by, btnSize, btnSize, TRUE);
    }
    y_cur += 3 * (btnSize + pad) + 12;

    // 3. "Toggle Candidates:" Header & Centered 3x3 Keypad
    if (g_hZoomCandLabel) MoveWindow(g_hZoomCandLabel, centerX - 80, y_cur, 160, 16, TRUE);
    y_cur += 20;

    for (int i = 0; i < 9; ++i) {
        int row = i / 3;
        int col = i % 3;
        int bx = startX + col * (btnSize + pad);
        int by = y_cur + row * (btnSize + pad);
        if (g_hZoomCandBtns[i]) MoveWindow(g_hZoomCandBtns[i], bx, by, btnSize, btnSize, TRUE);
    }
    y_cur += 3 * (btnSize + pad) + 16;

    // 4. "Choose Color for Cell:"
    int colorLeft = centerX - 95;
    if (g_hZoomColorLabel) MoveWindow(g_hZoomColorLabel, colorLeft, y_cur, 190, 16, TRUE);
    y_cur += 20;

    // Preview box on left (30x30)
    if (g_hZoomStatus) MoveWindow(g_hZoomStatus, colorLeft, y_cur, 30, 30, TRUE);

    // 10 Color Swatches in 2 rows of 5 + R button
    int col_w = 22;
    int col_h = 14;
    int swStartX = colorLeft + 36;
    for (int i = 0; i < 10; ++i) {
        int r = i / 5;
        int c = i % 5;
        int cx = swStartX + c * (col_w + 2);
        int cy = y_cur + r * (col_h + 2);
        if (g_hZoomColorBtns[i]) MoveWindow(g_hZoomColorBtns[i], cx, cy, col_w, col_h, TRUE);
    }
    if (g_hZoomClearBtn) MoveWindow(g_hZoomClearBtn, swStartX + 5 * (col_w + 2), y_cur + 2, 22, 26, TRUE);
    y_cur += 38;

    // 5. "Choose Color for Candidates:"
    if (g_hZoomDetailsLabel) MoveWindow(g_hZoomDetailsLabel, colorLeft, y_cur, 190, 16, TRUE);
    y_cur += 20;

    // Candidate Preview box on left (30x30)
    if (g_hZoomCandStatus) MoveWindow(g_hZoomCandStatus, colorLeft, y_cur, 30, 30, TRUE);

    for (int i = 0; i < 10; ++i) {
        int r = i / 5;
        int c = i % 5;
        int cx = swStartX + c * (col_w + 2);
        int cy = y_cur + r * (col_h + 2);
        if (g_hZoomCandColorBtns[i]) MoveWindow(g_hZoomCandColorBtns[i], cx, cy, col_w, col_h, TRUE);
    }
    if (g_hZoomCandClearBtn) MoveWindow(g_hZoomCandClearBtn, swStartX + 5 * (col_w + 2), y_cur + 2, 22, 26, TRUE);
}

inline void UpdateActiveCellPanel(const HoDoKuStudio& studio) {
    if (!g_hZoomTitle) return;
    int cell = studio.get_selected_cell();
    if (cell < 0 || cell >= TOTAL_CELLS) {
        for (int i = 0; i < 9; ++i) {
            if (g_hZoomSetBtns[i]) {
                SetWindowTextW(g_hZoomSetBtns[i], L"");
                EnableWindow(g_hZoomSetBtns[i], FALSE);
            }
            if (g_hZoomCandBtns[i]) {
                SetWindowTextW(g_hZoomCandBtns[i], L"");
                EnableWindow(g_hZoomCandBtns[i], FALSE);
            }
        }
        return;
    }

    bool isGiven = studio.get_board().is_given(cell);
    uint8_t val = studio.get_board().get_value(cell);
    CandidateMask mask = studio.get_board().get_candidates(cell);

    // Set Value Buttons (1..9): Shows number only if allowed/candidate
    for (int d = 1; d <= 9; ++d) {
        if (!g_hZoomSetBtns[d - 1]) continue;
        bool allowed = !isGiven && ((val == d) || (val == 0 && mask_has_digit(mask, d)));
        if (allowed) {
            SetWindowTextW(g_hZoomSetBtns[d - 1], std::to_wstring(d).c_str());
            EnableWindow(g_hZoomSetBtns[d - 1], TRUE);
        } else {
            SetWindowTextW(g_hZoomSetBtns[d - 1], L"");
            EnableWindow(g_hZoomSetBtns[d - 1], FALSE);
        }
    }

    // Toggle Candidate Buttons (1..9): Shows number if candidate is present
    for (int d = 1; d <= 9; ++d) {
        if (!g_hZoomCandBtns[d - 1]) continue;
        if (isGiven || val != 0) {
            SetWindowTextW(g_hZoomCandBtns[d - 1], L"");
            EnableWindow(g_hZoomCandBtns[d - 1], FALSE);
        } else {
            bool hasCand = mask_has_digit(mask, d);
            SetWindowTextW(g_hZoomCandBtns[d - 1], hasCand ? std::to_wstring(d).c_str() : L"");
            EnableWindow(g_hZoomCandBtns[d - 1], TRUE);
        }
    }

    if (g_hZoomStatus) InvalidateRect(g_hZoomStatus, NULL, TRUE);
    if (g_hZoomCandStatus) InvalidateRect(g_hZoomCandStatus, NULL, TRUE);
}

inline void PopulateListView(const HoDoKuStudio& studio) {
    if (!g_hListView) return;

    ListView_DeleteAllItems(g_hListView);

    while (ListView_DeleteColumn(g_hListView, 0));

    LVCOLUMNW lvc{};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;

    if (g_currentTab == TabView::SolutionPath) {
        lvc.cx = 36;  lvc.pszText = const_cast<wchar_t*>(L"#");         ListView_InsertColumn(g_hListView, 0, &lvc);
        lvc.cx = 220; lvc.pszText = const_cast<wchar_t*>(L"Technique"); ListView_InsertColumn(g_hListView, 1, &lvc);
        lvc.cx = 60;  lvc.pszText = const_cast<wchar_t*>(L"Score");     ListView_InsertColumn(g_hListView, 2, &lvc);

        const auto& path = studio.get_solution_path();
        for (size_t i = 0; i < path.size(); ++i) {
            const auto& step = path[i];
            LVITEMW item{};
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = static_cast<int>(i);
            item.lParam = static_cast<LPARAM>(i);

            std::wstring idxStr = std::to_wstring(i + 1);
            item.pszText = const_cast<wchar_t*>(idxStr.c_str());
            ListView_InsertItem(g_hListView, &item);

            std::wstring nameStr(step.name.begin(), step.name.end());
            ListView_SetItemText(g_hListView, static_cast<int>(i), 1, const_cast<wchar_t*>(nameStr.c_str()));

            std::wstring scoreStr = std::to_wstring(step.score);
            ListView_SetItemText(g_hListView, static_cast<int>(i), 2, const_cast<wchar_t*>(scoreStr.c_str()));
        }
    } else if (g_currentTab == TabView::AllSteps) {
        lvc.cx = 36;  lvc.pszText = const_cast<wchar_t*>(L"#");         ListView_InsertColumn(g_hListView, 0, &lvc);
        lvc.cx = 220; lvc.pszText = const_cast<wchar_t*>(L"Technique"); ListView_InsertColumn(g_hListView, 1, &lvc);
        lvc.cx = 60;  lvc.pszText = const_cast<wchar_t*>(L"Score");     ListView_InsertColumn(g_hListView, 2, &lvc);

        const auto& steps = studio.get_fas_steps();
        for (size_t i = 0; i < steps.size(); ++i) {
            const auto& step = steps[i];
            LVITEMW item{};
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = static_cast<int>(i);
            item.lParam = static_cast<LPARAM>(i);

            std::wstring idxStr = std::to_wstring(i + 1);
            item.pszText = const_cast<wchar_t*>(idxStr.c_str());
            ListView_InsertItem(g_hListView, &item);

            std::wstring nameStr(step.name.begin(), step.name.end());
            ListView_SetItemText(g_hListView, static_cast<int>(i), 1, const_cast<wchar_t*>(nameStr.c_str()));

            std::wstring scoreStr = std::to_wstring(step.score);
            ListView_SetItemText(g_hListView, static_cast<int>(i), 2, const_cast<wchar_t*>(scoreStr.c_str()));
        }
    } else if (g_currentTab == TabView::Summary) {
        lvc.cx = 200; lvc.pszText = const_cast<wchar_t*>(L"Technique"); ListView_InsertColumn(g_hListView, 0, &lvc);
        lvc.cx = 55;  lvc.pszText = const_cast<wchar_t*>(L"Count");     ListView_InsertColumn(g_hListView, 1, &lvc);
        lvc.cx = 70;  lvc.pszText = const_cast<wchar_t*>(L"Score");     ListView_InsertColumn(g_hListView, 2, &lvc);

        const auto& path = studio.get_solution_path();
        std::vector<std::pair<std::string, std::pair<int, int>>> summary;
        for (const auto& step : path) {
            bool found = false;
            for (auto& s : summary) {
                if (s.first == step.name) {
                    s.second.first += 1;
                    s.second.second += step.score;
                    found = true;
                    break;
                }
            }
            if (!found) {
                summary.push_back({step.name, {1, step.score}});
            }
        }

        for (size_t i = 0; i < summary.size(); ++i) {
            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = static_cast<int>(i);

            std::wstring nameStr(summary[i].first.begin(), summary[i].first.end());
            item.pszText = const_cast<wchar_t*>(nameStr.c_str());
            ListView_InsertItem(g_hListView, &item);

            std::wstring countStr = std::to_wstring(summary[i].second.first) + L"x";
            ListView_SetItemText(g_hListView, static_cast<int>(i), 1, const_cast<wchar_t*>(countStr.c_str()));

            std::wstring scoreStr = std::to_wstring(summary[i].second.second);
            ListView_SetItemText(g_hListView, static_cast<int>(i), 2, const_cast<wchar_t*>(scoreStr.c_str()));
        }
    }
}

inline void UpdateHintBoxText(const HoDoKuStudio& studio) {
    if (!g_hHintEdit) return;

    auto step = studio.get_selected_step();
    auto hintLvl = studio.get_hint_level();

    if (step) {
        std::wstring text;
        std::wstring name(step->name.begin(), step->name.end());
        std::wstring expl(step->explanation.begin(), step->explanation.end());

        if (hintLvl == HintLevel::Vague) {
            text = L"Vague Hint: Look for " + name + L" somewhere in the grid.";
        } else {
            text = name + L" (Score: " + std::to_wstring(step->score) + L")\r\n" + expl;
        }
        SetWindowTextW(g_hHintEdit, text.c_str());

        EnableWindow(g_hHintExecBtn, TRUE);
        EnableWindow(g_hHintCancelBtn, TRUE);
    } else {
        if (studio.get_game_mode() == GameMode::Learning) {
            auto next = StepFinder::find_next_step(studio.get_board());
            if (next) {
                std::string tutorMsg = "Tutor Guidance [Learning Mode]:\r\n"
                                       "Recommended next step is '" + next->name + "' (" +
                                       std::string(difficulty_name(next->difficulty)) +
                                       ", score: " + std::to_string(next->score) + ").\r\n\r\n" +
                                       next->explanation;
                std::wstring wmsg(tutorMsg.begin(), tutorMsg.end());
                SetWindowTextW(g_hHintEdit, wmsg.c_str());
                EnableWindow(g_hHintExecBtn, FALSE);
                EnableWindow(g_hHintCancelBtn, FALSE);
                return;
            }
        }
        SetWindowTextW(g_hHintEdit, L"Press [Next Hint (F12)] or [Vague Hint (Alt+F12)] to find logical deduction steps, or [Solve DLX] for instant solution.");
        EnableWindow(g_hHintExecBtn, FALSE);
        EnableWindow(g_hHintCancelBtn, FALSE);
    }
}

inline void UpdateStatusBarText(const HoDoKuStudio& studio) {
    if (!g_hStatusBar) return;

    std::wstring part1;
    int hCell = studio.get_hovered_cell();
    int hCand = studio.get_hovered_candidate();

    if (!studio.get_selected_cells().empty() && hCell < 0) {
        int count = studio.get_selected_cells().count();
        int actR = cell_row(studio.get_selected_cell()) + 1;
        int actC = cell_col(studio.get_selected_cell()) + 1;
        part1 = L" Multi-Selection: " + std::to_wstring(count) + L" cells selected  |  Anchor: r" + std::to_wstring(actR) + L"c" + std::to_wstring(actC);
    } else if (hCell >= 0 && hCell < TOTAL_CELLS) {
        int r = cell_row(hCell) + 1;
        int c = cell_col(hCell) + 1;
        const auto& board = studio.get_board();
        uint8_t val = board.get_value(hCell);

        part1 = L" Cell r" + std::to_wstring(r) + L"c" + std::to_wstring(c);
        if (val != 0) {
            part1 += L" = " + std::to_wstring(val) + (board.is_given(hCell) ? L" (Given)" : L" (User)");
        } else {
            CandidateMask mask = board.get_candidates(hCell);
            part1 += L"  Candidates: [";
            bool first = true;
            for (int d = 1; d <= 9; ++d) {
                if (mask_has_digit(mask, d)) {
                    if (!first) part1 += L", ";
                    part1 += std::to_wstring(d);
                    first = false;
                }
            }
            part1 += L"]";

            if (hCand > 0 && mask_has_digit(mask, hCand)) {
                part1 += L"  |  Target: Candidate " + std::to_wstring(hCand);
            }
        }
    } else {
        std::wstring lvlName = L"Easy";
        switch (studio.get_hardest_level()) {
            case DifficultyLevel::Easy: lvlName = L"Easy"; break;
            case DifficultyLevel::Medium: lvlName = L"Medium"; break;
            case DifficultyLevel::Hard: lvlName = L"Hard"; break;
            case DifficultyLevel::Unfair: lvlName = L"Unfair"; break;
            case DifficultyLevel::Extreme: lvlName = L"Extreme"; break;
        }

        int score = studio.get_total_score();
        int givens = studio.get_givens_count();
        part1 = L" [Colors: 1-9, R=Clear]  |  Level: " + lvlName + L"  |  Score: " + std::to_wstring(score) + L"  |  Givens: " + std::to_wstring(givens);
    }

    int freeCells = studio.get_unfilled_count();
    int progress = static_cast<int>((81 - freeCells) * 100.0f / 81.0f);

    std::wstring modeStr = L"Playing";
    if (studio.get_game_mode() == GameMode::Learning) {
        modeStr = L"Learning (Tutor Active)";
    } else if (studio.get_game_mode() == GameMode::Practicing) {
        modeStr = L"Practicing (Training Active)";
    }

    std::wstring part2 = L" Progress: " + std::to_wstring(progress) + L"% (" + std::to_wstring(freeCells) + L" free)  |  Mode: " + modeStr;

    SendMessageW(g_hStatusBar, SB_SETTEXT, 0, (LPARAM)part1.c_str());
    SendMessageW(g_hStatusBar, SB_SETTEXT, 1, (LPARAM)part2.c_str());
}

inline void LayoutHoDoKuControls(HWND hwnd, int width, int height) {
    (void)hwnd;
    int toolbarH = 36;
    int statusBarH = 20;
    int hintBoxH = 86;

    // Full-width Bottom Hints Box
    int hintBoxX = 6;
    int hintBoxW = width - 12;
    int hintBoxY = height - statusBarH - hintBoxH - 4;

    int hintBtnW = 92;
    int hintBtnH = 25;
    int hintButtonsAreaW = hintBtnW * 2 + 12;
    int hintTextW = hintBoxW - hintButtonsAreaW - 14;

    if (g_hHintEdit) {
        MoveWindow(g_hHintEdit, hintBoxX + 6, hintBoxY + 16, hintTextW, hintBoxH - 24, TRUE);
    }

    int btnCol1 = hintBoxX + hintTextW + 8;
    int btnCol2 = btnCol1 + hintBtnW + 4;
    int btnRow1 = hintBoxY + 16;
    int btnRow2 = hintBoxY + 45;

    if (g_hHintNextBtn) MoveWindow(g_hHintNextBtn, btnCol1, btnRow1, hintBtnW, hintBtnH, TRUE);
    if (g_hHintExecBtn) MoveWindow(g_hHintExecBtn, btnCol2, btnRow1, hintBtnW, hintBtnH, TRUE);
    if (g_hHintSolveUpBtn) MoveWindow(g_hHintSolveUpBtn, btnCol1, btnRow2, hintBtnW, hintBtnH, TRUE);
    if (g_hHintCancelBtn) MoveWindow(g_hHintCancelBtn, btnCol2, btnRow2, hintBtnW, hintBtnH, TRUE);

    // Middle Playing Area (Grid on Left, Side Panel on Right)
    int middleY = toolbarH + 2;
    int middleH = hintBoxY - middleY - 4;

    int gridSize = middleH;
    int leftPanelW = gridSize + 8;
    int rightPanelX = leftPanelW + 6;
    int rightPanelW = width - rightPanelX - 6;

    if (g_sudokuOnly) {
        if (g_hTab) ShowWindow(g_hTab, SW_HIDE);
        if (g_hListView) ShowWindow(g_hListView, SW_HIDE);
        ShowActiveCellControls(FALSE);
    } else {
        if (g_hTab) {
            ShowWindow(g_hTab, SW_SHOW);
            MoveWindow(g_hTab, rightPanelX, middleY, rightPanelW, middleH, TRUE);
        }

        int rx = rightPanelX + 4;
        int ry = middleY + 26;
        int rw = rightPanelW - 8;
        int rh = middleH - 32;

        if (g_currentTab == TabView::ActiveCell) {
            if (g_hListView) ShowWindow(g_hListView, SW_HIDE);
            ShowActiveCellControls(TRUE);
            LayoutActiveCellControls(rx, ry, rw, rh);
        } else {
            ShowActiveCellControls(FALSE);
            if (g_hListView) {
                ShowWindow(g_hListView, SW_SHOW);
                MoveWindow(g_hListView, rx, ry, rw, rh, TRUE);
            }
        }
    }

    // Status Bar
    if (g_hStatusBar) {
        MoveWindow(g_hStatusBar, 0, height - statusBarH, width, statusBarH, TRUE);
        int parts[2] = { static_cast<int>(width * 0.55f), -1 };
        SendMessageW(g_hStatusBar, SB_SETPARTS, 2, (LPARAM)parts);
    }
}

inline void SwitchTab(TabView tab, const HoDoKuStudio& studio) {
    g_currentTab = tab;
    g_sudokuOnly = false;
    if (g_hTab) TabCtrl_SetCurSel(g_hTab, static_cast<int>(tab));

    RECT rc;
    GetClientRect(g_hwnd, &rc);
    LayoutHoDoKuControls(g_hwnd, rc.right - rc.left, rc.bottom - rc.top);

    if (g_currentTab == TabView::ActiveCell) {
        UpdateActiveCellPanel(studio);
    } else {
        PopulateListView(studio);
    }
    InvalidateRect(g_hwnd, NULL, FALSE);
}

inline void ToggleSudokuOnly(const HoDoKuStudio& studio) {
    g_sudokuOnly = !g_sudokuOnly;
    RECT rc;
    GetClientRect(g_hwnd, &rc);
    LayoutHoDoKuControls(g_hwnd, rc.right - rc.left, rc.bottom - rc.top);
    if (!g_sudokuOnly) {
        if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel(studio);
        else PopulateListView(studio);
    }
    InvalidateRect(g_hwnd, NULL, FALSE);
}

inline void CreateHoDoKuUI(HWND hwnd) {
    g_hwnd = hwnd;
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    // 1. Toolbar Controls (Top matching HoDoKu toolbar)
    int tbX = 8;
    int tbY = 4;
    int btnH = 26;

    auto add_btn = [&](const wchar_t* text, int w, int id) -> HWND {
        HWND btn = CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                 tbX, tbY, w, btnH, hwnd, (HMENU)(INT_PTR)id, NULL, NULL);
        SendMessage(btn, WM_SETFONT, (WPARAM)hFont, TRUE);
        tbX += w + 4;
        g_toolbarButtons.push_back(btn);
        return btn;
    };

    add_btn(L"↶ Undo", 54, IDC_BTN_UNDO);
    add_btn(L"↷ Redo", 54, IDC_BTN_REDO);

    tbX += 4;
    add_btn(L"New", 42, IDC_BTN_NEW_PUZZLE);

    g_hLevelCombo = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                                  tbX, tbY + 1, 80, 140, hwnd, (HMENU)(INT_PTR)IDC_COMBO_LEVEL, NULL, NULL);
    SendMessage(g_hLevelCombo, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageW(g_hLevelCombo, CB_ADDSTRING, 0, (LPARAM)L"Easy");
    SendMessageW(g_hLevelCombo, CB_ADDSTRING, 0, (LPARAM)L"Medium");
    SendMessageW(g_hLevelCombo, CB_ADDSTRING, 0, (LPARAM)L"Hard");
    SendMessageW(g_hLevelCombo, CB_ADDSTRING, 0, (LPARAM)L"Unfair");
    SendMessageW(g_hLevelCombo, CB_ADDSTRING, 0, (LPARAM)L"Extreme");
    SendMessageW(g_hLevelCombo, CB_SETCURSEL, 2, 0); // Default to Hard
    tbX += 86;

    HWND hRgBtn = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                tbX, tbY + 2, 22, 22, hwnd, (HMENU)(INT_PTR)IDC_BTN_RED_GREEN, NULL, NULL);
    g_toolbarButtons.push_back(hRgBtn);
    tbX += 28;

    const wchar_t* filterLabels[10] = {L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"Xy"};
    for (int i = 0; i < 10; ++i) {
        int w = (i == 9) ? 32 : 24;
        int id = IDC_BTN_FILTER_1 + i;
        HWND btn = CreateWindowW(L"BUTTON", filterLabels[i], WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                 tbX, tbY, w, btnH, hwnd, (HMENU)(INT_PTR)id, NULL, NULL);
        tbX += w + 3;
        g_toolbarButtons.push_back(btn);
    }

    // 2. Right Tabbed Panel
    g_hTab = CreateWindowW(WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                           0, 0, 100, 100, hwnd, (HMENU)(INT_PTR)IDC_TAB_CONTROL, NULL, NULL);
    SendMessage(g_hTab, WM_SETFONT, (WPARAM)hFont, TRUE);

    TCITEMW tcItem{};
    tcItem.mask = TCIF_TEXT;

    tcItem.pszText = const_cast<wchar_t*>(L"Summary");
    TabCtrl_InsertItem(g_hTab, 0, &tcItem);

    tcItem.pszText = const_cast<wchar_t*>(L"Solution path");
    TabCtrl_InsertItem(g_hTab, 1, &tcItem);

    tcItem.pszText = const_cast<wchar_t*>(L"All possible steps");
    TabCtrl_InsertItem(g_hTab, 2, &tcItem);

    tcItem.pszText = const_cast<wchar_t*>(L"Active Cell");
    TabCtrl_InsertItem(g_hTab, 3, &tcItem);

    // 3. ListView for Steps
    g_hListView = CreateWindowW(WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER,
                                0, 0, 100, 100, hwnd, (HMENU)(INT_PTR)IDC_LIST_STEPS, NULL, NULL);
    SendMessage(g_hListView, WM_SETFONT, (WPARAM)hFont, TRUE);
    ListView_SetExtendedListViewStyle(g_hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_TRACKSELECT);

    // 4. Active Cell (CellZoomPanel) Controls
    HFONT hBannerFont = CreateFontW(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                    DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hKeypadFont = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                    DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hLabelFont = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                   DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    g_hZoomTitle = CreateWindowW(L"STATIC", L"Active Cell", WS_CHILD | SS_CENTER,
                                 0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)IDC_ZOOM_TITLE, NULL, NULL);
    SendMessage(g_hZoomTitle, WM_SETFONT, (WPARAM)hBannerFont, TRUE);

    g_hZoomSetLabel = CreateWindowW(L"STATIC", L"Set Value:", WS_CHILD | SS_CENTER,
                                    0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)IDC_ZOOM_SET_LABEL, NULL, NULL);
    SendMessage(g_hZoomSetLabel, WM_SETFONT, (WPARAM)hLabelFont, TRUE);

    for (int i = 0; i < 9; ++i) {
        g_hZoomSetBtns[i] = CreateWindowW(L"BUTTON", L"", WS_CHILD | BS_PUSHBUTTON,
                                          0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)(IDC_ZOOM_SET_BASE + 1 + i), NULL, NULL);
        SendMessage(g_hZoomSetBtns[i], WM_SETFONT, (WPARAM)hKeypadFont, TRUE);
    }

    g_hZoomCandLabel = CreateWindowW(L"STATIC", L"Toggle Candidates:", WS_CHILD | SS_CENTER,
                                     0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)IDC_ZOOM_CAND_LABEL, NULL, NULL);
    SendMessage(g_hZoomCandLabel, WM_SETFONT, (WPARAM)hLabelFont, TRUE);

    for (int i = 0; i < 9; ++i) {
        g_hZoomCandBtns[i] = CreateWindowW(L"BUTTON", L"", WS_CHILD | BS_PUSHBUTTON,
                                           0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)(IDC_ZOOM_CAND_BASE + 1 + i), NULL, NULL);
        SendMessage(g_hZoomCandBtns[i], WM_SETFONT, (WPARAM)hKeypadFont, TRUE);
    }

    g_hZoomColorLabel = CreateWindowW(L"STATIC", L"Choose Color for Cell:", WS_CHILD | SS_LEFT,
                                      0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)IDC_ZOOM_COLOR_LABEL, NULL, NULL);
    SendMessage(g_hZoomColorLabel, WM_SETFONT, (WPARAM)hLabelFont, TRUE);

    g_hZoomStatus = CreateWindowW(L"BUTTON", L"", WS_CHILD | BS_OWNERDRAW,
                                  0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)IDC_ZOOM_STATUS, NULL, NULL);

    for (int i = 0; i < 10; ++i) {
        g_hZoomColorBtns[i] = CreateWindowW(L"BUTTON", L"", WS_CHILD | BS_OWNERDRAW,
                                            0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)(IDC_ZOOM_COLOR_BASE + i), NULL, NULL);
    }

    g_hZoomClearBtn = CreateWindowW(L"BUTTON", L"R", WS_CHILD | BS_OWNERDRAW,
                                    0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)IDC_ZOOM_CLEAR_BTN, NULL, NULL);

    g_hZoomDetailsLabel = CreateWindowW(L"STATIC", L"Choose Color for Candidates:", WS_CHILD | SS_LEFT,
                                        0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)IDC_ZOOM_DETAILS, NULL, NULL);
    SendMessage(g_hZoomDetailsLabel, WM_SETFONT, (WPARAM)hLabelFont, TRUE);

    g_hZoomCandStatus = CreateWindowW(L"BUTTON", L"", WS_CHILD | BS_OWNERDRAW,
                                      0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)IDC_ZOOM_CAND_STATUS, NULL, NULL);

    for (int i = 0; i < 10; ++i) {
        g_hZoomCandColorBtns[i] = CreateWindowW(L"BUTTON", L"", WS_CHILD | BS_OWNERDRAW,
                                                0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)(IDC_ZOOM_CAND_COLOR_BASE + i), NULL, NULL);
    }

    g_hZoomCandClearBtn = CreateWindowW(L"BUTTON", L"R", WS_CHILD | BS_OWNERDRAW,
                                        0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)IDC_ZOOM_CAND_CLEAR_BTN, NULL, NULL);

    // 5. Full-width Bottom Hint Panel Controls
    g_hHintEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
                                0, 0, 100, 100, hwnd, (HMENU)(INT_PTR)IDC_HINT_EDIT, NULL, NULL);
    SendMessage(g_hHintEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

    g_hHintNextBtn = CreateWindowW(L"BUTTON", L"Next Hint", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                   0, 0, 92, 25, hwnd, (HMENU)(INT_PTR)IDC_BTN_HINT_BOX_NEXT, NULL, NULL);
    g_hHintExecBtn = CreateWindowW(L"BUTTON", L"Execute", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                   0, 0, 92, 25, hwnd, (HMENU)(INT_PTR)IDC_BTN_HINT_BOX_EXECUTE, NULL, NULL);
    g_hHintSolveUpBtn = CreateWindowW(L"BUTTON", L"Solve up to", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                      0, 0, 92, 25, hwnd, (HMENU)(INT_PTR)IDC_BTN_HINT_BOX_SOLVE_UP_TO, NULL, NULL);
    g_hHintCancelBtn = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                     0, 0, 92, 25, hwnd, (HMENU)(INT_PTR)IDC_BTN_HINT_BOX_CANCEL, NULL, NULL);

    SendMessage(g_hHintNextBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(g_hHintExecBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(g_hHintSolveUpBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(g_hHintCancelBtn, WM_SETFONT, (WPARAM)hFont, TRUE);

    // 6. Status Bar
    g_hStatusBar = CreateWindowW(STATUSCLASSNAMEW, L"", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                                 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    SendMessage(g_hStatusBar, WM_SETFONT, (WPARAM)hFont, TRUE);
}

inline HMENU CreateHoDoKuMenuBar() {
    HMENU hMenuBar = CreateMenu();

    // File Menu
    HMENU hFile = CreatePopupMenu();
    AppendMenuW(hFile, MF_STRING, IDM_FILE_NEW, L"&New Random Sudoku\tCtrl+N");
    AppendMenuW(hFile, MF_STRING, IDM_FILE_OPEN, L"&Open...\tCtrl+O");
    AppendMenuW(hFile, MF_STRING, IDM_FILE_SAVE, L"&Save\tCtrl+S");
    AppendMenuW(hFile, MF_STRING, IDM_FILE_SAVE_AS, L"Save &As...\tCtrl+Shift+S");
    AppendMenuW(hFile, MF_STRING, IDM_FILE_EXPORT_PNG, L"&Export Board Image (PNG)...");
    AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFile, MF_STRING, IDM_FILE_SET_GIVENS, L"&Set Givens...\tCtrl+G");
    AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFile, MF_STRING, IDM_FILE_COPY_GIVENS, L"Copy &Givens");
    AppendMenuW(hFile, MF_STRING, IDM_FILE_COPY_PM, L"&Copy Candidates (PM)\tCtrl+C");
    AppendMenuW(hFile, MF_STRING, IDM_FILE_PASTE, L"&Paste Sudoku\tCtrl+V");
    AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFile, MF_STRING, IDM_FILE_RESET, L"&Restart Game\tCtrl+R");
    AppendMenuW(hFile, MF_STRING, IDM_FILE_CLEAR, L"&Clear Grid");
    AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFile, MF_STRING, IDM_FILE_EXIT, L"E&xit\tAlt+F4");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hFile, L"&File");

    // Edit Menu
    HMENU hEdit = CreatePopupMenu();
    AppendMenuW(hEdit, MF_STRING, IDM_EDIT_UNDO, L"&Undo\tCtrl+Z");
    AppendMenuW(hEdit, MF_STRING, IDM_EDIT_REDO, L"&Redo\tCtrl+Y");
    AppendMenuW(hEdit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hEdit, MF_STRING, IDM_EDIT_ADD_SAVEPOINT, L"Add &Bookmark / Savepoint\tCtrl+M");
    AppendMenuW(hEdit, MF_STRING, IDM_EDIT_RESTORE_SAVEPOINT, L"Manage &Bookmarks...");
    AppendMenuW(hEdit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hEdit, MF_STRING, IDM_EDIT_CLEAR_COLORS, L"Clear All &Colors\tR");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hEdit, L"&Edit");

    // Mode Menu
    HMENU hMode = CreatePopupMenu();
    AppendMenuW(hMode, MF_STRING, IDM_MODE_PLAYING, L"&Playing Mode\tF2");
    AppendMenuW(hMode, MF_STRING, IDM_MODE_LEARNING, L"&Learning Mode\tF3");
    AppendMenuW(hMode, MF_STRING, IDM_MODE_PRACTICING, L"Prac&ticing Mode\tF4");
    AppendMenuW(hMode, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMode, MF_STRING, IDM_MODE_CONFIG_TRAINING, L"&Configure Training Techniques...");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hMode, L"&Mode");

    // Options Menu
    HMENU hOptions = CreatePopupMenu();
    AppendMenuW(hOptions, MF_STRING, IDM_OPTIONS_PREFERENCES, L"&Preferences...\tCtrl+P");
    AppendMenuW(hOptions, MF_STRING, 9202, L"&Color Configuration...");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hOptions, L"&Options");

    // Puzzle Menu
    HMENU hPuzzle = CreatePopupMenu();
    AppendMenuW(hPuzzle, MF_STRING, IDC_BTN_HINT_VAGUE, L"&Vague Hint\tAlt+F12");
    AppendMenuW(hPuzzle, MF_STRING, IDC_BTN_HINT_CONCRETE, L"&Concrete Hint\tCtrl+F12");
    AppendMenuW(hPuzzle, MF_STRING, IDC_BTN_HINT_NEXT, L"&Show Next Step\tF12");
    AppendMenuW(hPuzzle, MF_STRING, IDM_PUZZLE_EXECUTE_HINT, L"&Execute Step\tCtrl+E");
    AppendMenuW(hPuzzle, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hPuzzle, MF_STRING, IDM_SOLVER_FIND_BACKDOORS, L"Find &Backdoors...\tCtrl+B");
    AppendMenuW(hPuzzle, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hPuzzle, MF_STRING, IDM_PUZZLE_SET_SINGLES, L"Set All &Singles\tF11");
    AppendMenuW(hPuzzle, MF_STRING, IDM_PUZZLE_SOLVE_DLX, L"&Solve DLX\tCtrl+Shift+S");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hPuzzle, L"&Puzzle");

    // View Menu
    HMENU hView = CreatePopupMenu();
    AppendMenuW(hView, MF_STRING, IDM_VIEW_SUDOKU_ONLY, L"&Sudoku Only\tCtrl+Shift+0");
    AppendMenuW(hView, MF_STRING, IDM_VIEW_COLORKU, L"&ColorKu 3D Marble Mode\tCtrl+Shift+C");
    AppendMenuW(hView, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hView, MF_STRING, IDM_VIEW_SUMMARY, L"&Summary\tF6");
    AppendMenuW(hView, MF_STRING, IDM_VIEW_SOL_PATH, L"Solution &Path\tF7");
    AppendMenuW(hView, MF_STRING, IDM_VIEW_ALL_STEPS, L"&All Possible Steps\tF8");
    AppendMenuW(hView, MF_STRING, IDM_VIEW_ACTIVE_CELL, L"&Active Cell\tF5");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hView, L"&View");

    // Help Menu
    HMENU hHelp = CreatePopupMenu();
    AppendMenuW(hHelp, MF_STRING, IDM_HELP_MANUAL, L"User &Manual");
    AppendMenuW(hHelp, MF_STRING, IDM_HELP_TECHNIQUES, L"Solving &Techniques");
    AppendMenuW(hHelp, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hHelp, MF_STRING, IDM_HELP_ABOUT, L"&About HoDoKu Native");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hHelp, L"&Help");

    return hMenuBar;
}

} // namespace hodoku::ui

