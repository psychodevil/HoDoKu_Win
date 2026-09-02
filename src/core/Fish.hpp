#pragma once

#include <vector>
#include <string>
#include <array>
#include "BoardState.hpp"
#include "Step.hpp"

namespace hodoku::core {

class Fish {
public:
    // 1. X-Wing (Size 2 Basic Fish - Hard, Score: 140)
    static std::vector<Step> find_x_wings(const BoardState& board) {
        std::vector<Step> steps;

        for (int d = 1; d <= 9; ++d) {
            // Row-based X-Wing (Base: Rows, Cover: Cols)
            std::vector<std::pair<int, std::pair<int, int>>> row_cands;
            for (int r = 0; r < 9; ++r) {
                BitSet81 cands = board.get_candidates_in_house(r, d);
                if (cands.count() == 2) {
                    int c1 = cell_col(cands.pop_first_cell());
                    int c2 = cell_col(cands.pop_first_cell());
                    row_cands.push_back({r, {c1, c2}});
                }
            }

            size_t n_rows = row_cands.size();
            for (size_t i = 0; i < n_rows; ++i) {
                for (size_t j = i + 1; j < n_rows; ++j) {
                    if (row_cands[i].second == row_cands[j].second) {
                        int r1 = row_cands[i].first;
                        int r2 = row_cands[j].first;
                        int c1 = row_cands[i].second.first;
                        int c2 = row_cands[i].second.second;

                        std::vector<CandidateElimination> elims;
                        for (int r = 0; r < 9; ++r) {
                            if (r != r1 && r != r2) {
                                int cell_a = cell_index(r, c1);
                                if (board.is_unfilled(cell_a) && board.has_candidate(cell_a, d)) {
                                    elims.push_back({cell_a, d});
                                }
                                int cell_b = cell_index(r, c2);
                                if (board.is_unfilled(cell_b) && board.has_candidate(cell_b, d)) {
                                    elims.push_back({cell_b, d});
                                }
                            }
                        }

                        if (!elims.empty()) {
                            Step step;
                            step.type = TechniqueType::XWing;
                            step.name = "X-Wing";
                            step.difficulty = DifficultyLevel::Hard;
                            step.score = 140;
                            step.primary_cells.set(cell_index(r1, c1));
                            step.primary_cells.set(cell_index(r1, c2));
                            step.primary_cells.set(cell_index(r2, c1));
                            step.primary_cells.set(cell_index(r2, c2));
                            step.eliminations = elims;

                            step.explanation = "X-Wing on digit " + std::to_string(d) + " in rows " +
                                              std::to_string(r1 + 1) + "," + std::to_string(r2 + 1) +
                                              " eliminates digit " + std::to_string(d) + " from columns " +
                                              std::to_string(c1 + 1) + "," + std::to_string(c2 + 1) + ".";
                            steps.push_back(step);
                        }
                    }
                }
            }

            // Col-based X-Wing (Base: Cols, Cover: Rows)
            std::vector<std::pair<int, std::pair<int, int>>> col_cands;
            for (int c = 0; c < 9; ++c) {
                BitSet81 cands = board.get_candidates_in_house(9 + c, d);
                if (cands.count() == 2) {
                    int r1 = cell_row(cands.pop_first_cell());
                    int r2 = cell_row(cands.pop_first_cell());
                    col_cands.push_back({c, {r1, r2}});
                }
            }

            size_t n_cols = col_cands.size();
            for (size_t i = 0; i < n_cols; ++i) {
                for (size_t j = i + 1; j < n_cols; ++j) {
                    if (col_cands[i].second == col_cands[j].second) {
                        int c1 = col_cands[i].first;
                        int c2 = col_cands[j].first;
                        int r1 = col_cands[i].second.first;
                        int r2 = col_cands[i].second.second;

                        std::vector<CandidateElimination> elims;
                        for (int c = 0; c < 9; ++c) {
                            if (c != c1 && c != c2) {
                                int cell_a = cell_index(r1, c);
                                if (board.is_unfilled(cell_a) && board.has_candidate(cell_a, d)) {
                                    elims.push_back({cell_a, d});
                                }
                                int cell_b = cell_index(r2, c);
                                if (board.is_unfilled(cell_b) && board.has_candidate(cell_b, d)) {
                                    elims.push_back({cell_b, d});
                                }
                            }
                        }

                        if (!elims.empty()) {
                            Step step;
                            step.type = TechniqueType::XWing;
                            step.name = "X-Wing";
                            step.difficulty = DifficultyLevel::Hard;
                            step.score = 140;
                            step.primary_cells.set(cell_index(r1, c1));
                            step.primary_cells.set(cell_index(r1, c2));
                            step.primary_cells.set(cell_index(r2, c1));
                            step.primary_cells.set(cell_index(r2, c2));
                            step.eliminations = elims;

                            step.explanation = "X-Wing on digit " + std::to_string(d) + " in columns " +
                                              std::to_string(c1 + 1) + "," + std::to_string(c2 + 1) +
                                              " eliminates digit " + std::to_string(d) + " from rows " +
                                              std::to_string(r1 + 1) + "," + std::to_string(r2 + 1) + ".";
                            steps.push_back(step);
                        }
                    }
                }
            }
        }

        return steps;
    }

