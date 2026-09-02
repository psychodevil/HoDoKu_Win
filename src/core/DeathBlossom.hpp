#pragma once

#include <vector>
#include <string>
#include "BoardState.hpp"
#include "Step.hpp"
#include "AlmostLockedSets.hpp"

namespace hodoku::core {

class DeathBlossom {
public:
    static std::vector<Step> find_death_blossom(const BoardState& board) {
        std::vector<Step> steps;

        auto all_als = AlmostLockedSets::find_all_als(board);
        if (all_als.empty()) return steps;

        // Stem cells: unfilled cells with 2 candidates (bivalue stem)
        for (int stem = 0; stem < TOTAL_CELLS; ++stem) {
            if (!board.is_unfilled(stem)) continue;
            if (board.count_candidates(stem) != 2) continue;

            CandidateMask stem_mask = board.get_candidates(stem);
            int d1 = get_single_digit(stem_mask & (stem_mask - 1)); // second bit
            int d0 = get_single_digit(stem_mask & ~digit_to_mask(d1)); // first bit

            // Petals for d0 and d1
            std::vector<size_t> petals0, petals1;

            for (size_t i = 0; i < all_als.size(); ++i) {
                const auto& als = all_als[i];
                if (als.cells.test(stem)) continue;

                // Check if als can be petal for d0:
                // Must contain d0, and stem must see all cells in als containing d0
                if (mask_has_digit(als.candidates, d0)) {
                    bool all_seen = true;
                    als.cells.for_each_cell([&](int c) {
                        if (board.has_candidate(c, d0) && !GRID.peer_bitsets[stem].test(c)) {
                            all_seen = false;
                        }
                    });
                    if (all_seen) petals0.push_back(i);
                }

                // Check if als can be petal for d1:
                if (mask_has_digit(als.candidates, d1)) {
                    bool all_seen = true;
                    als.cells.for_each_cell([&](int c) {
                        if (board.has_candidate(c, d1) && !GRID.peer_bitsets[stem].test(c)) {
                            all_seen = false;
                        }
                    });
                    if (all_seen) petals1.push_back(i);
                }
            }

            // Pairwise check petals
            for (size_t idx0 : petals0) {
                const auto& p0 = all_als[idx0];

                for (size_t idx1 : petals1) {
                    if (idx0 == idx1) continue;
                    const auto& p1 = all_als[idx1];

                    // Petals must be disjoint
                    if ((p0.cells & p1.cells).any()) continue;

                    // Common candidates z in both petals (excluding stem digits d0, d1)
                    CandidateMask common_z = (p0.candidates & p1.candidates) & ~stem_mask;
                    if (common_z == EMPTY_MASK) continue;

                    for (int z = 1; z <= 9; ++z) {
                        if (!mask_has_digit(common_z, z)) continue;

                        // Find all cells in p0 and p1 having candidate z
                        BitSet81 z_cells;
                        p0.cells.for_each_cell([&](int c) {
                            if (board.has_candidate(c, z)) z_cells.set(c);
                        });
                        p1.cells.for_each_cell([&](int c) {
                            if (board.has_candidate(c, z)) z_cells.set(c);
                        });

                        // Candidate elimination: any cell that sees all z_cells
                        BitSet81 common_peers = BitSet81::all();
                        z_cells.for_each_cell([&](int c) {
                            common_peers &= GRID.peer_bitsets[c];
                        });

                        common_peers.reset(stem);
                        common_peers &= ~p0.cells;
                        common_peers &= ~p1.cells;

                        std::vector<CandidateElimination> elims;
                        common_peers.for_each_cell([&](int target) {
                            if (board.is_unfilled(target) && board.has_candidate(target, z)) {
                                elims.push_back({target, z});
                            }
                        });

                        if (!elims.empty()) {
                            Step step;
                            step.type = TechniqueType::DeathBlossom;
                            step.name = "Death Blossom";
                            step.difficulty = DifficultyLevel::Extreme;
                            step.score = 260;

                            step.primary_cells.set(stem);
                            step.secondary_cells |= p0.cells;
                            step.secondary_cells |= p1.cells;
                            step.eliminations = elims;

                            // Add links from stem to petals
                            z_cells.for_each_cell([&](int c) {
                                step.links.push_back({stem, d0, c, z, true});
                            });

                            step.explanation = "Death Blossom with stem " + format_cell(stem) +
                                              " {" + std::to_string(d0) + "," + std::to_string(d1) +
                                              "} and petals in houses " + std::to_string(p0.house + 1) +
                                              " and " + std::to_string(p1.house + 1) +
                                              " eliminates digit " + std::to_string(z) + " from common peers.";

                            steps.push_back(step);
                            return steps; // Return first found
                        }
                    }
                }
            }
        }

        return steps;
    }
};

} // namespace hodoku::core
