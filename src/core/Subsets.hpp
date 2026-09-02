#pragma once

#include <vector>
#include <string>
#include <array>
#include "BoardState.hpp"
#include "Step.hpp"

namespace hodoku::core {

class Subsets {
public:
    // 1. Naked Pairs (2 cells in a house sharing the exact same 2 candidates)
    static std::vector<Step> find_naked_pairs(const BoardState& board) {
        std::vector<Step> steps;

        for (int h = 0; h < TOTAL_HOUSES; ++h) {
            const auto& house_cells = GRID.house_cells[h];

            std::vector<int> bivalue_cells;
            for (int cell : house_cells) {
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

                    if (m1 == m2) {
                        std::vector<int> digits;
                        for (int d = 1; d <= 9; ++d) {
                            if (mask_has_digit(m1, d)) digits.push_back(d);
                        }

                        BitSet81 pair_cells;
                        pair_cells.set(c1);
                        pair_cells.set(c2);

                        std::vector<CandidateElimination> elims;
                        for (int cell : house_cells) {
                            if (cell != c1 && cell != c2 && board.is_unfilled(cell)) {
                                for (int d : digits) {
                                    if (board.has_candidate(cell, d)) {
                                        elims.push_back({cell, d});
                                    }
                                }
                            }
                        }

                        if (!elims.empty()) {
                            Step step;
                            step.type = TechniqueType::NakedPair;
                            step.name = "Naked Pair";
                            step.difficulty = DifficultyLevel::Medium;
                            step.score = 60;
                            step.primary_cells = pair_cells;
                            step.secondary_cells = GRID.house_bitsets[h];
                            step.eliminations = elims;

                            std::string house_name = get_house_name(h);
                            step.explanation = "Naked Pair (" + std::to_string(digits[0]) + "," + std::to_string(digits[1]) +
                                              ") in " + house_name + " at r" + std::to_string(cell_row(c1) + 1) + "c" + std::to_string(cell_col(c1) + 1) +
                                              " and r" + std::to_string(cell_row(c2) + 1) + "c" + std::to_string(cell_col(c2) + 1) +
                                              " eliminates candidates from other cells in " + house_name + ".";
                            steps.push_back(step);
                        }
                    }
                }
            }
        }

        return steps;
    }

    // 2. Hidden Pairs (2 digits appearing in only 2 cells of a house)
    static std::vector<Step> find_hidden_pairs(const BoardState& board) {
        std::vector<Step> steps;

        for (int h = 0; h < TOTAL_HOUSES; ++h) {
            for (int d1 = 1; d1 <= 8; ++d1) {
                BitSet81 c1 = board.get_candidates_in_house(h, d1);
                if (c1.count() != 2) continue;

                for (int d2 = d1 + 1; d2 <= 9; ++d2) {
                    BitSet81 c2 = board.get_candidates_in_house(h, d2);
                    if (c1 == c2) {
                        BitSet81 temp_c = c1;
                        int cellA = temp_c.pop_first_cell();
                        int cellB = temp_c.pop_first_cell();

                        CandidateMask pair_mask = digit_to_mask(d1) | digit_to_mask(d2);

                        std::vector<CandidateElimination> elims;
                        for (int cell : {cellA, cellB}) {
                            CandidateMask extra = board.get_candidates(cell) & ~pair_mask;
                            for (int d = 1; d <= 9; ++d) {
                                if (mask_has_digit(extra, d)) {
                                    elims.push_back({cell, d});
                                }
                            }
                        }

                        if (!elims.empty()) {
                            Step step;
                            step.type = TechniqueType::HiddenPair;
                            step.name = "Hidden Pair";
                            step.difficulty = DifficultyLevel::Medium;
                            step.score = 70;
                            step.primary_cells = c1;
                            step.secondary_cells = GRID.house_bitsets[h];
                            step.eliminations = elims;

                            std::string house_name = get_house_name(h);
                            step.explanation = "Hidden Pair (" + std::to_string(d1) + "," + std::to_string(d2) +
                                              ") in " + house_name + " at r" + std::to_string(cell_row(cellA) + 1) + "c" + std::to_string(cell_col(cellA) + 1) +
                                              " and r" + std::to_string(cell_row(cellB) + 1) + "c" + std::to_string(cell_col(cellB) + 1) +
                                              " eliminates all other candidates from these cells.";
                            steps.push_back(step);
                        }
                    }
                }
            }
        }

        return steps;
    }