    // 2. Swordfish (Size 3 Basic Fish - Hard, Score: 200)
    static std::vector<Step> find_swordfish(const BoardState& board) {
        std::vector<Step> steps;

        for (int d = 1; d <= 9; ++d) {
            // Row-based Swordfish
            std::vector<std::pair<int, uint16_t>> row_masks; // (row, col_bitmask)
            for (int r = 0; r < 9; ++r) {
                BitSet81 cands = board.get_candidates_in_house(r, d);
                int count = cands.count();
                if (count >= 2 && count <= 3) {
                    uint16_t mask = 0;
                    cands.for_each_cell([&](int c) {
                        mask |= (1 << cell_col(c));
                    });
                    row_masks.push_back({r, mask});
                }
            }

            size_t n_rows = row_masks.size();
            if (n_rows >= 3) {
                for (size_t i = 0; i < n_rows; ++i) {
                    for (size_t j = i + 1; j < n_rows; ++j) {
                        for (size_t k = j + 1; k < n_rows; ++k) {
                            uint16_t combined = row_masks[i].second | row_masks[j].second | row_masks[k].second;
                            if (std::popcount(combined) == 3) {
                                int r1 = row_masks[i].first;
                                int r2 = row_masks[j].first;
                                int r3 = row_masks[k].first;

                                std::vector<int> cols;
                                for (int c = 0; c < 9; ++c) {
                                    if (combined & (1 << c)) cols.push_back(c);
                                }

                                std::vector<CandidateElimination> elims;
                                for (int c : cols) {
                                    for (int r = 0; r < 9; ++r) {
                                        if (r != r1 && r != r2 && r != r3) {
                                            int target = cell_index(r, c);
                                            if (board.is_unfilled(target) && board.has_candidate(target, d)) {
                                                elims.push_back({target, d});
                                            }
                                        }
                                    }
                                }

                                if (!elims.empty()) {
                                    Step step;
                                    step.type = TechniqueType::Swordfish;
                                    step.name = "Swordfish";
                                    step.difficulty = DifficultyLevel::Hard;
                                    step.score = 200;

                                    for (int r : {r1, r2, r3}) {
                                        BitSet81 rc = board.get_candidates_in_house(r, d);
                                        step.primary_cells |= rc;
                                    }
                                    step.eliminations = elims;

                                    step.explanation = "Swordfish on digit " + std::to_string(d) + " in rows " +
                                                      std::to_string(r1 + 1) + "," + std::to_string(r2 + 1) + "," + std::to_string(r3 + 1) +
                                                      " eliminates digit " + std::to_string(d) + " from other cells in columns " +
                                                      std::to_string(cols[0] + 1) + "," + std::to_string(cols[1] + 1) + "," + std::to_string(cols[2] + 1) + ".";
                                    steps.push_back(step);
                                }
                            }
                        }
                    }
                }
            }

            // Col-based Swordfish
            std::vector<std::pair<int, uint16_t>> col_masks;
            for (int c = 0; c < 9; ++c) {
                BitSet81 cands = board.get_candidates_in_house(9 + c, d);
                int count = cands.count();
                if (count >= 2 && count <= 3) {
                    uint16_t mask = 0;
                    cands.for_each_cell([&](int cell) {
                        mask |= (1 << cell_row(cell));
                    });
                    col_masks.push_back({c, mask});
                }
            }

            size_t n_cols = col_masks.size();
            if (n_cols >= 3) {
                for (size_t i = 0; i < n_cols; ++i) {
                    for (size_t j = i + 1; j < n_cols; ++j) {
                        for (size_t k = j + 1; k < n_cols; ++k) {
                            uint16_t combined = col_masks[i].second | col_masks[j].second | col_masks[k].second;
                            if (std::popcount(combined) == 3) {
                                int c1 = col_masks[i].first;
                                int c2 = col_masks[j].first;
                                int c3 = col_masks[k].first;

                                std::vector<int> rows;
                                for (int r = 0; r < 9; ++r) {
                                    if (combined & (1 << r)) rows.push_back(r);
                                }

                                std::vector<CandidateElimination> elims;
                                for (int r : rows) {
                                    for (int c = 0; c < 9; ++c) {
                                        if (c != c1 && c != c2 && c != c3) {
                                            int target = cell_index(r, c);
                                            if (board.is_unfilled(target) && board.has_candidate(target, d)) {
                                                elims.push_back({target, d});
                                            }
                                        }
                                    }
                                }

                                if (!elims.empty()) {
                                    Step step;
                                    step.type = TechniqueType::Swordfish;
                                    step.name = "Swordfish";
                                    step.difficulty = DifficultyLevel::Hard;
                                    step.score = 200;

                                    for (int c : {c1, c2, c3}) {
                                        BitSet81 cc = board.get_candidates_in_house(9 + c, d);
                                        step.primary_cells |= cc;
                                    }
                                    step.eliminations = elims;

                                    step.explanation = "Swordfish on digit " + std::to_string(d) + " in columns " +
                                                      std::to_string(c1 + 1) + "," + std::to_string(c2 + 1) + "," + std::to_string(c3 + 1) +
                                                      " eliminates digit " + std::to_string(d) + " from other cells in rows " +
                                                      std::to_string(rows[0] + 1) + "," + std::to_string(rows[1] + 1) + "," + std::to_string(rows[2] + 1) + ".";
                                    steps.push_back(step);
                                }
                            }
                        }
                    }
                }
            }
        }

        return steps;
    }

