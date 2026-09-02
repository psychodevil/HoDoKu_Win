#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <memory>
#include <sstream>
#include <iomanip>
#include <fstream>

#include "../core/Types.hpp"
#include "../core/BitSet81.hpp"
#include "../core/GridConstants.hpp"
#include "../core/BoardState.hpp"
#include "../core/DlxSolver.hpp"
#include "../core/Step.hpp"
#include "../core/SimpleTechniques.hpp"
#include "../core/Subsets.hpp"
#include "../core/Fish.hpp"
#include "../core/Wings.hpp"
#include "../core/StepFinder.hpp"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")

using namespace Gdiplus;
using namespace hodoku::core;

// Sample puzzles for each HoDoKu level
static const std::vector<std::pair<DifficultyLevel, std::string>> PUZZLE_LIBRARY = {
    {DifficultyLevel::Easy, "530070000600195000098000060800060003400803001700020006060000280000419005000080079"},
    {DifficultyLevel::Medium, "000000010400000000020000000000050407008000300001090000300400200050100000000806000"},
    {DifficultyLevel::Hard, "000000000000003085001020000000507000004000100090000000500000073002010000000040009"},
    {DifficultyLevel::Unfair, "000000012000000003002300400001800005060070800000009000008500000900040500470006000"},
    {DifficultyLevel::Extreme, "100007090030020008009600500005300900010080002600004000300000010040000007007000300"}
};

// UI Modes matching HoDoKu
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

// Menu & Control IDs
enum MenuAndControlId {
    IDM_FILE_NEW = 2001,
    IDM_FILE_OPEN,
    IDM_FILE_SAVE,
    IDM_FILE_COPY_GIVENS,
    IDM_FILE_COPY_PM,
    IDM_FILE_PASTE,
    IDM_FILE_RESET,
    IDM_FILE_CLEAR,
    IDM_FILE_EXIT,

    IDM_EDIT_UNDO,
    IDM_EDIT_REDO,
    IDM_EDIT_CLEAR_COLORS,

    IDM_VIEW_SUDOKU_ONLY,
    IDM_VIEW_ACTIVE_CELL,
    IDM_VIEW_SUMMARY,
    IDM_VIEW_SOL_PATH,
    IDM_VIEW_ALL_STEPS,

    IDM_PUZZLE_VAGUE_HINT,
    IDM_PUZZLE_CONCRETE_HINT,
    IDM_PUZZLE_SHOW_NEXT_STEP,
    IDM_PUZZLE_EXECUTE_HINT,
    IDM_PUZZLE_SET_SINGLES,
    IDM_PUZZLE_SOLVE_DLX,

    IDM_HELP_MANUAL,
    IDM_HELP_TECHNIQUES,
    IDM_HELP_ABOUT,

    IDM_FILE_SET_GIVENS = 2020,
    IDM_OPTIONS_PREFERENCES = 9201,

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
    IDC_ZOOM_CAND_CLEAR_BTN = 6071
};

struct StudioSnapshot {
    BoardState board;
    std::array<uint8_t, TOTAL_CELLS> cellColors;
    std::array<std::array<uint8_t, 9>, TOTAL_CELLS> candColors;
};

class HoDoKuStudio {
public:
    HoDoKuStudio() {
        m_cellColors.fill(0);
        for (auto& row : m_candidateColors) row.fill(0);
        load_puzzle_by_level(DifficultyLevel::Easy);
    }

    void load_puzzle_by_level(DifficultyLevel level) {
        for (const auto& p : PUZZLE_LIBRARY) {
            if (p.first == level) {
                m_board.from_string(p.second);
                m_initialBoard = m_board;
                m_undoStack.clear();
                m_redoStack.clear();
                m_cellColors.fill(0);
                for (auto& row : m_candidateColors) row.fill(0);
                m_activeCandidateColor = -1;
                m_selectedCell = 0;
                m_selectedStep.reset();
                m_hintLevel = HintLevel::None;
                m_activeFilterDigit = 0;
                m_filterBivalue = false;
                recalculate_solution_path();
                recalculate_fas();
                return;
            }
        }
    }

    void new_puzzle(int levelIndex) {
        DifficultyLevel lvl = static_cast<DifficultyLevel>(std::clamp(levelIndex, 0, 4));
        load_puzzle_by_level(lvl);
    }

    void reset_puzzle() {
        push_undo();
        m_board = m_initialBoard;
        m_cellColors.fill(0);
        for (auto& row : m_candidateColors) row.fill(0);
        m_activeCandidateColor = -1;
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        recalculate_solution_path();
        recalculate_fas();
    }

    void clear_grid() {
        push_undo();
        m_board.clear();
        m_initialBoard.clear();
        m_selectedCell = 0;
        m_cellColors.fill(0);
        for (auto& row : m_candidateColors) row.fill(0);
        m_activeCandidateColor = -1;
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        m_solutionPath.clear();
        m_fasSteps.clear();
    }

    void push_undo() {
        m_undoStack.push_back({m_board, m_cellColors, m_candidateColors});
        m_redoStack.clear();
    }

    void undo() {
        if (!m_undoStack.empty()) {
            m_redoStack.push_back({m_board, m_cellColors, m_candidateColors});
            auto state = m_undoStack.back();
            m_undoStack.pop_back();
            m_board = state.board;
            m_cellColors = state.cellColors;
            m_candidateColors = state.candColors;
            m_selectedStep.reset();
            m_hintLevel = HintLevel::None;
            recalculate_solution_path();
            recalculate_fas();
        }
    }

    void redo() {
        if (!m_redoStack.empty()) {
            m_undoStack.push_back({m_board, m_cellColors, m_candidateColors});
            auto state = m_redoStack.back();
            m_redoStack.pop_back();
            m_board = state.board;
            m_cellColors = state.cellColors;
            m_candidateColors = state.candColors;
            m_selectedStep.reset();
            m_hintLevel = HintLevel::None;
            recalculate_solution_path();
            recalculate_fas();
        }
    }

    void set_candidate_color(int cell, int digit, int colorIdx) {
        if (cell < 0 || cell >= TOTAL_CELLS || digit < 1 || digit > 9) return;
        push_undo();
        m_candidateColors[cell][digit - 1] = static_cast<uint8_t>(colorIdx);
    }

    int get_candidate_color(int cell, int digit) const {
        if (cell < 0 || cell >= TOTAL_CELLS || digit < 1 || digit > 9) return 0;
        return m_candidateColors[cell][digit - 1];
    }

    void clear_candidate_colors_in_cell(int cell) {
        if (cell < 0 || cell >= TOTAL_CELLS) return;
        push_undo();
        m_candidateColors[cell].fill(0);
    }

    void set_active_candidate_color(int col) {
        m_activeCandidateColor = col;
    }

    int get_active_candidate_color() const {
        return m_activeCandidateColor;
    }

    void set_digit_at_cell(int cell, int digit) {
        if (cell < 0 || cell >= TOTAL_CELLS || digit < 1 || digit > 9) return;
        if (m_board.is_given(cell)) return;
        push_undo();
        m_board.set_value(cell, static_cast<uint8_t>(digit));
        recalculate_solution_path();
        recalculate_fas();
    }

    int hit_test_candidate(int x, int y, int cell) const {
        if (cell < 0 || cell >= TOTAL_CELLS || m_cellSize <= 0.0f) return 0;
        int r = cell_row(cell);
        int c = cell_col(cell);
        float cx = m_offsetX + c * m_cellSize;
        float cy = m_offsetY + r * m_cellSize;
        float relX = x - cx;
        float relY = y - cy;
        if (relX < 0.0f || relX >= m_cellSize || relY < 0.0f || relY >= m_cellSize) return 0;

        float sub = m_cellSize / 3.0f;
        int subC = static_cast<int>(relX / sub);
        int subR = static_cast<int>(relY / sub);
        if (subC >= 0 && subC < 3 && subR >= 0 && subR < 3) {
            int digit = subR * 3 + subC + 1;
            return digit;
        }
        return 0;
    }

    // Filter Operations
    void toggle_filter_digit(int d) {
        m_filterBivalue = false;
        if (m_activeFilterDigit == d) {
            m_activeFilterDigit = 0;
        } else {
            m_activeFilterDigit = d;
        }
    }

    void toggle_filter_bivalue() {
        m_activeFilterDigit = 0;
        m_filterBivalue = !m_filterBivalue;
    }

    void clear_filter() {
        m_activeFilterDigit = 0;
        m_filterBivalue = false;
    }

    void cycle_filter(int dir) {
        if (m_activeFilterDigit == 0) {
            m_activeFilterDigit = (dir > 0) ? 1 : 9;
        } else {
            m_activeFilterDigit += dir;
            if (m_activeFilterDigit > 9) m_activeFilterDigit = 1;
            if (m_activeFilterDigit < 1) m_activeFilterDigit = 9;
        }
        m_filterBivalue = false;
    }

