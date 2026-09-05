#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <map>
#include <bit>
#include "AppTypes.hpp"
#include "../core/StepFinder.hpp"
#include "../core/SimpleTechniques.hpp"

namespace hodoku::ui {

inline int filter_popcount(uint16_t mask) noexcept {
#if defined(__cpp_lib_bitops) && __cpp_lib_bitops >= 201907L
    return std::popcount(mask);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount(mask);
#else
    int c = 0;
    while (mask) { mask &= (mask - 1); ++c; }
    return c;
#endif
}

inline int filter_countr_zero(uint16_t mask) noexcept {
#if defined(__cpp_lib_bitops) && __cpp_lib_bitops >= 201907L
    return std::countr_zero(mask);
#elif defined(__GNUC__) || defined(__clang__)
    return mask == 0 ? 16 : __builtin_ctz(mask);
#else
    if (mask == 0) return 16;
    int c = 0;
    while ((mask & 1) == 0) { mask >>= 1; ++c; }
    return c;
#endif
}

class HoDoKuStudio {
public:
    HoDoKuStudio() {
        m_cellColors.fill(COLOR_NONE);
        for (auto& row : m_candidateColors) row.fill(COLOR_NONE);
        load_puzzle_by_level(DifficultyLevel::Easy);
        start_background_generator();
    }

    ~HoDoKuStudio() {
        stop_background_generator();
    }