    // 3. Jellyfish (Size 4 Basic Fish - Unfair, Score: 250)
    static std::vector<Step> find_jellyfish(const BoardState& board) {
        std::vector<Step> steps;

        for (int d = 1; d <= 9; ++d) {
            // Row-based Jellyfish
            std::vector<std::pair<int, uint16_t>> row_masks;
            for (int r = 0; r < 9; ++r) {
                BitSet81 cands = board.get_candidates_in_house(r, d);
                int count = cands.count();
                if (count >= 2 && count <= 4) {
                    uint16_t mask = 0;
                    cands.for_each_cell([&](int c) {
                        mask |= (1 << cell_col(c));
                    });
                    row_masks.push_back({r, mask});
                }
            }

            size_t n_rows = row_masks.size();
            if (n_rows >= 4) {
                for (size_t i = 0; i < n_rows; ++i) {
                    for (size_t j = i + 1; j < n_rows; ++j) {
                        for (size_t k = j + 1; k < n_rows; ++k) {
                            for (size_t l = k + 1; l < n_rows; ++l) {
                                uint16_t combined = row_masks[i].second | row_masks[j].second |
                                                    row_masks[k].second | row_masks[l].second;
                                if (std::popcount(combined) == 4) {
                                    int r1 = row_masks[i].first, r2 = row_masks[j].first;
                                    int r3 = row_masks[k].first, r4 = row_masks[l].first;

                                    std::vector<int> cols;
                                    for (int c = 0; c < 9; ++c) {
                                        if (combined & (1 << c)) cols.push_back(c);
                                    }

                                    std::vector<CandidateElimination> elims;
                                    for (int c : cols) {
                                        for (int r = 0; r < 9; ++r) {
                                            if (r != r1 && r != r2 && r != r3 && r != r4) {
                                                int target = cell_index(r, c);
                                                if (board.is_unfilled(target) && board.has_candidate(target, d)) {
                                                    elims.push_back({target, d});
                                                }
                                            }
                                        }
                                    }

                                    if (!elims.empty()) {
                                        Step step;
                                        step.type = TechniqueType::Jellyfish;
                                        step.name = "Jellyfish";
                                        step.difficulty = DifficultyLevel::Unfair;
                                        step.score = 250;

                                        for (int r : {r1, r2, r3, r4}) {
                                            step.primary_cells |= board.get_candidates_in_house(r, d);
                                        }
                                        step.eliminations = elims;

                                        step.explanation = "Jellyfish on digit " + std::to_string(d) +
                                                          " eliminates candidate " + std::to_string(d) + " from cover columns.";
                                        steps.push_back(step);
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

    // 4. Finned X-Wing (Unfair, Score: 130)
    static std::vector<Step> find_finned_x_wings(const BoardState& board) {
        std::vector<Step> steps;

        for (int d = 1; d <= 9; ++d) {
            for (int r1 = 0; r1 < 8; ++r1) {
                BitSet81 c1 = board.get_candidates_in_house(r1, d);
                int cnt1 = c1.count();
                if (cnt1 < 2 || cnt1 > 4) continue;

                for (int r2 = r1 + 1; r2 < 9; ++r2) {
                    BitSet81 c2 = board.get_candidates_in_house(r2, d);
                    int cnt2 = c2.count();
                    if (cnt2 < 2 || cnt2 > 4) continue;

                    // Exactly one of the rows must be a strict pair, other row has fins
                    if ((cnt1 == 2 && cnt2 > 2) || (cnt2 == 2 && cnt1 > 2)) {
                        int base_strict = (cnt1 == 2) ? r1 : r2;
                        int base_finned = (cnt1 == 2) ? r2 : r1;
                        BitSet81 strict_cells = board.get_candidates_in_house(base_strict, d);
                        BitSet81 finned_cells = board.get_candidates_in_house(base_finned, d);

                        int colA = cell_col(strict_cells.pop_first_cell());
                        int colB = cell_col(strict_cells.pop_first_cell());

                        // Check if finned row contains colA and colB
                        int fin_colA = cell_index(base_finned, colA);
                        int fin_colB = cell_index(base_finned, colB);

                        if (finned_cells.test(fin_colA) && finned_cells.test(fin_colB)) {
                            BitSet81 fins = finned_cells;
                            fins.reset(fin_colA);
                            fins.reset(fin_colB);

                            // All fins must reside in the same 3x3 box as one of the pincer columns
                            int fin_box = -1;
                            bool all_same_box = true;
                            fins.for_each_cell([&](int fc) {
                                if (fin_box == -1) fin_box = cell_box(fc);
                                else if (fin_box != cell_box(fc)) all_same_box = false;
                            });

                            if (all_same_box && fin_box != -1) {
                                // Eliminations must see the fin(s) AND the normal X-Wing elimination column
                                int target_col = (cell_box(fin_colA) == fin_box) ? colA :
                                                 (cell_box(fin_colB) == fin_box) ? colB : -1;

                                if (target_col != -1) {
                                    BitSet81 fin_peers = fins.count() == 1 ? get_peer_bitset(fins.first_cell()) : BitSet81();
                                    if (fins.count() > 1) {
                                        fin_peers = BitSet81();
                                        bool first = true;
                                        fins.for_each_cell([&](int fc) {
                                            if (first) { fin_peers = get_peer_bitset(fc); first = false; }
                                            else fin_peers &= get_peer_bitset(fc);
                                        });
                                    }

                                    std::vector<CandidateElimination> elims;
                                    for (int r = 0; r < 9; ++r) {
                                        if (r != base_strict && r != base_finned) {
                                            int cand_cell = cell_index(r, target_col);
                                            if (fin_peers.test(cand_cell) && board.is_unfilled(cand_cell) && board.has_candidate(cand_cell, d)) {
                                                elims.push_back({cand_cell, d});
                                            }
                                        }
                                    }

                                    if (!elims.empty()) {
                                        Step step;
                                        step.type = TechniqueType::Custom;
                                        step.name = "Finned X-Wing";
                                        step.difficulty = DifficultyLevel::Unfair;
                                        step.score = 130;
                                        step.primary_cells = strict_cells | finned_cells;
                                        step.secondary_cells = fins;
                                        step.eliminations = elims;

                                        step.explanation = "Finned X-Wing on digit " + std::to_string(d) +
                                                          " with fin in box " + std::to_string(fin_box + 1) +
                                                          " eliminates candidate " + std::to_string(d) + ".";
                                        steps.push_back(step);
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
};

} // namespace hodoku::core