    // Hint Operations (Chapter 2)
    void give_vague_hint() {
        auto step = StepFinder::find_next_step(m_board);
        if (step) {
            m_selectedStep = step;
            m_hintLevel = HintLevel::Vague;
        } else {
            m_selectedStep.reset();
            m_hintLevel = HintLevel::None;
        }
    }

    void give_concrete_hint() {
        auto step = StepFinder::find_next_step(m_board);
        if (step) {
            m_selectedStep = step;
            m_hintLevel = HintLevel::Concrete;
        } else {
            m_selectedStep.reset();
            m_hintLevel = HintLevel::None;
        }
    }

    void show_next_step() {
        give_concrete_hint();
    }

    void cancel_hint() {
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
    }

    void execute_hint() {
        if (!m_selectedStep) {
            give_concrete_hint();
        }
        if (m_selectedStep) {
            push_undo();
            for (const auto& a : m_selectedStep->assignments) {
                m_board.set_value(a.cell, a.digit);
            }
            for (const auto& e : m_selectedStep->eliminations) {
                m_board.remove_candidate(e.cell, e.digit);
            }
            m_selectedStep.reset();
            m_hintLevel = HintLevel::None;
            recalculate_solution_path();
            recalculate_fas();
        }
    }

    void set_all_singles() {
        push_undo();
        while (true) {
            auto ns = SimpleTechniques::find_naked_singles(m_board);
            if (!ns.empty()) {
                const auto& a = ns[0].assignments[0];
                m_board.set_value(a.cell, a.digit);
                continue;
            }
            auto hs = SimpleTechniques::find_hidden_singles(m_board);
            if (!hs.empty()) {
                const auto& a = hs[0].assignments[0];
                m_board.set_value(a.cell, a.digit);
                continue;
            }
            break;
        }
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        recalculate_solution_path();
        recalculate_fas();
    }

    void solve_dlx() {
        auto sol = m_solver.solve_one(m_board);
        if (sol) {
            push_undo();
            m_board = *sol;
            m_selectedStep.reset();
            m_hintLevel = HintLevel::None;
            m_solutionPath.clear();
            m_fasSteps.clear();
        }
    }

    // Cell & Candidate editing
    void set_digit_at_selected(int digit) {
        if (m_selectedCell >= 0 && !m_board.is_given(m_selectedCell)) {
            push_undo();
            m_board.set_value(m_selectedCell, digit);
            m_selectedStep.reset();
            m_hintLevel = HintLevel::None;
            recalculate_solution_path();
            recalculate_fas();
        }
    }

    void toggle_candidate_at_selected(int digit) {
        if (m_selectedCell >= 0 && m_board.is_unfilled(m_selectedCell)) {
            push_undo();
            if (m_board.has_candidate(m_selectedCell, digit)) {
                m_board.remove_candidate(m_selectedCell, digit);
            } else {
                m_board.add_candidate(m_selectedCell, digit);
            }
            m_selectedStep.reset();
            m_hintLevel = HintLevel::None;
            recalculate_solution_path();
            recalculate_fas();
        }
    }

    void clear_selected_cell() {
        if (m_selectedCell >= 0 && !m_board.is_given(m_selectedCell)) {
            push_undo();
            // Clear value: if filled, reset
            m_selectedStep.reset();
            m_hintLevel = HintLevel::None;
            recalculate_solution_path();
            recalculate_fas();
        }
    }

    // Cell Coloring (Chapter 2: Keys A..E and Palette)
    void set_cell_color(int cell, int colorIdx) {
        if (cell >= 0 && cell < TOTAL_CELLS) {
            if (m_cellColors[cell] == colorIdx) {
                m_cellColors[cell] = 0; // Toggle off if already that color
            } else {
                m_cellColors[cell] = colorIdx % 10;
            }
        }
    }

    void set_selected_cell_color(int colorIdx) {
        if (m_selectedCell >= 0) {
            set_cell_color(m_selectedCell, colorIdx);
        }
    }

    void clear_all_colors() {
        m_cellColors.fill(0);
    }

    // Navigation (Chapter 2)
    void move_selection(int dr, int dc) {
        if (m_selectedCell == -1) {
            m_selectedCell = 0;
            return;
        }
        int r = cell_row(m_selectedCell);
        int c = cell_col(m_selectedCell);
        r = (r + dr + 9) % 9;
        c = (c + dc + 9) % 9;
        m_selectedCell = cell_index(r, c);
    }

    void jump_next_unsolved_cell() {
        if (m_selectedCell == -1) m_selectedCell = 0;
        for (int i = 1; i <= TOTAL_CELLS; ++i) {
            int target = (m_selectedCell + i) % TOTAL_CELLS;
            if (m_board.is_unfilled(target)) {
                m_selectedCell = target;
                return;
            }
        }
    }

    void move_to_home(bool top) {
        if (m_selectedCell == -1) m_selectedCell = 0;
        int r = cell_row(m_selectedCell);
        int c = cell_col(m_selectedCell);
        if (top) {
            m_selectedCell = cell_index(0, c);
        } else {
            m_selectedCell = cell_index(r, 0);
        }
    }

    void move_to_end(bool bottom) {
        if (m_selectedCell == -1) m_selectedCell = 0;
        int r = cell_row(m_selectedCell);
        int c = cell_col(m_selectedCell);
        if (bottom) {
            m_selectedCell = cell_index(8, c);
        } else {
            m_selectedCell = cell_index(r, 8);
        }
    }

    void toggle_space_filter_candidate() {
        if (m_selectedCell >= 0 && m_activeFilterDigit >= 1 && m_activeFilterDigit <= 9) {
            toggle_candidate_at_selected(m_activeFilterDigit);
        }
    }

    void enter_filter_candidate() {
        if (m_selectedCell >= 0 && m_activeFilterDigit >= 1 && m_activeFilterDigit <= 9) {
            set_digit_at_selected(m_activeFilterDigit);
        }
    }

    // Clipboard Support (Chapter 2)
    std::string export_givens_string() const {
        return m_initialBoard.to_string();
    }

    std::string export_pm_grid() const {
        std::ostringstream oss;
        oss << ".---------------.--------------------.------------.\n";
        for (int r = 0; r < 9; ++r) {
            oss << "| ";
            for (int c = 0; c < 9; ++c) {
                int cell = cell_index(r, c);
                uint8_t val = m_board.get_value(cell);
                if (val != 0) {
                    oss << val << " ";
                } else {
                    CandidateMask mask = m_board.get_candidates(cell);
                    for (int d = 1; d <= 9; ++d) {
                        if (mask_has_digit(mask, d)) oss << d;
                    }
                    oss << " ";
                }
                if (c == 2 || c == 5) oss << "| ";
            }
            oss << "|\n";
            if (r == 2 || r == 5) {
                oss << ":---------------+--------------------+------------:\n";
            }
        }
        oss << "'---------------'--------------------'------------'\n";
        return oss.str();
    }

    bool import_from_string(std::string_view str) {
        push_undo();
        bool ok = m_board.from_string(str);
        if (ok) {
            m_initialBoard = m_board;
            m_selectedCell = 0;
            m_selectedStep.reset();
            m_hintLevel = HintLevel::None;
            recalculate_solution_path();
            recalculate_fas();
        }
        return ok;
    }

    // Solution calculation
    void recalculate_solution_path() {
        m_solutionPath.clear();
        m_totalScore = 0;
        m_hardestLevel = DifficultyLevel::Easy;

        BoardState sim = m_board;
        while (!sim.is_solved()) {
            auto step = StepFinder::find_next_step(sim);
            if (!step) break;

            m_solutionPath.push_back(*step);
            m_totalScore += step->score;
            if (static_cast<int>(step->difficulty) > static_cast<int>(m_hardestLevel)) {
                m_hardestLevel = step->difficulty;
            }

            for (const auto& a : step->assignments) {
                sim.set_value(a.cell, a.digit);
            }
            for (const auto& e : step->eliminations) {
                sim.remove_candidate(e.cell, e.digit);
            }
        }
    }

    void recalculate_fas() {
        m_fasSteps = StepFinder::find_all_steps(m_board);
    }

    void select_step_from_fas(size_t index) {
        if (index < m_fasSteps.size()) {
            m_selectedStep = m_fasSteps[index];
            m_hintLevel = HintLevel::Concrete;
        }
    }

    void select_step_from_path(size_t index) {
        if (index < m_solutionPath.size()) {
            m_selectedStep = m_solutionPath[index];
            m_hintLevel = HintLevel::Concrete;
        }
    }