    void start_background_generator() {
        m_bgRunning = true;
        m_bgWorker = std::thread([this]() {
            while (m_bgRunning) {
                DifficultyLevel targetLvl = DifficultyLevel::Easy;
                bool needGen = false;

                {
                    std::lock_guard<std::mutex> lock(m_bgMtx);
                    for (int l = 0; l <= 4; ++l) {
                        DifficultyLevel lvl = static_cast<DifficultyLevel>(l);
                        if (m_bgCache[lvl].size() < 2) {
                            targetLvl = lvl;
                            needGen = true;
                            break;
                        }
                    }
                }

                if (needGen && m_bgRunning) {
                    BoardState puz = m_generator.generate_puzzle(targetLvl, SymmetryType::Rotational180, 8);
                    {
                        std::lock_guard<std::mutex> lock(m_bgMtx);
                        m_bgCache[targetLvl].push(puz);
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                } else {
                    std::unique_lock<std::mutex> lock(m_bgMtx);
                    m_bgCv.wait_for(lock, std::chrono::seconds(2), [this]() {
                        return !m_bgRunning;
                    });
                }
            }
        });
    }

    void stop_background_generator() {
        m_bgRunning = false;
        m_bgCv.notify_all();
        if (m_bgWorker.joinable()) {
            m_bgWorker.join();
        }
    }

    void load_puzzle_by_level(DifficultyLevel level) {
        for (const auto& p : PUZZLE_LIBRARY) {
            if (p.first == level) {
                m_board.from_string(p.second);
                m_initialBoard = m_board;
                m_undoStack.clear();
                m_redoStack.clear();
                m_userLinks.clear();
                cancel_link_start();
                m_cellColors.fill(COLOR_NONE);
                for (auto& row : m_candidateColors) row.fill(COLOR_NONE);
                m_activeCandidateColor = -1;
                m_selectedCell = 0;
                m_selectedStep.reset();
                m_hintLevel = HintLevel::None;
                m_activeFilterDigit = 0;
                m_filterMask = 0;
                m_filterBivalue = false;
                recalculate_solution_path();
                recalculate_fas();
                update_solution();
                return;
            }
        }
    }

    void new_puzzle(int levelIndex) {
        DifficultyLevel lvl = static_cast<DifficultyLevel>(std::clamp(levelIndex, 0, 4));
        BoardState puz;

        if (m_gameMode == GameMode::Practicing && !m_trainingTechniques.empty()) {
            puz = m_generator.generate_training_puzzle(m_trainingTechniques, SymmetryType::Rotational180, 25);
        } else {
            bool fromCache = false;
            {
                std::lock_guard<std::mutex> lock(m_bgMtx);
                if (!m_bgCache[lvl].empty()) {
                    puz = m_bgCache[lvl].front();
                    m_bgCache[lvl].pop();
                    fromCache = true;
                }
            }
            if (fromCache) {
                m_bgCv.notify_one();
            } else {
                puz = m_generator.generate_puzzle(lvl, SymmetryType::Rotational180, 8);
            }
        }

        m_initialBoard = puz;
        m_board = m_initialBoard;
        m_hardestLevel = lvl;
        m_totalScore = 0;
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        m_cellColors.fill(COLOR_NONE);
        for (auto& row : m_candidateColors) row.fill(COLOR_NONE);
        m_activeCandidateColor = -1;
        m_undoStack.clear();
        m_redoStack.clear();
        m_userLinks.clear();
        cancel_link_start();
        stop_auto_play();
        clear_transition();
        recalculate_solution_path();
        recalculate_fas();
        update_solution();
    }

    void reset_puzzle() {
        stop_auto_play();
        clear_transition();
        push_undo();
        m_board = m_initialBoard;
        m_cellColors.fill(COLOR_NONE);
        for (auto& row : m_candidateColors) row.fill(COLOR_NONE);
        m_activeCandidateColor = -1;
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        recalculate_solution_path();
        recalculate_fas();
    }

    void clear_grid() {
        stop_auto_play();
        clear_transition();
        push_undo();
        m_board.clear();
        m_initialBoard.clear();
        m_selectedCell = 0;
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        m_cellColors.fill(COLOR_NONE);
        for (auto& row : m_candidateColors) row.fill(COLOR_NONE);
        m_activeCandidateColor = -1;
        m_undoStack.clear();
        m_redoStack.clear();
        m_userLinks.clear();
        cancel_link_start();
        recalculate_solution_path();
        recalculate_fas();
    }

    void push_undo() {
        m_undoStack.push_back({m_board, m_cellColors, m_candidateColors, m_userLinks});
        m_redoStack.clear();
    }

    bool can_undo() const { return !m_undoStack.empty(); }
    bool can_redo() const { return !m_redoStack.empty(); }

    void undo() {
        if (!can_undo()) return;
        clear_transition();
        m_redoStack.push_back({m_board, m_cellColors, m_candidateColors, m_userLinks});
        auto snap = m_undoStack.back();
        m_undoStack.pop_back();
        m_board = snap.board;
        m_cellColors = snap.cellColors;
        m_candidateColors = snap.candColors;
        m_userLinks = snap.userLinks;
        cancel_link_start();
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        recalculate_solution_path();
        recalculate_fas();
    }

    void redo() {
        if (!can_redo()) return;
        clear_transition();
        m_undoStack.push_back({m_board, m_cellColors, m_candidateColors, m_userLinks});
        auto snap = m_redoStack.back();
        m_redoStack.pop_back();
        m_board = snap.board;
        m_cellColors = snap.cellColors;
        m_candidateColors = snap.candColors;
        m_userLinks = snap.userLinks;
        cancel_link_start();
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        recalculate_solution_path();
        recalculate_fas();
    }

    void set_cell_digit(int cell, int digit) {
        if (cell < 0 || cell >= TOTAL_CELLS) return;
        if (m_board.is_given(cell)) return;

        push_undo();
        if (digit == 0 || m_board.get_value(cell) != 0) {
            std::string cur_s;
            for (int i = 0; i < TOTAL_CELLS; ++i) {
                if (i == cell) {
                    cur_s += (digit == 0) ? '.' : static_cast<char>('0' + digit);
                } else {
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
        if (colorIdx < 0 || colorIdx >= 10 || m_cellColors[cell] == static_cast<int8_t>(colorIdx)) {
            m_cellColors[cell] = COLOR_NONE; // Toggle off if clicked again
        } else {
            m_cellColors[cell] = static_cast<int8_t>(colorIdx);
        }
    }

    void set_candidate_color(int cell, int digit, int colorIdx) {
        if (cell < 0 || cell >= TOTAL_CELLS || digit < 1 || digit > 9) return;
        push_undo();
        int dIdx = digit - 1;
        if (colorIdx < 0 || colorIdx >= 10 || m_candidateColors[cell][dIdx] == static_cast<int8_t>(colorIdx)) {
            m_candidateColors[cell][dIdx] = COLOR_NONE;
        } else {
            m_candidateColors[cell][dIdx] = static_cast<int8_t>(colorIdx);
        }
    }

    int8_t get_candidate_color(int cell, int digit) const {
        if (cell >= 0 && cell < TOTAL_CELLS && digit >= 1 && digit <= 9) {
            return m_candidateColors[cell][digit - 1];
        }
        return COLOR_NONE;
    }

    int get_active_candidate_color() const { return m_activeCandidateColor; }
    void set_active_candidate_color(int c) { m_activeCandidateColor = c; }

    void toggle_filter_digit(int digit) {
        if (digit < 1 || digit > 9) return;
        if (m_filterMask == (1u << digit)) {
            m_filterMask = 0;
            m_activeFilterDigit = 0;
        } else {
            m_filterMask = (1u << digit);
            m_activeFilterDigit = digit;
            m_filterBivalue = false;
        }
    }

    void toggle_multi_filter_digit(int digit) {
        if (digit < 1 || digit > 9) return;
        m_filterMask ^= (1u << digit);
        m_filterBivalue = false;
        if (filter_popcount(m_filterMask) == 1) {
            m_activeFilterDigit = filter_countr_zero(m_filterMask);
        } else {
            m_activeFilterDigit = 0;
        }
    }

    void toggle_bivalue_filter() {
        m_filterBivalue = !m_filterBivalue;
        if (m_filterBivalue) {
            m_activeFilterDigit = 0;
            m_filterMask = 0;
        }
    }

    void clear_filters() {
        m_activeFilterDigit = 0;
        m_filterMask = 0;
        m_filterBivalue = false;
    }

    bool is_filter_active() const {
        return (m_filterMask != 0) || m_filterBivalue;
    }

    uint16_t get_filter_mask() const {
        return m_filterMask;
    }

    int get_single_filter_digit() const {
        if (filter_popcount(m_filterMask) == 1) {
            return filter_countr_zero(m_filterMask);
        }
        return m_activeFilterDigit;
    }

    void cycle_filter_digit(bool forward) {
        uint16_t presentMask = 0;
        for (int d = 1; d <= 9; ++d) {
            if (m_board.get_cells_with_candidate(d).any()) {
                presentMask |= (1u << d);
            }
        }
        if (presentMask == 0) return;

        int cur = get_single_filter_digit();
        if (cur <= 0) cur = forward ? 9 : 1;

        int next = cur;
        for (int step = 0; step < 9; ++step) {
            if (forward) {
                next++;
                if (next > 9) next = 1;
            } else {
                next--;
                if (next < 1) next = 9;
            }
            if (presentMask & (1u << next)) {
                toggle_filter_digit(next);
                return;
            }
        }
    }

    void jump_next_filtered_cell(int dr, int dc) {
        int cand = get_single_filter_digit();
        if (cand <= 0 || m_selectedCell < 0) return;

        int curR = cell_row(m_selectedCell);
        int curC = cell_col(m_selectedCell);

        if (dr > 0) { // Down
            for (int step = 1; step < 81; ++step) {
                int r = (curR + step) % 9;
                int c = (curC + (curR + step) / 9) % 9;
                int idx = cell_index(r, c);
                if (m_board.is_unfilled(idx) && m_board.has_candidate(idx, cand)) {
                    m_selectedCell = idx;
                    clear_multi_selection();
                    return;
                }
            }
        } else if (dr < 0) { // Up
            for (int step = 1; step < 81; ++step) {
                int r = (curR - step % 9 + 9) % 9;
                int c = (curC - (step / 9) + 9) % 9;
                int idx = cell_index(r, c);
                if (m_board.is_unfilled(idx) && m_board.has_candidate(idx, cand)) {
                    m_selectedCell = idx;
                    clear_multi_selection();
                    return;
                }
            }
        } else if (dc > 0) { // Right
            for (int i = m_selectedCell + 1; i < 81; ++i) {
                if (m_board.is_unfilled(i) && m_board.has_candidate(i, cand)) {
                    m_selectedCell = i;
                    clear_multi_selection();
                    return;
                }
            }
        } else if (dc < 0) { // Left
            for (int i = m_selectedCell - 1; i >= 0; --i) {
                if (m_board.is_unfilled(i) && m_board.has_candidate(i, cand)) {
                    m_selectedCell = i;
                    clear_multi_selection();
                    return;
                }
            }
        }
    }

    void toggle_filter_candidate_at_selected() {
        int cand = get_single_filter_digit();
        if (cand > 0) {
            toggle_candidate_at_selected(cand);
        }
    }

    bool is_hidden_single_in_house(int cell, int digit) const {
        if (!m_board.is_unfilled(cell) || !m_board.has_candidate(cell, digit)) return false;
        int r = cell_row(cell);
        int c = cell_col(cell);
        int b = cell_box(cell);

        int countR = 0;
        for (int cellIdx : GRID.row_cells[r]) {
            if (m_board.is_unfilled(cellIdx) && m_board.has_candidate(cellIdx, digit)) countR++;
        }
        if (countR == 1) return true;

        int countC = 0;
        for (int cellIdx : GRID.col_cells[c]) {
            if (m_board.is_unfilled(cellIdx) && m_board.has_candidate(cellIdx, digit)) countC++;
        }
        if (countC == 1) return true;

        int countB = 0;
        for (int cellIdx : GRID.box_cells[b]) {
            if (m_board.is_unfilled(cellIdx) && m_board.has_candidate(cellIdx, digit)) countB++;
        }
        return (countB == 1);
    }

    bool set_single_or_filtered_at_selected() {
        if (m_selectedCell < 0 || m_selectedCell >= TOTAL_CELLS) return false;
        if (!m_board.is_unfilled(m_selectedCell)) return false;

        int numCand = m_board.count_candidates(m_selectedCell);
        if (numCand == 1) {
            for (int d = 1; d <= 9; ++d) {
                if (m_board.has_candidate(m_selectedCell, d)) {
                    set_digit_at_selected(d);
                    return true;
                }
            }
        }

        int cand = get_single_filter_digit();
        if (cand > 0 && m_board.has_candidate(m_selectedCell, cand)) {
            set_digit_at_selected(cand);
            return true;
        }

        return false;
    }

    bool is_filter_excluded_mode() const { return m_filterExcluded; }
    void toggle_filter_mode() { m_filterExcluded = !m_filterExcluded; }
    void set_filter_excluded_mode(bool val) { m_filterExcluded = val; }

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

        m_lastTransition.placed_digits = m_selectedStep->assignments;
        m_lastTransition.eliminated_candidates = m_selectedStep->eliminations;
        m_lastTransition.technique_name = m_selectedStep->name;
        m_lastTransition.active = true;

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
        clear_transition();
    }

    // Auto-Play Solving Animation Controller (Plan 6.4)
    AutoPlayState get_auto_play_state() const noexcept { return m_autoPlayState; }
    bool is_auto_playing() const noexcept { return m_autoPlayState == AutoPlayState::Playing; }
    bool is_auto_play_paused() const noexcept { return m_autoPlayState == AutoPlayState::Paused; }
    int get_auto_play_delay() const noexcept { return m_autoPlayDelayMs; }
    void set_auto_play_delay(int ms) noexcept { m_autoPlayDelayMs = std::clamp(ms, 50, 5000); }

    bool start_auto_play(int delayMs = -1) {
        if (delayMs > 0) set_auto_play_delay(delayMs);
        if (m_board.is_solved()) {
            m_autoPlayState = AutoPlayState::Stopped;
            return false;
        }
        if (m_solutionPath.empty()) {
            recalculate_solution_path();
        }
        if (m_solutionPath.empty()) {
            m_autoPlayState = AutoPlayState::Stopped;
            return false;
        }

        m_autoPlayState = AutoPlayState::Playing;
        if (!m_selectedStep.has_value() || m_hintLevel == HintLevel::None) {
            give_concrete_hint();
        }
        return true;
    }

    void pause_auto_play() noexcept {
        if (m_autoPlayState == AutoPlayState::Playing) {
            m_autoPlayState = AutoPlayState::Paused;
        }
    }

    void resume_auto_play() noexcept {
        if (m_autoPlayState == AutoPlayState::Paused) {
            if (!m_board.is_solved() && !m_solutionPath.empty()) {
                m_autoPlayState = AutoPlayState::Playing;
            } else {
                m_autoPlayState = AutoPlayState::Stopped;
            }
        }
    }

    void stop_auto_play() noexcept {
        m_autoPlayState = AutoPlayState::Stopped;
    }

    bool toggle_auto_play() {
        if (m_autoPlayState == AutoPlayState::Playing) {
            pause_auto_play();
            return false;
        } else if (m_autoPlayState == AutoPlayState::Paused) {
            resume_auto_play();
            return true;
        } else {
            return start_auto_play();
        }
    }

    bool step_auto_play() {
        if (m_autoPlayState != AutoPlayState::Playing) return false;

        if (m_board.is_solved()) {
            m_autoPlayState = AutoPlayState::Stopped;
            m_selectedStep.reset();
            m_hintLevel = HintLevel::None;
            return false;
        }

        if (m_selectedStep.has_value()) {
            execute_hint();
        } else {
            if (m_solutionPath.empty()) recalculate_solution_path();
            if (!m_solutionPath.empty()) {
                give_concrete_hint();
                return true;
            }
        }

        if (m_board.is_solved() || m_solutionPath.empty()) {
            m_autoPlayState = AutoPlayState::Stopped;
            m_selectedStep.reset();
            m_hintLevel = HintLevel::None;
            return false;
        }

        give_concrete_hint();
        return true;
    }

    bool step_forward() {
        if (m_board.is_solved()) return false;

        if (m_selectedStep.has_value()) {
            execute_hint();
            if (!m_board.is_solved() && !m_solutionPath.empty()) {
                give_concrete_hint();
            }
            return true;
        }

        if (m_solutionPath.empty()) recalculate_solution_path();
        if (!m_solutionPath.empty()) {
            give_concrete_hint();
            return true;
        }
        return false;
    }

    bool step_backward() {
        if (can_undo()) {
            undo();
            if (m_solutionPath.empty()) recalculate_solution_path();
            if (!m_solutionPath.empty()) {
                give_concrete_hint();
            }
            return true;
        }
        return false;
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

    const BitSet81& get_selected_cells() const { return m_selectedCells; }
    bool is_cell_selected(int cell) const {
        if (m_selectedCells.empty()) return cell == m_selectedCell;
        return m_selectedCells.test(cell);
    }
    void clear_multi_selection() {
        m_selectedCells.clear();
        m_anchorCell = -1;
    }
    void add_to_selection(int cell) {
        if (cell >= 0 && cell < TOTAL_CELLS) m_selectedCells.set(cell);
    }
    void remove_from_selection(int cell) {
        if (cell >= 0 && cell < TOTAL_CELLS) m_selectedCells.reset(cell);
    }
    void toggle_in_selection(int cell) {
        if (cell >= 0 && cell < TOTAL_CELLS) {
            if (m_selectedCells.test(cell)) m_selectedCells.reset(cell);
            else m_selectedCells.set(cell);
        }
    }
    void select_region(int r1, int c1, int r2, int c2) {
        m_selectedCells.clear();
        int rStart = std::min(r1, r2);
        int rEnd   = std::max(r1, r2);
        int cStart = std::min(c1, c2);
        int cEnd   = std::max(c1, c2);
        for (int r = rStart; r <= rEnd; ++r) {
            for (int c = cStart; c <= cEnd; ++c) {
                m_selectedCells.set(cell_index(r, c));
            }
        }
    }
    void extend_selection_region(int dr, int dc) {
        if (m_anchorCell == -1) {
            m_anchorCell = m_selectedCell;
        }
        int r = cell_row(m_selectedCell);
        int c = cell_col(m_selectedCell);
        r = std::clamp(r + dr, 0, 8);
        c = std::clamp(c + dc, 0, 8);
        m_selectedCell = cell_index(r, c);
        select_region(cell_row(m_anchorCell), cell_col(m_anchorCell), r, c);
    }

    void set_digit_at_selected(int digit) {
        if (!m_selectedCells.empty()) {
            push_undo();
            m_selectedCells.for_each_cell([&](int cell) {
                if (digit == 0) {
                    if (!m_initialBoard.get_givens().test(cell)) {
                        m_board.set_value(cell, 0);
                    }
                } else {
                    if (!m_initialBoard.get_givens().test(cell)) {
                        m_board.set_value(cell, digit);
                    }
                }
            });
            m_selectedStep.reset();
            m_hintLevel = HintLevel::None;
            recalculate_solution_path();
            recalculate_fas();
        } else {
            set_cell_digit(m_selectedCell, digit);
        }
    }

    void toggle_candidate_at_selected(int digit) {
        if (!m_selectedCells.empty()) {
            push_undo();
            m_selectedCells.for_each_cell([&](int cell) {
                if (m_board.is_unfilled(cell) && !m_initialBoard.get_givens().test(cell)) {
                    if (m_board.has_candidate(cell, digit)) {
                        m_board.remove_candidate(cell, digit);
                    } else {
                        m_board.add_candidate(cell, digit);
                    }
                }
            });
            m_selectedStep.reset();
            m_hintLevel = HintLevel::None;
            recalculate_solution_path();
            recalculate_fas();
        } else {
            toggle_cell_candidate(m_selectedCell, digit);
        }
    }

    void set_selected_cell_color(int colorIdx) {
        if (!m_selectedCells.empty()) {
            push_undo();
            m_selectedCells.for_each_cell([&](int cell) {
                if (colorIdx < 0 || colorIdx >= 10 || m_cellColors[cell] == static_cast<int8_t>(colorIdx)) {
                    m_cellColors[cell] = COLOR_NONE;
                } else {
                    m_cellColors[cell] = static_cast<int8_t>(colorIdx);
                }
            });
        } else {
            set_cell_color(m_selectedCell, colorIdx);
        }
    }

    void move_selection(int dr, int dc) {
        clear_multi_selection();
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
        m_cellColors.fill(COLOR_NONE);
        for (auto& row : m_candidateColors) row.fill(COLOR_NONE);
        m_activeCandidateColor = -1;
        recalculate_solution_path();
        recalculate_fas();
        update_solution();
    }

    void update_solution() {
        m_solution = m_solver.solve_one(m_initialBoard);
    }

    MoveValidation validate_move(int cell, int digit) {
        if (digit == 0) return MoveValidation::Valid;
        if (cell < 0 || cell >= TOTAL_CELLS || digit < 1 || digit > 9) return MoveValidation::RuleViolation;

        // Check rule violation (house duplicate)
        for (int peer : GRID.peer_cells[cell]) {
            if (m_board.get_value(peer) == digit) {
                return MoveValidation::RuleViolation;
            }
        }

        // Check solution deviation
        if (!m_solution.has_value()) {
            update_solution();
        }
        if (m_solution.has_value()) {
            if (m_solution->get_value(cell) != digit) {
                return MoveValidation::SolutionDeviation;
            }
        }

        return MoveValidation::Valid;
    }

    std::vector<int> audit_progress() {
        std::vector<int> error_cells;
        if (!m_solution.has_value()) {
            update_solution();
        }
        if (!m_solution.has_value()) return error_cells;

        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            uint8_t val = m_board.get_value(cell);
            if (val != 0 && !m_initialBoard.get_givens().test(cell)) {
                if (val != m_solution->get_value(cell)) {
                    error_cells.push_back(cell);
                }
            }
        }
        return error_cells;
    }

    void clear_all_colors() {
        push_undo();
        m_cellColors.fill(COLOR_NONE);
        for (auto& row : m_candidateColors) row.fill(COLOR_NONE);
        m_activeCandidateColor = -1;
        m_activeColorIndex = -1;
    }

    struct StepTransition {
        std::vector<CandidateAssignment> placed_digits;
        std::vector<CandidateElimination> eliminated_candidates;
        std::string technique_name;
        bool active{false};

        void clear() noexcept {
            placed_digits.clear();
            eliminated_candidates.clear();
            technique_name.clear();
            active = false;
        }
    };

    const StepTransition& get_last_transition() const noexcept { return m_lastTransition; }
    bool has_transition() const noexcept { return m_lastTransition.active; }
    void clear_transition() noexcept { m_lastTransition.clear(); }

    bool is_recently_placed(int cell) const noexcept {
        if (!m_lastTransition.active) return false;
        for (const auto& a : m_lastTransition.placed_digits) {
            if (a.cell == cell) return true;
        }
        return false;
    }

    int get_recently_placed_digit(int cell) const noexcept {
        if (!m_lastTransition.active) return 0;
        for (const auto& a : m_lastTransition.placed_digits) {
            if (a.cell == cell) return a.digit;
        }
        return 0;
    }

    bool is_recently_eliminated(int cell, int digit) const noexcept {
        if (!m_lastTransition.active) return false;
        for (const auto& e : m_lastTransition.eliminated_candidates) {
            if (e.cell == cell && e.digit == digit) return true;
        }
        return false;
    }

    struct Savepoint {
        std::string name;
        BoardState board;
        std::array<int8_t, TOTAL_CELLS> cellColors{};
        std::array<std::array<int8_t, 9>, TOTAL_CELLS> candidateColors{};
        std::vector<ManualLink> userLinks{};
    };

    void add_savepoint(std::string name = "") {
        if (name.empty()) {
            name = "Bookmark " + std::to_string(m_savepoints.size() + 1);
        }
        m_savepoints.push_back({std::move(name), m_board, m_cellColors, m_candidateColors, m_userLinks});
    }

    bool restore_savepoint(size_t index) {
        if (index >= m_savepoints.size()) return false;
        push_undo();
        const auto& sp = m_savepoints[index];
        m_board = sp.board;
        m_cellColors = sp.cellColors;
        m_candidateColors = sp.candidateColors;
        m_userLinks = sp.userLinks;
        cancel_link_start();
        m_selectedStep.reset();
        m_hintLevel = HintLevel::None;
        recalculate_solution_path();
        recalculate_fas();
        return true;
    }

    const std::vector<Savepoint>& get_savepoints() const { return m_savepoints; }
    void clear_savepoints() { m_savepoints.clear(); }
    bool delete_savepoint(size_t index) {
        if (index >= m_savepoints.size()) return false;
        m_savepoints.erase(m_savepoints.begin() + index);
        return true;
    }

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
    int get_hovered_cell() const { return m_hoveredCell; }
    int get_hovered_candidate() const { return m_hoveredCandidate; }
    void set_hovered_cell(int cell, int candidate = 0) {
        m_hoveredCell = cell;
        m_hoveredCandidate = candidate;
    }
    void clear_hover() {
        m_hoveredCell = -1;
        m_hoveredCandidate = 0;
        m_hoveredStep.reset();
    }
    const std::optional<Step>& get_hovered_step() const { return m_hoveredStep; }
    void set_hovered_step(const std::optional<Step>& step) { m_hoveredStep = step; }

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

    int8_t get_cell_color(int cell) const {
        return (cell >= 0 && cell < TOTAL_CELLS) ? m_cellColors[cell] : COLOR_NONE;
    }

    int get_active_color_index() const { return m_activeColorIndex; }
    void set_active_color_index(int idx) { m_activeColorIndex = idx; }
    bool is_color_cells_mode() const { return m_colorCellsMode; }
    void set_color_cells_mode(bool val) { m_colorCellsMode = val; }
    void toggle_color_cells_mode() { m_colorCellsMode = !m_colorCellsMode; }

    const std::vector<TechniqueType>& get_training_techniques() const { return m_trainingTechniques; }
    void set_training_techniques(const std::vector<TechniqueType>& techs) { m_trainingTechniques = techs; }

    const std::wstring& get_current_file_path() const { return m_currentFilePath; }
    void set_current_file_path(std::wstring p) { m_currentFilePath = std::move(p); }

    const std::vector<Step>& get_solution_path() const { return m_solutionPath; }
    const std::vector<Step>& get_fas_steps() const { return m_fasSteps; }
    const BoardState& get_board() const { return m_board; }

    // Manual Link Creation and Management
    bool is_link_mode() const noexcept { return m_linkMode; }
    void set_link_mode(bool enable) {
        m_linkMode = enable;
        if (!enable) cancel_link_start();
    }
    void toggle_link_mode() {
        set_link_mode(!m_linkMode);
    }
    bool is_drawing_strong_link() const noexcept { return m_drawStrongLinks; }
    void set_drawing_strong_link(bool strong) noexcept { m_drawStrongLinks = strong; }
    void toggle_link_type() noexcept { m_drawStrongLinks = !m_drawStrongLinks; }

    bool has_link_start() const noexcept { return m_linkStartCell >= 0 && m_linkStartDigit >= 1; }
    int get_link_start_cell() const noexcept { return m_linkStartCell; }
    int get_link_start_digit() const noexcept { return m_linkStartDigit; }

    void start_link(int cell, int digit) noexcept {
        if (cell < 0 || cell >= TOTAL_CELLS || digit < 1 || digit > 9) return;
        m_linkStartCell = cell;
        m_linkStartDigit = digit;
    }

    void cancel_link_start() noexcept {
        m_linkStartCell = -1;
        m_linkStartDigit = -1;
    }

    bool finish_link(int to_cell, int to_digit) {
        if (!has_link_start()) return false;
        if (to_cell < 0 || to_cell >= TOTAL_CELLS || to_digit < 1 || to_digit > 9) {
            cancel_link_start();
            return false;
        }
        if (m_linkStartCell == to_cell && m_linkStartDigit == to_digit) {
            cancel_link_start();
            return false;
        }

        push_undo();

        int8_t linkCol = (m_activeColorIndex >= 0 && m_activeColorIndex < 10) ? static_cast<int8_t>(m_activeColorIndex) : COLOR_NONE;
        ManualLink newLink{m_linkStartCell, m_linkStartDigit, to_cell, to_digit, m_drawStrongLinks, linkCol};

        auto it = std::find_if(m_userLinks.begin(), m_userLinks.end(), [&](const ManualLink& l) {
            return (l.from_cell == m_linkStartCell && l.from_digit == m_linkStartDigit &&
                    l.to_cell == to_cell && l.to_digit == to_digit) ||
                   (l.from_cell == to_cell && l.from_digit == to_digit &&
                    l.to_cell == m_linkStartCell && l.to_digit == m_linkStartDigit);
        });

        if (it != m_userLinks.end()) {
            if (it->is_strong == m_drawStrongLinks && it->color_index == linkCol) {
                // Clicking identical link toggles it off
                m_userLinks.erase(it);
            } else {
                it->from_cell = m_linkStartCell;
                it->from_digit = m_linkStartDigit;
                it->to_cell = to_cell;
                it->to_digit = to_digit;
                it->is_strong = m_drawStrongLinks;
                it->color_index = linkCol;
            }
        } else {
            m_userLinks.push_back(newLink);
        }

        cancel_link_start();
        return true;
    }

    void handle_candidate_link_click(int cell, int digit) {
        if (!has_link_start()) {
            start_link(cell, digit);
        } else {
            finish_link(cell, digit);
        }
    }

    const std::vector<ManualLink>& get_user_links() const noexcept { return m_userLinks; }

    void add_user_link(const ManualLink& link) {
        auto it = std::find_if(m_userLinks.begin(), m_userLinks.end(), [&](const ManualLink& l) {
            return (l.from_cell == link.from_cell && l.from_digit == link.from_digit &&
                    l.to_cell == link.to_cell && l.to_digit == link.to_digit) ||
                   (l.from_cell == link.to_cell && l.from_digit == link.to_digit &&
                    l.to_cell == link.from_cell && l.to_digit == link.from_digit);
        });
        if (it != m_userLinks.end()) {
            *it = link;
        } else {
            m_userLinks.push_back(link);
        }
    }

    void set_user_links(const std::vector<ManualLink>& links) {
        m_userLinks = links;
    }

    bool set_link_color(int from_cell, int from_digit, int to_cell, int to_digit, int8_t color_index) {
        for (auto& l : m_userLinks) {
            if ((l.from_cell == from_cell && l.from_digit == from_digit &&
                 l.to_cell == to_cell && l.to_digit == to_digit) ||
                (l.from_cell == to_cell && l.from_digit == to_digit &&
                 l.to_cell == from_cell && l.to_digit == from_digit)) {
                push_undo();
                l.color_index = color_index;
                return true;
            }
        }
        return false;
    }

    void clear_user_links() {
        if (!m_userLinks.empty()) {
            push_undo();
            m_userLinks.clear();
        }
        cancel_link_start();
    }

private:
    BoardState m_board;
    BoardState m_initialBoard;
    DlxSolver m_solver;
    SudokuGenerator m_generator;
    GameMode m_gameMode{GameMode::Playing};

    std::vector<StudioSnapshot> m_undoStack;
    std::vector<StudioSnapshot> m_redoStack;
    std::vector<ManualLink> m_userLinks;
    bool m_linkMode{false};
    bool m_drawStrongLinks{true};
    int m_linkStartCell{-1};
    int m_linkStartDigit{-1};
    std::array<int8_t, TOTAL_CELLS> m_cellColors{};
    std::array<std::array<int8_t, 9>, TOTAL_CELLS> m_candidateColors{};
    int m_activeCandidateColor{-1};
    int m_activeColorIndex{-1};
    bool m_colorCellsMode{true};

    std::vector<Step> m_solutionPath;
    std::vector<Step> m_fasSteps;
    std::optional<Step> m_selectedStep;
    HintLevel m_hintLevel{HintLevel::None};

    int m_selectedCell{0};
    BitSet81 m_selectedCells;
    int m_anchorCell{-1};
    std::optional<BoardState> m_solution;
    int m_hoveredCell{-1};
    int m_hoveredCandidate{0};
    std::optional<Step> m_hoveredStep;

    int m_activeFilterDigit{0};
    uint16_t m_filterMask{0};
    bool m_filterBivalue{false};
    bool m_filterExcluded{false};

    int m_totalScore{0};
    DifficultyLevel m_hardestLevel{DifficultyLevel::Easy};
    std::vector<Savepoint> m_savepoints;
    bool m_colorKuMode{false};
    std::wstring m_currentFilePath;

    AutoPlayState m_autoPlayState{AutoPlayState::Stopped};
    int m_autoPlayDelayMs{750};
    StepTransition m_lastTransition;

    std::vector<TechniqueType> m_trainingTechniques;

    std::mutex m_bgMtx;
    std::condition_variable m_bgCv;
    std::atomic<bool> m_bgRunning{false};
    std::map<DifficultyLevel, std::queue<BoardState>> m_bgCache;
    std::thread m_bgWorker;
};

} // namespace hodoku::ui
