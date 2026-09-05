#pragma once

#include <vector>
#include <array>
#include <random>
#include <algorithm>
#include <numeric>
#include <set>
#include <optional>
#include <chrono>
#include <functional>

#include "Types.hpp"
#include "BoardState.hpp"
#include "DlxSolver.hpp"
#include "StepFinder.hpp"

namespace hodoku::core {

enum class SymmetryType {
    None,          // Asymmetric
    Rotational180, // Centrosymmetric (standard HoDoKu default)
    Rotational90,  // 4-fold rotational
    Horizontal,    // Mirror over horizontal axis
    Vertical,      // Mirror over vertical axis
    Diagonal,      // Main diagonal
    AntiDiagonal   // Anti-diagonal
};

class SudokuGenerator {
public:
    SudokuGenerator()
        : m_rng(static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count())) {}

    explicit SudokuGenerator(unsigned int seed) : m_rng(seed) {}

    // Generates a random valid full 81-cell terminal grid
    BoardState generate_terminal_grid() {
        while (true) {
            BoardState board;

            // Fill independent diagonal boxes 0, 4, 8 with random permutations
            std::array<int, 9> digits{1, 2, 3, 4, 5, 6, 7, 8, 9};

            // Box 0 (top-left)
            std::shuffle(digits.begin(), digits.end(), m_rng);
            int idx = 0;
            for (int r = 0; r < 3; ++r) {
                for (int c = 0; c < 3; ++c) {
                    board.set_value(cell_index(r, c), static_cast<uint8_t>(digits[idx++]));
                }
            }

            // Box 4 (center)
            std::shuffle(digits.begin(), digits.end(), m_rng);
            idx = 0;
            for (int r = 3; r < 6; ++r) {
                for (int c = 3; c < 6; ++c) {
                    board.set_value(cell_index(r, c), static_cast<uint8_t>(digits[idx++]));
                }
            }

            // Box 8 (bottom-right)
            std::shuffle(digits.begin(), digits.end(), m_rng);
            idx = 0;
            for (int r = 6; r < 9; ++r) {
                for (int c = 6; c < 9; ++c) {
                    board.set_value(cell_index(r, c), static_cast<uint8_t>(digits[idx++]));
                }
            }

            DlxSolver dlx;
            auto sol = dlx.solve_one(board);
            if (sol.has_value()) {
                return *sol;
            }
        }
    }

    // Computes cell orbits under the given symmetry
    std::vector<std::vector<int>> compute_orbits(SymmetryType symmetry) {
        std::vector<std::vector<int>> orbits;
        std::array<bool, TOTAL_CELLS> visited{};
        visited.fill(false);

        for (int i = 0; i < TOTAL_CELLS; ++i) {
            if (visited[i]) continue;

            int r = cell_row(i);
            int c = cell_col(i);
            std::set<int> orbit_set;
            orbit_set.insert(i);

            switch (symmetry) {
            case SymmetryType::None:
                break;
            case SymmetryType::Rotational180: {
                int symm = cell_index(8 - r, 8 - c);
                orbit_set.insert(symm);
                break;
            }
            case SymmetryType::Rotational90: {
                orbit_set.insert(cell_index(c, 8 - r));
                orbit_set.insert(cell_index(8 - r, 8 - c));
                orbit_set.insert(cell_index(8 - c, r));
                break;
            }
            case SymmetryType::Horizontal: {
                int symm = cell_index(8 - r, c);
                orbit_set.insert(symm);
                break;
            }
            case SymmetryType::Vertical: {
                int symm = cell_index(r, 8 - c);
                orbit_set.insert(symm);
                break;
            }
            case SymmetryType::Diagonal: {
                int symm = cell_index(c, r);
                orbit_set.insert(symm);
                break;
            }
            case SymmetryType::AntiDiagonal: {
                int symm = cell_index(8 - c, 8 - r);
                orbit_set.insert(symm);
                break;
            }
            }

            std::vector<int> orbit_vec;
            for (int cell : orbit_set) {
                visited[cell] = true;
                orbit_vec.push_back(cell);
            }
            orbits.push_back(std::move(orbit_vec));
        }

        return orbits;
    }

    // Digs clues from a full grid respecting symmetry to produce a unique-solution puzzle
    BoardState dig_puzzle(const BoardState& full_board, SymmetryType symmetry) {
        std::array<uint8_t, TOTAL_CELLS> values;
        for (int i = 0; i < TOTAL_CELLS; ++i) {
            values[i] = full_board.get_value(i);
        }

        auto orbits = compute_orbits(symmetry);
        std::shuffle(orbits.begin(), orbits.end(), m_rng);

        DlxSolver dlx;

        auto make_board = [&]() {
            std::string s;
            s.reserve(TOTAL_CELLS);
            for (int i = 0; i < TOTAL_CELLS; ++i) {
                s += (values[i] == 0) ? '.' : static_cast<char>('0' + values[i]);
            }
            return BoardState(s);
        };

        for (const auto& orbit : orbits) {
            std::vector<std::pair<int, uint8_t>> saved;
            for (int cell : orbit) {
                saved.push_back({cell, values[cell]});
                values[cell] = 0;
            }

            BoardState test_board = make_board();
            // Uniqueness check: must have exactly 1 solution
            if (dlx.count_solutions(test_board, 2) != 1) {
                // Not unique, restore orbit
                for (const auto& p : saved) {
                    values[p.first] = p.second;
                }
            }
        }

        return make_board();
    }