    // Rendering Canvas (Sudoku Grid matching HoDoKu visual theme)
    void render_grid_canvas(Graphics& g, int x, int y, int width, int height) {
        int margin = 0; // Exactly matching screenshot (no coordinate margins)
        int availW = width - 4;
        int availH = height - 4;
        int size = std::min(availW, availH);
        if (size < 100) size = 100;

        m_gridSize = size;
        m_cellSize = static_cast<float>(size) / 9.0f;
        m_offsetX = x + (availW - size) / 2;
        m_offsetY = y + (availH - size) / 2;

        // Background
        SolidBrush bgCanvas(Color(255, 255, 255, 255));
        g.FillRectangle(&bgCanvas, m_offsetX, m_offsetY, m_gridSize, m_gridSize);

        // Cell Background Highlights
        SolidBrush primHintBrush(Color(255, 255, 242, 117));     // HoDoKu Yellow Hint (#fff275)
        SolidBrush secHintBrush(Color(255, 255, 211, 182));      // HoDoKu Peach Secondary (#ffd3b6)
        SolidBrush filterHitBrush(Color(255, 185, 255, 185));    // HoDoKu POSSIBLE_CELL_COLOR from Options.java
        SolidBrush bivalueHitBrush(Color(255, 220, 252, 231));   // Bi-value Green

        StringFormat centerFmt;
        centerFmt.SetAlignment(StringAlignmentCenter);
        centerFmt.SetLineAlignment(StringAlignmentCenter);

        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            int r = cell_row(cell);
            int c = cell_col(cell);
            float cx = m_offsetX + c * m_cellSize;
            float cy = m_offsetY + r * m_cellSize;

            // 1. User cell custom color from palette (if set)
            int userCol = m_cellColors[cell];
            if (userCol >= 0 && userCol < 10) {
                SolidBrush uBrush(HODOKU_PALETTE[userCol]);
                g.FillRectangle(&uBrush, cx, cy, m_cellSize, m_cellSize);
            }

            // 2. Overlays
            if (m_hintLevel == HintLevel::Concrete && m_selectedStep && m_selectedStep->primary_cells.test(cell)) {
                g.FillRectangle(&primHintBrush, cx, cy, m_cellSize, m_cellSize);
            } else if (m_hintLevel == HintLevel::Concrete && m_selectedStep && m_selectedStep->secondary_cells.test(cell)) {
                g.FillRectangle(&secHintBrush, cx, cy, m_cellSize, m_cellSize);
            } else if (m_activeFilterDigit > 0 && (m_board.get_value(cell) == m_activeFilterDigit || m_board.has_candidate(cell, m_activeFilterDigit))) {
                g.FillRectangle(&filterHitBrush, cx, cy, m_cellSize, m_cellSize);
            } else if (m_filterBivalue && m_board.is_unfilled(cell) && m_board.count_candidates(cell) == 2) {
                g.FillRectangle(&bivalueHitBrush, cx, cy, m_cellSize, m_cellSize);
            }
        }

        // Grid Lines
        Pen thinLine(Color(255, 190, 195, 200), 1.0f);
        Pen thickLine(Color(255, 0, 0, 0), 2.5f);

        for (int i = 0; i <= 9; ++i) {
            float pos = i * m_cellSize;
            if (i % 3 != 0) {
                g.DrawLine(&thinLine, m_offsetX + pos, static_cast<float>(m_offsetY), m_offsetX + pos, static_cast<float>(m_offsetY + m_gridSize));
                g.DrawLine(&thinLine, static_cast<float>(m_offsetX), m_offsetY + pos, static_cast<float>(m_offsetX + m_gridSize), m_offsetY + pos);
            }
        }

        for (int i = 0; i <= 9; i += 3) {
            float pos = i * m_cellSize;
            g.DrawLine(&thickLine, m_offsetX + pos, static_cast<float>(m_offsetY), m_offsetX + pos, static_cast<float>(m_offsetY + m_gridSize));
            g.DrawLine(&thickLine, static_cast<float>(m_offsetX), m_offsetY + pos, static_cast<float>(m_offsetX + m_gridSize), m_offsetY + pos);
        }

        // Focus Cursor Accent Border (Exact Yellow outline from screenshot)
        if (m_selectedCell >= 0) {
            int r = cell_row(m_selectedCell);
            int c = cell_col(m_selectedCell);
            Pen cursorPen(Color(255, 234, 179, 8), 2.5f); // Gold/Yellow border
            g.DrawRectangle(&cursorPen, m_offsetX + c * m_cellSize + 1.0f, m_offsetY + r * m_cellSize + 1.0f, m_cellSize - 2.0f, m_cellSize - 2.0f);
        }

        // Digits & Candidates
        FontFamily ff(L"Segoe UI");
        Font digitFont(&ff, m_cellSize * 0.58f, FontStyleBold, UnitPixel);
        Font candFont(&ff, m_cellSize * 0.22f, FontStyleRegular, UnitPixel);
        Font candBoldFont(&ff, m_cellSize * 0.22f, FontStyleBold, UnitPixel);

        SolidBrush givenBrush(Color(255, 0, 0, 0));             // Black Given
        SolidBrush userBrush(Color(255, 0, 34, 204));           // Bold Blue User Value (#0022cc)
        SolidBrush candNormalBrush(Color(255, 35, 35, 35));     // Crisp Dark Candidate
        SolidBrush candHighlightBrush(Color(255, 0, 0, 0));     // Filtered Candidate
        SolidBrush candElimBrush(Color(255, 220, 38, 38));      // Red Eliminated
        Pen elimStrikePen(Color(255, 220, 38, 38), 1.5f);

        float subCell = m_cellSize / 3.0f;

        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            int r = cell_row(cell);
            int c = cell_col(cell);
            float cx = m_offsetX + c * m_cellSize;
            float cy = m_offsetY + r * m_cellSize;

            uint8_t val = m_board.get_value(cell);
            if (val != 0) {
                RectF cellRect(cx, cy, m_cellSize, m_cellSize);
                std::wstring text = std::to_wstring(val);
                Brush* b = m_board.is_given(cell) ? static_cast<Brush*>(&givenBrush) : static_cast<Brush*>(&userBrush);
                g.DrawString(text.c_str(), -1, &digitFont, cellRect, &centerFmt, b);
            } else {
                CandidateMask mask = m_board.get_candidates(cell);
                for (int d = 1; d <= 9; ++d) {
                    if (mask_has_digit(mask, d)) {
                        int dr = (d - 1) / 3;
                        int dc = (d - 1) % 3;
                        float kx = cx + dc * subCell;
                        float ky = cy + dr * subCell;
                        RectF candRect(kx, ky, subCell, subCell);

                        // Candidate-level custom coloring background
                        int candCol = m_candidateColors[cell][d - 1];
                        if (candCol > 0 && candCol < 10) {
                            SolidBrush candBg(HODOKU_PALETTE[candCol]);
                            g.FillRectangle(&candBg, kx + 1.0f, ky + 1.0f, subCell - 2.0f, subCell - 2.0f);
                        }

                        bool isElim = false;
                        if (m_hintLevel == HintLevel::Concrete && m_selectedStep) {
                            for (const auto& elim : m_selectedStep->eliminations) {
                                if (elim.cell == cell && elim.digit == d) {
                                    isElim = true;
                                    break;
                                }
                            }
                        }

                        bool isFilterMatch = (m_activeFilterDigit == d);
                        Brush* cBrush = isElim ? static_cast<Brush*>(&candElimBrush)
                                      : isFilterMatch ? static_cast<Brush*>(&candHighlightBrush)
                                      : static_cast<Brush*>(&candNormalBrush);

                        Font* f = isFilterMatch ? &candBoldFont : &candFont;
                        std::wstring candStr = std::to_wstring(d);
                        g.DrawString(candStr.c_str(), -1, f, candRect, &centerFmt, cBrush);

                        if (isElim) {
                            g.DrawLine(&elimStrikePen, kx + 2, ky + 2, kx + subCell - 2, ky + subCell - 2);
                        }
                    }
                }
            }
        }
    }

    int hit_test_grid(int x, int y) const {
        if (x < m_offsetX || x >= m_offsetX + m_gridSize ||
            y < m_offsetY || y >= m_offsetY + m_gridSize || m_cellSize <= 0.0f) {
            return -1;
        }
        int c = static_cast<int>((x - m_offsetX) / m_cellSize);
        int r = static_cast<int>((y - m_offsetY) / m_cellSize);
        if (r >= 0 && r < 9 && c >= 0 && c < 9) {
            return cell_index(r, c);
        }
        return -1;
    }

    // Getters
    int get_selected_cell() const { return m_selectedCell; }
    void set_selected_cell(int c) { m_selectedCell = c; }
    int get_unfilled_count() const { return m_board.unfilled_count(); }
    DifficultyLevel get_hardest_level() const { return m_hardestLevel; }
    int get_total_score() const { return m_totalScore; }
    int get_givens_count() const { return m_initialBoard.get_givens().count(); }
    int get_active_filter() const { return m_activeFilterDigit; }
    bool is_bivalue_filter() const { return m_filterBivalue; }
    std::optional<Step> get_selected_step() const { return m_selectedStep; }
    HintLevel get_hint_level() const { return m_hintLevel; }

    int get_cell_color(int cell) const {
        return (cell >= 0 && cell < TOTAL_CELLS) ? m_cellColors[cell] : 0;
    }

    const std::vector<Step>& get_solution_path() const { return m_solutionPath; }
    const std::vector<Step>& get_fas_steps() const { return m_fasSteps; }
    const BoardState& get_board() const { return m_board; }

