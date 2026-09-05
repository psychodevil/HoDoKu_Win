#pragma once

#include "AppTypes.hpp"
#include "StudioModel.hpp"
#include "FileManager.hpp"
#include "Settings.hpp"
#include "GridRenderer.hpp"
#include <fstream>
#include <functional>
#include <commctrl.h>

namespace hodoku::ui {

inline HWND g_hGivensDlg = NULL;
inline HWND g_hGivensEdit = NULL;
inline HWND g_hGivensStatus = NULL;

inline std::function<void()> g_onPuzzleStateChanged = nullptr;

inline HFONT GetHoDoKuDialogFont() {
    static HFONT s_hFont = NULL;
    if (!s_hFont) {
        s_hFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        if (!s_hFont) s_hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    }
    return s_hFont;
}

inline void SetClipboardText(HWND hwndOwner, const std::string& text) {
    if (!OpenClipboard(hwndOwner)) return;
    EmptyClipboard();
    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (hGlob) {
        char* pMem = (char*)GlobalLock(hGlob);
        if (pMem) {
            memcpy(pMem, text.c_str(), text.size() + 1);
            GlobalUnlock(hGlob);
            SetClipboardData(CF_TEXT, hGlob);
        }
    }
    CloseClipboard();
}

inline std::string GetClipboardText(HWND hwndOwner) {
    std::string result;
    if (!OpenClipboard(hwndOwner)) return result;

    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData) {
        char* pMem = (char*)GlobalLock(hData);
        if (pMem) {
            result = pMem;
            GlobalUnlock(hData);
        }
    }
    CloseClipboard();
    return result;
}

inline void DoFileOpen(HWND hwnd, HoDoKuStudio& studio) {
    wchar_t szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"All Supported Sudoku Files (*.sdk;*.ss;*.hsol;*.txt)\0*.sdk;*.ss;*.hsol;*.txt\0Sudoku (*.sdk)\0*.sdk\0Simple Sudoku (*.ss)\0*.ss\0HoDoKu Solution (*.hsol)\0*.hsol\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameW(&ofn)) {
        std::string err;
        if (FileManager::load_file(szFile, studio, err)) {
            if (g_onPuzzleStateChanged) g_onPuzzleStateChanged();
            InvalidateRect(hwnd, NULL, FALSE);
        } else {
            std::wstring wErr(err.begin(), err.end());
            MessageBoxW(hwnd, wErr.c_str(), L"Error Opening File", MB_OK | MB_ICONERROR);
        }
    }
}

inline void DoFileSaveAs(HWND hwnd, HoDoKuStudio& studio) {
    wchar_t szFile[MAX_PATH] = L"sudoku.sdk";
    if (!studio.get_current_file_path().empty()) {
        wcsncpy_s(szFile, studio.get_current_file_path().c_str(), MAX_PATH);
    }
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"Sudoku (*.sdk)\0*.sdk\0Simple Sudoku (*.ss)\0*.ss\0HoDoKu Solution (*.hsol)\0*.hsol\0Text Sudoku (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    if (GetSaveFileNameW(&ofn)) {
        FileFormat fmt = FileFormat::Auto;
        if (ofn.nFilterIndex == 1) fmt = FileFormat::Sdk;
        else if (ofn.nFilterIndex == 2) fmt = FileFormat::SimpleSudoku;
        else if (ofn.nFilterIndex == 3) fmt = FileFormat::HoDoKuSolution;
        else if (ofn.nFilterIndex == 4) fmt = FileFormat::PlainText;

        std::string err;
        if (FileManager::save_file(szFile, studio, fmt, err)) {
            MessageBoxW(hwnd, L"Puzzle successfully saved!", L"Save - HoDoKu", MB_OK | MB_ICONINFORMATION);
        } else {
            std::wstring wErr(err.begin(), err.end());
            MessageBoxW(hwnd, wErr.c_str(), L"Error Saving File", MB_OK | MB_ICONERROR);
        }
    }
}

inline void DoFileSave(HWND hwnd, HoDoKuStudio& studio) {
    if (studio.get_current_file_path().empty()) {
        DoFileSaveAs(hwnd, studio);
    } else {
        std::string err;
        if (FileManager::save_file(studio.get_current_file_path(), studio, FileFormat::Auto, err)) {
            MessageBoxW(hwnd, L"Puzzle successfully saved!", L"Save - HoDoKu", MB_OK | MB_ICONINFORMATION);
        } else {
            std::wstring wErr(err.begin(), err.end());
            MessageBoxW(hwnd, wErr.c_str(), L"Error Saving File", MB_OK | MB_ICONERROR);
        }
    }
}

inline LRESULT CALLBACK SetGivensDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDOK || id == 1) { // Load
            int len = GetWindowTextLengthW(g_hGivensEdit);
            std::vector<wchar_t> buf(len + 1);
            GetWindowTextW(g_hGivensEdit, buf.data(), len + 1);
            std::string text;
            for (wchar_t wc : buf) {
                if (wc > 0 && wc < 128) text += static_cast<char>(wc);
            }
            HWND hOwner = GetWindow(hwnd, GW_OWNER);
            HoDoKuStudio* pStudio = reinterpret_cast<HoDoKuStudio*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (pStudio && !text.empty()) {
                pStudio->import_from_string(text);
                if (g_onPuzzleStateChanged) g_onPuzzleStateChanged();
                InvalidateRect(hOwner, NULL, FALSE);
            }
            DestroyWindow(hwnd);
            return 0;
        } else if (id == IDCANCEL || id == 2) { // Cancel
            DestroyWindow(hwnd);
            return 0;
        } else if (id == 3) { // Clear
            SetWindowTextW(g_hGivensEdit, L"");
            SetWindowTextW(g_hGivensStatus, L"Clues: 0 / 81");
            return 0;
        } else if (id == 4 && HIWORD(wParam) == EN_CHANGE) {
            int len = GetWindowTextLengthW(g_hGivensEdit);
            std::vector<wchar_t> buf(len + 1);
            GetWindowTextW(g_hGivensEdit, buf.data(), len + 1);
            int digits = 0;
            for (wchar_t wc : buf) {
                if (wc >= L'1' && wc <= L'9') digits++;
            }
            std::wstring st = L"Clues: " + std::to_wstring(digits) + L" / 81";
            SetWindowTextW(g_hGivensStatus, st.c_str());
        }
        break;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        EnableWindow(GetWindow(hwnd, GW_OWNER), TRUE);
        SetForegroundWindow(GetWindow(hwnd, GW_OWNER));
        g_hGivensDlg = NULL;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

inline void ShowSetGivensDialog(HWND hParent, HoDoKuStudio& studio) {
    if (g_hGivensDlg) {
        SetForegroundWindow(g_hGivensDlg);
        return;
    }

    HINSTANCE hInst = GetModuleHandle(NULL);
    const wchar_t DLG_CLASS[] = L"HoDoKuSetGivensClass";

    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = SetGivensDlgProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = DLG_CLASS;
        RegisterClassExW(&wc);
        registered = true;
    }

    RECT rcParent;
    GetWindowRect(hParent, &rcParent);
    int dw = 520, dh = 380;
    int dx = rcParent.left + (rcParent.right - rcParent.left - dw) / 2;
    int dy = rcParent.top + (rcParent.bottom - rcParent.top - dh) / 2;

    g_hGivensDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        DLG_CLASS,
        L"Set Givens - HoDoKu",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        dx, dy, dw, dh,
        hParent, NULL, hInst, NULL
    );

    if (!g_hGivensDlg) return;
    SetWindowLongPtrW(g_hGivensDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&studio));
    EnableWindow(hParent, FALSE);

    HFONT hFont = GetHoDoKuDialogFont();
    HFONT hMono = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             FIXED_PITCH | FF_MODERN, L"Consolas");

    HWND hLbl = CreateWindowW(L"STATIC", L"Enter or paste 81-character puzzle string, 9-line grid, or .ss format:",
                              WS_CHILD | WS_VISIBLE, 16, 12, 480, 20, g_hGivensDlg, NULL, hInst, NULL);
    SendMessage(hLbl, WM_SETFONT, (WPARAM)hFont, TRUE);

    g_hGivensEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
                                    16, 36, 470, 220, g_hGivensDlg, (HMENU)4, hInst, NULL);
    SendMessage(g_hGivensEdit, WM_SETFONT, (WPARAM)hMono, TRUE);

    std::string curGivens = studio.export_givens_string();
    std::wstring wGivens(curGivens.begin(), curGivens.end());
    SetWindowTextW(g_hGivensEdit, wGivens.c_str());

    g_hGivensStatus = CreateWindowW(L"STATIC", L"Clues: 0 / 81",
                                   WS_CHILD | WS_VISIBLE, 16, 266, 470, 20, g_hGivensDlg, NULL, hInst, NULL);
    SendMessage(g_hGivensStatus, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hBtnLoad = CreateWindowW(L"BUTTON", L"Load Puzzle", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                  200, 298, 100, 28, g_hGivensDlg, (HMENU)IDOK, hInst, NULL);
    HWND hBtnClear = CreateWindowW(L"BUTTON", L"Clear", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                   310, 298, 80, 28, g_hGivensDlg, (HMENU)3, hInst, NULL);
    HWND hBtnCancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                    400, 298, 86, 28, g_hGivensDlg, (HMENU)IDCANCEL, hInst, NULL);

    SendMessage(hBtnLoad, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnClear, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)hFont, TRUE);

    SendMessage(g_hGivensDlg, WM_COMMAND, MAKEWPARAM(4, EN_CHANGE), (LPARAM)g_hGivensEdit);
}

