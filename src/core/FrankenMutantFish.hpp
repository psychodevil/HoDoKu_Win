#pragma once

#include <vector>
#include <string>
#include "BoardState.hpp"
#include "Step.hpp"

namespace hodoku::core {

class FrankenMutantFish {
public:
    static std::vector<Step> find_franken_fish(const BoardState& board) {
        std::vector<Step> steps;

        for (int d = 1; d <= 9; ++d) {
            // Find Franken Fish of size 2 (Franken X-Wing)
            // Base: 1 row/col + 1 box (total 2 sets)
            // Cover: 2 cols/rows (total 2 sets)
            for (int line1 = 0; line1 < 18; ++line1) {
                BitSet81 cands1 = board.get_candidates_in_house(line1, d);
                if (cands1.count() < 2 || cands1.count() > 5) continue;

                for (int b = 18; b < 27; ++b) {
                    // Box must NOT intersect line1 (disjoint base houses)
                    if (GRID.house_bitsets[line1].intersects(GRID.house_bitsets[b])) {
                        continue;
                    }

                    BitSet81 candsB = board.get_candidates_in_house(b, d);
                    if (candsB.count() < 2 || candsB.count() > 5) continue;

                    BitSet81 base_cands = cands1 | candsB;
                    if (base_cands.count() < 4) continue;

                    // Search for 2 cover houses from orthogonal lines
                    int cover_start = (line1 < 9) ? 9 : 0;
                    int cover_end = cover_start + 9;

                    for (int c1 = cover_start; c1 < cover_end; ++c1) {
                        for (int c2 = c1 + 1; c2 < cover_end; ++c2) {
                            BitSet81 cover_mask = GRID.house_bitsets[c1] | GRID.house_bitsets[c2];
                            BitSet81 fins = base_cands & ~cover_mask;

                            if (fins.empty()) {
                                // Basic Franken Fish
                                BitSet81 elims = (cover_mask & board.get_cells_with_candidate(d)) & ~base_cands;
                                if (elims.any()) {
                                    Step step;
                                    step.type = TechniqueType::FrankenFish;
                                    step.difficulty = DifficultyLevel::Extreme;
                                    step.score = 220;
                                    step.name = "Franken X-Wing";

                                    base_cands.for_each_cell([&](int c) { step.primary_cells.set(c); });
                                    elims.for_each_cell([&](int c) {
                                        step.eliminations.push_back({c, d});
                                    });

                                    step.explanation = "Franken X-Wing on digit " + std::to_string(d) +
                                                       " eliminates candidate " + std::to_string(d) +
                                                       " from " + std::to_string(step.eliminations.size()) + " cell(s).";

                                    steps.push_back(step);
                                    return steps;
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