private:
    BoardState m_board;
    BoardState m_initialBoard;
    DlxSolver m_solver;

    std::vector<StudioSnapshot> m_undoStack;
    std::vector<StudioSnapshot> m_redoStack;
    std::array<uint8_t, TOTAL_CELLS> m_cellColors{};
    std::array<std::array<uint8_t, 9>, TOTAL_CELLS> m_candidateColors{};
    int m_activeCandidateColor{-1};

    std::vector<Step> m_solutionPath;
    std::vector<Step> m_fasSteps;
    std::optional<Step> m_selectedStep;
    HintLevel m_hintLevel{HintLevel::None};

    int m_selectedCell{0};
    int m_activeFilterDigit{0};
    bool m_filterBivalue{false};

    int m_totalScore{0};
    DifficultyLevel m_hardestLevel{DifficultyLevel::Easy};

    int m_gridSize{480};
    float m_cellSize{53.3f};
    int m_offsetX{20};
    int m_offsetY{20};
};

static std::unique_ptr<HoDoKuStudio> g_studio;
static HWND g_hwnd = NULL;
static HWND g_hTab = NULL;
static HWND g_hListView = NULL;
static HWND g_hLevelCombo = NULL;
static HWND g_hStatusBar = NULL;
static HWND g_hHintEdit = NULL;

static HWND g_hHintNextBtn = NULL;
static HWND g_hHintExecBtn = NULL;
static HWND g_hHintSolveUpBtn = NULL;
static HWND g_hHintCancelBtn = NULL;

// Active Cell (CellZoomPanel) Controls
static bool g_sudokuOnly = false;
static HWND g_hZoomTitle = NULL;
static HWND g_hZoomStatus = NULL;
static HWND g_hZoomClearBtn = NULL;
static HWND g_hZoomSetLabel = NULL;
static HWND g_hZoomSetBtns[9] = {NULL};
static HWND g_hZoomCandLabel = NULL;
static HWND g_hZoomCandBtns[9] = {NULL};
static HWND g_hZoomColorLabel = NULL;
static HWND g_hZoomColorBtns[10] = {NULL};
static HWND g_hZoomDetailsLabel = NULL;
static HWND g_hZoomCandStatus = NULL;
static HWND g_hZoomCandClearBtn = NULL;
static HWND g_hZoomCandColorBtns[10] = {NULL};

static TabView g_currentTab = TabView::SolutionPath;
static std::vector<HWND> g_toolbarButtons;

// Forward declarations
void SwitchTab(TabView tab);
void ToggleSudokuOnly();
void UpdateActiveCellPanel();
void ShowActiveCellControls(BOOL show);
void LayoutActiveCellControls(int x, int y, int w, int h);
void LayoutHoDoKuControls(HWND hwnd, int width, int height);
void PopulateListView();
void UpdateHintBoxText();
void UpdateStatusBarText();