// ============================================================================
// 1. About Dialog
// ============================================================================
inline LRESULT CALLBACK AboutDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDOK || id == IDCANCEL) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY: {
        HWND hOwner = GetWindow(hwnd, GW_OWNER);
        if (hOwner) {
            EnableWindow(hOwner, TRUE);
            SetForegroundWindow(hOwner);
        }
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

inline void ShowAboutDialog(HWND hParent) {
    HINSTANCE hInst = GetModuleHandle(NULL);
    const wchar_t DLG_CLASS[] = L"HoDoKuAboutClass";

    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = AboutDlgProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = DLG_CLASS;
        RegisterClassExW(&wc);
        registered = true;
    }

    RECT rcParent;
    GetWindowRect(hParent, &rcParent);
    int dw = 500, dh = 420;
    int dx = rcParent.left + (rcParent.right - rcParent.left - dw) / 2;
    int dy = rcParent.top + (rcParent.bottom - rcParent.top - dh) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        DLG_CLASS,
        L"About HoDoKu Native Studio",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        dx, dy, dw, dh,
        hParent, NULL, hInst, NULL
    );
    if (!hDlg) return;
    EnableWindow(hParent, FALSE);

    HFONT hFont = GetHoDoKuDialogFont();
    HFONT hTitleFont = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                  DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT hSubFont = CreateFontW(15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    HWND hTitle = CreateWindowW(L"STATIC", L"HoDoKu Sudoku Studio", WS_CHILD | WS_VISIBLE | SS_CENTER,
                                20, 18, 444, 28, hDlg, NULL, hInst, NULL);
    SendMessage(hTitle, WM_SETFONT, (WPARAM)hTitleFont, TRUE);

    HWND hVer = CreateWindowW(L"STATIC", L"Version 2.2.0 (Native Windows C++20 Edition)", WS_CHILD | WS_VISIBLE | SS_CENTER,
                              20, 48, 444, 20, hDlg, NULL, hInst, NULL);
    SendMessage(hVer, WM_SETFONT, (WPARAM)hSubFont, TRUE);

    CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                  20, 76, 444, 2, hDlg, NULL, hInst, NULL);

    std::wstring credits =
        L"Original Algorithm & UI Design:\n"
        L"    Bernhard Hobiger (SourceForge: hodoku)\n\n"
        L"High-Performance Native Windows Port:\n"
        L"    psychodevil (GitHub: https://github.com/psychodevil/HoDoKu_Win)\n\n"
        L"License: GNU General Public License v3.0 (GPLv3)\n\n"
        L"Key Capabilities:\n"
        L" • Complete 45+ Solving Technique Hierarchy & FAS (Find All Steps)\n"
        L" • SIMD Bitboard Acceleration (AVX2, FMA, POPCNT)\n"
        L" • Knuth's Dancing Links (DLX) Exact Cover Engine\n"
        L" • ColorKu 3D Marble Sphere Rendering (Ctrl+Shift+C)\n"
        L" • Step-by-Step Interactive Tutor & Technique Training Mode";

    HWND hText = CreateWindowW(L"STATIC", credits.c_str(), WS_CHILD | WS_VISIBLE,
                               24, 88, 440, 225, hDlg, NULL, hInst, NULL);
    SendMessage(hText, WM_SETFONT, (WPARAM)hFont, TRUE);

    CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                  20, 326, 444, 2, hDlg, NULL, hInst, NULL);

    HWND hBtnOk = CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                200, 340, 90, 28, hDlg, (HMENU)IDOK, hInst, NULL);
    SendMessage(hBtnOk, WM_SETFONT, (WPARAM)hFont, TRUE);
}

// ============================================================================
// 2. Bookmarks / Savepoints Dialog (RestoreSavePointDialog replica)
// ============================================================================
struct SavepointsDlgContext {
    HoDoKuStudio* studio{nullptr};
    HWND hList{NULL};
    HWND hOwner{NULL};
};

inline void PopulateSavepointsList(HWND hList, const HoDoKuStudio& studio) {
    ListView_DeleteAllItems(hList);
    const auto& sps = studio.get_savepoints();
    for (size_t i = 0; i < sps.size(); ++i) {
        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        item.iItem = static_cast<int>(i);

        std::wstring idxStr = std::to_wstring(i + 1);
        item.pszText = const_cast<LPWSTR>(idxStr.c_str());
        ListView_InsertItem(hList, &item);

        std::wstring nameStr(sps[i].name.begin(), sps[i].name.end());
        ListView_SetItemText(hList, static_cast<int>(i), 1, const_cast<LPWSTR>(nameStr.c_str()));

        std::wstring unfilledStr = std::to_wstring(sps[i].board.unfilled_count()) + L" cells";
        ListView_SetItemText(hList, static_cast<int>(i), 2, const_cast<LPWSTR>(unfilledStr.c_str()));

        std::wstring givensStr = std::to_wstring(sps[i].board.get_givens().count()) + L" clues";
        ListView_SetItemText(hList, static_cast<int>(i), 3, const_cast<LPWSTR>(givensStr.c_str()));
    }
}

