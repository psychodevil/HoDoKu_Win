#pragma once

#include <vector>
#include <string>
#include "BoardState.hpp"
#include "Step.hpp"

namespace hodoku::core {

class SingleDigitPatterns {
public:
    // 1. Skyscraper (Hard, Score: 130)
    static std::vector<Step> find_skyscrapers(const BoardState& board) {
        std::vector<Step> steps;

        for (int d = 1; d <= 9; ++d) {
            // Row-based Skyscraper (Base: Rows, shared cover: Column)
            std::vector<std::pair<int, std::pair<int, int>>> row_pairs; // (row, (col1, col2))
            for (int r = 0; r < 9; ++r) {
                BitSet81 r_cells = board.get_candidates_in_house(r, d);
                if (r_cells.count() == 2) {
                    int c1 = cell_col(r_cells.pop_first_cell());
                    int c2 = cell_col(r_cells.pop_first_cell());
                    row_pairs.push_back({r, {c1, c2}});
                }
            }

            size_t n_rows = row_pairs.size();
            for (size_t i = 0; i < n_rows; ++i) {
                for (size_t j = i + 1; j < n_rows; ++j) {
                    int r1 = row_pairs[i].first;
                    int r2 = row_pairs[j].first;
                    int c1a = row_pairs[i].second.first;
                    int c1b = row_pairs[i].second.second;
                    int c2a = row_pairs[j].second.first;
                    int c2b = row_pairs[j].second.second;

                    // Check if exactly one column is shared
                    int shared_col = -1;
                    int roof1 = -1, roof2 = -1;

                    if (c1a == c2a && c1b != c2b) {
                        shared_col = c1a;
                        roof1 = cell_index(r1, c1b);
                        roof2 = cell_index(r2, c2b);
                    } else if (c1a == c2b && c1b != c2a) {
                        shared_col = c1a;
                        roof1 = cell_index(r1, c1b);
                        roof2 = cell_index(r2, c2a);
                    } else if (c1b == c2a && c1a != c2b) {
                        shared_col = c1b;
                        roof1 = cell_index(r1, c1a);
                        roof2 = cell_index(r2, c2b);
                    } else if (c1b == c2b && c1a != c2a) {
                        shared_col = c1b;
                        roof1 = cell_index(r1, c1a);
                        roof2 = cell_index(r2, c2a);
                    }

                    // Free ends must NOT be in the same column (which would be an X-Wing)
                    if (shared_col != -1 && roof1 != -1 && roof2 != -1 && cell_col(roof1) != cell_col(roof2)) {
                        BitSet81 common_peers = get_peer_bitset(roof1) & get_peer_bitset(roof2);
                        std::vector<CandidateElimination> elims;

                        common_peers.for_each_cell([&](int elim_cell) {
                            if (board.is_unfilled(elim_cell) && board.has_candidate(elim_cell, d)) {
                                elims.push_back({elim_cell, d});
                            }
                        });

                        if (!elims.empty()) {
                            Step step;
                            step.type = TechniqueType::Custom;
                            step.name = "Skyscraper";
                            step.difficulty = DifficultyLevel::Hard;
                            step.score = 130;
                            step.primary_cells.set(cell_index(r1, c1a));
                            step.primary_cells.set(cell_index(r1, c1b));
                            step.primary_cells.set(cell_index(r2, c2a));
                            step.primary_cells.set(cell_index(r2, c2b));
                            step.eliminations = elims;

                            step.explanation = "Skyscraper on digit " + std::to_string(d) + " in rows " +
                                              std::to_string(r1 + 1) + " and " + std::to_string(r2 + 1) +
                                              " connected by column " + std::to_string(shared_col + 1) +
                                              " eliminates digit " + std::to_string(d) + " from common peers.";
                            steps.push_back(step);
                        }
                    }
                }
            }

            // Col-based Skyscraper (Base: Columns, shared cover: Row)
            std::vector<std::pair<int, std::pair<int, int>>> col_pairs;
            for (int c = 0; c < 9; ++c) {
                BitSet81 c_cells = board.get_candidates_in_house(9 + c, d);
                if (c_cells.count() == 2) {
                    int r1 = cell_row(c_cells.pop_first_cell());
                    int r2 = cell_row(c_cells.pop_first_cell());
                    col_pairs.push_back({c, {r1, r2}});
                }
            }

            size_t n_cols = col_pairs.size();
            for (size_t i = 0; i < n_cols; ++i) {
                for (size_t j = i + 1; j < n_cols; ++j) {
                    int c1 = col_pairs[i].first;
                    int c2 = col_pairs[j].first;
                    int r1a = col_pairs[i].second.first;
                    int r1b = col_pairs[i].second.second;
                    int r2a = col_pairs[j].second.first;
                    int r2b = col_pairs[j].second.second;

                    int shared_row = -1;
                    int roof1 = -1, roof2 = -1;

                    if (r1a == r2a && r1b != r2b) {
                        shared_row = r1a;
                        roof1 = cell_index(r1b, c1);
                        roof2 = cell_index(r2b, c2);
                    } else if (r1a == r2b && r1b != r2a) {
                        shared_row = r1a;
                        roof1 = cell_index(r1b, c1);
                        roof2 = cell_index(r2a, c2);
                    } else if (r1b == r2a && r1a != r2b) {
                        shared_row = r1b;
                        roof1 = cell_index(r1a, c1);
                        roof2 = cell_index(r2b, c2);
                    } else if (r1b == r2b && r1a != r2a) {
                        shared_row = r1b;
                        roof1 = cell_index(r1a, c1);
                        roof2 = cell_index(r2a, c2);
                    }

                    if (shared_row != -1 && roof1 != -1 && roof2 != -1 && cell_row(roof1) != cell_row(roof2)) {
                        BitSet81 common_peers = get_peer_bitset(roof1) & get_peer_bitset(roof2);
                        std::vector<CandidateElimination> elims;

                        common_peers.for_each_cell([&](int elim_cell) {
                            if (board.is_unfilled(elim_cell) && board.has_candidate(elim_cell, d)) {
                                elims.push_back({elim_cell, d});
                            }
                        });

                        if (!elims.empty()) {
                            Step step;
                            step.type = TechniqueType::Custom;
                            step.name = "Skyscraper";
                            step.difficulty = DifficultyLevel::Hard;
                            step.score = 130;
                            step.primary_cells.set(cell_index(r1a, c1));
                            step.primary_cells.set(cell_index(r1b, c1));
                            step.primary_cells.set(cell_index(r2a, c2));
                            step.primary_cells.set(cell_index(r2b, c2));
                            step.eliminations = elims;

                            step.explanation = "Skyscraper on digit " + std::to_string(d) + " in columns " +
                                              std::to_string(c1 + 1) + " and " + std::to_string(c2 + 1) +
                                              " connected by row " + std::to_string(shared_row + 1) +
                                              " eliminates digit " + std::to_string(d) + " from common peers.";
                            steps.push_back(step);
                        }
                    }
                }
            }
        }

        return steps;
    }

