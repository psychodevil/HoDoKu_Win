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
    bool filter_excluded{false};
    bool show_candidates{true};
    bool show_deviations{true};
    bool show_wrong_values{true};
    bool color_values{true};
    int default_png_size{1080};
    int default_png_dpi{300};
    int default_png_unit{2}; // 0 = mm, 1 = inch, 2 = pixel
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
        s.filter_excluded = (GetPrivateProfileIntW(L"State", L"FilterExcluded", 0, ini.c_str()) != 0);
        s.show_candidates = (GetPrivateProfileIntW(L"Preferences", L"ShowCandidates", 1, ini.c_str()) != 0);
        s.show_deviations = (GetPrivateProfileIntW(L"Preferences", L"ShowDeviations", 1, ini.c_str()) != 0);
        s.show_wrong_values = (GetPrivateProfileIntW(L"Preferences", L"ShowWrongValues", 1, ini.c_str()) != 0);
        s.color_values = (GetPrivateProfileIntW(L"Preferences", L"ColorValues", 1, ini.c_str()) != 0);
        s.default_png_size = GetPrivateProfileIntW(L"Export", L"PngSize", 1080, ini.c_str());
        s.default_png_dpi = GetPrivateProfileIntW(L"Export", L"PngDpi", 300, ini.c_str());
        s.default_png_unit = GetPrivateProfileIntW(L"Export", L"PngUnit", 2, ini.c_str());

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
        WritePrivateProfileStringW(L"State", L"FilterExcluded", s.filter_excluded ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Preferences", L"ShowCandidates", s.show_candidates ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Preferences", L"ShowDeviations", s.show_deviations ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Preferences", L"ShowWrongValues", s.show_wrong_values ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Preferences", L"ColorValues", s.color_values ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Export", L"PngSize", std::to_wstring(s.default_png_size).c_str(), ini.c_str());
        WritePrivateProfileStringW(L"Export", L"PngDpi", std::to_wstring(s.default_png_dpi).c_str(), ini.c_str());
        WritePrivateProfileStringW(L"Export", L"PngUnit", std::to_wstring(s.default_png_unit).c_str(), ini.c_str());
    }
};

} // namespace hodoku::ui
