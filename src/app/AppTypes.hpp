#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <objbase.h>
#include <commctrl.h>
#include <commdlg.h>
#include <gdiplus.h>

#include <vector>
#include <string>
#include <array>
#include <optional>
#include <algorithm>
#include <sstream>

#include "../core/Types.hpp"
#include "../core/BitSet81.hpp"
#include "../core/GridConstants.hpp"
#include "../core/BoardState.hpp"
#include "../core/DlxSolver.hpp"
#include "../core/Step.hpp"
#include "../core/Generator.hpp"

namespace hodoku::ui {

using namespace Gdiplus;
using namespace hodoku::core;

enum class TabView {
    Summary = 0,
    SolutionPath = 1,
    AllSteps = 2,
    ActiveCell = 3
};

enum class HintLevel {
    None,
    Vague,
    Concrete
};

enum class GameMode {
    Playing = 0,
    Learning = 1,
    Practicing = 2
};

enum class MoveValidation {
    Valid,
    RuleViolation,
    SolutionDeviation
};

// HoDoKu Color Palette for cell/candidate coloring (10 colors from Options.java)
static const Color HODOKU_PALETTE[10] = {
    Color(255, 255, 192, 89),  // 0: Orange ('a')
    Color(255, 247, 222, 143), // 1: Pale Yellow ('A')
    Color(255, 177, 165, 243), // 2: Violet ('b')
    Color(255, 220, 212, 252), // 3: Lavender ('B')
    Color(255, 247, 165, 167), // 4: Pink ('c')
    Color(255, 255, 210, 210), // 5: Pale Rose ('C')
    Color(255, 134, 232, 208), // 6: Aqua/Teal ('d')
    Color(255, 206, 251, 237), // 7: Pale Aqua ('D')
    Color(255, 134, 242, 128), // 8: Green ('e')
    Color(255, 215, 255, 215)  // 9: Pale Green ('E')
};

// Swatch grid mapping for StatusColorPanel 2x5 arrangement matching CellZoomPanel.java
static const int SWATCH_COLOR_MAP[10] = {
    0, 2, 4, 6, 8, // Row 0
    1, 3, 5, 7, 9  // Row 1
};

// 9 ColorKu distinct marble colors
static const Color COLORKU_PALETTE[10] = {
    Color(255, 200, 200, 200), // 0: None
    Color(255, 220, 38, 38),   // 1: Red
    Color(255, 249, 115, 22),  // 2: Orange
    Color(255, 234, 179, 8),   // 3: Yellow
    Color(255, 22, 163, 74),   // 4: Dark Green
    Color(255, 14, 165, 233),  // 5: Sky Blue
    Color(255, 37, 99, 235),   // 6: Royal Blue
    Color(255, 147, 51, 234),  // 7: Violet / Purple
    Color(255, 219, 39, 119),  // 8: Pink / Magenta
    Color(255, 248, 250, 252)  // 9: Pearl White
};

// Resource IDs
constexpr int IDI_APP_ICON = 101;

// Menu & Control IDs
enum MenuAndControlId {
    IDM_FILE_NEW = 2001,
    IDM_FILE_OPEN,
    IDM_FILE_SAVE,
    IDM_FILE_SAVE_AS,
    IDM_FILE_PRINT,
    IDM_FILE_COPY_GIVENS,
    IDM_FILE_COPY_PM,
    IDM_FILE_EXPORT_PNG,
    IDM_FILE_PASTE,
    IDM_FILE_RESET,
    IDM_FILE_CLEAR,
    IDM_FILE_EXIT,

    IDM_EDIT_UNDO,
    IDM_EDIT_REDO,
    IDM_EDIT_ADD_SAVEPOINT,
    IDM_EDIT_RESTORE_SAVEPOINT,
    IDM_EDIT_CLEAR_COLORS,
    IDM_SOLVER_FIND_BACKDOORS,

    IDM_VIEW_SUDOKU_ONLY,
    IDM_VIEW_COLORKU = 9301,
    IDM_VIEW_ACTIVE_CELL,
    IDM_VIEW_SUMMARY,
    IDM_VIEW_SOL_PATH,
    IDM_VIEW_ALL_STEPS,

    IDM_PUZZLE_SHOW_NEXT_STEP = 2030,
    IDM_PUZZLE_VAGUE_HINT = 2031,
    IDM_PUZZLE_CONCRETE_HINT = 2032,
    IDM_PUZZLE_EXECUTE_HINT = 2033,
    IDM_PUZZLE_SET_SINGLES = 2034,
    IDM_PUZZLE_SOLVE_DLX = 2035,