    // 2. 2-String Kite (Hard, Score: 150)
    // Matches HoDoKu SingleDigitPatternSolver.java lines 668-760
    static std::vector<Step> find_two_string_kites(const BoardState& board) {
        std::vector<Step> steps;

        for (int d = 1; d <= 9; ++d) {
            std::vector<std::pair<int, int>> row_links;
            for (int r = 0; r < 9; ++r) {
                BitSet81 cands = board.get_candidates_in_house(r, d);
                if (cands.count() == 2) {
                    int c1 = cands.pop_first_cell();
                    int c2 = cands.pop_first_cell();
                    row_links.push_back({c1, c2});
                }
            }

            std::vector<std::pair<int, int>> col_links;
            for (int c = 0; c < 9; ++c) {
                BitSet81 cands = board.get_candidates_in_house(9 + c, d);
                if (cands.count() == 2) {
                    int c1 = cands.pop_first_cell();
                    int c2 = cands.pop_first_cell();
                    col_links.push_back({c1, c2});
                }
            }

            for (const auto& rlink : row_links) {
                for (const auto& clink : col_links) {
                    // Try all 4 combinations to find connecting box
                    int r_base = -1, r_end = -1;
                    int c_base = -1, c_end = -1;

                    if (cell_box(rlink.first) == cell_box(clink.first)) {
                        r_base = rlink.first; r_end = rlink.second;
                        c_base = clink.first; c_end = clink.second;
                    } else if (cell_box(rlink.first) == cell_box(clink.second)) {
                        r_base = rlink.first; r_end = rlink.second;
                        c_base = clink.second; c_end = clink.first;
                    } else if (cell_box(rlink.second) == cell_box(clink.first)) {
                        r_base = rlink.second; r_end = rlink.first;
                        c_base = clink.first; c_end = clink.second;
                    } else if (cell_box(rlink.second) == cell_box(clink.second)) {
                        r_base = rlink.second; r_end = rlink.first;
                        c_base = clink.second; c_end = clink.first;
                    }

                    if (r_base != -1) {
                        // The two connecting cells must not be the same cell
                        if (r_base == c_base || r_base == c_end || r_end == c_base || r_end == c_end) {
                            continue;
                        }

                        // HoDoKu exact elimination:
                        // Row of the column free end, Column of the row free end!
                        int target_cell = cell_index(cell_row(c_end), cell_col(r_end));
                        if (board.is_unfilled(target_cell) && board.has_candidate(target_cell, d)) {
                            Step step;
                            step.type = TechniqueType::Custom;
                            step.name = "2-String Kite";
                            step.difficulty = DifficultyLevel::Hard;
                            step.score = 150;
                            step.primary_cells.set(r_base);
                            step.primary_cells.set(r_end);
                            step.primary_cells.set(c_base);
                            step.primary_cells.set(c_end);
                            step.eliminations.push_back({target_cell, d});

                            step.explanation = "2-String Kite on digit " + std::to_string(d) +
                                              " connected in box " + std::to_string(cell_box(r_base) + 1) +
                                              " eliminates digit " + std::to_string(d) + " at r" +
                                              std::to_string(cell_row(target_cell) + 1) + "c" + std::to_string(cell_col(target_cell) + 1) + ".";
                            steps.push_back(step);
                        }
                    }
                }
            }
        }

        return steps;
    }

