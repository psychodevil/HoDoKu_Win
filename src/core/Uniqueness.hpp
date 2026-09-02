#pragma once

#include <vector>
#include <string>
#include "BoardState.hpp"
#include "Step.hpp"

namespace hodoku::core {

class Uniqueness {
public:
    // 1. Unique Rectangles Types 1, 2, and 4 (Hard, Score: 100)
    static std::vector<Step> find_unique_rectangles(const BoardState& board) {
        std::vector<Step> steps;

        // Iterate over all pairs of rows
        for (int r1 = 0; r1 < 8; ++r1) {
            for (int r2 = r1 + 1; r2 < 9; ++r2) {
                for (int c1 = 0; c1 < 8; ++c1) {
                    for (int c2 = c1 + 1; c2 < 9; ++c2) {
                        int cell11 = cell_index(r1, c1);
                        int cell12 = cell_index(r1, c2);
                        int cell21 = cell_index(r2, c1);
                        int cell22 = cell_index(r2, c2);

                        // All 4 cells must be unfilled
                        if (!board.is_unfilled(cell11) || !board.is_unfilled(cell12) ||
                            !board.is_unfilled(cell21) || !board.is_unfilled(cell22)) {
                            continue;
                        }

                        // Must touch exactly 2 boxes (either horizontal pair or vertical pair)
                        int b1 = cell_box(cell11);
                        int b2 = cell_box(cell12);
                        int b3 = cell_box(cell21);
                        int b4 = cell_box(cell22);
                        bool horiz = (b1 == b2 && b3 == b4 && b1 != b3);
                        bool vert = (b1 == b3 && b2 == b4 && b1 != b2);
                        if (!horiz && !vert) continue;

                            CandidateMask m11 = board.get_candidates(cell11);
                            CandidateMask m12 = board.get_candidates(cell12);
                            CandidateMask m21 = board.get_candidates(cell21);
                            CandidateMask m22 = board.get_candidates(cell22);

                            // Test all pairs of digits (d1, d2)
                            CandidateMask common = m11 & m12 & m21 & m22;
                            for (int d1 = 1; d1 <= 8; ++d1) {
                                if (!mask_has_digit(common, d1)) continue;
                                for (int d2 = d1 + 1; d2 <= 9; ++d2) {
                                    if (!mask_has_digit(common, d2)) continue;

                                    CandidateMask pair_mask = digit_to_mask(d1) | digit_to_mask(d2);

                                    int pure_count = 0;
                                    int extra_idx = -1;
                                    int cells[4] = {cell11, cell12, cell21, cell22};
                                    CandidateMask masks[4] = {m11, m12, m21, m22};

                                    for (int k = 0; k < 4; ++k) {
                                        if (masks[k] == pair_mask) {
                                            pure_count++;
                                        } else {
                                            extra_idx = k;
                                        }
                                    }

                                    // Type 1: Exactly 3 cells are pure bivalue (d1, d2)
                                    if (pure_count == 3 && extra_idx != -1) {
                                        int target = cells[extra_idx];
                                        std::vector<CandidateElimination> elims;
                                        elims.push_back({target, d1});
                                        elims.push_back({target, d2});

                                        Step step;
                                        step.type = TechniqueType::UniqueRectangle;
                                        step.name = "Unique Rectangle Type 1";
                                        step.difficulty = DifficultyLevel::Hard;
                                        step.score = 100;
                                        for (int k = 0; k < 4; ++k) step.primary_cells.set(cells[k]);
                                        step.eliminations = elims;

                                        step.explanation = "Unique Rectangle Type 1 (" + std::to_string(d1) + "/" + std::to_string(d2) +
                                                          ") eliminates (" + std::to_string(d1) + "," + std::to_string(d2) +
                                                          ") from r" + std::to_string(cell_row(target) + 1) + "c" + std::to_string(cell_col(target) + 1) +
                                                          " to prevent a deadly pattern.";
                                        steps.push_back(step);
                                    }

                                    // Type 2: Exactly 2 cells are pure, other 2 have the same extra candidate x
                                    if (pure_count == 2) {
                                        std::vector<int> extras;
                                        for (int k = 0; k < 4; ++k) {
                                            if (masks[k] != pair_mask) extras.push_back(k);
                                        }
                                        if (extras.size() == 2) {
                                            CandidateMask extra_mask1 = masks[extras[0]] & ~pair_mask;
                                            CandidateMask extra_mask2 = masks[extras[1]] & ~pair_mask;

                                            if (count_candidates(extra_mask1) == 1 && extra_mask1 == extra_mask2) {
                                                int x = get_single_digit(extra_mask1);
                                                int cA = cells[extras[0]];
                                                int cB = cells[extras[1]];

                                                BitSet81 common_peers = get_peer_bitset(cA) & get_peer_bitset(cB);
                                                std::vector<CandidateElimination> elims;

                                                common_peers.for_each_cell([&](int target) {
                                                    if (board.is_unfilled(target) && board.has_candidate(target, x)) {
                                                        elims.push_back({target, x});
                                                    }
                                                });

                                                if (!elims.empty()) {
                                                    Step step;
                                                    step.type = TechniqueType::UniqueRectangle;
                                                    step.name = "Unique Rectangle Type 2";
                                                    step.difficulty = DifficultyLevel::Hard;
                                                    step.score = 100;
                                                    for (int k = 0; k < 4; ++k) step.primary_cells.set(cells[k]);
                                                    step.eliminations = elims;

                                                    step.explanation = "Unique Rectangle Type 2 (" + std::to_string(d1) + "/" + std::to_string(d2) +
                                                                      ") with extra candidate " + std::to_string(x) +
                                                                      " eliminates " + std::to_string(x) + " from common peers.";
                                                    steps.push_back(step);
                                                }
                                            }
                                        }
                                    }
                            }
                        }
                    }
                }
            }
        }

        return steps;
    }