    // 3. Naked Triples (3 cells in a house containing at most 3 distinct candidates)
    static std::vector<Step> find_naked_triples(const BoardState& board) {
        std::vector<Step> steps;

        for (int h = 0; h < TOTAL_HOUSES; ++h) {
            const auto& house_cells = GRID.house_cells[h];

            std::vector<int> candidates_cells;
            for (int cell : house_cells) {
                int count = board.count_candidates(cell);
                if (board.is_unfilled(cell) && count >= 2 && count <= 3) {
                    candidates_cells.push_back(cell);
                }
            }

            size_t n = candidates_cells.size();
            if (n < 3) continue;

            for (size_t i = 0; i < n; ++i) {
                for (size_t j = i + 1; j < n; ++j) {
                    for (size_t k = j + 1; k < n; ++k) {
                        int c1 = candidates_cells[i];
                        int c2 = candidates_cells[j];
                        int c3 = candidates_cells[k];

                        CandidateMask combined = board.get_candidates(c1) |
                                                 board.get_candidates(c2) |
                                                 board.get_candidates(c3);

                        if (count_candidates(combined) == 3) {
                            std::vector<int> digits;
                            for (int d = 1; d <= 9; ++d) {
                                if (mask_has_digit(combined, d)) digits.push_back(d);
                            }

                            BitSet81 triple_cells;
                            triple_cells.set(c1);
                            triple_cells.set(c2);
                            triple_cells.set(c3);

                            std::vector<CandidateElimination> elims;
                            for (int cell : house_cells) {
                                if (cell != c1 && cell != c2 && cell != c3 && board.is_unfilled(cell)) {
                                    for (int d : digits) {
                                        if (board.has_candidate(cell, d)) {
                                            elims.push_back({cell, d});
                                        }
                                    }
                                }
                            }

                            if (!elims.empty()) {
                                Step step;
                                step.type = TechniqueType::NakedTriple;
                                step.name = "Naked Triple";
                                step.difficulty = DifficultyLevel::Medium;
                                step.score = 80;
                                step.primary_cells = triple_cells;
                                step.secondary_cells = GRID.house_bitsets[h];
                                step.eliminations = elims;

                                std::string house_name = get_house_name(h);
                                step.explanation = "Naked Triple (" + std::to_string(digits[0]) + "," +
                                                  std::to_string(digits[1]) + "," + std::to_string(digits[2]) +
                                                  ") in " + house_name + " eliminates candidates from other cells in " + house_name + ".";
                                steps.push_back(step);
                            }
                        }
                    }
                }
            }
        }

        return steps;
    }

    // 4. Hidden Triples (3 digits appearing in only 3 cells of a house)
    static std::vector<Step> find_hidden_triples(const BoardState& board) {
        std::vector<Step> steps;

        for (int h = 0; h < TOTAL_HOUSES; ++h) {
            for (int d1 = 1; d1 <= 7; ++d1) {
                BitSet81 b1 = board.get_candidates_in_house(h, d1);
                if (b1.count() < 2 || b1.count() > 3) continue;

                for (int d2 = d1 + 1; d2 <= 8; ++d2) {
                    BitSet81 b2 = board.get_candidates_in_house(h, d2);
                    if (b2.count() < 2 || b2.count() > 3) continue;

                    for (int d3 = d2 + 1; d3 <= 9; ++d3) {
                        BitSet81 b3 = board.get_candidates_in_house(h, d3);
                        if (b3.count() < 2 || b3.count() > 3) continue;

                        BitSet81 combined = b1 | b2 | b3;
                        if (combined.count() == 3) {
                            CandidateMask triple_mask = digit_to_mask(d1) | digit_to_mask(d2) | digit_to_mask(d3);

                            std::vector<CandidateElimination> elims;
                            combined.for_each_cell([&](int cell) {
                                CandidateMask extra = board.get_candidates(cell) & ~triple_mask;
                                for (int d = 1; d <= 9; ++d) {
                                    if (mask_has_digit(extra, d)) {
                                        elims.push_back({cell, d});
                                    }
                                }
                            });

                            if (!elims.empty()) {
                                Step step;
                                step.type = TechniqueType::HiddenTriple;
                                step.name = "Hidden Triple";
                                step.difficulty = DifficultyLevel::Medium;
                                step.score = 100;
                                step.primary_cells = combined;
                                step.secondary_cells = GRID.house_bitsets[h];
                                step.eliminations = elims;

                                std::string house_name = get_house_name(h);
                                step.explanation = "Hidden Triple (" + std::to_string(d1) + "," +
                                                  std::to_string(d2) + "," + std::to_string(d3) +
                                                  ") in " + house_name + " eliminates other candidates from these 3 cells.";
                                steps.push_back(step);
                            }
                        }
                    }
                }
            }
        }

        return steps;
    }