// Clipboard Helpers
void SetClipboardText(const std::string& text) {
    if (!OpenClipboard(g_hwnd)) return;
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

std::string GetClipboardText() {
    std::string result;
    if (!OpenClipboard(g_hwnd)) return result;

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

void DoFileOpen(HWND hwnd) {
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
            if (g_studio && !content.empty()) {
                g_studio->import_from_string(content);
                if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel();
                else PopulateListView();
                UpdateHintBoxText();
                UpdateStatusBarText();
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
    }
}

void DoFileSave(HWND hwnd) {
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
        if (f.is_open() && g_studio) {
            f << g_studio->export_givens_string() << "\n";
        }
    }
}

static HWND g_hGivensDlg = NULL;
static HWND g_hGivensEdit = NULL;
static HWND g_hGivensStatus = NULL;

LRESULT CALLBACK SetGivensDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
            if (g_studio && !text.empty()) {
                g_studio->import_from_string(text);
                if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel();
                else PopulateListView();
                UpdateHintBoxText();
                UpdateStatusBarText();
                InvalidateRect(g_hwnd, NULL, FALSE);
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

void ShowSetGivensDialog(HWND hParent) {
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

    if (g_studio) {
        std::string curGivens = g_studio->export_givens_string();
        std::wstring wGivens(curGivens.begin(), curGivens.end());
        SetWindowTextW(g_hGivensEdit, wGivens.c_str());
    }

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

void ShowPreferencesDialog(HWND hParent) {
    MessageBoxW(hParent,
        L"HoDoKu Preferences:\n\n"
        L"• Show Candidates: Enabled\n"
        L"• Auto-Compute FAS (Find All Steps): Enabled\n"
        L"• Active Solver Mode: Complete HoDoKu Hierarchy\n"
        L"• Color Configuration: HoDoKu 10-Color Palette Active\n",
        L"Preferences - HoDoKu", MB_OK | MB_ICONINFORMATION);
}

void ShowActiveCellControls(BOOL show) {
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

void LayoutActiveCellControls(int x, int y, int w, int h) {
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

void UpdateActiveCellPanel() {
    if (!g_studio || !g_hZoomTitle) return;
    int cell = g_studio->get_selected_cell();
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

    bool isGiven = g_studio->get_board().is_given(cell);
    uint8_t val = g_studio->get_board().get_value(cell);
    CandidateMask mask = g_studio->get_board().get_candidates(cell);

    // 3. Set Value Buttons (1..9): Exactly matching screenshot!
    // Shows number only if allowed/candidate, otherwise blank white tile
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

    // 4. Toggle Candidate Buttons (1..9): Exactly matching screenshot!
    // Shows number if candidate is present, otherwise blank tile
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

void SwitchTab(TabView tab) {
    g_currentTab = tab;
    g_sudokuOnly = false;
    if (g_hTab) TabCtrl_SetCurSel(g_hTab, static_cast<int>(tab));

    RECT rc;
    GetClientRect(g_hwnd, &rc);
    LayoutHoDoKuControls(g_hwnd, rc.right - rc.left, rc.bottom - rc.top);

    if (g_currentTab == TabView::ActiveCell) {
        UpdateActiveCellPanel();
    } else {
        PopulateListView();
    }
    InvalidateRect(g_hwnd, NULL, FALSE);
}

void ToggleSudokuOnly() {
    g_sudokuOnly = !g_sudokuOnly;
    RECT rc;
    GetClientRect(g_hwnd, &rc);
    LayoutHoDoKuControls(g_hwnd, rc.right - rc.left, rc.bottom - rc.top);
    if (!g_sudokuOnly) {
        if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel();
        else PopulateListView();
    }
    InvalidateRect(g_hwnd, NULL, FALSE);
}

void PopulateListView() {
    if (!g_hListView || !g_studio) return;

    ListView_DeleteAllItems(g_hListView);

    // Reconfigure columns depending on tab
    while (ListView_DeleteColumn(g_hListView, 0));

    LVCOLUMNW lvc{};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;

    if (g_currentTab == TabView::SolutionPath) {
        lvc.cx = 36;  lvc.pszText = const_cast<wchar_t*>(L"#");         ListView_InsertColumn(g_hListView, 0, &lvc);
        lvc.cx = 220; lvc.pszText = const_cast<wchar_t*>(L"Technique"); ListView_InsertColumn(g_hListView, 1, &lvc);
        lvc.cx = 60;  lvc.pszText = const_cast<wchar_t*>(L"Score");     ListView_InsertColumn(g_hListView, 2, &lvc);

        const auto& path = g_studio->get_solution_path();
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

        const auto& steps = g_studio->get_fas_steps();
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

        const auto& path = g_studio->get_solution_path();
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

void UpdateHintBoxText() {
    if (!g_hHintEdit || !g_studio) return;

    auto step = g_studio->get_selected_step();
    auto hintLvl = g_studio->get_hint_level();

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
        SetWindowTextW(g_hHintEdit, L"Press [Next Hint (F12)] or [Vague Hint (Alt+F12)] to find logical deduction steps, or [Solve DLX] for instant solution.");
        EnableWindow(g_hHintExecBtn, FALSE);
        EnableWindow(g_hHintCancelBtn, FALSE);
    }
}

void UpdateStatusBarText() {
    if (!g_hStatusBar || !g_studio) return;

    std::wstring lvlName = L"Easy";
    switch (g_studio->get_hardest_level()) {
        case DifficultyLevel::Easy: lvlName = L"Easy"; break;
        case DifficultyLevel::Medium: lvlName = L"Medium"; break;
        case DifficultyLevel::Hard: lvlName = L"Hard"; break;
        case DifficultyLevel::Unfair: lvlName = L"Unfair"; break;
        case DifficultyLevel::Extreme: lvlName = L"Extreme"; break;
    }

    int score = g_studio->get_total_score();
    int givens = g_studio->get_givens_count();
    int freeCells = g_studio->get_unfilled_count();
    int progress = static_cast<int>((81 - freeCells) * 100.0f / 81.0f);

    std::wstring part1 = L" [Colors: 1-9, R=Clear]  |  Level: " + lvlName + L"  |  Score: " + std::to_wstring(score) + L"  |  Givens: " + std::to_wstring(givens);
    std::wstring part2 = L" Progress: " + std::to_wstring(progress) + L"% (" + std::to_wstring(freeCells) + L" free)  |  Mode: Playing";

    SendMessageW(g_hStatusBar, SB_SETTEXT, 0, (LPARAM)part1.c_str());
    SendMessageW(g_hStatusBar, SB_SETTEXT, 1, (LPARAM)part2.c_str());
}

void LayoutHoDoKuControls(HWND hwnd, int width, int height) {
    (void)hwnd;
    int toolbarH = 36;
    int statusBarH = 20;
    int hintBoxH = 86;

    // Full-width Bottom Hints Box (matches HoDoKu screenshot)
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

void CreateHoDoKuUI(HWND hwnd) {
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
    SendMessageW(g_hLevelCombo, CB_SETCURSEL, 2, 0); // Default to Hard as in screenshot
    tbX += 86;

    // Small Color Indicator Swatch next to level combo (matches screenshot)
    HWND hRgBtn = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                tbX, tbY + 2, 22, 22, hwnd, (HMENU)(INT_PTR)IDC_BTN_RED_GREEN, NULL, NULL);
    g_toolbarButtons.push_back(hRgBtn);
    tbX += 28;

    // Filter buttons (1 2 3 4 5 6 7 8 9 Xy) with BS_OWNERDRAW
    const wchar_t* filterLabels[10] = {L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"Xy"};
    for (int i = 0; i < 10; ++i) {
        int w = (i == 9) ? 32 : 24;
        int id = IDC_BTN_FILTER_1 + i;
        HWND btn = CreateWindowW(L"BUTTON", filterLabels[i], WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                 tbX, tbY, w, btnH, hwnd, (HMENU)(INT_PTR)id, NULL, NULL);
        tbX += w + 3;
        g_toolbarButtons.push_back(btn);
    }

    // 2. Right Tabbed Panel (Summary, Solution path, All possible steps, Active Cell)
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

    // 3. ListView for Steps / Solution Path / Summary / All Steps
    g_hListView = CreateWindowW(WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER,
                                0, 0, 100, 100, hwnd, (HMENU)(INT_PTR)IDC_LIST_STEPS, NULL, NULL);
    SendMessage(g_hListView, WM_SETFONT, (WPARAM)hFont, TRUE);
    ListView_SetExtendedListViewStyle(g_hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

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

    // Blue Header Banner
    g_hZoomTitle = CreateWindowW(L"STATIC", L"Active Cell", WS_CHILD | SS_CENTER,
                                 0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)IDC_ZOOM_TITLE, NULL, NULL);
    SendMessage(g_hZoomTitle, WM_SETFONT, (WPARAM)hBannerFont, TRUE);

    // Set Value Section
    g_hZoomSetLabel = CreateWindowW(L"STATIC", L"Set Value:", WS_CHILD | SS_CENTER,
                                    0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)IDC_ZOOM_SET_LABEL, NULL, NULL);
    SendMessage(g_hZoomSetLabel, WM_SETFONT, (WPARAM)hLabelFont, TRUE);

    for (int i = 0; i < 9; ++i) {
        g_hZoomSetBtns[i] = CreateWindowW(L"BUTTON", L"", WS_CHILD | BS_PUSHBUTTON,
                                          0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)(IDC_ZOOM_SET_BASE + 1 + i), NULL, NULL);
        SendMessage(g_hZoomSetBtns[i], WM_SETFONT, (WPARAM)hKeypadFont, TRUE);
    }

    // Toggle Candidates Section
    g_hZoomCandLabel = CreateWindowW(L"STATIC", L"Toggle Candidates:", WS_CHILD | SS_CENTER,
                                     0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)IDC_ZOOM_CAND_LABEL, NULL, NULL);
    SendMessage(g_hZoomCandLabel, WM_SETFONT, (WPARAM)hLabelFont, TRUE);

    for (int i = 0; i < 9; ++i) {
        g_hZoomCandBtns[i] = CreateWindowW(L"BUTTON", L"", WS_CHILD | BS_PUSHBUTTON,
                                           0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)(IDC_ZOOM_CAND_BASE + 1 + i), NULL, NULL);
        SendMessage(g_hZoomCandBtns[i], WM_SETFONT, (WPARAM)hKeypadFont, TRUE);
    }

    // Color Swatches Section
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

    // 5. Full-width Bottom Hint Panel Controls (Exact HoDoKu layout)
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

    SwitchTab(TabView::ActiveCell);
    UpdateHintBoxText();
    UpdateStatusBarText();
}

HMENU CreateHoDoKuMenuBar() {
    HMENU hMenuBar = CreateMenu();

    // File Menu
    HMENU hFile = CreatePopupMenu();
    AppendMenuW(hFile, MF_STRING, IDM_FILE_NEW, L"&New Random Sudoku\tCtrl+N");
    AppendMenuW(hFile, MF_STRING, IDM_FILE_OPEN, L"&Open...\tCtrl+O");
    AppendMenuW(hFile, MF_STRING, IDM_FILE_SAVE, L"&Save...\tCtrl+S");
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
    AppendMenuW(hEdit, MF_STRING, IDM_EDIT_CLEAR_COLORS, L"Clear All &Colors\tR");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hEdit, L"&Edit");

    // Mode Menu (Matching HoDoKu)
    HMENU hMode = CreatePopupMenu();
    AppendMenuW(hMode, MF_STRING, 9101, L"&Playing Mode\tF2");
    AppendMenuW(hMode, MF_STRING, 9102, L"&Learning Mode\tF3");
    AppendMenuW(hMode, MF_STRING, 9103, L"Prac&ticing Mode\tF4");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hMode, L"&Mode");

    // Options Menu (Matching HoDoKu)
    HMENU hOptions = CreatePopupMenu();
    AppendMenuW(hOptions, MF_STRING, 9201, L"&Preferences...\tCtrl+P");
    AppendMenuW(hOptions, MF_STRING, 9202, L"&Color Configuration...");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hOptions, L"&Options");

    // Puzzle / Hint Menu
    HMENU hPuzzle = CreatePopupMenu();
    AppendMenuW(hPuzzle, MF_STRING, IDM_PUZZLE_VAGUE_HINT, L"&Vague Hint\tAlt+F12");
    AppendMenuW(hPuzzle, MF_STRING, IDM_PUZZLE_CONCRETE_HINT, L"&Concrete Hint\tCtrl+F12");
    AppendMenuW(hPuzzle, MF_STRING, IDM_PUZZLE_SHOW_NEXT_STEP, L"&Show Next Step\tF12");
    AppendMenuW(hPuzzle, MF_STRING, IDM_PUZZLE_EXECUTE_HINT, L"&Execute Step\tCtrl+E");
    AppendMenuW(hPuzzle, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hPuzzle, MF_STRING, IDM_PUZZLE_SET_SINGLES, L"Set All &Singles\tF11");
    AppendMenuW(hPuzzle, MF_STRING, IDM_PUZZLE_SOLVE_DLX, L"&Solve DLX\tCtrl+Shift+S");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hPuzzle, L"&Puzzle");

    // View Menu
    HMENU hView = CreatePopupMenu();
    AppendMenuW(hView, MF_STRING, IDM_VIEW_SUDOKU_ONLY, L"&Sudoku Only\tCtrl+Shift+0");
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

