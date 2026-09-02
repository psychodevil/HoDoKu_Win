#pragma once

#include <vector>
#include <string>
#include "BoardState.hpp"
#include "Step.hpp"

namespace hodoku::core {

class SueDeCoq {
public:
    static std::vector<Step> find_sue_de_coq(const BoardState& board) {
        std::vector<Step> steps;

        // Iterate over all 9 boxes
        for (int b = 0; b < 9; ++b) {
            int br = (b / 3) * 3;
            int bc = (b % 3) * 3;

            // 1. Check Row Intersections
            for (int r = br; r < br + 3; ++r) {
                std::vector<int> inter_cells;
                for (int c = bc; c < bc + 3; ++c) {
                    int cell = cell_index(r, c);
                    if (board.is_unfilled(cell)) inter_cells.push_back(cell);
                }

                if (inter_cells.size() == 2) {
                    int c1 = inter_cells[0];
                    int c2 = inter_cells[1];
                    CandidateMask m_int = board.get_candidates(c1) | board.get_candidates(c2);
                    int n_int_cands = count_candidates(m_int);

                    if (n_int_cands >= 4 && n_int_cands <= 5) {
                        for (int ob_cell : GRID.box_cells[b]) {
                            if (cell_row(ob_cell) == r || !board.is_unfilled(ob_cell)) continue;
                            CandidateMask m_box = board.get_candidates(ob_cell);

                            for (int ol_cell : GRID.row_cells[r]) {
                                if (cell_box(ol_cell) == b || !board.is_unfilled(ol_cell)) continue;
                                CandidateMask m_line = board.get_candidates(ol_cell);

                                if ((m_box & m_line) == 0) {
                                    CandidateMask total_cands = m_int | m_box | m_line;
                                    if (count_candidates(total_cands) == 4) {
                                        std::vector<CandidateElimination> elims;
                                        for (int b_cand : GRID.box_cells[b]) {
                                            if (b_cand != c1 && b_cand != c2 && b_cand != ob_cell && board.is_unfilled(b_cand)) {
                                                CandidateMask cb = board.get_candidates(b_cand) & m_box;
                                                for (int d = 1; d <= 9; ++d) {
                                                    if (mask_has_digit(cb, d)) elims.push_back({b_cand, d});
                                                }
                                            }
                                        }

                                        for (int l_cand : GRID.row_cells[r]) {
                                            if (l_cand != c1 && l_cand != c2 && l_cand != ol_cell && board.is_unfilled(l_cand)) {
                                                CandidateMask cl = board.get_candidates(l_cand) & m_line;
                                                for (int d = 1; d <= 9; ++d) {
                                                    if (mask_has_digit(cl, d)) elims.push_back({l_cand, d});
                                                }
                                            }
                                        }

                                        if (!elims.empty()) {
                                            Step step;
                                            step.type = TechniqueType::SueDeCoq;
                                            step.name = "Sue de Coq";
                                            step.difficulty = DifficultyLevel::Extreme;
                                            step.score = 250;
                                            step.primary_cells.set(c1);
                                            step.primary_cells.set(c2);
                                            step.secondary_cells.set(ob_cell);
                                            step.secondary_cells.set(ol_cell);
                                            step.eliminations = elims;

                                            step.explanation = "Sue de Coq in box " + std::to_string(b + 1) +
                                                              " and row " + std::to_string(r + 1) +
                                                              " eliminates candidates from peer cells.";
                                            steps.push_back(step);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 2. Check Column Intersections
            for (int c = bc; c < bc + 3; ++c) {
                std::vector<int> inter_cells;
                for (int r = br; r < br + 3; ++r) {
                    int cell = cell_index(r, c);
                    if (board.is_unfilled(cell)) inter_cells.push_back(cell);
                }

                if (inter_cells.size() == 2) {
                    int c1 = inter_cells[0];
                    int c2 = inter_cells[1];
                    CandidateMask m_int = board.get_candidates(c1) | board.get_candidates(c2);
                    int n_int_cands = count_candidates(m_int);

                    if (n_int_cands >= 4 && n_int_cands <= 5) {
                        for (int ob_cell : GRID.box_cells[b]) {
                            if (cell_col(ob_cell) == c || !board.is_unfilled(ob_cell)) continue;
                            CandidateMask m_box = board.get_candidates(ob_cell);

                            for (int ol_cell : GRID.col_cells[c]) {
                                if (cell_box(ol_cell) == b || !board.is_unfilled(ol_cell)) continue;
                                CandidateMask m_line = board.get_candidates(ol_cell);

                                if ((m_box & m_line) == 0) {
                                    CandidateMask total_cands = m_int | m_box | m_line;
                                    if (count_candidates(total_cands) == 4) {
                                        std::vector<CandidateElimination> elims;
                                        for (int b_cand : GRID.box_cells[b]) {
                                            if (b_cand != c1 && b_cand != c2 && b_cand != ob_cell && board.is_unfilled(b_cand)) {
                                                CandidateMask cb = board.get_candidates(b_cand) & m_box;
                                                for (int d = 1; d <= 9; ++d) {
                                                    if (mask_has_digit(cb, d)) elims.push_back({b_cand, d});
                                                }
                                            }
                                        }

                                        for (int l_cand : GRID.col_cells[c]) {
                                            if (l_cand != c1 && l_cand != c2 && l_cand != ol_cell && board.is_unfilled(l_cand)) {
                                                CandidateMask cl = board.get_candidates(l_cand) & m_line;
                                                for (int d = 1; d <= 9; ++d) {
                                                    if (mask_has_digit(cl, d)) elims.push_back({l_cand, d});
                                                }
                                            }
                                        }

                                        if (!elims.empty()) {
                                            Step step;
                                            step.type = TechniqueType::SueDeCoq;
                                            step.name = "Sue de Coq";
                                            step.difficulty = DifficultyLevel::Extreme;
                                            step.score = 250;
                                            step.primary_cells.set(c1);
                                            step.primary_cells.set(c2);
                                            step.secondary_cells.set(ob_cell);
                                            step.secondary_cells.set(ol_cell);
                                            step.eliminations = elims;

                                            step.explanation = "Sue de Coq in box " + std::to_string(b + 1) +
                                                              " and column " + std::to_string(c + 1) +
                                                              " eliminates candidates from peer cells.";
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

        return steps;
    }
};

} // namespace hodoku::core
