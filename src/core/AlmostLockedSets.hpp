#pragma once

#include <vector>
#include <string>
#include "BoardState.hpp"
#include "Step.hpp"

namespace hodoku::core {

struct ALS {
    int house{-1};
    BitSet81 cells;
    CandidateMask candidates{0};
    int size{0}; // number of cells
};

class AlmostLockedSets {
public:
    // Finds all ALSs in all houses (size 1 to 4)
    static std::vector<ALS> find_all_als(const BoardState& board) {
        std::vector<ALS> result;

        for (int h = 0; h < TOTAL_HOUSES; ++h) {
            std::vector<int> house_unfilled;
            for (int c : GRID.house_cells[h]) {
                if (board.is_unfilled(c)) {
                    house_unfilled.push_back(c);
                }
            }

            size_t n = house_unfilled.size();
            if (n < 2) continue;

            // Size 1 ALS: 1 cell with 2 candidates
            for (size_t i = 0; i < n; ++i) {
                int c = house_unfilled[i];
                if (board.count_candidates(c) == 2) {
                    ALS als;
                    als.house = h;
                    als.cells.set(c);
                    als.candidates = board.get_candidates(c);
                    als.size = 1;
                    result.push_back(als);
                }
            }

            // Size 2 ALS: 2 cells with 3 candidates
            for (size_t i = 0; i < n; ++i) {
                for (size_t j = i + 1; j < n; ++j) {
                    int c1 = house_unfilled[i];
                    int c2 = house_unfilled[j];
                    CandidateMask m = board.get_candidates(c1) | board.get_candidates(c2);
                    if (count_candidates(m) == 3) {
                        ALS als;
                        als.house = h;
                        als.cells.set(c1);
                        als.cells.set(c2);
                        als.candidates = m;
                        als.size = 2;
                        result.push_back(als);
                    }
                }
            }

            // Size 3 ALS: 3 cells with 4 candidates
            for (size_t i = 0; i < n; ++i) {
                for (size_t j = i + 1; j < n; ++j) {
                    for (size_t k = j + 1; k < n; ++k) {
                        int c1 = house_unfilled[i];
                        int c2 = house_unfilled[j];
                        int c3 = house_unfilled[k];
                        CandidateMask m = board.get_candidates(c1) | board.get_candidates(c2) | board.get_candidates(c3);
                        if (count_candidates(m) == 4) {
                            ALS als;
                            als.house = h;
                            als.cells.set(c1);
                            als.cells.set(c2);
                            als.cells.set(c3);
                            als.candidates = m;
                            als.size = 3;
                            result.push_back(als);
                        }
                    }
                }
            }
        }

        return result;
    }

    // 1. ALS-XZ Singly & Doubly Linked (Extreme, Score: 200 - 250)
    static std::vector<Step> find_als_xz(const BoardState& board) {
        std::vector<Step> steps;
        auto als_list = find_all_als(board);
        size_t n = als_list.size();

        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                const auto& a1 = als_list[i];
                const auto& a2 = als_list[j];

                // ALSs must be disjoint
                if ((a1.cells & a2.cells).any()) continue;

                CandidateMask common = a1.candidates & a2.candidates;
                if (count_candidates(common) < 2) continue;

                // Check for Restricted Common Candidates (RCC)
                std::vector<int> rccs;
                for (int d = 1; d <= 9; ++d) {
                    if (!mask_has_digit(common, d)) continue;

                    // d is RCC if all cells with d in a1 see all cells with d in a2
                    bool is_rcc = true;
                    a1.cells.for_each_cell([&](int c1) {
                        if (board.has_candidate(c1, d)) {
                            a2.cells.for_each_cell([&](int c2) {
                                if (board.has_candidate(c2, d)) {
                                    if (!get_peer_bitset(c1).test(c2)) {
                                        is_rcc = false;
                                    }
                                }
                            });
                        }
                    });

                    if (is_rcc) {
                        rccs.push_back(d);
                    }
                }

                // Singly-Linked ALS-XZ: Exactly 1 RCC
                if (rccs.size() == 1) {
                    int x = rccs[0];

                    // Any other common candidate z != x can be eliminated from common peers
                    for (int z = 1; z <= 9; ++z) {
                        if (z == x || !mask_has_digit(common, z)) continue;

                        BitSet81 seen_z1;
                        bool first1 = true;
                        a1.cells.for_each_cell([&](int c1) {
                            if (board.has_candidate(c1, z)) {
                                if (first1) { seen_z1 = get_peer_bitset(c1); first1 = false; }
                                else seen_z1 &= get_peer_bitset(c1);
                            }
                        });

                        BitSet81 seen_z2;
                        bool first2 = true;
                        a2.cells.for_each_cell([&](int c2) {
                            if (board.has_candidate(c2, z)) {
                                if (first2) { seen_z2 = get_peer_bitset(c2); first2 = false; }
                                else seen_z2 &= get_peer_bitset(c2);
                            }
                        });

                        BitSet81 target_peers = seen_z1 & seen_z2;
                        target_peers &= ~(a1.cells | a2.cells);

                        std::vector<CandidateElimination> elims;
                        target_peers.for_each_cell([&](int target) {
                            if (board.is_unfilled(target) && board.has_candidate(target, z)) {
                                elims.push_back({target, z});
                            }
                        });

                        if (!elims.empty()) {
                            Step step;
                            step.type = TechniqueType::AlsXz;
                            step.name = "ALS-XZ (Singly Linked)";
                            step.difficulty = DifficultyLevel::Extreme;
                            step.score = 200;
                            step.primary_cells = a1.cells;
                            step.secondary_cells = a2.cells;
                            step.eliminations = elims;

                            step.explanation = "ALS-XZ with RCC " + std::to_string(x) +
                                              " eliminates candidate " + std::to_string(z) +
                                              " from common peers of both ALSs.";
                            steps.push_back(step);
                        }
                    }
                }
            }
        }

        return steps;
    }
};

} // namespace hodoku::core