// Global Keystroke Processing (Pre-Filter for 100% Reliable Shortcuts)
bool ProcessGlobalKeyShortcuts(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!g_studio) return false;
    if (msg != WM_KEYDOWN && msg != WM_SYSKEYDOWN) return false;

    bool isCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool isShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool isAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;

    // 1. Function Keys (F1..F12)
    if (wParam >= VK_F1 && wParam <= VK_F9) {
        int d = static_cast<int>(wParam - VK_F1 + 1);
        g_studio->toggle_filter_digit(d);
        for (HWND b : g_toolbarButtons) InvalidateRect(b, NULL, TRUE);
        InvalidateRect(g_hwnd, NULL, FALSE);
        return true;
    }
    if (wParam == VK_F11) {
        g_studio->set_all_singles();
        PopulateListView();
        UpdateHintBoxText();
        UpdateStatusBarText();
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
        UpdateHintBoxText();
        InvalidateRect(g_hwnd, NULL, FALSE);
        return true;
    }
    if (wParam == VK_F5) {
        SwitchTab(TabView::ActiveCell);
        return true;
    }
    if (wParam == VK_F6) {
        SwitchTab(TabView::Summary);
        return true;
    }
    if (wParam == VK_F7) {
        SwitchTab(TabView::SolutionPath);
        return true;
    }
    if (wParam == VK_F8) {
        SwitchTab(TabView::AllSteps);
        return true;
    }

    if (isCtrl && isShift) {
        if (wParam == '0') {
            ToggleSudokuOnly();
            return true;
        }
        if (wParam == '1') {
            SwitchTab(TabView::ActiveCell);
            return true;
        }
        if (wParam == '2') {
            SwitchTab(TabView::Summary);
            return true;
        }
        if (wParam == '3') {
            SwitchTab(TabView::SolutionPath);
            return true;
        }
        if (wParam == '4') {
            SwitchTab(TabView::AllSteps);
            return true;
        }
    }

    // 2. Control Shortcuts (Ctrl+Z, Ctrl+Y, Ctrl+N, Ctrl+C, Ctrl+V, Ctrl+G, Ctrl+R, Ctrl+E, Ctrl+H)
    if (isCtrl && !isAlt) {
        if (wParam == 'Z') {
            g_studio->undo();
            PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == 'Y') {
            g_studio->redo();
            PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == 'N') {
            int selLvl = SendMessageW(g_hLevelCombo, CB_GETCURSEL, 0, 0);
            g_studio->new_puzzle(selLvl);
            PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == 'R') {
            g_studio->reset_puzzle();
            PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == 'E') {
            g_studio->execute_hint();
            PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == 'H') {
            if (isShift) {
                g_studio->give_concrete_hint();
            } else {
                g_studio->give_vague_hint();
            }
            UpdateHintBoxText();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == 'O') {
            DoFileOpen(g_hwnd);
            return true;
        }
        if (wParam == 'S') {
            DoFileSave(g_hwnd);
            return true;
        }
        if (wParam == 'P') {
            ShowPreferencesDialog(g_hwnd);
            return true;
        }
        if (wParam == 'C') {
            SetClipboardText(g_studio->export_pm_grid());
            return true;
        }
        if (wParam == 'G') {
            ShowSetGivensDialog(g_hwnd);
            return true;
        }
        if (wParam == 'V') {
            std::string clip = GetClipboardText();
            if (!clip.empty()) {
                g_studio->import_from_string(clip);
                PopulateListView();
                UpdateHintBoxText();
                UpdateStatusBarText();
                InvalidateRect(g_hwnd, NULL, FALSE);
            }
            return true;
        }

        // Ctrl + 1..9: Toggle candidate in focused cell
        if (wParam >= '1' && wParam <= '9') {
            int d = static_cast<int>(wParam - '0');
            g_studio->toggle_candidate_at_selected(d);
            PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }

        // Ctrl + Arrows: Jump to next unsolved cell
        if (wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN) {
            g_studio->jump_next_unsolved_cell();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == VK_HOME) {
            g_studio->move_to_home(true);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == VK_END) {
            g_studio->move_to_end(true);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
    }

    // 3. Navigation (Arrows, Home, End)
    if (!isCtrl && !isAlt) {
        if (wParam == VK_LEFT) {
            g_studio->move_selection(0, -1);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == VK_RIGHT) {
            g_studio->move_selection(0, 1);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == VK_UP) {
            g_studio->move_selection(-1, 0);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == VK_DOWN) {
            g_studio->move_selection(1, 0);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == VK_HOME) {
            g_studio->move_to_home(false);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == VK_END) {
            g_studio->move_to_end(false);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }

        // 4. Value Entry (1..9 and Numpad 1..9)
        if (wParam >= '1' && wParam <= '9') {
            int d = static_cast<int>(wParam - '0');
            if (isShift) {
                g_studio->toggle_candidate_at_selected(d);
            } else {
                g_studio->set_digit_at_selected(d);
            }
            PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam >= VK_NUMPAD1 && wParam <= VK_NUMPAD9) {
            int d = static_cast<int>(wParam - VK_NUMPAD0);
            g_studio->set_digit_at_selected(d);
            PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }

        // Clear Value (0, Delete, Backspace)
        if (wParam == '0' || wParam == VK_NUMPAD0 || wParam == VK_DELETE || wParam == VK_BACK) {
            g_studio->clear_selected_cell();
            PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }

        // Filter Controls: Space toggles filtered candidate, Enter sets it, < and > cycles
        if (wParam == VK_SPACE) {
            g_studio->toggle_space_filter_candidate();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == VK_RETURN) {
            g_studio->enter_filter_candidate();
            PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == VK_OEM_COMMA || wParam == VK_OEM_PERIOD) {
            g_studio->cycle_filter((wParam == VK_OEM_PERIOD) ? 1 : -1);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }

        // Coloring Keys (A..E and R)
        if (wParam >= 'A' && wParam <= 'E') {
            int pairIdx = static_cast<int>(wParam - 'A' + 1);
            int colIdx = isShift ? (pairIdx + 4) : pairIdx;
            g_studio->set_selected_cell_color(colIdx);
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == 'R') {
            g_studio->clear_all_colors();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }

        // Hint Shortcut Keys (H, E, S)
        if (wParam == 'H') {
            g_studio->show_next_step();
            UpdateHintBoxText();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == 'E') {
            g_studio->execute_hint();
            PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
        if (wParam == 'S') {
            g_studio->solve_dlx();
            PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(g_hwnd, NULL, FALSE);
            return true;
        }
    }

    return false;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CreateHoDoKuUI(hwnd);
        return 0;
    }
    case WM_SIZE: {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        LayoutHoDoKuControls(hwnd, width, height);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        bool isShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool isCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        if (g_studio) {
            int cell = g_studio->hit_test_grid(x, y);
            if (cell != -1) {
                int candDigit = g_studio->hit_test_candidate(x, y, cell);
                if (g_studio->get_board().is_unfilled(cell) && candDigit >= 1 && candDigit <= 9 &&
                    g_studio->get_board().has_candidate(cell, candDigit)) {
                    int actCandCol = g_studio->get_active_candidate_color();
                    if ((isShift || isCtrl || actCandCol >= 0) && actCandCol >= 0) {
                        // Apply active candidate color
                        g_studio->set_candidate_color(cell, candDigit, actCandCol);
                    } else if (isShift || isCtrl) {
                        // Toggle candidate
                        g_studio->set_selected_cell(cell);
                        g_studio->toggle_candidate_at_selected(candDigit);
                    } else {
                        // Direct left-click on candidate pencilmark sets cell value!
                        g_studio->set_selected_cell(cell);
                        g_studio->set_digit_at_cell(cell, candDigit);
                    }
                } else {
                    g_studio->set_selected_cell(cell);
                }
                if (g_currentTab == TabView::ActiveCell) {
                    UpdateActiveCellPanel();
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return 0;
    }
    case WM_RBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if (g_studio) {
            int cell = g_studio->hit_test_grid(x, y);
            if (cell != -1) {
                g_studio->set_selected_cell(cell);
                int candDigit = g_studio->hit_test_candidate(x, y, cell);
                if (g_studio->get_board().is_unfilled(cell) && candDigit >= 1 && candDigit <= 9) {
                    // Direct right-click on candidate toggles candidate!
                    g_studio->toggle_candidate_at_selected(candDigit);
                }
                if (g_currentTab == TabView::ActiveCell) {
                    UpdateActiveCellPanel();
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return 0;
    }
    case WM_NOTIFY: {
        LPNMHDR pnm = (LPNMHDR)lParam;
        if (pnm->idFrom == IDC_TAB_CONTROL && pnm->code == TCN_SELCHANGE) {
            int curSel = TabCtrl_GetCurSel(g_hTab);
            if (curSel == 0) SwitchTab(TabView::Summary);
            else if (curSel == 1) SwitchTab(TabView::SolutionPath);
            else if (curSel == 2) SwitchTab(TabView::AllSteps);
            else if (curSel == 3) SwitchTab(TabView::ActiveCell);
        } else if (pnm->idFrom == IDC_LIST_STEPS && (pnm->code == NM_CLICK || pnm->code == NM_DBLCLK)) {
            LPNMITEMACTIVATE pItem = (LPNMITEMACTIVATE)lParam;
            if (pItem->iItem >= 0 && g_studio) {
                if (g_currentTab == TabView::SolutionPath) {
                    g_studio->select_step_from_path(pItem->iItem);
                } else if (g_currentTab == TabView::AllSteps) {
                    g_studio->select_step_from_fas(pItem->iItem);
                }
                if (pnm->code == NM_DBLCLK) {
                    g_studio->execute_hint();
                    PopulateListView();
                }
                UpdateHintBoxText();
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        break;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
        if (!pdis) break;

        // 1. Color Palette Buttons
        if (pdis->CtlID >= IDC_ZOOM_COLOR_BASE && pdis->CtlID <= IDC_ZOOM_COLOR_BASE + 9) {
            int slotIdx = pdis->CtlID - IDC_ZOOM_COLOR_BASE;
            int colIdx = SWATCH_COLOR_MAP[slotIdx];
            Color c = HODOKU_PALETTE[colIdx];
            HBRUSH hBrush = CreateSolidBrush(RGB(c.GetR(), c.GetG(), c.GetB()));
            FillRect(pdis->hDC, &pdis->rcItem, hBrush);
            DeleteObject(hBrush);

            HBRUSH hBorder = CreateSolidBrush(RGB(150, 150, 150));
            FrameRect(pdis->hDC, &pdis->rcItem, hBorder);
            DeleteObject(hBorder);
            return TRUE;
        }
        if (pdis->CtlID == IDC_ZOOM_CLEAR_BTN) {
            HBRUSH hBrush = CreateSolidBrush(RGB(245, 245, 245));
            FillRect(pdis->hDC, &pdis->rcItem, hBrush);
            DeleteObject(hBrush);

            HBRUSH hBorder = CreateSolidBrush(RGB(150, 150, 150));
            FrameRect(pdis->hDC, &pdis->rcItem, hBorder);
            DeleteObject(hBorder);

            SetBkMode(pdis->hDC, TRANSPARENT);
            SetTextColor(pdis->hDC, RGB(40, 40, 40));
            HFONT hFont = CreateFontW(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                     DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            HFONT hOld = (HFONT)SelectObject(pdis->hDC, hFont);
            DrawTextW(pdis->hDC, L"R", 1, &pdis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(pdis->hDC, hOld);
            DeleteObject(hFont);
            return TRUE;
        }
        if (pdis->CtlID == IDC_ZOOM_STATUS) {
            int cell = g_studio ? g_studio->get_selected_cell() : -1;
            int curCol = (cell >= 0 && g_studio) ? g_studio->get_cell_color(cell) : -1;
            HBRUSH hBrush;
            if (curCol >= 0 && curCol < 10) {
                Color c = HODOKU_PALETTE[curCol];
                hBrush = CreateSolidBrush(RGB(c.GetR(), c.GetG(), c.GetB()));
            } else {
                hBrush = CreateSolidBrush(RGB(255, 255, 255));
            }
            FillRect(pdis->hDC, &pdis->rcItem, hBrush);
            DeleteObject(hBrush);

            HBRUSH hBorder = CreateSolidBrush(RGB(80, 80, 80));
            FrameRect(pdis->hDC, &pdis->rcItem, hBorder);
            DeleteObject(hBorder);
            return TRUE;
        }

        // 1b. Candidate Color Palette Buttons
        if (pdis->CtlID >= IDC_ZOOM_CAND_COLOR_BASE && pdis->CtlID <= IDC_ZOOM_CAND_COLOR_BASE + 9) {
            int slotIdx = pdis->CtlID - IDC_ZOOM_CAND_COLOR_BASE;
            int colIdx = SWATCH_COLOR_MAP[slotIdx];
            Color c = HODOKU_PALETTE[colIdx];
            HBRUSH hBrush = CreateSolidBrush(RGB(c.GetR(), c.GetG(), c.GetB()));
            FillRect(pdis->hDC, &pdis->rcItem, hBrush);
            DeleteObject(hBrush);

            HBRUSH hBorder = CreateSolidBrush(RGB(150, 150, 150));
            FrameRect(pdis->hDC, &pdis->rcItem, hBorder);
            DeleteObject(hBorder);
            return TRUE;
        }
        if (pdis->CtlID == IDC_ZOOM_CAND_CLEAR_BTN) {
            HBRUSH hBrush = CreateSolidBrush(RGB(245, 245, 245));
            FillRect(pdis->hDC, &pdis->rcItem, hBrush);
            DeleteObject(hBrush);

            HBRUSH hBorder = CreateSolidBrush(RGB(150, 150, 150));
            FrameRect(pdis->hDC, &pdis->rcItem, hBorder);
            DeleteObject(hBorder);

            SetBkMode(pdis->hDC, TRANSPARENT);
            SetTextColor(pdis->hDC, RGB(40, 40, 40));
            HFONT hFont = CreateFontW(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                     DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            HFONT hOld = (HFONT)SelectObject(pdis->hDC, hFont);
            DrawTextW(pdis->hDC, L"R", 1, &pdis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(pdis->hDC, hOld);
            DeleteObject(hFont);
            return TRUE;
        }
        if (pdis->CtlID == IDC_ZOOM_CAND_STATUS) {
            int curCol = g_studio ? g_studio->get_active_candidate_color() : -1;
            HBRUSH hBrush;
            if (curCol >= 0 && curCol < 10) {
                Color c = HODOKU_PALETTE[curCol];
                hBrush = CreateSolidBrush(RGB(c.GetR(), c.GetG(), c.GetB()));
            } else {
                hBrush = CreateSolidBrush(RGB(255, 255, 255));
            }
            FillRect(pdis->hDC, &pdis->rcItem, hBrush);
            DeleteObject(hBrush);

            HBRUSH hBorder = CreateSolidBrush(RGB(80, 80, 80));
            FrameRect(pdis->hDC, &pdis->rcItem, hBorder);
            DeleteObject(hBorder);
            return TRUE;
        }

        // 2. Toolbar Red/Green Swatch Button
        if (pdis->CtlID == IDC_BTN_RED_GREEN) {
            HBRUSH hBrush = CreateSolidBrush(RGB(185, 255, 185)); // Soft green swatch matching screenshot
            FillRect(pdis->hDC, &pdis->rcItem, hBrush);
            DeleteObject(hBrush);
            HBRUSH hBorder = CreateSolidBrush(RGB(130, 180, 130));
            FrameRect(pdis->hDC, &pdis->rcItem, hBorder);
            DeleteObject(hBorder);
            return TRUE;
        }

        // 3. Candidate Filter Buttons (1..9, Xy)
        if (pdis->CtlID >= IDC_BTN_FILTER_1 && pdis->CtlID <= IDC_BTN_FILTER_CLEAR) {
            int filterDigit = pdis->CtlID - IDC_BTN_FILTER_1 + 1;
            bool isSelected = false;
            if (pdis->CtlID == IDC_BTN_FILTER_BIVALUE) {
                isSelected = g_studio ? g_studio->is_bivalue_filter() : false;
            } else if (filterDigit >= 1 && filterDigit <= 9) {
                isSelected = g_studio ? (g_studio->get_active_filter() == filterDigit) : false;
            }

            COLORREF bgCol = isSelected ? RGB(204, 232, 255) : RGB(245, 245, 245);
            COLORREF borderCol = isSelected ? RGB(0, 120, 215) : RGB(215, 215, 215);

            HBRUSH hBrush = CreateSolidBrush(bgCol);
            FillRect(pdis->hDC, &pdis->rcItem, hBrush);
            DeleteObject(hBrush);

            HBRUSH hBorder = CreateSolidBrush(borderCol);
            FrameRect(pdis->hDC, &pdis->rcItem, hBorder);
            DeleteObject(hBorder);

            wchar_t text[16] = {0};
            GetWindowTextW(pdis->hwndItem, text, 16);

            SetBkMode(pdis->hDC, TRANSPARENT);
            SetTextColor(pdis->hDC, isSelected ? RGB(0, 80, 180) : RGB(20, 20, 20));
            HFONT hFont = CreateFontW(14, 0, 0, 0, isSelected ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                     DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            HFONT hOld = (HFONT)SelectObject(pdis->hDC, hFont);
            DrawTextW(pdis->hDC, text, -1, &pdis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(pdis->hDC, hOld);
            DeleteObject(hFont);
            return TRUE;
        }
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (!g_studio) break;

        if (id >= IDC_BTN_FILTER_1 && id <= IDC_BTN_FILTER_9) {
            g_studio->toggle_filter_digit(id - IDC_BTN_FILTER_1 + 1);
            for (HWND b : g_toolbarButtons) InvalidateRect(b, NULL, TRUE);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_FILTER_BIVALUE) {
            g_studio->toggle_filter_bivalue();
            for (HWND b : g_toolbarButtons) InvalidateRect(b, NULL, TRUE);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_FILTER_CLEAR) {
            g_studio->clear_filter();
            for (HWND b : g_toolbarButtons) InvalidateRect(b, NULL, TRUE);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_UNDO || id == IDM_EDIT_UNDO) {
            g_studio->undo();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel();
            else PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_REDO || id == IDM_EDIT_REDO) {
            g_studio->redo();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel();
            else PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_NEW_PUZZLE || id == IDM_FILE_NEW) {
            int selLvl = SendMessageW(g_hLevelCombo, CB_GETCURSEL, 0, 0);
            g_studio->new_puzzle(selLvl);
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel();
            else PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_HINT_VAGUE || id == IDM_PUZZLE_VAGUE_HINT) {
            g_studio->give_vague_hint();
            UpdateHintBoxText();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_HINT_CONCRETE || id == IDM_PUZZLE_CONCRETE_HINT) {
            g_studio->give_concrete_hint();
            UpdateHintBoxText();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_HINT_NEXT || id == IDC_BTN_HINT_BOX_NEXT || id == IDM_PUZZLE_SHOW_NEXT_STEP) {
            g_studio->show_next_step();
            UpdateHintBoxText();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_HINT_EXECUTE || id == IDC_BTN_HINT_BOX_EXECUTE || id == IDM_PUZZLE_EXECUTE_HINT) {
            g_studio->execute_hint();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel();
            else PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_HINT_BOX_CANCEL) {
            g_studio->cancel_hint();
            UpdateHintBoxText();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_SINGLES || id == IDM_PUZZLE_SET_SINGLES || id == IDC_BTN_HINT_BOX_SOLVE_UP_TO) {
            g_studio->set_all_singles();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel();
            else PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_BTN_SOLVE || id == IDM_PUZZLE_SOLVE_DLX) {
            g_studio->solve_dlx();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel();
            else PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDM_FILE_RESET) {
            g_studio->reset_puzzle();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel();
            else PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDM_FILE_CLEAR) {
            g_studio->clear_grid();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel();
            else PopulateListView();
            UpdateHintBoxText();
            UpdateStatusBarText();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDM_FILE_OPEN) {
            DoFileOpen(hwnd);
        } else if (id == IDM_FILE_SAVE) {
            DoFileSave(hwnd);
        } else if (id == IDM_FILE_SET_GIVENS) {
            ShowSetGivensDialog(hwnd);
        } else if (id == 9201 || id == IDM_OPTIONS_PREFERENCES) {
            ShowPreferencesDialog(hwnd);
        } else if (id == IDM_FILE_COPY_GIVENS) {
            SetClipboardText(g_studio->export_givens_string());
        } else if (id == IDM_FILE_COPY_PM) {
            SetClipboardText(g_studio->export_pm_grid());
        } else if (id == IDM_FILE_PASTE) {
            std::string clip = GetClipboardText();
            if (!clip.empty()) {
                g_studio->import_from_string(clip);
                if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel();
                else PopulateListView();
                UpdateHintBoxText();
                UpdateStatusBarText();
                InvalidateRect(hwnd, NULL, FALSE);
            }
        } else if (id == IDM_EDIT_CLEAR_COLORS) {
            g_studio->clear_all_colors();
            if (g_currentTab == TabView::ActiveCell) UpdateActiveCellPanel();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDM_VIEW_SUDOKU_ONLY) {
            ToggleSudokuOnly();
        } else if (id == IDM_VIEW_ACTIVE_CELL) {
            SwitchTab(TabView::ActiveCell);
        } else if (id == IDM_VIEW_SUMMARY) {
            SwitchTab(TabView::Summary);
        } else if (id == IDM_VIEW_SOL_PATH) {
            SwitchTab(TabView::SolutionPath);
        } else if (id == IDM_VIEW_ALL_STEPS) {
            SwitchTab(TabView::AllSteps);
        } else if (id == IDC_ZOOM_CLEAR_BTN) {
            g_studio->clear_selected_cell();
            UpdateActiveCellPanel();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id >= IDC_ZOOM_SET_BASE + 1 && id <= IDC_ZOOM_SET_BASE + 9) {
            int digit = id - IDC_ZOOM_SET_BASE;
            g_studio->set_digit_at_selected(digit);
            UpdateActiveCellPanel();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id >= IDC_ZOOM_CAND_BASE + 1 && id <= IDC_ZOOM_CAND_BASE + 9) {
            int digit = id - IDC_ZOOM_CAND_BASE;
            g_studio->toggle_candidate_at_selected(digit);
            UpdateActiveCellPanel();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id >= IDC_ZOOM_COLOR_BASE && id <= IDC_ZOOM_COLOR_BASE + 9) {
            int slotIdx = id - IDC_ZOOM_COLOR_BASE;
            int colorIdx = SWATCH_COLOR_MAP[slotIdx];
            g_studio->set_selected_cell_color(colorIdx);
            UpdateActiveCellPanel();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id >= IDC_ZOOM_CAND_COLOR_BASE && id <= IDC_ZOOM_CAND_COLOR_BASE + 9) {
            int slotIdx = id - IDC_ZOOM_CAND_COLOR_BASE;
            int colorIdx = SWATCH_COLOR_MAP[slotIdx];
            g_studio->set_active_candidate_color(colorIdx);
            UpdateActiveCellPanel();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDC_ZOOM_CAND_CLEAR_BTN) {
            g_studio->set_active_candidate_color(-1);
            int cell = g_studio->get_selected_cell();
            if (cell >= 0) {
                g_studio->clear_candidate_colors_in_cell(cell);
            }
            UpdateActiveCellPanel();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == IDM_FILE_EXIT) {
            PostQuitMessage(0);
        } else if (id == IDM_HELP_ABOUT) {
            MessageBoxW(hwnd, L"HoDoKu Native - Modern C++20 Sudoku Engine\nFaithfully ported from HoDoKu specifications.\nZero WinRT / Zero OS dependencies in Engine Core.",
                        L"About HoDoKu Native", MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, w, h);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

        {
            Graphics graphics(memDC);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);
            graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

            // Window Background Fill (HoDoKu Slate/Light Gray)
            SolidBrush bgBrush(Color(255, 241, 245, 249));
            graphics.FillRectangle(&bgBrush, 0, 0, w, h);

            // Toolbar bottom divider
            Pen tbDivider(Color(255, 203, 213, 225), 1.0f);
            graphics.DrawLine(&tbDivider, 0.0f, 38.0f, static_cast<float>(w), 38.0f);

            int toolbarH = 36;
            int statusBarH = 20;
            int hintBoxH = 86;
            int hintBoxX = 6;
            int hintBoxW = w - 12;
            int hintBoxY = h - statusBarH - hintBoxH - 4;

            int middleY = toolbarH + 2;
            int middleH = hintBoxY - middleY - 4;
            int gridSize = middleH;

            if (g_studio) {
                // Draw 9x9 Grid (matching screenshot)
                g_studio->render_grid_canvas(graphics, 6, middleY, gridSize, gridSize);
            }

            // Draw full-width "Hints" Titled Border Box
            Pen boxPen(Color(255, 180, 185, 195), 1.0f);
            graphics.DrawRectangle(&boxPen, hintBoxX, hintBoxY, hintBoxW, hintBoxH);

            // Draw "Hints" Title Header
            SolidBrush boxHeaderBg(Color(255, 241, 245, 249));
            graphics.FillRectangle(&boxHeaderBg, hintBoxX + 10, hintBoxY - 6, 45, 12);

            FontFamily ff(L"Segoe UI");
            Font titleFont(&ff, 11.0f, FontStyleBold, UnitPixel);
            SolidBrush textBrush(Color(255, 51, 65, 85));
            graphics.DrawString(L"Hints", -1, &titleFont, PointF(static_cast<float>(hintBoxX + 14), static_cast<float>(hintBoxY - 7)), &textBrush);
        }

        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HWND hCtrl = (HWND)lParam;
        HDC hdcStatic = (HDC)wParam;
        if (hCtrl == g_hZoomTitle) {
            SetBkColor(hdcStatic, RGB(0, 114, 206)); // Solid HoDoKu Blue Banner
            SetTextColor(hdcStatic, RGB(255, 255, 255)); // White text
            static HBRUSH hBlueBrush = CreateSolidBrush(RGB(0, 114, 206));
            return (LRESULT)hBlueBrush;
        }
        SetBkColor(hdcStatic, RGB(241, 245, 249));
        SetTextColor(hdcStatic, RGB(30, 41, 59));
        static HBRUSH hDefaultBrush = CreateSolidBrush(RGB(241, 245, 249));
        return (LRESULT)hDefaultBrush;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int nCmdShow) {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    INITCOMMONCONTROLSEX icce{};
    icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icce.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icce);

    g_studio = std::make_unique<HoDoKuStudio>();
    g_studio->new_puzzle(2); // Hard level default matching screenshot
    g_studio->set_selected_cell(12); // r2c4 selected matching screenshot
    g_studio->toggle_filter_digit(9); // Candidate 9 filtered matching screenshot

    const wchar_t CLASS_NAME[] = L"HoDoKuStudioClass";
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassExW(&wc);

    HMENU hMenu = CreateHoDoKuMenuBar();

    g_hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"HoDoKu - v2.2.0",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1120, 780,
        NULL, hMenu, hInstance, NULL
    );

    if (!g_hwnd) {
        GdiplusShutdown(gdiplusToken);
        return 0;
    }

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    SwitchTab(TabView::ActiveCell);
    UpdateActiveCellPanel();

    // Global Message Pump with Centralized Keystroke Interceptor
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        // Intercept all keyboard shortcuts globally before dispatching to child controls
        if (ProcessGlobalKeyShortcuts(msg.message, msg.wParam, msg.lParam)) {
            continue; // Handled globally!
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    g_studio.reset();
    GdiplusShutdown(gdiplusToken);
    return static_cast<int>(msg.wParam);
}

int main() {
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWDEFAULT);
}