    // 3. Turbot Fish (Hard, Score: 120)
    // 3-link single-digit chain (A = B - C = D)
    static std::vector<Step> find_turbot_fish(const BoardState& board) {
        std::vector<Step> steps;

        for (int d = 1; d <= 9; ++d) {
            std::vector<std::pair<int, int>> strong_links;
            for (int h = 0; h < TOTAL_HOUSES; ++h) {
                BitSet81 cands = board.get_candidates_in_house(h, d);
                if (cands.count() == 2) {
                    int c1 = cands.pop_first_cell();
                    int c2 = cands.pop_first_cell();
                    strong_links.push_back({c1, c2});
                    strong_links.push_back({c2, c1});
                }
            }

            size_t n = strong_links.size();
            for (size_t i = 0; i < n; ++i) {
                int a = strong_links[i].first;
                int b = strong_links[i].second;

                for (size_t j = 0; j < n; ++j) {
                    if (i == j) continue;
                    int c = strong_links[j].first;
                    int d_cell = strong_links[j].second;

                    // Weak link between b and c: they must see each other and be distinct
                    if (b != c && a != d_cell && a != c && b != d_cell && get_peer_bitset(b).test(c)) {
                        // a and d_cell must not see each other directly or be in the same house
                        if (!get_peer_bitset(a).test(d_cell)) {
                            BitSet81 common = get_peer_bitset(a) & get_peer_bitset(d_cell);
                            common.reset(b);
                            common.reset(c);

                            std::vector<CandidateElimination> elims;
                            common.for_each_cell([&](int target) {
                                if (board.is_unfilled(target) && board.has_candidate(target, d)) {
                                    elims.push_back({target, d});
                                }
                            });

                            if (!elims.empty()) {
                                Step step;
                                step.type = TechniqueType::Custom;
                                step.name = "Turbot Fish";
                                step.difficulty = DifficultyLevel::Hard;
                                step.score = 120;
                                step.primary_cells.set(a);
                                step.primary_cells.set(b);
                                step.primary_cells.set(c);
                                step.primary_cells.set(d_cell);
                                step.eliminations = elims;

                                step.explanation = "Turbot Fish on digit " + std::to_string(d) +
                                                  " eliminates digit " + std::to_string(d) + " from common peers.";
                                steps.push_back(step);
                            }
                        }
                    }
                }
            }
        }

        return steps;
    }

