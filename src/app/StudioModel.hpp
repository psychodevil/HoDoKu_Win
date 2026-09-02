#pragma once

#include "AppTypes.hpp"
#include "../core/StepFinder.hpp"
#include "../core/SimpleTechniques.hpp"

namespace hodoku::ui {

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
        BoardState puz = m_generator.generate_puzzle(lvl, SymmetryType::Rotational180, 8);
        m_initialBoard = puz;
        m_board = m_initialBoard;
        m_hardestLevel = lvl;
        m_totalScore = 0;
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        m_cellColors.fill(0);
        for (auto& row : m_candidateColors) row.fill(0);
        m_activeCandidateColor = -1;
        m_undoStack.clear();
        m_redoStack.clear();
        recalculate_solution_path();
        recalculate_fas();
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
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        m_cellColors.fill(0);
        for (auto& row : m_candidateColors) row.fill(0);
        m_activeCandidateColor = -1;
        m_undoStack.clear();
        m_redoStack.clear();
        recalculate_solution_path();
        recalculate_fas();
    }

    void push_undo() {
        m_undoStack.push_back({m_board, m_cellColors, m_candidateColors});
        m_redoStack.clear();
    }

    bool can_undo() const { return !m_undoStack.empty(); }
    bool can_redo() const { return !m_redoStack.empty(); }

    void undo() {
        if (!can_undo()) return;
        m_redoStack.push_back({m_board, m_cellColors, m_candidateColors});
        auto snap = m_undoStack.back();
        m_undoStack.pop_back();
        m_board = snap.board;
        m_cellColors = snap.cellColors;
        m_candidateColors = snap.candColors;
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        recalculate_solution_path();
        recalculate_fas();
    }

    void redo() {
        if (!can_redo()) return;
        m_undoStack.push_back({m_board, m_cellColors, m_candidateColors});
        auto snap = m_redoStack.back();
        m_redoStack.pop_back();
        m_board = snap.board;
        m_cellColors = snap.cellColors;
        m_candidateColors = snap.candColors;
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        recalculate_solution_path();
        recalculate_fas();
    }

    void set_cell_digit(int cell, int digit) {
        if (cell < 0 || cell >= TOTAL_CELLS) return;
        if (m_board.is_given(cell)) return;

        push_undo();
        if (digit == 0) {
            std::string cur_s;
            for (int i = 0; i < TOTAL_CELLS; ++i) {
                if (i == cell) cur_s += '.';
                else {
                    uint8_t v = m_board.get_value(i);
                    cur_s += (v == 0) ? '.' : static_cast<char>('0' + v);
                }
            }
            m_board.from_string(cur_s);
        } else {
            m_board.set_value(cell, digit);
        }
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        recalculate_solution_path();
        recalculate_fas();
    }

    void toggle_cell_candidate(int cell, int digit) {
        if (cell < 0 || cell >= TOTAL_CELLS || digit < 1 || digit > 9) return;
        if (!m_board.is_unfilled(cell)) return;

        push_undo();
        if (m_board.has_candidate(cell, digit)) {
            m_board.remove_candidate(cell, digit);
        } else {
            m_board.add_candidate(cell, digit);
        }
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        recalculate_solution_path();
        recalculate_fas();
    }

    void set_cell_color(int cell, int colorIdx) {
        if (cell < 0 || cell >= TOTAL_CELLS) return;
        push_undo();
        if (m_cellColors[cell] == static_cast<uint8_t>(colorIdx)) {
            m_cellColors[cell] = 0; // Toggle off if clicked again
        } else {
            m_cellColors[cell] = static_cast<uint8_t>(colorIdx);
        }
    }

    void set_candidate_color(int cell, int digit, int colorIdx) {
        if (cell < 0 || cell >= TOTAL_CELLS || digit < 1 || digit > 9) return;
        push_undo();
        int dIdx = digit - 1;
        if (m_candidateColors[cell][dIdx] == static_cast<uint8_t>(colorIdx)) {
            m_candidateColors[cell][dIdx] = 0;
        } else {
            m_candidateColors[cell][dIdx] = static_cast<uint8_t>(colorIdx);
        }
    }

    uint8_t get_candidate_color(int cell, int digit) const {
        if (cell >= 0 && cell < TOTAL_CELLS && digit >= 1 && digit <= 9) {
            return m_candidateColors[cell][digit - 1];
        }
        return 0;
    }

    int get_active_candidate_color() const { return m_activeCandidateColor; }
    void set_active_candidate_color(int c) { m_activeCandidateColor = c; }

    void toggle_filter_digit(int digit) {
        if (m_activeFilterDigit == digit) {
            m_activeFilterDigit = 0;
        } else {
            m_activeFilterDigit = digit;
            m_filterBivalue = false;
        }
    }

    void toggle_bivalue_filter() {
        m_filterBivalue = !m_filterBivalue;
        if (m_filterBivalue) m_activeFilterDigit = 0;
    }

    void clear_filters() {
        m_activeFilterDigit = 0;
        m_filterBivalue = false;
    }

    void give_vague_hint() {
        if (m_solutionPath.empty()) recalculate_solution_path();
        if (!m_solutionPath.empty()) {
            m_selectedStep = m_solutionPath.front();
            m_hintLevel = HintLevel::Vague;
        }
    }

    void give_concrete_hint() {
        if (m_solutionPath.empty()) recalculate_solution_path();
        if (!m_solutionPath.empty()) {
            m_selectedStep = m_solutionPath.front();
            m_hintLevel = HintLevel::Concrete;
        }
    }

    void show_next_step() {
        give_concrete_hint();
    }

    void execute_hint() {
        if (!m_selectedStep) return;
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

    void cancel_hint() {
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
    }

    void set_all_singles() {
        push_undo();
        bool progress = true;
        while (progress) {
            progress = false;
            auto naked = SimpleTechniques::find_naked_singles(m_board);
            if (!naked.empty()) {
                for (const auto& a : naked.front().assignments) {
                    m_board.set_value(a.cell, a.digit);
                }
                progress = true;
                continue;
            }
            auto hidden = SimpleTechniques::find_hidden_singles(m_board);
            if (!hidden.empty()) {
                for (const auto& a : hidden.front().assignments) {
                    m_board.set_value(a.cell, a.digit);
                }
                progress = true;
                continue;
            }
        }
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        recalculate_solution_path();
        recalculate_fas();
    }

    void solve_dlx() {
        push_undo();
        auto sol = m_solver.solve_one(m_board);
        if (sol) {
            m_board = *sol;
        }
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        recalculate_solution_path();
        recalculate_fas();
    }

    void recalculate_solution_path() {
        m_solutionPath.clear();
        m_totalScore = 0;
        m_hardestLevel = DifficultyLevel::Easy;

        BoardState sim = m_board;
        while (sim.unfilled_count() > 0) {
            auto step = StepFinder::find_next_step(sim);
            if (!step) break;

            if (step->difficulty > m_hardestLevel) {
                m_hardestLevel = step->difficulty;
            }
            m_totalScore += step->score;
            m_solutionPath.push_back(*step);

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

    void select_step(const Step& s) {
        m_selectedStep = s;
        m_hintLevel = HintLevel::Concrete;
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

    void set_digit_at_selected(int digit) {
        set_cell_digit(m_selectedCell, digit);
    }

    void toggle_candidate_at_selected(int digit) {
        toggle_cell_candidate(m_selectedCell, digit);
    }

    void set_selected_cell_color(int colorIdx) {
        set_cell_color(m_selectedCell, colorIdx);
    }

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
        m_selectedCell = top ? cell_index(0, c) : cell_index(r, 0);
    }

    void move_to_end(bool bottom) {
        if (m_selectedCell == -1) m_selectedCell = 0;
        int r = cell_row(m_selectedCell);
        int c = cell_col(m_selectedCell);
        m_selectedCell = bottom ? cell_index(8, c) : cell_index(r, 8);
    }

    std::string export_givens_string() const {
        std::string s;
        s.reserve(TOTAL_CELLS);
        for (int i = 0; i < TOTAL_CELLS; ++i) {
            if (m_board.is_given(i)) {
                s += static_cast<char>('0' + m_board.get_value(i));
            } else {
                s += '.';
            }
        }
        return s;
    }

    std::string export_pm_grid() const {
        std::string s;
        for (int r = 0; r < 9; ++r) {
            for (int c = 0; c < 9; ++c) {
                int cell = cell_index(r, c);
                uint8_t v = m_board.get_value(cell);
                s += (v == 0) ? '.' : static_cast<char>('0' + v);
            }
            s += "\r\n";
        }
        return s;
    }

    void import_from_string(std::string_view puz_str) {
        push_undo();
        m_board.from_string(puz_str);
        m_initialBoard = m_board;
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        m_cellColors.fill(0);
        for (auto& row : m_candidateColors) row.fill(0);
        m_activeCandidateColor = -1;
        recalculate_solution_path();
        recalculate_fas();
    }

    void clear_all_colors() {
        push_undo();
        m_cellColors.fill(0);
        for (auto& row : m_candidateColors) row.fill(0);
        m_activeCandidateColor = -1;
    }

    struct Savepoint {
        std::string name;
        BoardState board;
        std::array<uint8_t, TOTAL_CELLS> cellColors{};
        std::array<std::array<uint8_t, 9>, TOTAL_CELLS> candidateColors{};
    };

    void add_savepoint(std::string name = "") {
        if (name.empty()) {
            name = "Bookmark " + std::to_string(m_savepoints.size() + 1);
        }
        m_savepoints.push_back({std::move(name), m_board, m_cellColors, m_candidateColors});
    }

    bool restore_savepoint(size_t index) {
        if (index >= m_savepoints.size()) return false;
        push_undo();
        const auto& sp = m_savepoints[index];
        m_board = sp.board;
        m_cellColors = sp.cellColors;
        m_candidateColors = sp.candidateColors;
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        recalculate_solution_path();
        recalculate_fas();
        return true;
    }

    const std::vector<Savepoint>& get_savepoints() const { return m_savepoints; }
    void clear_savepoints() { m_savepoints.clear(); }

    struct BackdoorCandidate {
        int cell;
        int digit;
    };

    std::vector<BackdoorCandidate> find_backdoors() const {
        std::vector<BackdoorCandidate> backdoors;
        if (m_board.is_solved()) return backdoors;

        DlxSolver solver;
        auto sol = solver.solve_one(m_board);
        if (!sol.has_value()) return backdoors;

        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            if (!m_board.is_unfilled(cell)) continue;
            int correct_digit = sol->get_value(cell);
            if (!m_board.has_candidate(cell, correct_digit)) continue;

            BoardState testBoard = m_board;
            testBoard.set_value(cell, correct_digit);

            bool progress = true;
            while (progress && !testBoard.is_solved()) {
                progress = false;
                auto ns = SimpleTechniques::find_naked_singles(testBoard);
                if (!ns.empty()) {
                    for (const auto& a : ns.front().assignments) {
                        testBoard.set_value(a.cell, a.digit);
                    }
                    progress = true;
                    continue;
                }
                auto hs = SimpleTechniques::find_hidden_singles(testBoard);
                if (!hs.empty()) {
                    for (const auto& a : hs.front().assignments) {
                        testBoard.set_value(a.cell, a.digit);
                    }
                    progress = true;
                    continue;
                }
            }

            if (testBoard.is_solved()) {
                backdoors.push_back({cell, correct_digit});
            }
        }
        return backdoors;
    }

    // Getters & Setters
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
    GameMode get_game_mode() const { return m_gameMode; }
    void set_game_mode(GameMode mode) { m_gameMode = mode; }
    bool is_colorku_mode() const { return m_colorKuMode; }
    void toggle_colorku_mode() { m_colorKuMode = !m_colorKuMode; }
    void set_colorku_mode(bool val) { m_colorKuMode = val; }

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
    SudokuGenerator m_generator;
    GameMode m_gameMode{GameMode::Playing};

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
    std::vector<Savepoint> m_savepoints;
    bool m_colorKuMode{false};
};

} // namespace hodoku::ui