    // 2. BUG+1 (Binary Universal Grave + 1 - Hard, Score: 100)
    static std::vector<Step> find_bug_plus_one(const BoardState& board) {
        std::vector<Step> steps;

        int tri_cell = -1;
        int unfilled_count = 0;

        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            if (board.is_unfilled(cell)) {
                unfilled_count++;
                int cnt = board.count_candidates(cell);
                if (cnt == 2) {
                    continue;
                } else if (cnt == 3) {
                    if (tri_cell == -1) {
                        tri_cell = cell;
                    } else {
                        return steps; // More than 1 tri-value cell -> not BUG+1
                    }
                } else {
                    return steps; // Cells with count != 2 and != 3 -> not BUG+1
                }
            }
        }

        if (tri_cell != -1 && unfilled_count >= 4) {
            CandidateMask m = board.get_candidates(tri_cell);
            int r = cell_row(tri_cell);
            int c = cell_col(tri_cell);

            // Find the candidate that appears 3 times in the row, col, or box
            int bug_digit = -1;
            for (int d = 1; d <= 9; ++d) {
                if (mask_has_digit(m, d)) {
                    int r_count = board.get_candidates_in_house(r, d).count();
                    int c_count = board.get_candidates_in_house(9 + c, d).count();

                    if (r_count == 3 || c_count == 3) {
                        bug_digit = d;
                        break;
                    }
                }
            }

            if (bug_digit != -1) {
                Step step;
                step.type = TechniqueType::Custom;
                step.name = "BUG+1";
                step.difficulty = DifficultyLevel::Hard;
                step.score = 100;
                step.primary_cells.set(tri_cell);
                step.assignments.push_back({tri_cell, static_cast<uint8_t>(bug_digit)});

                step.explanation = "BUG+1 at r" + std::to_string(r + 1) + "c" + std::to_string(c + 1) +
                                  " sets value " + std::to_string(bug_digit) + " to avoid a Binary Universal Grave.";
                steps.push_back(step);
            }
        }

        return steps;
    }
};

} // namespace hodoku::core

