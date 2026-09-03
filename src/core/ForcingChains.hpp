#pragma once

#include <vector>
#include <array>
#include <optional>
#include <string>

#include "Types.hpp"
#include "BitSet81.hpp"
#include "BoardState.hpp"
#include "Step.hpp"

#if defined(__AVX2__)
#include <immintrin.h>
#endif

namespace hodoku::core {

class ForcingChains {
public:
    // Propagates singles on a simulation board until fixed point or contradiction
    static bool propagate_singles(BoardState& sim) {
        bool changed = true;
        while (changed) {
            changed = false;
            if (sim.has_contradiction()) return false;

            // 1. Naked Singles
            for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
                if (sim.is_unfilled(cell)) {
                    int c_cnt = sim.count_candidates(cell);
                    if (c_cnt == 0) return false; // Contradiction
                    if (c_cnt == 1) {
                        int d = get_single_digit(sim.get_candidates(cell));
                        if (!sim.set_value(cell, d)) return false;
                        changed = true;
                    }
                }
            }

            if (sim.has_contradiction()) return false;

            // 2. Hidden Singles
            for (int h = 0; h < TOTAL_HOUSES; ++h) {
                for (int d = 1; d <= 9; ++d) {
                    if (sim.get_house_candidate_count(h, d) == 1) {
                        BitSet81 cand_cells = sim.get_candidates_in_house(h, d);
                        int cell = cand_cells.first_cell();
                        if (cell != -1 && sim.is_unfilled(cell)) {
                            if (!sim.set_value(cell, d)) return false;
                            changed = true;
                        }
                    }
                }
            }
        }
        return !sim.has_contradiction();
    }

    // 1. Contradiction / Digit Forcing Chains: If (c = d) causes a contradiction, eliminate (c, d)
    static std::vector<Step> find_contradiction_chains(const BoardState& board) {
        std::vector<Step> steps;

        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            if (!board.is_unfilled(cell)) continue;

            CandidateMask mask = board.get_candidates(cell);
            for (int d = 1; d <= 9; ++d) {
                if (!mask_has_digit(mask, d)) continue;

                BoardState sim = board;
                sim.set_value(cell, d);

                bool valid = propagate_singles(sim);
                if (!valid || sim.has_contradiction()) {
                    Step step;
                    step.type = TechniqueType::Custom;
                    step.name = "Forcing Chain (Contradiction)";
                    step.difficulty = DifficultyLevel::Extreme;
                    step.score = 280;
                    step.explanation = "Setting r" + std::to_string(cell_row(cell) + 1) +
                                       "c" + std::to_string(cell_col(cell) + 1) + " = " +
                                       std::to_string(d) + " leads to a contradiction.";
                    step.primary_cells.set(cell);
                    step.eliminations.push_back(CandidateElimination{cell, d});
                    steps.push_back(std::move(step));
                }
            }
        }