    // 4. Empty Rectangle (Hard, Score: 120)
    // Matches HoDoKu SingleDigitPatternSolver.java lines 264-427
    static std::vector<Step> find_empty_rectangles(const BoardState& board) {
        std::vector<Step> steps;

        for (int d = 1; d <= 9; ++d) {
            for (int b = 0; b < 9; ++b) {
                BitSet81 box_cands = board.get_candidates_in_house(18 + b, d);
                int count = box_cands.count();
                if (count < 2 || count > 5) continue;

                int b_row_start = (b / 3) * 3;
                int b_col_start = (b % 3) * 3;

                for (int erLine = b_row_start; erLine < b_row_start + 3; ++erLine) {
                    for (int erCol = b_col_start; erCol < b_col_start + 3; ++erCol) {
                        // Check if all box candidates lie in row erLine or col erCol
                        bool all_in_cross = true;
                        int cands_in_row_only = 0;
                        int cands_in_col_only = 0;

                        box_cands.for_each_cell([&](int c) {
                            int r = cell_row(c);
                            int col = cell_col(c);
                            if (r == erLine && col == erCol) {
                                // Pivot cell
                            } else if (r == erLine) {
                                cands_in_row_only++;
                            } else if (col == erCol) {
                                cands_in_col_only++;
                            } else {
                                all_in_cross = false;
                            }
                        });

                        if (!all_in_cross) continue;
                        // Must have candidates in row outside col, and col outside row (true L-shape)
                        if (cands_in_row_only == 0 || cands_in_col_only == 0) continue;

                        // Case A: Look for conjugate pair in a column intersecting erLine outside box b
                        for (int col = 0; col < 9; ++col) {
                            if (col / 3 == b % 3) continue; // Outside box b
                            int cell1 = cell_index(erLine, col);
                            if (board.is_unfilled(cell1) && board.has_candidate(cell1, d)) {
                                // Check if column 'col' has exactly 2 candidates (conjugate pair)
                                BitSet81 col_cells = board.get_candidates_in_house(9 + col, d);
                                if (col_cells.count() == 2) {
                                    int cA = col_cells.pop_first_cell();
                                    int cB = col_cells.pop_first_cell();
                                    int cell2 = (cA == cell1) ? cB : cA;
                                    int actLine = cell_row(cell2);

                                    // Elimination cell: row = actLine, col = erCol!
                                    int target = cell_index(actLine, erCol);
                                    if (cell_box(target) != b && board.is_unfilled(target) && board.has_candidate(target, d)) {
                                        Step step;
                                        step.type = TechniqueType::Custom;
                                        step.name = "Empty Rectangle";
                                        step.difficulty = DifficultyLevel::Hard;
                                        step.score = 120;
                                        step.primary_cells = box_cands;
                                        step.primary_cells.set(cell1);
                                        step.primary_cells.set(cell2);
                                        step.eliminations.push_back({target, d});

                                        step.explanation = "Empty Rectangle on digit " + std::to_string(d) +
                                                          " in box " + std::to_string(b + 1) +
                                                          " with conjugate pair in column " + std::to_string(col + 1) +
                                                          " eliminates digit " + std::to_string(d) + " at r" +
                                                          std::to_string(actLine + 1) + "c" + std::to_string(erCol + 1) + ".";
                                        steps.push_back(step);
                                    }
                                }
                            }
                        }

                        // Case B: Look for conjugate pair in a row intersecting erCol outside box b
                        for (int row = 0; row < 9; ++row) {
                            if (row / 3 == b / 3) continue; // Outside box b
                            int cell1 = cell_index(row, erCol);
                            if (board.is_unfilled(cell1) && board.has_candidate(cell1, d)) {
                                BitSet81 row_cells = board.get_candidates_in_house(row, d);
                                if (row_cells.count() == 2) {
                                    int cA = row_cells.pop_first_cell();
                                    int cB = row_cells.pop_first_cell();
                                    int cell2 = (cA == cell1) ? cB : cA;
                                    int actCol = cell_col(cell2);

                                    // Elimination cell: row = erLine, col = actCol!
                                    int target = cell_index(erLine, actCol);
                                    if (cell_box(target) != b && board.is_unfilled(target) && board.has_candidate(target, d)) {
                                        Step step;
                                        step.type = TechniqueType::Custom;
                                        step.name = "Empty Rectangle";
                                        step.difficulty = DifficultyLevel::Hard;
                                        step.score = 120;
                                        step.primary_cells = box_cands;
                                        step.primary_cells.set(cell1);
                                        step.primary_cells.set(cell2);
                                        step.eliminations.push_back({target, d});

                                        step.explanation = "Empty Rectangle on digit " + std::to_string(d) +
                                                          " in box " + std::to_string(b + 1) +
                                                          " with conjugate pair in row " + std::to_string(row + 1) +
                                                          " eliminates digit " + std::to_string(d) + " at r" +
                                                          std::to_string(erLine + 1) + "c" + std::to_string(actCol + 1) + ".";
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
