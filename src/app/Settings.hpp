#pragma once

#include <windows.h>
#include <string>

namespace hodoku::ui {

struct AppSettings {
    int window_x{CW_USEDEFAULT};
    int window_y{CW_USEDEFAULT};
    int window_w{1024};
    int window_h{768};
    bool maximized{false};
    int game_mode{0}; // 0 = Playing, 1 = Learning, 2 = Practicing
    bool colorku_mode{false};
};

class SettingsManager {
public:
    static std::wstring get_ini_path() {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        std::wstring s(path);
        size_t pos = s.find_last_of(L"\\/");
        if (pos != std::wstring::npos) {
            s = s.substr(0, pos + 1);
        }
        s += L"hodoku.ini";
        return s;
    }

    static AppSettings load() {
        AppSettings s;
        std::wstring ini = get_ini_path();

        s.window_x = GetPrivateProfileIntW(L"Window", L"X", CW_USEDEFAULT, ini.c_str());
        s.window_y = GetPrivateProfileIntW(L"Window", L"Y", CW_USEDEFAULT, ini.c_str());
        s.window_w = GetPrivateProfileIntW(L"Window", L"Width", 1024, ini.c_str());
        s.window_h = GetPrivateProfileIntW(L"Window", L"Height", 768, ini.c_str());
        s.maximized = (GetPrivateProfileIntW(L"Window", L"Maximized", 0, ini.c_str()) != 0);

        s.game_mode = GetPrivateProfileIntW(L"State", L"GameMode", 0, ini.c_str());
        s.colorku_mode = (GetPrivateProfileIntW(L"State", L"ColorKu", 0, ini.c_str()) != 0);

        // Bounds validation
        if (s.window_w < 640) s.window_w = 1024;
        if (s.window_h < 480) s.window_h = 768;

        return s;
    }

    static void save(const AppSettings& s) {
        std::wstring ini = get_ini_path();

        WritePrivateProfileStringW(L"Window", L"X", std::to_wstring(s.window_x).c_str(), ini.c_str());
        WritePrivateProfileStringW(L"Window", L"Y", std::to_wstring(s.window_y).c_str(), ini.c_str());
        WritePrivateProfileStringW(L"Window", L"Width", std::to_wstring(s.window_w).c_str(), ini.c_str());
        WritePrivateProfileStringW(L"Window", L"Height", std::to_wstring(s.window_h).c_str(), ini.c_str());
        WritePrivateProfileStringW(L"Window", L"Maximized", s.maximized ? L"1" : L"0", ini.c_str());

        WritePrivateProfileStringW(L"State", L"GameMode", std::to_wstring(s.game_mode).c_str(), ini.c_str());
        WritePrivateProfileStringW(L"State", L"ColorKu", s.colorku_mode ? L"1" : L"0", ini.c_str());
    }
};

} // namespace hodoku::ui