        return steps;
    }

    // 2. Cell Forcing Chains: If every candidate in cell c leads to eliminating (x, y), eliminate (x, y)
    static std::vector<Step> find_cell_forcing_chains(const BoardState& board) {
        std::vector<Step> steps;

        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            if (!board.is_unfilled(cell)) continue;

            int cand_count = board.count_candidates(cell);
            if (cand_count < 2 || cand_count > 4) continue; // Focus on bi/tri-value cells for efficiency

            CandidateMask mask = board.get_candidates(cell);
            std::vector<int> digits;
            for (int d = 1; d <= 9; ++d) {
                if (mask_has_digit(mask, d)) digits.push_back(d);
            }

            std::vector<std::array<CandidateMask, 96>> branch_eliminations;
            bool all_branches_valid = true;

            for (int d : digits) {
                BoardState sim = board;
                sim.set_value(cell, d);
                if (!propagate_singles(sim)) {
                    all_branches_valid = false;
                    break;
                }

                alignas(32) std::array<CandidateMask, 96> branch_elims{};
                branch_elims.fill(EMPTY_MASK);
                for (int c2 = 0; c2 < TOTAL_CELLS; ++c2) {
                    if (board.is_unfilled(c2) && sim.is_unfilled(c2)) {
                        CandidateMask orig = board.get_candidates(c2);
                        CandidateMask curr = sim.get_candidates(c2);
                        branch_elims[c2] = orig & ~curr;
                    }
                }
                branch_eliminations.push_back(branch_elims);
            }

            if (!all_branches_valid || branch_eliminations.empty()) continue;

            // Intersect eliminations across all branches
            alignas(32) std::array<CandidateMask, 96> common_elims = branch_eliminations[0];
#if defined(__AVX2__)
            for (size_t i = 1; i < branch_eliminations.size(); ++i) {
                __m256i* dest = reinterpret_cast<__m256i*>(common_elims.data());
                const __m256i* src = reinterpret_cast<const __m256i*>(branch_eliminations[i].data());
                for (int k = 0; k < 6; ++k) {
                    dest[k] = _mm256_and_si256(dest[k], src[k]);
                }
            }
#else
            for (size_t i = 1; i < branch_eliminations.size(); ++i) {
                for (int c2 = 0; c2 < TOTAL_CELLS; ++c2) {
                    common_elims[c2] &= branch_eliminations[i][c2];
                }
            }
#endif

            Step step;
            step.type = TechniqueType::Custom;
            step.name = "Cell Forcing Chain";
            step.difficulty = DifficultyLevel::Extreme;
            step.score = 300;
            step.primary_cells.set(cell);

            for (int c2 = 0; c2 < TOTAL_CELLS; ++c2) {
                if (common_elims[c2] != EMPTY_MASK) {
                    for (int d = 1; d <= 9; ++d) {
                        if (mask_has_digit(common_elims[c2], d)) {
                            step.eliminations.push_back(CandidateElimination{c2, d});
                            step.secondary_cells.set(c2);
                        }
                    }
                }
            }

            if (!step.eliminations.empty()) {
                step.explanation = "Cell Forcing Chain on r" + std::to_string(cell_row(cell) + 1) +
                                   "c" + std::to_string(cell_col(cell) + 1) + ": all candidates lead to common eliminations.";
                steps.push_back(std::move(step));
            }
        }

        return steps;
    }

    // 3. Region / House Forcing Chains
    static std::vector<Step> find_house_forcing_chains(const BoardState& board) {
        std::vector<Step> steps;

        for (int h = 0; h < TOTAL_HOUSES; ++h) {
            for (int d = 1; d <= 9; ++d) {
                int count = board.get_house_candidate_count(h, d);
                if (count != 2) continue; // Binary choice in house for maximum speed

                BitSet81 cands = board.get_candidates_in_house(h, d);
                std::vector<int> cand_cells;
                cands.for_each_cell([&](int c) { cand_cells.push_back(c); });
                if (cand_cells.size() != 2) continue;

                int c1 = cand_cells[0];
                int c2 = cand_cells[1];

                BoardState sim1 = board;
                sim1.set_value(c1, d);
                if (!propagate_singles(sim1)) continue;

                BoardState sim2 = board;
                sim2.set_value(c2, d);
                if (!propagate_singles(sim2)) continue;

                Step step;
                step.type = TechniqueType::Custom;
                step.name = "Region Forcing Chain";
                step.difficulty = DifficultyLevel::Extreme;
                step.score = 320;
                step.primary_cells.set(c1);
                step.primary_cells.set(c2);

                for (int test_c = 0; test_c < TOTAL_CELLS; ++test_c) {
                    if (board.is_unfilled(test_c) && sim1.is_unfilled(test_c) && sim2.is_unfilled(test_c)) {
                        CandidateMask elims1 = board.get_candidates(test_c) & ~sim1.get_candidates(test_c);
                        CandidateMask elims2 = board.get_candidates(test_c) & ~sim2.get_candidates(test_c);
                        CandidateMask common = elims1 & elims2;

                        if (common != EMPTY_MASK) {
                            for (int rem_d = 1; rem_d <= 9; ++rem_d) {
                                if (mask_has_digit(common, rem_d)) {
                                    step.eliminations.push_back(CandidateElimination{test_c, rem_d});
                                    step.secondary_cells.set(test_c);
                                }
                            }
                        }
                    }
                }

                if (!step.eliminations.empty()) {
                    step.explanation = "Region Forcing Chain on digit " + std::to_string(d) +
                                       " in house " + std::to_string(h + 1) + ": both placements eliminate candidates.";
                    steps.push_back(std::move(step));
                }
            }
        }

        return steps;
    }
};

} // namespace hodoku::core