    IDM_HELP_MANUAL,
    IDM_HELP_TECHNIQUES,
    IDM_HELP_ABOUT,

    IDM_FILE_SET_GIVENS = 2020,
    IDM_OPTIONS_PREFERENCES = 9201,

    IDM_MODE_PLAYING = 9101,
    IDM_MODE_LEARNING = 9102,
    IDM_MODE_PRACTICING = 9103,
    IDM_MODE_CONFIG_TRAINING = 9104,
    IDM_MODE_CHECK_PROGRESS = 9105,

    // Toolbar Controls
    IDC_BTN_UNDO = 3001,
    IDC_BTN_REDO,
    IDC_BTN_NEW_PUZZLE,
    IDC_COMBO_LEVEL,
    IDC_BTN_RED_GREEN,
    IDC_BTN_FILTER_1,
    IDC_BTN_FILTER_2,
    IDC_BTN_FILTER_3,
    IDC_BTN_FILTER_4,
    IDC_BTN_FILTER_5,
    IDC_BTN_FILTER_6,
    IDC_BTN_FILTER_7,
    IDC_BTN_FILTER_8,
    IDC_BTN_FILTER_9,
    IDC_BTN_FILTER_BIVALUE,
    IDC_BTN_FILTER_CLEAR,
    IDC_BTN_HINT_VAGUE,
    IDC_BTN_HINT_CONCRETE,
    IDC_BTN_HINT_NEXT,
    IDC_BTN_HINT_EXECUTE,
    IDC_BTN_SINGLES,
    IDC_BTN_SOLVE,

    // Tabs & Right Panels
    IDC_TAB_CONTROL = 4001,
    IDC_LIST_STEPS = 4002,

    // Bottom Hint Box Controls
    IDC_HINT_EDIT = 5001,
    IDC_BTN_HINT_BOX_NEXT,
    IDC_BTN_HINT_BOX_EXECUTE,
    IDC_BTN_HINT_BOX_SOLVE_UP_TO,
    IDC_BTN_HINT_BOX_CANCEL,

    // Active Cell (CellZoomPanel) Controls
    IDC_ZOOM_TITLE = 6001,
    IDC_ZOOM_STATUS,
    IDC_ZOOM_CLEAR_BTN,
    IDC_ZOOM_SET_LABEL,
    IDC_ZOOM_SET_BASE = 6010,   // 6011..6019
    IDC_ZOOM_CAND_LABEL = 6020,
    IDC_ZOOM_CAND_BASE = 6021,  // 6021..6029
    IDC_ZOOM_COLOR_LABEL = 6030,
    IDC_ZOOM_COLOR_BASE = 6031, // 6031..6040 (0..9)
    IDC_ZOOM_DETAILS = 6050,
    IDC_ZOOM_CAND_STATUS = 6060,
    IDC_ZOOM_CAND_COLOR_BASE = 6061, // 6061..6070 (0..9)
    IDC_ZOOM_CAND_CLEAR_BTN = 6071,

    // Status Bar Color Palette Controls
    IDC_STATUS_COLOR_BASE = 6200, // 6200..6209 (Colors 0..9)
    IDC_STATUS_COLOR_RESET = 6210
};

constexpr int8_t COLOR_NONE = -1;

// Snapshot for Undo / Redo
struct StudioSnapshot {
    BoardState board;
    std::array<int8_t, TOTAL_CELLS> cellColors;
    std::array<std::array<int8_t, 9>, TOTAL_CELLS> candColors;
};

// Sample puzzles for each HoDoKu level
static const std::vector<std::pair<DifficultyLevel, std::string>> PUZZLE_LIBRARY = {
    {DifficultyLevel::Easy, "530070000600195000098000060800060003400803001700020006060000280000419005000080079"},
    {DifficultyLevel::Medium, "000000010400000000020000000000050407008000300001090000300400200050100000000806000"},
    {DifficultyLevel::Hard, "000000000000003085001020000000507000004000100090000000500000073002010000000040009"},
    {DifficultyLevel::Unfair, "000000012000000003002300400001800005060070800000009000008500000900040500470006000"},
    {DifficultyLevel::Extreme, "100007090030020008009600500005300900010080002600004000300000010040000007007000300"}
};

} // namespace hodoku::ui