inline LRESULT CALLBACK SavepointsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    SavepointsDlgContext* ctx = reinterpret_cast<SavepointsDlgContext*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (!ctx || !ctx->studio) break;

        if (id == 101) { // Restore
            int sel = ListView_GetNextItem(ctx->hList, -1, LVNI_SELECTED);
            if (sel >= 0) {
                ctx->studio->restore_savepoint(static_cast<size_t>(sel));
                if (g_onPuzzleStateChanged) g_onPuzzleStateChanged();
                InvalidateRect(ctx->hOwner, NULL, FALSE);
                DestroyWindow(hwnd);
            } else {
                MessageBoxW(hwnd, L"Please select a bookmark to restore.", L"Restore Bookmark", MB_OK | MB_ICONINFORMATION);
            }
            return 0;
        } else if (id == 102) { // Add Bookmark
            ctx->studio->add_savepoint();
            PopulateSavepointsList(ctx->hList, *ctx->studio);
            int count = ListView_GetItemCount(ctx->hList);
            if (count > 0) {
                ListView_SetItemState(ctx->hList, count - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                ListView_EnsureVisible(ctx->hList, count - 1, FALSE);
            }
            return 0;
        } else if (id == 103) { // Delete
            int sel = ListView_GetNextItem(ctx->hList, -1, LVNI_SELECTED);
            if (sel >= 0) {
                ctx->studio->delete_savepoint(static_cast<size_t>(sel));
                PopulateSavepointsList(ctx->hList, *ctx->studio);
            }
            return 0;
        } else if (id == 104) { // Clear All
            if (MessageBoxW(hwnd, L"Are you sure you want to clear all bookmarks?", L"Clear All Bookmarks", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                ctx->studio->clear_savepoints();
                PopulateSavepointsList(ctx->hList, *ctx->studio);
            }
            return 0;
        } else if (id == IDCANCEL || id == 2) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }
    case WM_NOTIFY: {
        LPNMHDR pnm = (LPNMHDR)lParam;
        if (ctx && pnm->hwndFrom == ctx->hList && (pnm->code == NM_DBLCLK)) {
            int sel = ListView_GetNextItem(ctx->hList, -1, LVNI_SELECTED);
            if (sel >= 0 && ctx->studio) {
                ctx->studio->restore_savepoint(static_cast<size_t>(sel));
                if (g_onPuzzleStateChanged) g_onPuzzleStateChanged();
                InvalidateRect(ctx->hOwner, NULL, FALSE);
                DestroyWindow(hwnd);
            }
            return 0;
        }
        break;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY: {
        HWND hOwner = GetWindow(hwnd, GW_OWNER);
        if (hOwner) {
            EnableWindow(hOwner, TRUE);
            SetForegroundWindow(hOwner);
        }
        delete ctx;
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

inline void ShowSavepointsDialog(HWND hParent, HoDoKuStudio& studio) {
    HINSTANCE hInst = GetModuleHandle(NULL);
    const wchar_t DLG_CLASS[] = L"HoDoKuBookmarksClass";

    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = SavepointsDlgProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = DLG_CLASS;
        RegisterClassExW(&wc);
        registered = true;
    }

    RECT rcParent;
    GetWindowRect(hParent, &rcParent);
    int dw = 580, dh = 420;
    int dx = rcParent.left + (rcParent.right - rcParent.left - dw) / 2;
    int dy = rcParent.top + (rcParent.bottom - rcParent.top - dh) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        DLG_CLASS,
        L"Bookmarks & Savepoints - HoDoKu",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        dx, dy, dw, dh,
        hParent, NULL, hInst, NULL
    );
    if (!hDlg) return;
    EnableWindow(hParent, FALSE);

    SavepointsDlgContext* ctx = new SavepointsDlgContext();
    ctx->studio = &studio;
    ctx->hOwner = hParent;
    SetWindowLongPtrW(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));

    HFONT hFont = GetHoDoKuDialogFont();

    HWND hLbl = CreateWindowW(L"STATIC", L"Saved Bookmarks / Savepoints for Current Puzzle:",
                              WS_CHILD | WS_VISIBLE, 16, 12, 400, 20, hDlg, NULL, hInst, NULL);
    SendMessage(hLbl, WM_SETFONT, (WPARAM)hFont, TRUE);

    ctx->hList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER,
                                 16, 36, 420, 320, hDlg, (HMENU)500, hInst, NULL);
    SendMessage(ctx->hList, WM_SETFONT, (WPARAM)hFont, TRUE);
    ListView_SetExtendedListViewStyle(ctx->hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    col.cx = 40;
    col.pszText = const_cast<LPWSTR>(L"#");
    ListView_InsertColumn(ctx->hList, 0, &col);

    col.cx = 160;
    col.pszText = const_cast<LPWSTR>(L"Bookmark Name");
    ListView_InsertColumn(ctx->hList, 1, &col);

    col.cx = 105;
    col.pszText = const_cast<LPWSTR>(L"Unfilled Cells");
    ListView_InsertColumn(ctx->hList, 2, &col);

    col.cx = 95;
    col.pszText = const_cast<LPWSTR>(L"Givens / Clues");
    ListView_InsertColumn(ctx->hList, 3, &col);

    PopulateSavepointsList(ctx->hList, studio);
    if (ListView_GetItemCount(ctx->hList) > 0) {
        ListView_SetItemState(ctx->hList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }

    HWND hBtnRestore = CreateWindowW(L"BUTTON", L"Restore", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                    448, 36, 106, 28, hDlg, (HMENU)101, hInst, NULL);
    HWND hBtnAdd = CreateWindowW(L"BUTTON", L"Add Bookmark", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                 448, 72, 106, 28, hDlg, (HMENU)102, hInst, NULL);
    HWND hBtnDelete = CreateWindowW(L"BUTTON", L"Delete", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                    448, 108, 106, 28, hDlg, (HMENU)103, hInst, NULL);
    HWND hBtnClear = CreateWindowW(L"BUTTON", L"Clear All", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                   448, 144, 106, 28, hDlg, (HMENU)104, hInst, NULL);
    HWND hBtnClose = CreateWindowW(L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                   448, 328, 106, 28, hDlg, (HMENU)IDCANCEL, hInst, NULL);

    SendMessage(hBtnRestore, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnAdd, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnDelete, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnClear, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnClose, WM_SETFONT, (WPARAM)hFont, TRUE);
}

// ============================================================================
// 3. Backdoors Dialog (BackdoorSearchDialog replica)
// ============================================================================
struct BackdoorsDlgContext {
    HoDoKuStudio* studio{nullptr};
    HWND hList{NULL};
    HWND hStatus{NULL};
    HWND hOwner{NULL};
    std::vector<HoDoKuStudio::BackdoorCandidate> backdoors;
};

inline void PopulateBackdoorsList(BackdoorsDlgContext* ctx) {
    if (!ctx || !ctx->studio) return;
    ListView_DeleteAllItems(ctx->hList);
    ctx->backdoors = ctx->studio->find_backdoors();

    for (size_t i = 0; i < ctx->backdoors.size(); ++i) {
        int cell = ctx->backdoors[i].cell;
        int d = ctx->backdoors[i].digit;
        int r = cell_row(cell) + 1;
        int c = cell_col(cell) + 1;

        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        item.iItem = static_cast<int>(i);

        std::wstring cellStr = L"r" + std::to_wstring(r) + L"c" + std::to_wstring(c);
        item.pszText = const_cast<LPWSTR>(cellStr.c_str());
        ListView_InsertItem(ctx->hList, &item);

        std::wstring dStr = std::to_wstring(d);
        ListView_SetItemText(ctx->hList, static_cast<int>(i), 1, const_cast<LPWSTR>(dStr.c_str()));

        std::wstring coordStr = L"Row " + std::to_wstring(r) + L", Col " + std::to_wstring(c);
        ListView_SetItemText(ctx->hList, static_cast<int>(i), 2, const_cast<LPWSTR>(coordStr.c_str()));

        ListView_SetItemText(ctx->hList, static_cast<int>(i), 3, const_cast<LPWSTR>(L"Reduces directly to Singles"));
    }

    std::wstring statusText = L"Found " + std::to_wstring(ctx->backdoors.size()) + L" Level-1 Backdoor(s).";
    if (ctx->backdoors.empty()) {
        statusText = L"No single-step Level-1 backdoors found for the current state.";
    }
    SetWindowTextW(ctx->hStatus, statusText.c_str());
}

inline LRESULT CALLBACK BackdoorsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    BackdoorsDlgContext* ctx = reinterpret_cast<BackdoorsDlgContext*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (!ctx || !ctx->studio) break;

        if (id == 101) { // Apply to Grid
            int sel = ListView_GetNextItem(ctx->hList, -1, LVNI_SELECTED);
            if (sel >= 0 && static_cast<size_t>(sel) < ctx->backdoors.size()) {
                int cell = ctx->backdoors[sel].cell;
                int d = ctx->backdoors[sel].digit;
                ctx->studio->set_cell_digit(cell, d);
                ctx->studio->set_selected_cell(cell);
                if (g_onPuzzleStateChanged) g_onPuzzleStateChanged();
                InvalidateRect(ctx->hOwner, NULL, FALSE);
                DestroyWindow(hwnd);
            } else {
                MessageBoxW(hwnd, L"Please select a backdoor from the list to apply.", L"Apply Backdoor", MB_OK | MB_ICONINFORMATION);
            }
            return 0;
        } else if (id == 102) { // Highlight / Select Cell
            int sel = ListView_GetNextItem(ctx->hList, -1, LVNI_SELECTED);
            if (sel >= 0 && static_cast<size_t>(sel) < ctx->backdoors.size()) {
                int cell = ctx->backdoors[sel].cell;
                ctx->studio->set_selected_cell(cell);
                if (g_onPuzzleStateChanged) g_onPuzzleStateChanged();
                InvalidateRect(ctx->hOwner, NULL, FALSE);
            }
            return 0;
        } else if (id == 103) { // Refresh
            PopulateBackdoorsList(ctx);
            return 0;
        } else if (id == IDCANCEL || id == 2) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }
    case WM_NOTIFY: {
        LPNMHDR pnm = (LPNMHDR)lParam;
        if (ctx && pnm->hwndFrom == ctx->hList && (pnm->code == NM_DBLCLK)) {
            int sel = ListView_GetNextItem(ctx->hList, -1, LVNI_SELECTED);
            if (sel >= 0 && static_cast<size_t>(sel) < ctx->backdoors.size() && ctx->studio) {
                int cell = ctx->backdoors[sel].cell;
                int d = ctx->backdoors[sel].digit;
                ctx->studio->set_cell_digit(cell, d);
                ctx->studio->set_selected_cell(cell);
                if (g_onPuzzleStateChanged) g_onPuzzleStateChanged();
                InvalidateRect(ctx->hOwner, NULL, FALSE);
                DestroyWindow(hwnd);
            }
            return 0;
        }
        break;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY: {
        HWND hOwner = GetWindow(hwnd, GW_OWNER);
        if (hOwner) {
            EnableWindow(hOwner, TRUE);
            SetForegroundWindow(hOwner);
        }
        delete ctx;
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

inline void ShowBackdoorsDialog(HWND hParent, HoDoKuStudio& studio) {
    HINSTANCE hInst = GetModuleHandle(NULL);
    const wchar_t DLG_CLASS[] = L"HoDoKuBackdoorsClass";

    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = BackdoorsDlgProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = DLG_CLASS;
        RegisterClassExW(&wc);
        registered = true;
    }

    RECT rcParent;
    GetWindowRect(hParent, &rcParent);
    int dw = 620, dh = 460;
    int dx = rcParent.left + (rcParent.right - rcParent.left - dw) / 2;
    int dy = rcParent.top + (rcParent.bottom - rcParent.top - dh) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        DLG_CLASS,
        L"Backdoor Search - HoDoKu",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        dx, dy, dw, dh,
        hParent, NULL, hInst, NULL
    );
    if (!hDlg) return;
    EnableWindow(hParent, FALSE);

    BackdoorsDlgContext* ctx = new BackdoorsDlgContext();
    ctx->studio = &studio;
    ctx->hOwner = hParent;
    SetWindowLongPtrW(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));

    HFONT hFont = GetHoDoKuDialogFont();

    HWND hLbl = CreateWindowW(L"STATIC",
                              L"Level-1 Backdoors: setting any of these cells immediately collapses the puzzle to Singles:",
                              WS_CHILD | WS_VISIBLE, 16, 12, 570, 20, hDlg, NULL, hInst, NULL);
    SendMessage(hLbl, WM_SETFONT, (WPARAM)hFont, TRUE);

    ctx->hList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER,
                                 16, 36, 460, 340, hDlg, (HMENU)500, hInst, NULL);
    SendMessage(ctx->hList, WM_SETFONT, (WPARAM)hFont, TRUE);
    ListView_SetExtendedListViewStyle(ctx->hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    col.cx = 75;
    col.pszText = const_cast<LPWSTR>(L"Cell");
    ListView_InsertColumn(ctx->hList, 0, &col);

    col.cx = 65;
    col.pszText = const_cast<LPWSTR>(L"Digit");
    ListView_InsertColumn(ctx->hList, 1, &col);

    col.cx = 110;
    col.pszText = const_cast<LPWSTR>(L"Coordinates");
    ListView_InsertColumn(ctx->hList, 2, &col);

    col.cx = 190;
    col.pszText = const_cast<LPWSTR>(L"Resolution Effect");
    ListView_InsertColumn(ctx->hList, 3, &col);

    ctx->hStatus = CreateWindowW(L"STATIC", L"Searching for backdoors...",
                                 WS_CHILD | WS_VISIBLE, 16, 386, 460, 20, hDlg, NULL, hInst, NULL);
    SendMessage(ctx->hStatus, WM_SETFONT, (WPARAM)hFont, TRUE);

    PopulateBackdoorsList(ctx);
    if (ListView_GetItemCount(ctx->hList) > 0) {
        ListView_SetItemState(ctx->hList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }

    HWND hBtnApply = CreateWindowW(L"BUTTON", L"Apply to Grid", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                   486, 36, 110, 28, hDlg, (HMENU)101, hInst, NULL);
    HWND hBtnSelect = CreateWindowW(L"BUTTON", L"Select Cell", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                    486, 72, 110, 28, hDlg, (HMENU)102, hInst, NULL);
    HWND hBtnRefresh = CreateWindowW(L"BUTTON", L"Refresh", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                     486, 108, 110, 28, hDlg, (HMENU)103, hInst, NULL);
    HWND hBtnClose = CreateWindowW(L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                   486, 348, 110, 28, hDlg, (HMENU)IDCANCEL, hInst, NULL);

    SendMessage(hBtnApply, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnSelect, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnRefresh, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnClose, WM_SETFONT, (WPARAM)hFont, TRUE);
}

// ============================================================================
// 4. Configure Training & Practice Dialog (ConfigTrainingDialog replica)
// ============================================================================
struct TrainingConfigDlgContext {
    HoDoKuStudio* studio{nullptr};
    HWND hOwner{NULL};
    HWND checkBoxes[12]{NULL};
    TechniqueType techTypes[12]{
        TechniqueType::NakedSingle,
        TechniqueType::LockedCandidatesPointing,
        TechniqueType::NakedPair,
        TechniqueType::NakedTriple,
        TechniqueType::XWing,
        TechniqueType::Skyscraper,
        TechniqueType::TurbotFish,
        TechniqueType::XYWing,
        TechniqueType::SimpleColors,
        TechniqueType::UniqueRectangle,
        TechniqueType::RemotePair,
        TechniqueType::AlsXz
    };
};

inline LRESULT CALLBACK TrainingConfigDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    TrainingConfigDlgContext* ctx = reinterpret_cast<TrainingConfigDlgContext*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (!ctx || !ctx->studio) break;

        if (id == 101 || id == 102) { // 101: Start Training, 102: Save & Close
            std::vector<TechniqueType> selected;
            for (int i = 0; i < 12; ++i) {
                if (SendMessageW(ctx->checkBoxes[i], BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    selected.push_back(ctx->techTypes[i]);
                    if (ctx->techTypes[i] == TechniqueType::NakedSingle) {
                        selected.push_back(TechniqueType::HiddenSingle);
                        selected.push_back(TechniqueType::FullHouse);
                    } else if (ctx->techTypes[i] == TechniqueType::LockedCandidatesPointing) {
                        selected.push_back(TechniqueType::LockedCandidatesClaiming);
                    } else if (ctx->techTypes[i] == TechniqueType::NakedPair) {
                        selected.push_back(TechniqueType::HiddenPair);
                    } else if (ctx->techTypes[i] == TechniqueType::NakedTriple) {
                        selected.push_back(TechniqueType::HiddenTriple);
                        selected.push_back(TechniqueType::NakedQuadruple);
                        selected.push_back(TechniqueType::HiddenQuadruple);
                    } else if (ctx->techTypes[i] == TechniqueType::XWing) {
                        selected.push_back(TechniqueType::Swordfish);
                        selected.push_back(TechniqueType::Jellyfish);
                    } else if (ctx->techTypes[i] == TechniqueType::Skyscraper) {
                        selected.push_back(TechniqueType::TwoStringKite);
                    } else if (ctx->techTypes[i] == TechniqueType::TurbotFish) {
                        selected.push_back(TechniqueType::EmptyRectangle);
                        selected.push_back(TechniqueType::DualEmptyRectangle);
                    } else if (ctx->techTypes[i] == TechniqueType::XYWing) {
                        selected.push_back(TechniqueType::XYZWing);
                        selected.push_back(TechniqueType::WWing);
                    } else if (ctx->techTypes[i] == TechniqueType::SimpleColors) {
                        selected.push_back(TechniqueType::MultiColors1);
                        selected.push_back(TechniqueType::MultiColors2);
                    } else if (ctx->techTypes[i] == TechniqueType::UniqueRectangle) {
                        selected.push_back(TechniqueType::AvoidableRectangle);
                        selected.push_back(TechniqueType::BUG);
                    } else if (ctx->techTypes[i] == TechniqueType::RemotePair) {
                        selected.push_back(TechniqueType::GroupedAIC);
                    } else if (ctx->techTypes[i] == TechniqueType::AlsXz) {
                        selected.push_back(TechniqueType::SueDeCoq);
                        selected.push_back(TechniqueType::DeathBlossom);
                    }
                }
            }

            ctx->studio->set_training_techniques(selected);
            if (id == 101) {
                ctx->studio->set_game_mode(GameMode::Practicing);
                ctx->studio->new_puzzle(1);
            }
            if (g_onPuzzleStateChanged) g_onPuzzleStateChanged();
            InvalidateRect(ctx->hOwner, NULL, FALSE);
            DestroyWindow(hwnd);
            return 0;
        } else if (id == 201) { // Select All
            for (int i = 0; i < 12; ++i) SendMessageW(ctx->checkBoxes[i], BM_SETCHECK, BST_CHECKED, 0);
            return 0;
        } else if (id == 202) { // Clear All
            for (int i = 0; i < 12; ++i) SendMessageW(ctx->checkBoxes[i], BM_SETCHECK, BST_UNCHECKED, 0);
            return 0;
        } else if (id == 203) { // Preset: Intermediate
            for (int i = 0; i < 12; ++i) {
                bool on = (i == 1 || i == 2 || i == 5 || i == 7);
                SendMessageW(ctx->checkBoxes[i], BM_SETCHECK, on ? BST_CHECKED : BST_UNCHECKED, 0);
            }
            return 0;
        } else if (id == 204) { // Preset: Advanced
            for (int i = 0; i < 12; ++i) {
                bool on = (i == 4 || i == 7 || i == 8 || i == 9 || i == 10 || i == 11);
                SendMessageW(ctx->checkBoxes[i], BM_SETCHECK, on ? BST_CHECKED : BST_UNCHECKED, 0);
            }
            return 0;
        } else if (id == IDCANCEL || id == 2) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY: {
        HWND hOwner = GetWindow(hwnd, GW_OWNER);
        if (hOwner) {
            EnableWindow(hOwner, TRUE);
            SetForegroundWindow(hOwner);
        }
        delete ctx;
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

inline void ShowTrainingConfigDialog(HWND hParent, HoDoKuStudio& studio) {
    HINSTANCE hInst = GetModuleHandle(NULL);
    const wchar_t DLG_CLASS[] = L"HoDoKuTrainingConfigClass";

    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = TrainingConfigDlgProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = DLG_CLASS;
        RegisterClassExW(&wc);
        registered = true;
    }

    RECT rcParent;
    GetWindowRect(hParent, &rcParent);
    int dw = 560, dh = 520;
    int dx = rcParent.left + (rcParent.right - rcParent.left - dw) / 2;
    int dy = rcParent.top + (rcParent.bottom - rcParent.top - dh) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        DLG_CLASS,
        L"Configure Practice & Training Mode - HoDoKu",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        dx, dy, dw, dh,
        hParent, NULL, hInst, NULL
    );
    if (!hDlg) return;
    EnableWindow(hParent, FALSE);

    TrainingConfigDlgContext* ctx = new TrainingConfigDlgContext();
    ctx->studio = &studio;
    ctx->hOwner = hParent;
    SetWindowLongPtrW(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));

    HFONT hFont = GetHoDoKuDialogFont();

    HWND hLbl = CreateWindowW(L"STATIC",
                              L"Select the technique categories to target during Practice & Training:",
                              WS_CHILD | WS_VISIBLE, 18, 12, 510, 20, hDlg, NULL, hInst, NULL);
    SendMessage(hLbl, WM_SETFONT, (WPARAM)hFont, TRUE);

    const wchar_t* labels[12] = {
        L"1. Full House, Naked & Hidden Singles",
        L"2. Locked Candidates (Pointing & Claiming)",
        L"3. Naked & Hidden Pairs",
        L"4. Naked & Hidden Triples and Quads",
        L"5. Basic Fish (X-Wing, Swordfish, Jellyfish)",
        L"6. Single Digit Patterns (Skyscrapers, 2-String Kites)",
        L"7. Turbot Fish & Empty Rectangles",
        L"8. Wings (XY-Wing, XYZ-Wing, W-Wing)",
        L"9. Simple Colors & Multi-Colors (Wrap & Trap)",
        L"10. Uniqueness (Unique Rectangles 1-6, BUG+1)",
        L"11. Chains (Remote Pairs, Forcing Chains, AIC)",
        L"12. Almost Locked Sets (ALS-XZ, Sue de Coq)"
    };

    const auto& currentTechs = studio.get_training_techniques();
    auto hasTech = [&](TechniqueType t) {
        if (currentTechs.empty()) return true;
        for (auto cur : currentTechs) {
            if (cur == t) return true;
        }
        return false;
    };

    int startY = 40;
    int rowH = 26;
    for (int i = 0; i < 12; ++i) {
        ctx->checkBoxes[i] = CreateWindowW(L"BUTTON", labels[i],
                                           WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                           22, startY + i * rowH, 500, 22, hDlg, (HMENU)(INT_PTR)(300 + i), hInst, NULL);
        SendMessage(ctx->checkBoxes[i], WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(ctx->checkBoxes[i], BM_SETCHECK, hasTech(ctx->techTypes[i]) ? BST_CHECKED : BST_UNCHECKED, 0);
    }

    int btnY = startY + 12 * rowH + 12;
    HWND hBtnAll = CreateWindowW(L"BUTTON", L"Select All", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                 22, btnY, 96, 26, hDlg, (HMENU)201, hInst, NULL);
    HWND hBtnClear = CreateWindowW(L"BUTTON", L"Clear All", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                   124, btnY, 96, 26, hDlg, (HMENU)202, hInst, NULL);
    HWND hBtnInter = CreateWindowW(L"BUTTON", L"Intermediate", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                   226, btnY, 110, 26, hDlg, (HMENU)203, hInst, NULL);
    HWND hBtnAdv = CreateWindowW(L"BUTTON", L"Advanced", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                 342, btnY, 100, 26, hDlg, (HMENU)204, hInst, NULL);

    SendMessage(hBtnAll, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnClear, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnInter, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnAdv, WM_SETFONT, (WPARAM)hFont, TRUE);

    CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                  18, btnY + 36, 510, 2, hDlg, NULL, hInst, NULL);

    HWND hBtnStart = CreateWindowW(L"BUTTON", L"Start Practice", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                   170, btnY + 46, 120, 28, hDlg, (HMENU)101, hInst, NULL);
    HWND hBtnSave = CreateWindowW(L"BUTTON", L"Save & Close", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                  298, btnY + 46, 110, 28, hDlg, (HMENU)102, hInst, NULL);
    HWND hBtnCancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                    416, btnY + 46, 100, 28, hDlg, (HMENU)IDCANCEL, hInst, NULL);

    SendMessage(hBtnStart, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnSave, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)hFont, TRUE);
}

