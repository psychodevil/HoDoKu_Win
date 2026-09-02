#pragma once

#include <vector>
#include <string>
#include "BoardState.hpp"
#include "Step.hpp"

namespace hodoku::core {

class Wings {
public:
    // 1. XY-Wing (Hard, Score: 160)
    static std::vector<Step> find_xy_wings(const BoardState& board) {
        std::vector<Step> steps;

        std::vector<int> bivalue_cells;
        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            if (board.is_unfilled(cell) && board.count_candidates(cell) == 2) {
                bivalue_cells.push_back(cell);
            }
        }

        size_t n = bivalue_cells.size();
        for (size_t p = 0; p < n; ++p) {
            int pivot = bivalue_cells[p];
            CandidateMask m_pivot = board.get_candidates(pivot);

            int x = std::countr_zero(static_cast<unsigned int>(m_pivot)) + 1;
            int y = std::countr_zero(static_cast<unsigned int>(m_pivot & ~digit_to_mask(x))) + 1;

            const auto& pivot_peers = get_peer_bitset(pivot);

            std::vector<int> x_pincers;
            std::vector<int> y_pincers;

            for (size_t i = 0; i < n; ++i) {
                int cand_cell = bivalue_cells[i];
                if (cand_cell == pivot || !pivot_peers.test(cand_cell)) continue;

                CandidateMask m_cand = board.get_candidates(cand_cell);
                bool has_x = mask_has_digit(m_cand, x);
                bool has_y = mask_has_digit(m_cand, y);

                if (has_x && !has_y) {
                    x_pincers.push_back(cand_cell);
                } else if (has_y && !has_x) {
                    y_pincers.push_back(cand_cell);
                }
            }

            for (int p1 : x_pincers) {
                CandidateMask m1 = board.get_candidates(p1);
                int z1 = std::countr_zero(static_cast<unsigned int>(m1 & ~digit_to_mask(x))) + 1;

                for (int p2 : y_pincers) {
                    CandidateMask m2 = board.get_candidates(p2);
                    int z2 = std::countr_zero(static_cast<unsigned int>(m2 & ~digit_to_mask(y))) + 1;

                    if (z1 == z2 && p1 != p2) {
                        int z = z1;

                        BitSet81 common_peers = get_peer_bitset(p1) & get_peer_bitset(p2);
                        common_peers.reset(pivot);

                        std::vector<CandidateElimination> elims;
                        common_peers.for_each_cell([&](int target_cell) {
                            if (board.is_unfilled(target_cell) && board.has_candidate(target_cell, z)) {
                                elims.push_back({target_cell, z});
                            }
                        });

                        if (!elims.empty()) {
                            Step step;
                            step.type = TechniqueType::XYWing;
                            step.name = "XY-Wing";
                            step.difficulty = DifficultyLevel::Hard;
                            step.score = 160;

                            step.primary_cells.set(pivot);
                            step.primary_cells.set(p1);
                            step.primary_cells.set(p2);

                            step.eliminations = elims;

                            step.explanation = "XY-Wing with pivot r" + std::to_string(cell_row(pivot) + 1) + "c" + std::to_string(cell_col(pivot) + 1) +
                                              " (" + std::to_string(x) + "," + std::to_string(y) + ") and pincers r" +
                                              std::to_string(cell_row(p1) + 1) + "c" + std::to_string(cell_col(p1) + 1) + " and r" +
                                              std::to_string(cell_row(p2) + 1) + "c" + std::to_string(cell_col(p2) + 1) +
                                              " eliminates candidate " + std::to_string(z) + " from common peers.";
                            steps.push_back(step);
                        }
                    }
                }
            }
        }

        return steps;
    }

    // 2. XYZ-Wing (Hard, Score: 180)
    static std::vector<Step> find_xyz_wings(const BoardState& board) {
        std::vector<Step> steps;

        std::vector<int> trivalue_cells;
        std::vector<int> bivalue_cells;

        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            if (board.is_unfilled(cell)) {
                int count = board.count_candidates(cell);
                if (count == 3) trivalue_cells.push_back(cell);
                else if (count == 2) bivalue_cells.push_back(cell);
            }
        }