    // 5. Naked Quads (4 cells in a house containing at most 4 distinct candidates)
    static std::vector<Step> find_naked_quads(const BoardState& board) {
        std::vector<Step> steps;

        for (int h = 0; h < TOTAL_HOUSES; ++h) {
            const auto& house_cells = GRID.house_cells[h];

            std::vector<int> candidates_cells;
            for (int cell : house_cells) {
                int count = board.count_candidates(cell);
                if (board.is_unfilled(cell) && count >= 2 && count <= 4) {
                    candidates_cells.push_back(cell);
                }
            }

            size_t n = candidates_cells.size();
            if (n < 4) continue;

            for (size_t i = 0; i < n; ++i) {
                for (size_t j = i + 1; j < n; ++j) {
                    for (size_t k = j + 1; k < n; ++k) {
                        for (size_t l = k + 1; l < n; ++l) {
                            int c1 = candidates_cells[i];
                            int c2 = candidates_cells[j];
                            int c3 = candidates_cells[k];
                            int c4 = candidates_cells[l];

                            CandidateMask combined = board.get_candidates(c1) |
                                                     board.get_candidates(c2) |
                                                     board.get_candidates(c3) |
                                                     board.get_candidates(c4);

                            if (count_candidates(combined) == 4) {
                                std::vector<int> digits;
                                for (int d = 1; d <= 9; ++d) {
                                    if (mask_has_digit(combined, d)) digits.push_back(d);
                                }

                                BitSet81 quad_cells;
                                quad_cells.set(c1);
                                quad_cells.set(c2);
                                quad_cells.set(c3);
                                quad_cells.set(c4);

                                std::vector<CandidateElimination> elims;
                                for (int cell : house_cells) {
                                    if (cell != c1 && cell != c2 && cell != c3 && cell != c4 && board.is_unfilled(cell)) {
                                        for (int d : digits) {
                                            if (board.has_candidate(cell, d)) {
                                                elims.push_back({cell, d});
                                            }
                                        }
                                    }
                                }

                                if (!elims.empty()) {
                                    Step step;
                                    step.type = TechniqueType::NakedQuadruple;
                                    step.name = "Naked Quadruple";
                                    step.difficulty = DifficultyLevel::Hard;
                                    step.score = 120;
                                    step.primary_cells = quad_cells;
                                    step.secondary_cells = GRID.house_bitsets[h];
                                    step.eliminations = elims;

                                    std::string house_name = get_house_name(h);
                                    step.explanation = "Naked Quadruple in " + house_name + " eliminates candidates from other cells in " + house_name + ".";
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

    // 6. Hidden Quads (4 digits appearing in only 4 cells of a house)
    static std::vector<Step> find_hidden_quads(const BoardState& board) {
        std::vector<Step> steps;

        for (int h = 0; h < TOTAL_HOUSES; ++h) {
            for (int d1 = 1; d1 <= 6; ++d1) {
                BitSet81 b1 = board.get_candidates_in_house(h, d1);
                if (b1.count() < 2 || b1.count() > 4) continue;

                for (int d2 = d1 + 1; d2 <= 7; ++d2) {
                    BitSet81 b2 = board.get_candidates_in_house(h, d2);
                    if (b2.count() < 2 || b2.count() > 4) continue;

                    for (int d3 = d2 + 1; d3 <= 8; ++d3) {
                        BitSet81 b3 = board.get_candidates_in_house(h, d3);
                        if (b3.count() < 2 || b3.count() > 4) continue;

                        for (int d4 = d3 + 1; d4 <= 9; ++d4) {
                            BitSet81 b4 = board.get_candidates_in_house(h, d4);
                            if (b4.count() < 2 || b4.count() > 4) continue;

                            BitSet81 combined = b1 | b2 | b3 | b4;
                            if (combined.count() == 4) {
                                CandidateMask quad_mask = digit_to_mask(d1) | digit_to_mask(d2) |
                                                          digit_to_mask(d3) | digit_to_mask(d4);

                                std::vector<CandidateElimination> elims;
                                combined.for_each_cell([&](int cell) {
                                    CandidateMask extra = board.get_candidates(cell) & ~quad_mask;
                                    for (int d = 1; d <= 9; ++d) {
                                        if (mask_has_digit(extra, d)) {
                                            elims.push_back({cell, d});
                                        }
                                    }
                                });

                                if (!elims.empty()) {
                                    Step step;
                                    step.type = TechniqueType::HiddenQuadruple;
                                    step.name = "Hidden Quadruple";
                                    step.difficulty = DifficultyLevel::Hard;
                                    step.score = 150;
                                    step.primary_cells = combined;
                                    step.secondary_cells = GRID.house_bitsets[h];
                                    step.eliminations = elims;

                                    std::string house_name = get_house_name(h);
                                    step.explanation = "Hidden Quadruple in " + house_name + " eliminates other candidates from these 4 cells.";
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

private:
    static std::string get_house_name(int h) {
        if (h < 9) return "row " + std::to_string(h + 1);
        if (h < 18) return "column " + std::to_string(h - 9 + 1);
        return "box " + std::to_string(h - 18 + 1);
    }
};

} // namespace hodoku::core