    // Evaluates a puzzle's difficulty level using StepFinder
    DifficultyLevel evaluate_difficulty(const BoardState& puzzle, int& out_score) {
        BoardState sim = puzzle;
        DifficultyLevel hardest = DifficultyLevel::Easy;
        out_score = 0;

        while (sim.unfilled_count() > 0) {
            auto step = StepFinder::find_next_step(sim);
            if (!step) break;

            if (step->difficulty > hardest) {
                hardest = step->difficulty;
            }
            out_score += step->score;

            for (const auto& a : step->assignments) {
                sim.set_value(a.cell, a.digit);
            }
            for (const auto& e : step->eliminations) {
                sim.remove_candidate(e.cell, e.digit);
            }
        }

        return hardest;
    }

    // Generates a puzzle matching target difficulty level
    BoardState generate_puzzle(DifficultyLevel target_level, SymmetryType symmetry = SymmetryType::Rotational180, int max_attempts = 15) {
        BoardState best_puzzle;
        DifficultyLevel best_level = DifficultyLevel::Easy;

        for (int attempt = 0; attempt < max_attempts; ++attempt) {
            BoardState full = generate_terminal_grid();
            BoardState puzzle = dig_puzzle(full, symmetry);

            int score = 0;
            DifficultyLevel lvl = evaluate_difficulty(puzzle, score);

            if (lvl == target_level) {
                return puzzle;
            }

            // If target is Easy and lvl is Easy, or if target is Extreme and lvl >= Unfair
            if (target_level == DifficultyLevel::Easy && lvl == DifficultyLevel::Easy) {
                return puzzle;
            }
            if (target_level == DifficultyLevel::Extreme && lvl >= DifficultyLevel::Unfair) {
                return puzzle;
            }

            if (attempt == 0 || std::abs(static_cast<int>(lvl) - static_cast<int>(target_level)) <
                                std::abs(static_cast<int>(best_level) - static_cast<int>(target_level))) {
                best_puzzle = puzzle;
                best_level = lvl;
            }
        }

        return best_puzzle;
    }

    BoardState generate_training_puzzle(const std::vector<TechniqueType>& target_techniques, SymmetryType symmetry = SymmetryType::Rotational180, int max_attempts = 30) {
        if (target_techniques.empty()) {
            return generate_puzzle(DifficultyLevel::Medium, symmetry, max_attempts);
        }

        for (int attempt = 0; attempt < max_attempts; ++attempt) {
            BoardState full = generate_terminal_grid();
            BoardState puzzle = dig_puzzle(full, symmetry);

            BoardState sim = puzzle;
            bool found_target = false;
            while (sim.unfilled_count() > 0) {
                auto step = StepFinder::find_next_step(sim);
                if (!step) break;

                for (auto t : target_techniques) {
                    if (step->type == t) {
                        found_target = true;
                        break;
                    }
                }
                if (found_target) break;

                for (const auto& a : step->assignments) sim.set_value(a.cell, a.digit);
                for (const auto& e : step->eliminations) sim.remove_candidate(e.cell, e.digit);
            }

            if (found_target) {
                return puzzle;
            }
        }

        return generate_puzzle(DifficultyLevel::Hard, symmetry, max_attempts);
    }

    // Returns all cells related to 'cell' under the specified symmetry
    static std::vector<int> get_symmetric_cells(int cell, SymmetryType symmetry) {
        int r = cell_row(cell);
        int c = cell_col(cell);
        std::set<int> orbit_set;
        orbit_set.insert(cell);

        switch (symmetry) {
        case SymmetryType::None:
            break;
        case SymmetryType::Rotational180: {
            orbit_set.insert(cell_index(8 - r, 8 - c));
            break;
        }
        case SymmetryType::Rotational90: {
            orbit_set.insert(cell_index(c, 8 - r));
            orbit_set.insert(cell_index(8 - r, 8 - c));
            orbit_set.insert(cell_index(8 - c, r));
            break;
        }
        case SymmetryType::Horizontal: {
            orbit_set.insert(cell_index(8 - r, c));
            break;
        }
        case SymmetryType::Vertical: {
            orbit_set.insert(cell_index(r, 8 - c));
            break;
        }
        case SymmetryType::Diagonal: {
            orbit_set.insert(cell_index(c, r));
            break;
        }
        case SymmetryType::AntiDiagonal: {
            orbit_set.insert(cell_index(8 - c, 8 - r));
            break;
        }
        }

        return std::vector<int>(orbit_set.begin(), orbit_set.end());
    }