        for (int pivot : trivalue_cells) {
            CandidateMask m_pivot = board.get_candidates(pivot);
            const auto& pivot_peers = get_peer_bitset(pivot);

            // Find pincers seeing pivot whose 2 candidates are a subset of {x, y, z}
            std::vector<int> pincers;
            for (int bi_cell : bivalue_cells) {
                if (pivot_peers.test(bi_cell)) {
                    CandidateMask m_bi = board.get_candidates(bi_cell);
                    if ((m_bi & ~m_pivot) == 0) {
                        pincers.push_back(bi_cell);
                    }
                }
            }

            size_t n_p = pincers.size();
            for (size_t i = 0; i < n_p; ++i) {
                for (size_t j = i + 1; j < n_p; ++j) {
                    int p1 = pincers[i];
                    int p2 = pincers[j];

                    CandidateMask m1 = board.get_candidates(p1);
                    CandidateMask m2 = board.get_candidates(p2);

                    if (m1 == m2) continue; // Pincers must have different candidate pairs

                    // The common candidate between all three is z
                    CandidateMask common_mask = m_pivot & m1 & m2;
                    if (count_candidates(common_mask) == 1) {
                        int common_z = get_single_digit(common_mask);

                        // Target cells must see pivot, p1, and p2
                        BitSet81 common_peers = get_peer_bitset(pivot) & get_peer_bitset(p1) & get_peer_bitset(p2);

                        std::vector<CandidateElimination> elims;
                        common_peers.for_each_cell([&](int target) {
                            if (board.is_unfilled(target) && board.has_candidate(target, common_z)) {
                                elims.push_back({target, common_z});
                            }
                        });

                        if (!elims.empty()) {
                            Step step;
                            step.type = TechniqueType::XYZWing;
                            step.name = "XYZ-Wing";
                            step.difficulty = DifficultyLevel::Hard;
                            step.score = 180;
                            step.primary_cells.set(pivot);
                            step.primary_cells.set(p1);
                            step.primary_cells.set(p2);
                            step.eliminations = elims;

                            step.explanation = "XYZ-Wing with pivot r" + std::to_string(cell_row(pivot) + 1) + "c" + std::to_string(cell_col(pivot) + 1) +
                                              " and pincers r" + std::to_string(cell_row(p1) + 1) + "c" + std::to_string(cell_col(p1) + 1) +
                                              " and r" + std::to_string(cell_row(p2) + 1) + "c" + std::to_string(cell_col(p2) + 1) +
                                              " eliminates candidate " + std::to_string(common_z) + " from common peers.";
                            steps.push_back(step);
                        }
                    }
                }
            }
        }

        return steps;
    }

    // 3. W-Wing (Hard, Score: 150)
    static std::vector<Step> find_w_wings(const BoardState& board) {
        std::vector<Step> steps;

        std::vector<int> bivalue_cells;
        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            if (board.is_unfilled(cell) && board.count_candidates(cell) == 2) {
                bivalue_cells.push_back(cell);
            }
        }

        size_t n = bivalue_cells.size();
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                int c1 = bivalue_cells[i];
                int c2 = bivalue_cells[j];

                CandidateMask m1 = board.get_candidates(c1);
                CandidateMask m2 = board.get_candidates(c2);

                if (m1 == m2 && !get_peer_bitset(c1).test(c2)) {
                    // Two identical bivalue cells not in the same house
                    int x = std::countr_zero(static_cast<unsigned int>(m1)) + 1;
                    int y = std::countr_zero(static_cast<unsigned int>(m1 & ~digit_to_mask(x))) + 1;

                    // Try both digits as bridge
                    for (int bridge_d : {x, y}) {
                        int elim_d = (bridge_d == x) ? y : x;

                        // Search for a strong link on bridge_d in any house
                        for (int h = 0; h < TOTAL_HOUSES; ++h) {
                            BitSet81 h_cands = board.get_candidates_in_house(h, bridge_d);
                            if (h_cands.count() == 2) {
                                int s1 = h_cands.pop_first_cell();
                                int s2 = h_cands.pop_first_cell();

                                bool c1_sees_s1 = get_peer_bitset(c1).test(s1) && (c1 != s1);
                                bool c2_sees_s2 = get_peer_bitset(c2).test(s2) && (c2 != s2);

                                bool c1_sees_s2 = get_peer_bitset(c1).test(s2) && (c1 != s2);
                                bool c2_sees_s1 = get_peer_bitset(c2).test(s1) && (c2 != s1);

                                if ((c1_sees_s1 && c2_sees_s2) || (c1_sees_s2 && c2_sees_s1)) {
                                    BitSet81 common = get_peer_bitset(c1) & get_peer_bitset(c2);
                                    std::vector<CandidateElimination> elims;

                                    common.for_each_cell([&](int target) {
                                        if (board.is_unfilled(target) && board.has_candidate(target, elim_d)) {
                                            elims.push_back({target, elim_d});
                                        }
                                    });

                                    if (!elims.empty()) {
                                        Step step;
                                        step.type = TechniqueType::Custom;
                                        step.name = "W-Wing";
                                        step.difficulty = DifficultyLevel::Hard;
                                        step.score = 150;
                                        step.primary_cells.set(c1);
                                        step.primary_cells.set(c2);
                                        step.secondary_cells.set(s1);
                                        step.secondary_cells.set(s2);
                                        step.eliminations = elims;

                                        step.explanation = "W-Wing: (" + std::to_string(x) + "/" + std::to_string(y) + ") at r" +
                                                          std::to_string(cell_row(c1) + 1) + "c" + std::to_string(cell_col(c1) + 1) +
                                                          " and r" + std::to_string(cell_row(c2) + 1) + "c" + std::to_string(cell_col(c2) + 1) +
                                                          " connected by strong link on " + std::to_string(bridge_d) +
                                                          " eliminates candidate " + std::to_string(elim_d) + " from common peers.";
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