inline void ShowPracticingDialog(HWND hParent, HoDoKuStudio& studio) {
    int choice = MessageBoxW(hParent,
        L"Practicing Mode Active!\n\n"
        L"In this mode, newly generated puzzles will specifically target\n"
        L"intermediate and advanced solving techniques.\n\n"
        L"Would you like to configure target training techniques now?",
        L"Practicing Mode - HoDoKu", MB_YESNO | MB_ICONQUESTION);
    if (choice == IDYES) {
        ShowTrainingConfigDialog(hParent, studio);
    }
}

// ============================================================================
// Print Puzzle
// ============================================================================
inline void DoPrintPuzzle(HWND hwnd, const HoDoKuStudio& studio) {
    PRINTDLGW pd = {};
    pd.lStructSize = sizeof(pd);
    pd.hwndOwner = hwnd;
    pd.Flags = PD_RETURNDC | PD_USEDEVMODECOPIESANDCOLLATE | PD_NOSELECTION;

    if (PrintDlgW(&pd)) {
        DOCINFOW di = {};
        di.cbSize = sizeof(DOCINFOW);
        di.lpszDocName = L"HoDoKu Sudoku Puzzle";

        if (StartDocW(pd.hDC, &di) > 0) {
            if (StartPage(pd.hDC) > 0) {
                int pWidth = GetDeviceCaps(pd.hDC, HORZRES);
                int pHeight = GetDeviceCaps(pd.hDC, VERTRES);

                Gdiplus::Graphics g(pd.hDC);
                g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

                // Print header
                Gdiplus::FontFamily fontFamily(L"Arial");
                Gdiplus::Font titleFont(&fontFamily, 22, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
                Gdiplus::Font subFont(&fontFamily, 12, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
                Gdiplus::SolidBrush blackBrush(Gdiplus::Color(255, 0, 0, 0));

                std::wstring lvlName = L"Easy";
                switch (studio.get_hardest_level()) {
                    case DifficultyLevel::Easy: lvlName = L"Easy"; break;
                    case DifficultyLevel::Medium: lvlName = L"Medium"; break;
                    case DifficultyLevel::Hard: lvlName = L"Hard"; break;
                    case DifficultyLevel::Unfair: lvlName = L"Unfair"; break;
                    case DifficultyLevel::Extreme: lvlName = L"Extreme"; break;
                }

                std::wstring title = L"HoDoKu Sudoku - Level: " + lvlName + L" (Score: " + std::to_wstring(studio.get_total_score()) + L")";
                std::wstring sub = L"Clues: " + std::to_wstring(studio.get_givens_count()) + L"  |  Printed: " + std::to_wstring(81 - studio.get_unfilled_count()) + L" set cells";

                int topMargin = static_cast<int>(pHeight * 0.06f);
                int leftMargin = static_cast<int>(pWidth * 0.10f);
                int gridPrintSize = static_cast<int>(pWidth * 0.80f);

                Gdiplus::PointF titlePt(static_cast<float>(leftMargin), static_cast<float>(topMargin));
                g.DrawString(title.c_str(), -1, &titleFont, titlePt, &blackBrush);

                Gdiplus::PointF subPt(static_cast<float>(leftMargin), static_cast<float>(topMargin + pHeight * 0.04f));
                g.DrawString(sub.c_str(), -1, &subFont, subPt, &blackBrush);

                // Render grid
                int gridTop = static_cast<int>(topMargin + pHeight * 0.08f);
                GridRenderer printRenderer;
                printRenderer.render_grid_canvas(g, studio, leftMargin, gridTop, gridPrintSize, gridPrintSize);

                // Print footer
                std::wstring footer = L"Printed with HoDoKu Native (C++20 High-Performance Edition)";
                Gdiplus::PointF footerPt(static_cast<float>(leftMargin), static_cast<float>(gridTop + gridPrintSize + pHeight * 0.03f));
                g.DrawString(footer.c_str(), -1, &subFont, footerPt, &blackBrush);

                EndPage(pd.hDC);
            }
            EndDoc(pd.hDC);
        }
        DeleteDC(pd.hDC);
        if (pd.hDevMode) GlobalFree(pd.hDevMode);
        if (pd.hDevNames) GlobalFree(pd.hDevNames);
    }
}

// ============================================================================
// 5. Export PNG Dialog (WriteAsPNGDialog replica)
// ============================================================================
struct ExportPngDlgContext {
    const HoDoKuStudio* studio{nullptr};
    HWND hEditSize{NULL};
    HWND hEditDpi{NULL};
    HWND hEditPath{NULL};
    HWND hChkCand{NULL};
    HWND hChkColors{NULL};
    HWND hOwner{NULL};
};

inline LRESULT CALLBACK ExportPngDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    ExportPngDlgContext* ctx = reinterpret_cast<ExportPngDlgContext*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (!ctx || !ctx->studio) break;

        if (id == 103) { // Browse...
            wchar_t szFile[MAX_PATH] = L"sudoku_board.png";
            OPENFILENAMEW ofn = {};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = L"PNG Image (*.png)\0*.png\0All Files (*.*)\0*.*\0";
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_OVERWRITEPROMPT;
            if (GetSaveFileNameW(&ofn)) {
                SetWindowTextW(ctx->hEditPath, szFile);
            }
            return 0;
        } else if (id == IDOK || id == 1) { // Export PNG
            wchar_t szSize[32] = L"";
            wchar_t szDpi[32] = L"";
            wchar_t szPath[MAX_PATH] = L"";

            GetWindowTextW(ctx->hEditSize, szSize, 32);
            GetWindowTextW(ctx->hEditDpi, szDpi, 32);
            GetWindowTextW(ctx->hEditPath, szPath, MAX_PATH);

            int imgW = _wtoi(szSize);
            if (imgW < 200) imgW = 1080;
            if (imgW > 8192) imgW = 8192;
            int imgH = imgW;

            int dpi = _wtoi(szDpi);
            if (dpi < 72) dpi = 300;

            if (wcslen(szPath) == 0) {
                MessageBoxW(hwnd, L"Please select a valid destination file path.", L"Export PNG Error", MB_OK | MB_ICONERROR);
                return 0;
            }

            Gdiplus::Bitmap bmp(imgW, imgH, PixelFormat32bppARGB);
            bmp.SetResolution(static_cast<float>(dpi), static_cast<float>(dpi));

            Gdiplus::Graphics g(&bmp);
            g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

            GridRenderer exportRenderer;
            exportRenderer.render_grid_canvas(g, *ctx->studio, 0, 0, imgW, imgH);

            CLSID clsidPng = { 0x557cf406, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e } };
            Gdiplus::Status st = bmp.Save(szPath, &clsidPng, NULL);

            if (st == Gdiplus::Ok) {
                std::wstring msgOk = L"Sudoku board successfully exported to:\n" + std::wstring(szPath) +
                                    L"\n\nResolution: " + std::to_wstring(imgW) + L"x" + std::to_wstring(imgH) +
                                    L" (" + std::to_wstring(dpi) + L" DPI)";
                MessageBoxW(hwnd, msgOk.c_str(), L"Export PNG - HoDoKu", MB_OK | MB_ICONINFORMATION);
                DestroyWindow(hwnd);
            } else {
                MessageBoxW(hwnd, L"Failed to save PNG image to the specified path.", L"Export Error", MB_OK | MB_ICONERROR);
            }
            return 0;
        } else if (id == IDCANCEL || id == 2) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY: {
        HWND hOwner = GetWindow(hwnd, GW_OWNER);
        if (hOwner) {
            EnableWindow(hOwner, TRUE);
            SetForegroundWindow(hOwner);
        }
        delete ctx;
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

inline void DoExportPng(HWND hParent, const HoDoKuStudio& studio) {
    HINSTANCE hInst = GetModuleHandle(NULL);
    const wchar_t DLG_CLASS[] = L"HoDoKuExportPngClass";

    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = ExportPngDlgProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = DLG_CLASS;
        RegisterClassExW(&wc);
        registered = true;
    }

    RECT rcParent;
    GetWindowRect(hParent, &rcParent);
    int dw = 520, dh = 340;
    int dx = rcParent.left + (rcParent.right - rcParent.left - dw) / 2;
    int dy = rcParent.top + (rcParent.bottom - rcParent.top - dh) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        DLG_CLASS,
        L"Export Board as PNG Image - HoDoKu",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        dx, dy, dw, dh,
        hParent, NULL, hInst, NULL
    );
    if (!hDlg) return;
    EnableWindow(hParent, FALSE);

    ExportPngDlgContext* ctx = new ExportPngDlgContext();
    ctx->studio = &studio;
    ctx->hOwner = hParent;
    SetWindowLongPtrW(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));

    HFONT hFont = GetHoDoKuDialogFont();

    HWND hGrp = CreateWindowW(L"BUTTON", L"PNG Image Parameters",
                              WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                              16, 12, 472, 130, hDlg, NULL, hInst, NULL);
    SendMessage(hGrp, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hLblSize = CreateWindowW(L"STATIC", L"Image Dimensions (Width & Height):",
                                  WS_CHILD | WS_VISIBLE, 32, 38, 220, 20, hDlg, NULL, hInst, NULL);
    SendMessage(hLblSize, WM_SETFONT, (WPARAM)hFont, TRUE);

    ctx->hEditSize = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"1080",
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL,
                                     260, 36, 80, 22, hDlg, NULL, hInst, NULL);
    SendMessage(ctx->hEditSize, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hLblPx = CreateWindowW(L"STATIC", L"pixels",
                                WS_CHILD | WS_VISIBLE, 348, 38, 50, 20, hDlg, NULL, hInst, NULL);
    SendMessage(hLblPx, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hLblDpi = CreateWindowW(L"STATIC", L"Target Resolution / DPI:",
                                 WS_CHILD | WS_VISIBLE, 32, 68, 220, 20, hDlg, NULL, hInst, NULL);
    SendMessage(hLblDpi, WM_SETFONT, (WPARAM)hFont, TRUE);

    ctx->hEditDpi = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"300",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL,
                                    260, 66, 80, 22, hDlg, NULL, hInst, NULL);
    SendMessage(ctx->hEditDpi, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hLblDpiUnit = CreateWindowW(L"STATIC", L"DPI (print quality)",
                                     WS_CHILD | WS_VISIBLE, 348, 68, 110, 20, hDlg, NULL, hInst, NULL);
    SendMessage(hLblDpiUnit, WM_SETFONT, (WPARAM)hFont, TRUE);

    ctx->hChkCand = CreateWindowW(L"BUTTON", L"Render Candidates (Pencilmarks)",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                  32, 102, 220, 22, hDlg, NULL, hInst, NULL);
    SendMessage(ctx->hChkCand, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(ctx->hChkCand, BM_SETCHECK, BST_CHECKED, 0);

    ctx->hChkColors = CreateWindowW(L"BUTTON", L"Render Cell & Candidate Colors",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                    260, 102, 210, 22, hDlg, NULL, hInst, NULL);
    SendMessage(ctx->hChkColors, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(ctx->hChkColors, BM_SETCHECK, BST_CHECKED, 0);

    HWND hGrpPath = CreateWindowW(L"BUTTON", L"Destination File",
                                  WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                  16, 152, 472, 74, hDlg, NULL, hInst, NULL);
    SendMessage(hGrpPath, WM_SETFONT, (WPARAM)hFont, TRUE);

    ctx->hEditPath = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"sudoku_board.png",
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                     32, 180, 320, 24, hDlg, NULL, hInst, NULL);
    SendMessage(ctx->hEditPath, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hBtnBrowse = CreateWindowW(L"BUTTON", L"Browse...",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                    362, 180, 106, 24, hDlg, (HMENU)103, hInst, NULL);
    SendMessage(hBtnBrowse, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hBtnExport = CreateWindowW(L"BUTTON", L"Export PNG",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                    260, 248, 110, 28, hDlg, (HMENU)IDOK, hInst, NULL);
    HWND hBtnCancel = CreateWindowW(L"BUTTON", L"Cancel",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                    378, 248, 106, 28, hDlg, (HMENU)IDCANCEL, hInst, NULL);

    SendMessage(hBtnExport, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)hFont, TRUE);
}

// ============================================================================
// 6. Preferences Dialog (ConfigDialog replica with SysTabControl32)
// ============================================================================
struct PreferencesDlgContext {
    HoDoKuStudio* studio{nullptr};
    HWND hTab{NULL};
    HWND hOwner{NULL};
    // Tab 0 (General)
    HWND hChkShowCand{NULL};
    HWND hChkShowDev{NULL};
    HWND hChkShowWrong{NULL};
    HWND hChkColorValues{NULL};
    HWND hChkColorKu{NULL};
    // Tab 1 (Colors)
    HWND hLblPaletteDesc{NULL};
    // Tab 2 (Solver)
    HWND hChkFas{NULL};
    HWND hChkFilterExc{NULL};
    HWND hLblSimdInfo{NULL};
};

inline void ShowPreferencesTab(PreferencesDlgContext* ctx, int tabIndex) {
    int cmd0 = (tabIndex == 0) ? SW_SHOW : SW_HIDE;
    int cmd1 = (tabIndex == 1) ? SW_SHOW : SW_HIDE;
    int cmd2 = (tabIndex == 2) ? SW_SHOW : SW_HIDE;

    if (ctx->hChkShowCand) ShowWindow(ctx->hChkShowCand, cmd0);
    if (ctx->hChkShowDev) ShowWindow(ctx->hChkShowDev, cmd0);
    if (ctx->hChkShowWrong) ShowWindow(ctx->hChkShowWrong, cmd0);
    if (ctx->hChkColorValues) ShowWindow(ctx->hChkColorValues, cmd0);
    if (ctx->hChkColorKu) ShowWindow(ctx->hChkColorKu, cmd0);

    if (ctx->hLblPaletteDesc) ShowWindow(ctx->hLblPaletteDesc, cmd1);

    if (ctx->hChkFas) ShowWindow(ctx->hChkFas, cmd2);
    if (ctx->hChkFilterExc) ShowWindow(ctx->hChkFilterExc, cmd2);
    if (ctx->hLblSimdInfo) ShowWindow(ctx->hLblSimdInfo, cmd2);
}

inline void ApplyPreferencesSettings(PreferencesDlgContext* ctx) {
    if (!ctx || !ctx->studio) return;
    AppSettings s = SettingsManager::load();

    s.show_candidates = (SendMessageW(ctx->hChkShowCand, BM_GETCHECK, 0, 0) == BST_CHECKED);
    s.show_deviations = (SendMessageW(ctx->hChkShowDev, BM_GETCHECK, 0, 0) == BST_CHECKED);
    s.show_wrong_values = (SendMessageW(ctx->hChkShowWrong, BM_GETCHECK, 0, 0) == BST_CHECKED);
    s.color_values = (SendMessageW(ctx->hChkColorValues, BM_GETCHECK, 0, 0) == BST_CHECKED);
    s.colorku_mode = (SendMessageW(ctx->hChkColorKu, BM_GETCHECK, 0, 0) == BST_CHECKED);

    s.filter_excluded = (SendMessageW(ctx->hChkFilterExc, BM_GETCHECK, 0, 0) == BST_CHECKED);

    ctx->studio->set_colorku_mode(s.colorku_mode);

    SettingsManager::save(s);
    if (g_onPuzzleStateChanged) g_onPuzzleStateChanged();
    InvalidateRect(ctx->hOwner, NULL, FALSE);
}

inline LRESULT CALLBACK PreferencesDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PreferencesDlgContext* ctx = reinterpret_cast<PreferencesDlgContext*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (!ctx) break;

        if (id == IDOK || id == 1) { // OK
            ApplyPreferencesSettings(ctx);
            DestroyWindow(hwnd);
            return 0;
        } else if (id == 3) { // Apply
            ApplyPreferencesSettings(ctx);
            return 0;
        } else if (id == IDCANCEL || id == 2) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }
    case WM_NOTIFY: {
        LPNMHDR pnm = (LPNMHDR)lParam;
        if (ctx && pnm->hwndFrom == ctx->hTab && pnm->code == TCN_SELCHANGE) {
            int sel = TabCtrl_GetCurSel(ctx->hTab);
            ShowPreferencesTab(ctx, sel);
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (ctx && TabCtrl_GetCurSel(ctx->hTab) == 1) {
            Graphics g(hdc);
            g.SetSmoothingMode(SmoothingModeAntiAlias);

            int startX = 40;
            int startY = 160;
            int swatchW = 38;
            int swatchH = 26;
            int gap = 8;

            FontFamily ff(L"Segoe UI");
            Font f(&ff, 11, FontStyleBold, UnitPixel);
            SolidBrush textB(Color(255, 30, 30, 30));
            StringFormat fmt;
            fmt.SetAlignment(StringAlignmentCenter);
            fmt.SetLineAlignment(StringAlignmentCenter);

            const wchar_t* names[10] = { L"a", L"A", L"b", L"B", L"c", L"C", L"d", L"D", L"e", L"E" };
            for (int i = 0; i < 10; ++i) {
                int bx = startX + (i % 5) * (swatchW + gap + 40);
                int by = startY + (i / 5) * (swatchH + gap + 20);

                SolidBrush b(HODOKU_PALETTE[i]);
                g.FillRectangle(&b, bx, by, swatchW, swatchH);
                Pen bdr(Color(255, 90, 90, 90), 1.0f);
                g.DrawRectangle(&bdr, bx, by, swatchW, swatchH);

                RectF r(static_cast<float>(bx + swatchW + 4), static_cast<float>(by), 32.0f, static_cast<float>(swatchH));
                g.DrawString(names[i], -1, &f, r, &fmt, &textB);
            }
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY: {
        HWND hOwner = GetWindow(hwnd, GW_OWNER);
        if (hOwner) {
            EnableWindow(hOwner, TRUE);
            SetForegroundWindow(hOwner);
        }
        delete ctx;
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

inline void ShowPreferencesDialog(HWND hParent, HoDoKuStudio& studio) {
    HINSTANCE hInst = GetModuleHandle(NULL);
    const wchar_t DLG_CLASS[] = L"HoDoKuPreferencesClass";

    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = PreferencesDlgProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = DLG_CLASS;
        RegisterClassExW(&wc);
        registered = true;
    }

    RECT rcParent;
    GetWindowRect(hParent, &rcParent);
    int dw = 580, dh = 480;
    int dx = rcParent.left + (rcParent.right - rcParent.left - dw) / 2;
    int dy = rcParent.top + (rcParent.bottom - rcParent.top - dh) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        DLG_CLASS,
        L"Preferences - HoDoKu",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        dx, dy, dw, dh,
        hParent, NULL, hInst, NULL
    );
    if (!hDlg) return;
    EnableWindow(hParent, FALSE);

    PreferencesDlgContext* ctx = new PreferencesDlgContext();
    ctx->studio = &studio;
    ctx->hOwner = hParent;
    SetWindowLongPtrW(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));

    HFONT hFont = GetHoDoKuDialogFont();
    AppSettings s = SettingsManager::load();

    ctx->hTab = CreateWindowW(WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS,
                              16, 12, 532, 380, hDlg, (HMENU)600, hInst, NULL);
    SendMessage(ctx->hTab, WM_SETFONT, (WPARAM)hFont, TRUE);

    TCITEMW tie = {};
    tie.mask = TCIF_TEXT;
    tie.pszText = const_cast<LPWSTR>(L"General");
    TabCtrl_InsertItem(ctx->hTab, 0, &tie);
    tie.pszText = const_cast<LPWSTR>(L"Colors");
    TabCtrl_InsertItem(ctx->hTab, 1, &tie);
    tie.pszText = const_cast<LPWSTR>(L"Solver & Engine");
    TabCtrl_InsertItem(ctx->hTab, 2, &tie);

    // Tab 0 Controls
    int ty = 52;
    ctx->hChkShowCand = CreateWindowW(L"BUTTON", L"Show Candidates in unsolved cells",
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                      36, ty, 460, 24, hDlg, NULL, hInst, NULL);
    ctx->hChkShowDev = CreateWindowW(L"BUTTON", L"Show tutor deviations from unique puzzle solution",
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                     36, ty + 32, 460, 24, hDlg, NULL, hInst, NULL);
    ctx->hChkShowWrong = CreateWindowW(L"BUTTON", L"Show duplicate / rule-violating values in grid",
                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                       36, ty + 64, 460, 24, hDlg, NULL, hInst, NULL);
    ctx->hChkColorValues = CreateWindowW(L"BUTTON", L"Differentiate givens (black) from user entries (blue)",
                                         WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                         36, ty + 96, 460, 24, hDlg, NULL, hInst, NULL);
    ctx->hChkColorKu = CreateWindowW(L"BUTTON", L"ColorKu 3D Marble Sphere Mode (Ctrl+Shift+C)",
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                     36, ty + 128, 460, 24, hDlg, NULL, hInst, NULL);

    SendMessage(ctx->hChkShowCand, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(ctx->hChkShowDev, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(ctx->hChkShowWrong, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(ctx->hChkColorValues, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(ctx->hChkColorKu, WM_SETFONT, (WPARAM)hFont, TRUE);

    SendMessage(ctx->hChkShowCand, BM_SETCHECK, s.show_candidates ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(ctx->hChkShowDev, BM_SETCHECK, s.show_deviations ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(ctx->hChkShowWrong, BM_SETCHECK, s.show_wrong_values ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(ctx->hChkColorValues, BM_SETCHECK, s.color_values ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(ctx->hChkColorKu, BM_SETCHECK, studio.is_colorku_mode() ? BST_CHECKED : BST_UNCHECKED, 0);

    // Tab 1 Controls
    std::wstring palDesc =
        L"HoDoKu 10-Color Palette Configuration:\n\n"
        L"Colors 0..9 correspond to keyboard shortcuts A-E (primary colors) and\n"
        L"Shift+A through Shift+E (secondary tints). Press R to clear all colors.\n\n"
        L"The swatches below show the active theme colors mapped to indices 0..9:";
    ctx->hLblPaletteDesc = CreateWindowW(L"STATIC", palDesc.c_str(),
                                         WS_CHILD, 36, ty, 480, 95, hDlg, NULL, hInst, NULL);
    SendMessage(ctx->hLblPaletteDesc, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Tab 2 Controls
    ctx->hChkFas = CreateWindowW(L"BUTTON", L"Automatically compute Find All Steps (FAS)",
                                 WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, 36, ty, 460, 24, hDlg, NULL, hInst, NULL);
    ctx->hChkFilterExc = CreateWindowW(L"BUTTON", L"Filter excluded candidates in toolbar digit filters",
                                       WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, 36, ty + 32, 460, 24, hDlg, NULL, hInst, NULL);

    std::wstring simdText =
        L"Hardware Acceleration & Architecture:\n"
        L" • SIMD Engine: Intel/AMD AVX2 + FMA + POPCNT (Active)\n"
        L" • Exact Cover: Knuth's Dancing Links (DLX) Vectorized\n"
        L" • Solvers: 45 Logical Hierarchy Solvers (Singles through ALS & Chains)\n"
        L" • Background Generator: Dual-worker thread pool";
    ctx->hLblSimdInfo = CreateWindowW(L"STATIC", simdText.c_str(),
                                      WS_CHILD, 36, ty + 74, 480, 110, hDlg, NULL, hInst, NULL);

    SendMessage(ctx->hChkFas, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(ctx->hChkFilterExc, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(ctx->hLblSimdInfo, WM_SETFONT, (WPARAM)hFont, TRUE);

    SendMessage(ctx->hChkFas, BM_SETCHECK, BST_CHECKED, 0);
    SendMessage(ctx->hChkFilterExc, BM_SETCHECK, s.filter_excluded ? BST_CHECKED : BST_UNCHECKED, 0);

    ShowPreferencesTab(ctx, 0);

    // Bottom buttons
    HWND hBtnOk = CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                264, 404, 90, 28, hDlg, (HMENU)IDOK, hInst, NULL);
    HWND hBtnCancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                    362, 404, 90, 28, hDlg, (HMENU)IDCANCEL, hInst, NULL);
    HWND hBtnApply = CreateWindowW(L"BUTTON", L"Apply", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                   460, 404, 88, 28, hDlg, (HMENU)3, hInst, NULL);

    SendMessage(hBtnOk, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnApply, WM_SETFONT, (WPARAM)hFont, TRUE);
}

inline void ShowPreferencesDialog(HWND hParent) {
    HoDoKuStudio* pStudio = reinterpret_cast<HoDoKuStudio*>(GetWindowLongPtrW(hParent, GWLP_USERDATA));
    if (pStudio) {
        ShowPreferencesDialog(hParent, *pStudio);
    }
}

// ============================================================================
// 7. Custom Pattern Designer Dialog (Plan 6.5)
// ============================================================================
struct PatternDesignerDlgContext {
    HoDoKuStudio* studio{nullptr};
    HWND hDlg{NULL};
    HWND hOwner{NULL};
    HWND hCanvas{NULL};
    HWND hLblGivens{NULL};
    HWND hComboSymmetry{NULL};
    HWND hComboPresets{NULL};
    HWND hLblStatus{NULL};
    hodoku::core::BitSet81 patternMask;
    int hoveredCell{-1};
    hodoku::core::SymmetryType currentSymmetry{hodoku::core::SymmetryType::Rotational180};
    std::optional<hodoku::core::BoardState> cachedPuzzle;
};

enum PatternDlgControlId {
    IDC_PATTERN_CANVAS = 6101,
    IDC_PATTERN_GIVENS_LBL,
    IDC_PATTERN_SYMMETRY_COMBO,
    IDC_PATTERN_PRESETS_COMBO,
    IDC_PATTERN_CLEAR_BTN,
    IDC_PATTERN_INVERT_BTN,
    IDC_PATTERN_COPY_BTN,
    IDC_PATTERN_PASTE_BTN,
    IDC_PATTERN_RANDOM_BTN,
    IDC_PATTERN_CHECK_BTN,
    IDC_PATTERN_GENERATE_BTN,
    IDC_PATTERN_STATUS_LBL
};

inline void UpdatePatternGivensLabel(PatternDesignerDlgContext* ctx) {
    if (!ctx || !ctx->hLblGivens) return;
    int count = ctx->patternMask.count();
    std::wstring text = L"Givens: " + std::to_wstring(count) + L" / 81";
    if (count < 17) {
        text += L"  (Warning: Min 17 required!)";
    } else {
        text += L"  (Valid count >= 17)";
    }
    SetWindowTextW(ctx->hLblGivens, text.c_str());
}

inline LRESULT CALLBACK PatternCanvasWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PatternDesignerDlgContext* ctx = reinterpret_cast<PatternDesignerDlgContext*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
        HGDIOBJ oldBmp = SelectObject(memDC, memBmp);

        HBRUSH hBgBrush = CreateSolidBrush(RGB(248, 250, 252));
        FillRect(memDC, &rc, hBgBrush);
        DeleteObject(hBgBrush);

        int margin = 10;
        int cellSize = 36;

        if (ctx) {
            for (int r = 0; r < 9; ++r) {
                for (int c = 0; c < 9; ++c) {
                    int cell = r * 9 + c;
                    int x = margin + c * cellSize;
                    int y = margin + r * cellSize;
                    RECT cellRc = { x, y, x + cellSize, y + cellSize };

                    bool isGiven = ctx->patternMask.test(cell);
                    bool isHovered = (ctx->hoveredCell == cell);

                    if (isGiven) {
                        HBRUSH hGivenBrush = CreateSolidBrush(RGB(30, 58, 138));
                        FillRect(memDC, &cellRc, hGivenBrush);
                        DeleteObject(hGivenBrush);

                        RECT innerRc = { x + 6, y + 6, x + cellSize - 6, y + cellSize - 6 };
                        HBRUSH hInnerBrush = CreateSolidBrush(isHovered ? RGB(96, 165, 250) : RGB(59, 130, 246));
                        FillRect(memDC, &innerRc, hInnerBrush);
                        DeleteObject(hInnerBrush);
                    } else {
                        HBRUSH hEmptyBrush = CreateSolidBrush(isHovered ? RGB(239, 246, 255) : RGB(255, 255, 255));
                        FillRect(memDC, &cellRc, hEmptyBrush);
                        DeleteObject(hEmptyBrush);
                    }

                    HPEN hCellPen = CreatePen(PS_SOLID, 1, RGB(203, 213, 225));
                    HGDIOBJ oldPen = SelectObject(memDC, hCellPen);
                    MoveToEx(memDC, x, y + cellSize, NULL);
                    LineTo(memDC, x + cellSize, y + cellSize);
                    LineTo(memDC, x + cellSize, y);
                    SelectObject(memDC, oldPen);
                    DeleteObject(hCellPen);

                    if (isHovered && !isGiven) {
                        HPEN hHoverPen = CreatePen(PS_SOLID, 2, RGB(59, 130, 246));
                        HGDIOBJ oldHoverPen = SelectObject(memDC, hHoverPen);
                        HGDIOBJ oldNullBrush = SelectObject(memDC, GetStockObject(NULL_BRUSH));
                        Rectangle(memDC, x + 1, y + 1, x + cellSize, y + cellSize);
                        SelectObject(memDC, oldNullBrush);
                        SelectObject(memDC, oldHoverPen);
                        DeleteObject(hHoverPen);
                    }
                }
            }

            HPEN hThickPen = CreatePen(PS_SOLID, 3, RGB(15, 23, 42));
            HGDIOBJ oldThickPen = SelectObject(memDC, hThickPen);
            int gridSpan = 9 * cellSize;

            for (int i = 0; i <= 3; ++i) {
                int offset = margin + i * (3 * cellSize);
                MoveToEx(memDC, margin, offset, NULL);
                LineTo(memDC, margin + gridSpan, offset);
                MoveToEx(memDC, offset, margin, NULL);
                LineTo(memDC, offset, margin + gridSpan);
            }
            SelectObject(memDC, oldThickPen);
            DeleteObject(hThickPen);
        }

        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (ctx) {
            TRACKMOUSEEVENT tme = {};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);

            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            int margin = 10;
            int cellSize = 36;
            int c = (x - margin) / cellSize;
            int r = (y - margin) / cellSize;
            int newHover = -1;
            if (c >= 0 && c < 9 && r >= 0 && r < 9 && x >= margin && y >= margin &&
                x < margin + 9 * cellSize && y < margin + 9 * cellSize) {
                newHover = r * 9 + c;
            }
            if (newHover != ctx->hoveredCell) {
                ctx->hoveredCell = newHover;
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return 0;
    }
    case WM_MOUSELEAVE: {
        if (ctx && ctx->hoveredCell != -1) {
            ctx->hoveredCell = -1;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_LBUTTONDOWN: {
        if (ctx) {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            int margin = 10;
            int cellSize = 36;
            int c = (x - margin) / cellSize;
            int r = (y - margin) / cellSize;
            if (c >= 0 && c < 9 && r >= 0 && r < 9 && x >= margin && y >= margin &&
                x < margin + 9 * cellSize && y < margin + 9 * cellSize) {
                int cell = r * 9 + c;
                bool targetSet = !ctx->patternMask.test(cell);
                auto orbit = hodoku::core::SudokuGenerator::get_symmetric_cells(cell, ctx->currentSymmetry);
                for (int symCell : orbit) {
                    if (targetSet) ctx->patternMask.set(symCell);
                    else ctx->patternMask.reset(symCell);
                }
                ctx->cachedPuzzle.reset();
                UpdatePatternGivensLabel(ctx);
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

inline LRESULT CALLBACK PatternDesignerDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PatternDesignerDlgContext* ctx = reinterpret_cast<PatternDesignerDlgContext*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        if (!ctx) break;

        if (id == IDC_PATTERN_SYMMETRY_COMBO && code == CBN_SELCHANGE) {
            int sel = (int)SendMessageW(ctx->hComboSymmetry, CB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel <= 6) {
                const hodoku::core::SymmetryType symms[] = {
                    hodoku::core::SymmetryType::None,
                    hodoku::core::SymmetryType::Rotational180,
                    hodoku::core::SymmetryType::Rotational90,
                    hodoku::core::SymmetryType::Horizontal,
                    hodoku::core::SymmetryType::Vertical,
                    hodoku::core::SymmetryType::Diagonal,
                    hodoku::core::SymmetryType::AntiDiagonal
                };
                ctx->currentSymmetry = symms[sel];
            }
            return 0;
        } else if (id == IDC_PATTERN_PRESETS_COMBO && code == CBN_SELCHANGE) {
            int sel = (int)SendMessageW(ctx->hComboPresets, CB_GETCURSEL, 0, 0);
            if (sel == 1) ctx->patternMask = hodoku::core::SudokuGenerator::make_preset_diamond();
            else if (sel == 2) ctx->patternMask = hodoku::core::SudokuGenerator::make_preset_cross();
            else if (sel == 3) ctx->patternMask = hodoku::core::SudokuGenerator::make_preset_picture_frame();
            else if (sel == 4) ctx->patternMask = hodoku::core::SudokuGenerator::make_preset_checkerboard();
            else if (sel == 5) {
                hodoku::core::SudokuGenerator gen;
                ctx->patternMask = gen.make_preset_random_symmetric(28, ctx->currentSymmetry);
            } else if (sel == 6) {
                hodoku::core::SudokuGenerator gen;
                ctx->patternMask = gen.make_preset_random_symmetric(32, ctx->currentSymmetry);
            }
            if (sel > 0) {
                ctx->cachedPuzzle.reset();
                UpdatePatternGivensLabel(ctx);
                InvalidateRect(ctx->hCanvas, NULL, FALSE);
            }
            return 0;
        } else if (id == IDC_PATTERN_CLEAR_BTN) {
            ctx->patternMask.clear();
            ctx->cachedPuzzle.reset();
            UpdatePatternGivensLabel(ctx);
            InvalidateRect(ctx->hCanvas, NULL, FALSE);
            return 0;
        } else if (id == IDC_PATTERN_INVERT_BTN) {
            ctx->patternMask = ~ctx->patternMask;
            ctx->cachedPuzzle.reset();
            UpdatePatternGivensLabel(ctx);
            InvalidateRect(ctx->hCanvas, NULL, FALSE);
            return 0;
        } else if (id == IDC_PATTERN_COPY_BTN) {
            std::string s;
            s.reserve(hodoku::core::TOTAL_CELLS);
            for (int i = 0; i < hodoku::core::TOTAL_CELLS; ++i) {
                s += ctx->patternMask.test(i) ? '1' : '0';
            }
            SetClipboardText(hwnd, s);
            if (ctx->hLblStatus) {
                SetWindowTextW(ctx->hLblStatus, L"Pattern mask copied to clipboard (81-character binary string).");
            }
            return 0;
        } else if (id == IDC_PATTERN_PASTE_BTN) {
            std::string clip = GetClipboardText(hwnd);
            int count = 0;
            hodoku::core::BitSet81 newMask;
            for (char ch : clip) {
                if (count >= hodoku::core::TOTAL_CELLS) break;
                if (ch == '1' || ch == 'X' || ch == 'x' || (ch >= '2' && ch <= '9')) {
                    newMask.set(count++);
                } else if (ch == '0' || ch == '.' || ch == '_' || ch == '-') {
                    count++;
                }
            }
            if (count == hodoku::core::TOTAL_CELLS) {
                ctx->patternMask = newMask;
                ctx->cachedPuzzle.reset();
                UpdatePatternGivensLabel(ctx);
                InvalidateRect(ctx->hCanvas, NULL, FALSE);
                if (ctx->hLblStatus) {
                    SetWindowTextW(ctx->hLblStatus, L"Pattern successfully imported from clipboard.");
                }
            } else {
                if (ctx->hLblStatus) {
                    SetWindowTextW(ctx->hLblStatus, L"Clipboard text did not contain 81 recognizable cell values.");
                }
            }
            return 0;
        } else if (id == IDC_PATTERN_RANDOM_BTN) {
            hodoku::core::SudokuGenerator gen;
            ctx->patternMask = gen.make_preset_random_symmetric(28, ctx->currentSymmetry);
            ctx->cachedPuzzle.reset();
            UpdatePatternGivensLabel(ctx);
            InvalidateRect(ctx->hCanvas, NULL, FALSE);
            if (ctx->hLblStatus) {
                SetWindowTextW(ctx->hLblStatus, L"Generated random symmetric pattern (28 clues).");
            }
            return 0;
        } else if (id == IDC_PATTERN_CHECK_BTN) {
            if (ctx->patternMask.count() < 17) {
                if (ctx->hLblStatus) {
                    SetWindowTextW(ctx->hLblStatus, L"Invalid: Minimum 17 clues required. Uniqueness is impossible.");
                }
                return 0;
            }
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            if (ctx->hLblStatus) {
                SetWindowTextW(ctx->hLblStatus, L"Checking pattern validity (searching for unique puzzle)...");
                UpdateWindow(ctx->hLblStatus);
            }
            hodoku::core::SudokuGenerator gen;
            auto puz = gen.generate_pattern_puzzle(ctx->patternMask, 500);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            if (puz.has_value()) {
                ctx->cachedPuzzle = puz;
                if (ctx->hLblStatus) {
                    SetWindowTextW(ctx->hLblStatus, L"Pattern is VALID! Unique Sudoku puzzle found.");
                }
            } else {
                if (ctx->hLblStatus) {
                    SetWindowTextW(ctx->hLblStatus, L"Pattern test inconclusive or over-constrained after 500 attempts.");
                }
            }
            return 0;
        } else if (id == IDC_PATTERN_GENERATE_BTN || id == IDOK) {
            if (ctx->patternMask.count() < 17) {
                MessageBoxW(hwnd, L"The pattern must contain at least 17 givens to produce a unique Sudoku.", L"Invalid Pattern", MB_OK | MB_ICONWARNING);
                return 0;
            }
            hodoku::core::BoardState target;
            if (ctx->cachedPuzzle.has_value()) {
                target = *ctx->cachedPuzzle;
            } else {
                SetCursor(LoadCursor(NULL, IDC_WAIT));
                if (ctx->hLblStatus) {
                    SetWindowTextW(ctx->hLblStatus, L"Digging clues to match exact pattern (up to 3000 attempts)...");
                    UpdateWindow(ctx->hLblStatus);
                }
                hodoku::core::SudokuGenerator gen;
                auto puz = gen.generate_pattern_puzzle(ctx->patternMask, 3000);
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                if (!puz.has_value()) {
                    MessageBoxW(hwnd, L"Could not generate a unique Sudoku with this exact clue pattern within 3000 attempts.\n\nTip: Try adding a few more clues or choosing a symmetric preset.", L"Pattern Digging Timed Out", MB_OK | MB_ICONINFORMATION);
                    return 0;
                }
                target = *puz;
            }

            ctx->studio->load_puzzle(target);
            if (g_onPuzzleStateChanged) g_onPuzzleStateChanged();
            InvalidateRect(ctx->hOwner, NULL, FALSE);
            DestroyWindow(hwnd);
            return 0;
        } else if (id == IDCANCEL || id == 2) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY: {
        HWND hOwner = GetWindow(hwnd, GW_OWNER);
        if (hOwner) {
            EnableWindow(hOwner, TRUE);
            SetForegroundWindow(hOwner);
        }
        delete ctx;
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

inline void ShowPatternDesignerDialog(HWND hParent, HoDoKuStudio& studio) {
    HINSTANCE hInst = GetModuleHandle(NULL);
    const wchar_t CANVAS_CLASS[] = L"HoDoKuPatternCanvasClass";
    const wchar_t DLG_CLASS[] = L"HoDoKuPatternDesignerClass";

    static bool canvasRegistered = false;
    if (!canvasRegistered) {
        WNDCLASSEXW cwc = {};
        cwc.cbSize = sizeof(WNDCLASSEXW);
        cwc.lpfnWndProc = PatternCanvasWndProc;
        cwc.hInstance = hInst;
        cwc.hCursor = LoadCursor(NULL, IDC_CROSS);
        cwc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        cwc.lpszClassName = CANVAS_CLASS;
        RegisterClassExW(&cwc);
        canvasRegistered = true;
    }

    static bool dlgRegistered = false;
    if (!dlgRegistered) {
        WNDCLASSEXW dwc = {};
        dwc.cbSize = sizeof(WNDCLASSEXW);
        dwc.lpfnWndProc = PatternDesignerDlgProc;
        dwc.hInstance = hInst;
        dwc.hCursor = LoadCursor(NULL, IDC_ARROW);
        dwc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        dwc.lpszClassName = DLG_CLASS;
        RegisterClassExW(&dwc);
        dlgRegistered = true;
    }

    RECT rcParent;
    GetWindowRect(hParent, &rcParent);
    int dw = 680, dh = 480;
    int dx = rcParent.left + (rcParent.right - rcParent.left - dw) / 2;
    int dy = rcParent.top + (rcParent.bottom - rcParent.top - dh) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        DLG_CLASS,
        L"Custom Pattern Designer & Clue Digger - HoDoKu",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        dx, dy, dw, dh,
        hParent, NULL, hInst, NULL
    );
    if (!hDlg) return;
    EnableWindow(hParent, FALSE);

    PatternDesignerDlgContext* ctx = new PatternDesignerDlgContext();
    ctx->studio = &studio;
    ctx->hDlg = hDlg;
    ctx->hOwner = hParent;

    if (studio.get_board().get_givens().count() >= 17) {
        ctx->patternMask = studio.get_board().get_givens();
    } else {
        ctx->patternMask = hodoku::core::SudokuGenerator::make_preset_diamond();
    }

    SetWindowLongPtrW(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));

    HFONT hFont = GetHoDoKuDialogFont();
    HFONT hBoldFont = CreateFontW(-12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    // Left Side: Interactive Canvas
    ctx->hCanvas = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        CANVAS_CLASS,
        L"",
        WS_CHILD | WS_VISIBLE,
        18, 16, 344, 344,
        hDlg, (HMENU)IDC_PATTERN_CANVAS, hInst, NULL
    );
    SetWindowLongPtrW(ctx->hCanvas, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));

    ctx->hLblGivens = CreateWindowW(L"STATIC", L"Givens: 0 / 81",
                                   WS_CHILD | WS_VISIBLE | SS_LEFT,
                                   18, 368, 344, 20, hDlg, (HMENU)IDC_PATTERN_GIVENS_LBL, hInst, NULL);
    SendMessage(ctx->hLblGivens, WM_SETFONT, (WPARAM)hBoldFont, TRUE);

    HWND hLblSymm = CreateWindowW(L"STATIC", L"Symmetry:",
                                 WS_CHILD | WS_VISIBLE | SS_LEFT,
                                 18, 398, 68, 20, hDlg, NULL, hInst, NULL);
    SendMessage(hLblSymm, WM_SETFONT, (WPARAM)hFont, TRUE);

    ctx->hComboSymmetry = CreateWindowW(L"COMBOBOX", L"",
                                        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                                        90, 395, 272, 200, hDlg, (HMENU)IDC_PATTERN_SYMMETRY_COMBO, hInst, NULL);
    SendMessage(ctx->hComboSymmetry, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageW(ctx->hComboSymmetry, CB_ADDSTRING, 0, (LPARAM)L"None (Freehand)");
    SendMessageW(ctx->hComboSymmetry, CB_ADDSTRING, 0, (LPARAM)L"180° Rotational (Standard)");
    SendMessageW(ctx->hComboSymmetry, CB_ADDSTRING, 0, (LPARAM)L"90° Rotational (4-Fold)");
    SendMessageW(ctx->hComboSymmetry, CB_ADDSTRING, 0, (LPARAM)L"Horizontal Mirror");
    SendMessageW(ctx->hComboSymmetry, CB_ADDSTRING, 0, (LPARAM)L"Vertical Mirror");
    SendMessageW(ctx->hComboSymmetry, CB_ADDSTRING, 0, (LPARAM)L"Diagonal Mirror");
    SendMessageW(ctx->hComboSymmetry, CB_ADDSTRING, 0, (LPARAM)L"Anti-Diagonal Mirror");
    SendMessageW(ctx->hComboSymmetry, CB_SETCURSEL, 1, 0); // Default to 180° Rotational

    // Right Side: Presets & Tools Group
    HWND hGrpPresets = CreateWindowW(L"BUTTON", L"Pattern Presets & Editing",
                                     WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                     380, 10, 272, 176, hDlg, NULL, hInst, NULL);
    SendMessage(hGrpPresets, WM_SETFONT, (WPARAM)hFont, TRUE);

    ctx->hComboPresets = CreateWindowW(L"COMBOBOX", L"",
                                       WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                                       394, 34, 244, 180, hDlg, (HMENU)IDC_PATTERN_PRESETS_COMBO, hInst, NULL);
    SendMessage(ctx->hComboPresets, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageW(ctx->hComboPresets, CB_ADDSTRING, 0, (LPARAM)L"-- Select Preset Shape --");
    SendMessageW(ctx->hComboPresets, CB_ADDSTRING, 0, (LPARAM)L"Preset: Diamond (25 Clues)");
    SendMessageW(ctx->hComboPresets, CB_ADDSTRING, 0, (LPARAM)L"Preset: Cross / Plus (25 Clues)");
    SendMessageW(ctx->hComboPresets, CB_ADDSTRING, 0, (LPARAM)L"Preset: Picture Frame (29 Clues)");
    SendMessageW(ctx->hComboPresets, CB_ADDSTRING, 0, (LPARAM)L"Preset: Checkerboard (25 Clues)");
    SendMessageW(ctx->hComboPresets, CB_ADDSTRING, 0, (LPARAM)L"Preset: Random 180° (28 Clues)");
    SendMessageW(ctx->hComboPresets, CB_ADDSTRING, 0, (LPARAM)L"Preset: Random 180° (32 Clues)");
    SendMessageW(ctx->hComboPresets, CB_SETCURSEL, 0, 0);

    HWND hBtnClear = CreateWindowW(L"BUTTON", L"Clear All", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                   394, 68, 116, 26, hDlg, (HMENU)IDC_PATTERN_CLEAR_BTN, hInst, NULL);
    HWND hBtnInvert = CreateWindowW(L"BUTTON", L"Invert Mask", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                    522, 68, 116, 26, hDlg, (HMENU)IDC_PATTERN_INVERT_BTN, hInst, NULL);
    HWND hBtnCopy = CreateWindowW(L"BUTTON", L"Copy Pattern", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                  394, 100, 116, 26, hDlg, (HMENU)IDC_PATTERN_COPY_BTN, hInst, NULL);
    HWND hBtnPaste = CreateWindowW(L"BUTTON", L"Paste Pattern", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                   522, 100, 116, 26, hDlg, (HMENU)IDC_PATTERN_PASTE_BTN, hInst, NULL);
    HWND hBtnRandom = CreateWindowW(L"BUTTON", L"🎲 Random Symmetric Pattern", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                    394, 132, 244, 26, hDlg, (HMENU)IDC_PATTERN_RANDOM_BTN, hInst, NULL);

    SendMessage(hBtnClear, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnInvert, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnCopy, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnPaste, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnRandom, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Right Side: Generation & Validation Group
    HWND hGrpGen = CreateWindowW(L"BUTTON", L"Validation & Puzzle Digging",
                                 WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                 380, 196, 272, 224, hDlg, NULL, hInst, NULL);
    SendMessage(hGrpGen, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hBtnCheck = CreateWindowW(L"BUTTON", L"✓ Check Pattern Validity", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                   394, 220, 244, 28, hDlg, (HMENU)IDC_PATTERN_CHECK_BTN, hInst, NULL);
    SendMessage(hBtnCheck, WM_SETFONT, (WPARAM)hFont, TRUE);

    ctx->hLblStatus = CreateWindowW(L"STATIC",
                                   L"Click cells to toggle clues. Symmetries toggle counterparts automatically.",
                                   WS_CHILD | WS_VISIBLE | SS_LEFT,
                                   394, 254, 244, 52, hDlg, (HMENU)IDC_PATTERN_STATUS_LBL, hInst, NULL);
    SendMessage(ctx->hLblStatus, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hBtnGen = CreateWindowW(L"BUTTON", L"⚡ Generate & Play Puzzle", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                 394, 312, 244, 34, hDlg, (HMENU)IDC_PATTERN_GENERATE_BTN, hInst, NULL);
    HWND hBtnClose = CreateWindowW(L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                   394, 354, 244, 26, hDlg, (HMENU)IDCANCEL, hInst, NULL);

    SendMessage(hBtnGen, WM_SETFONT, (WPARAM)hBoldFont, TRUE);
    SendMessage(hBtnClose, WM_SETFONT, (WPARAM)hFont, TRUE);

    UpdatePatternGivensLabel(ctx);
}

} // namespace hodoku::ui