    // Preset pattern masks for generator designer
    static BitSet81 make_preset_diamond() {
        BitSet81 mask;
        const int diamond_cells[] = {
            cell_index(0, 4),
            cell_index(1, 3), cell_index(1, 4), cell_index(1, 5),
            cell_index(2, 2), cell_index(2, 4), cell_index(2, 6),
            cell_index(3, 1), cell_index(3, 4), cell_index(3, 7),
            cell_index(4, 0), cell_index(4, 2), cell_index(4, 4), cell_index(4, 6), cell_index(4, 8),
            cell_index(5, 1), cell_index(5, 4), cell_index(5, 7),
            cell_index(6, 2), cell_index(6, 4), cell_index(6, 6),
            cell_index(7, 3), cell_index(7, 4), cell_index(7, 5),
            cell_index(8, 4)
        };
        for (int c : diamond_cells) mask.set(c);
        return mask;
    }

    static BitSet81 make_preset_cross() {
        BitSet81 mask;
        for (int i = 0; i < 9; ++i) {
            mask.set(cell_index(4, i)); // Middle row
            mask.set(cell_index(i, 4)); // Middle column
        }
        mask.set(cell_index(1, 1)); mask.set(cell_index(1, 7));
        mask.set(cell_index(7, 1)); mask.set(cell_index(7, 7));
        mask.set(cell_index(0, 0)); mask.set(cell_index(0, 8));
        mask.set(cell_index(8, 0)); mask.set(cell_index(8, 8));
        return mask;
    }

    static BitSet81 make_preset_picture_frame() {
        BitSet81 mask;
        for (int i = 1; i <= 7; ++i) {
            if (i == 4) continue;
            mask.set(cell_index(0, i));
            mask.set(cell_index(8, i));
            mask.set(cell_index(i, 0));
            mask.set(cell_index(i, 8));
        }
        mask.set(cell_index(3, 3)); mask.set(cell_index(3, 5));
        mask.set(cell_index(4, 4));
        mask.set(cell_index(5, 3)); mask.set(cell_index(5, 5));
        return mask;
    }

    static BitSet81 make_preset_checkerboard() {
        BitSet81 mask;
        for (int r = 0; r < 9; ++r) {
            for (int c = 0; c < 9; ++c) {
                if ((r + c) % 2 == 0 && (r % 2 == 0)) {
                    mask.set(cell_index(r, c));
                }
            }
        }
        mask.set(cell_index(4, 4));
        return mask;
    }

    BitSet81 make_preset_random_symmetric(int target_clues, SymmetryType symm) {
        auto orbits = compute_orbits(symm);
        std::shuffle(orbits.begin(), orbits.end(), m_rng);

        BitSet81 mask;
        int curClues = 0;
        for (const auto& orbit : orbits) {
            if (curClues + static_cast<int>(orbit.size()) <= target_clues) {
                for (int cell : orbit) {
                    mask.set(cell);
                }
                curClues += static_cast<int>(orbit.size());
            }
            if (curClues >= target_clues) break;
        }
        return mask;
    }

    // Digs clues from full_board matching pattern_mask.
    // Retains clues ONLY on cells where pattern_mask.test(cell) is true.
    // Returns the board if it has a unique solution, otherwise std::nullopt.
    std::optional<BoardState> dig_pattern(const BoardState& full_board, const BitSet81& pattern_mask) {
        if (pattern_mask.count() < 17) {
            return std::nullopt;
        }

        std::string s;
        s.reserve(TOTAL_CELLS);
        for (int i = 0; i < TOTAL_CELLS; ++i) {
            if (pattern_mask.test(i)) {
                s += static_cast<char>('0' + full_board.get_value(i));
            } else {
                s += '.';
            }
        }

        BoardState test_board(s);
        DlxSolver dlx;
        if (dlx.count_solutions(test_board, 2) == 1) {
            return test_board;
        }
        return std::nullopt;
    }

    // Generates a random puzzle whose clues strictly match pattern_mask.
    // Tries up to max_attempts random terminal grids.
    std::optional<BoardState> generate_pattern_puzzle(const BitSet81& pattern_mask, int max_attempts = 1000, std::function<bool(int)> progress_cb = nullptr) {
        if (pattern_mask.count() < 17) {
            return std::nullopt;
        }

        for (int attempt = 0; attempt < max_attempts; ++attempt) {
            if (progress_cb && !progress_cb(attempt)) {
                return std::nullopt;
            }

            BoardState full = generate_terminal_grid();
            auto puzzle = dig_pattern(full, pattern_mask);
            if (puzzle.has_value()) {
                return puzzle;
            }
        }

        return std::nullopt;
    }

private:
    std::mt19937 m_rng;
};

} // namespace hodoku::core
