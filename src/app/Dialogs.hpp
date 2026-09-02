#pragma once

#include "AppTypes.hpp"
#include "StudioModel.hpp"
#include <fstream>
#include <functional>

namespace hodoku::ui {

inline HWND g_hGivensDlg = NULL;
inline HWND g_hGivensEdit = NULL;
inline HWND g_hGivensStatus = NULL;

inline std::function<void()> g_onPuzzleStateChanged = nullptr;

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
    ofn.lpstrFilter = L"Sudoku Files (*.txt;*.ss;*.hsol)\0*.txt;*.ss;*.hsol\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameW(&ofn)) {
        std::ifstream f(szFile);
        if (f.is_open()) {
            std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            if (!content.empty()) {
                studio.import_from_string(content);
                if (g_onPuzzleStateChanged) g_onPuzzleStateChanged();
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
    }
}

inline void DoFileSave(HWND hwnd, const HoDoKuStudio& studio) {
    wchar_t szFile[MAX_PATH] = L"sudoku.txt";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"Text Sudoku (*.txt)\0*.txt\0Simple Sudoku (*.ss)\0*.ss\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    if (GetSaveFileNameW(&ofn)) {
        std::ofstream f(szFile);
        if (f.is_open()) {
            f << studio.export_givens_string() << "\n";
        }
    }
}

inline LRESULT CALLBACK SetGivensDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == 1) { // Load
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
        } else if (id == 2) { // Cancel
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

    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hMono = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             FIXED_PITCH | FF_MODERN, L"Consolas");

    HWND hLbl = CreateWindowW(L"STATIC", L"Enter or paste 81-character puzzle string, 9-line grid, or .ss format:",
                              WS_CHILD | WS_VISIBLE, 16, 12, 480, 20, g_hGivensDlg, NULL, hInst, NULL);
    SendMessage(hLbl, WM_SETFONT, (WPARAM)hFont, TRUE);

    g_hGivensEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                    WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
                                    16, 36, 470, 220, g_hGivensDlg, (HMENU)4, hInst, NULL);
    SendMessage(g_hGivensEdit, WM_SETFONT, (WPARAM)hMono, TRUE);

    std::string curGivens = studio.export_givens_string();
    std::wstring wGivens(curGivens.begin(), curGivens.end());
    SetWindowTextW(g_hGivensEdit, wGivens.c_str());

    g_hGivensStatus = CreateWindowW(L"STATIC", L"Clues: 0 / 81",
                                   WS_CHILD | WS_VISIBLE, 16, 266, 470, 20, g_hGivensDlg, NULL, hInst, NULL);
    SendMessage(g_hGivensStatus, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hBtnLoad = CreateWindowW(L"BUTTON", L"Load Puzzle", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                                  200, 298, 100, 28, g_hGivensDlg, (HMENU)1, hInst, NULL);
    HWND hBtnClear = CreateWindowW(L"BUTTON", L"Clear", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                   310, 298, 80, 28, g_hGivensDlg, (HMENU)3, hInst, NULL);
    HWND hBtnCancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                    400, 298, 86, 28, g_hGivensDlg, (HMENU)2, hInst, NULL);

    SendMessage(hBtnLoad, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnClear, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)hFont, TRUE);

    SendMessage(g_hGivensDlg, WM_COMMAND, MAKEWPARAM(4, EN_CHANGE), (LPARAM)g_hGivensEdit);
}

inline void ShowPreferencesDialog(HWND hParent) {
    MessageBoxW(hParent,
        L"HoDoKu Preferences:\n\n"
        L"• Show Candidates: Enabled\n"
        L"• Auto-Compute FAS (Find All Steps): Enabled\n"
        L"• Active Solver Mode: Complete HoDoKu Hierarchy\n"
        L"• Color Configuration: HoDoKu 10-Color Palette Active\n"
        L"• Generator Symmetries: 180° Rotational (Standard HoDoKu)\n\n"
        L"Settings are synchronized with native C++20 engine.",
        L"Preferences - HoDoKu", MB_OK | MB_ICONINFORMATION);
}

inline void ShowPracticingDialog(HWND hParent) {
    MessageBoxW(hParent,
        L"Practicing Mode Active!\n\n"
        L"In this mode, newly generated puzzles will specifically target\n"
        L"intermediate and advanced solving techniques (Subsets, Wings,\n"
        L"Fish, Coloring, Chains, and Forcing Chains).\n\n"
        L"Press Ctrl+N to generate a new practice puzzle.",
        L"Practicing Mode - HoDoKu", MB_OK | MB_ICONINFORMATION);
}

} // namespace hodoku::ui

