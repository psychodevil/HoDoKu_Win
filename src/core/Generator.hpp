#pragma once

#include <vector>
#include <array>
#include <random>
#include <algorithm>
#include <numeric>
#include <set>
#include <optional>
#include <chrono>

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
        int best_score = 0;

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
                best_score = score;
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

private:
    std::mt19937 m_rng;
};

} // namespace hodoku::core
