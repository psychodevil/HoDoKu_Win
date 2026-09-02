#pragma once

#include <vector>
#include <string>
#include "BoardState.hpp"
#include "Step.hpp"

namespace hodoku::core {

class SimpleTechniques {
public:
    // 1. Find Naked Singles (cells with exactly 1 candidate)
    static std::vector<Step> find_naked_singles(const BoardState& board) {
        std::vector<Step> steps;

        board.get_unfilled_cells().for_each_cell([&](int cell) {
            CandidateMask mask = board.get_candidates(cell);
            if (count_candidates(mask) == 1) {
                int digit = get_single_digit(mask);
                int r = cell_row(cell) + 1;
                int c = cell_col(cell) + 1;

                Step step;
                step.type = TechniqueType::NakedSingle;
                step.name = "Naked Single";
                step.difficulty = DifficultyLevel::Easy;
                step.score = 4;
                step.explanation = "Cell r" + std::to_string(r) + "c" + std::to_string(c) +
                                  " has only one possible candidate: " + std::to_string(digit);
                step.primary_cells.set(cell);
                step.assignments.push_back({cell, digit});

                steps.push_back(step);
            }
        });

        return steps;
    }

    // 2. Find Hidden Singles (digits that appear only once in a house)
    static std::vector<Step> find_hidden_singles(const BoardState& board) {
        std::vector<Step> steps;

        for (int h = 0; h < TOTAL_HOUSES; ++h) {
            for (int d = 1; d <= 9; ++d) {
                BitSet81 cand_cells = board.get_candidates_in_house(h, d);
                if (cand_cells.count() == 1) {
                    int cell = cand_cells.first_cell();

                    // If this is also a naked single, skip to avoid duplicate simpler steps
                    if (board.count_candidates(cell) == 1) {
                        continue;
                    }

                    std::string house_name;
                    if (h < 9) {
                        house_name = "row " + std::to_string(h + 1);
                    } else if (h < 18) {
                        house_name = "column " + std::to_string(h - 9 + 1);
                    } else {
                        house_name = "box " + std::to_string(h - 18 + 1);
                    }

                    int r = cell_row(cell) + 1;
                    int c = cell_col(cell) + 1;

                    Step step;
                    step.type = TechniqueType::HiddenSingle;
                    step.name = "Hidden Single";
                    step.difficulty = DifficultyLevel::Easy;
                    step.score = 10;
                    step.explanation = "Digit " + std::to_string(d) + " in " + house_name +
                                      " can only be placed at r" + std::to_string(r) + "c" + std::to_string(c);
                    step.primary_cells.set(cell);
                    step.secondary_cells = GRID.house_bitsets[h];
                    step.assignments.push_back({cell, d});

                    steps.push_back(step);
                }
            }
        }

        return steps;
    }

    // 3. Find Locked Candidates (Pointing and Claiming)
    static std::vector<Step> find_locked_candidates(const BoardState& board) {
        std::vector<Step> steps;

        // Type 1: Pointing (All candidates for a digit in a box are restricted to a single row/col)
        for (int b = 0; b < 9; ++b) {
            int box_house = 18 + b;
            for (int d = 1; d <= 9; ++d) {
                BitSet81 in_box = board.get_candidates_in_house(box_house, d);
                int count = in_box.count();
                if (count >= 2 && count <= 3) {
                    // Check if all are in the same row
                    int first_cell = in_box.first_cell();
                    int r = cell_row(first_cell);
                    int c = cell_col(first_cell);

                    bool same_row = true;
                    bool same_col = true;

                    in_box.for_each_cell([&](int cell) {
                        if (cell_row(cell) != r) same_row = false;
                        if (cell_col(cell) != c) same_col = false;
                    });

                    if (same_row) {
                        int row_house = r;
                        BitSet81 row_cands = board.get_candidates_in_house(row_house, d);
                        BitSet81 elim_cells = row_cands & ~in_box;

                        if (!elim_cells.empty()) {
                            Step step;
                            step.type = TechniqueType::LockedCandidatesPointing;
                            step.name = "Locked Candidates (Pointing)";
                            step.difficulty = DifficultyLevel::Medium;
                            step.score = 50;
                            step.explanation = "In box " + std::to_string(b + 1) + ", digit " +
                                              std::to_string(d) + " is locked into row " + std::to_string(r + 1) +
                                              ", eliminating it from the rest of the row.";
                            step.primary_cells = in_box;
                            step.secondary_cells = GRID.box_bitsets[b];

                            elim_cells.for_each_cell([&](int elim_c) {
                                step.eliminations.push_back({elim_c, d});
                            });
                            steps.push_back(step);
                        }
                    }

                    if (same_col) {
                        int col_house = 9 + c;
                        BitSet81 col_cands = board.get_candidates_in_house(col_house, d);
                        BitSet81 elim_cells = col_cands & ~in_box;

                        if (!elim_cells.empty()) {
                            Step step;
                            step.type = TechniqueType::LockedCandidatesPointing;
                            step.name = "Locked Candidates (Pointing)";
                            step.difficulty = DifficultyLevel::Medium;
                            step.score = 50;
                            step.explanation = "In box " + std::to_string(b + 1) + ", digit " +
                                              std::to_string(d) + " is locked into column " + std::to_string(c + 1) +
                                              ", eliminating it from the rest of the column.";
                            step.primary_cells = in_box;
                            step.secondary_cells = GRID.box_bitsets[b];

                            elim_cells.for_each_cell([&](int elim_c) {
                                step.eliminations.push_back({elim_c, d});
                            });
                            steps.push_back(step);
                        }
                    }
                }
            }
        }

        // Type 2: Claiming (All candidates for a digit in a row/col are restricted to a single box)
        for (int line = 0; line < 18; ++line) {
            for (int d = 1; d <= 9; ++d) {
                BitSet81 in_line = board.get_candidates_in_house(line, d);
                int count = in_line.count();
                if (count >= 2 && count <= 3) {
                    int first_cell = in_line.first_cell();
                    int b = cell_box(first_cell);

                    bool same_box = true;
                    in_line.for_each_cell([&](int cell) {
                        if (cell_box(cell) != b) same_box = false;
                    });

                    if (same_box) {
                        int box_house = 18 + b;
                        BitSet81 box_cands = board.get_candidates_in_house(box_house, d);
                        BitSet81 elim_cells = box_cands & ~in_line;

                        if (!elim_cells.empty()) {
                            std::string line_desc = (line < 9) ? ("row " + std::to_string(line + 1))
                                                               : ("column " + std::to_string(line - 9 + 1));

                            Step step;
                            step.type = TechniqueType::LockedCandidatesClaiming;
                            step.name = "Locked Candidates (Claiming)";
                            step.difficulty = DifficultyLevel::Medium;
                            step.score = 50;
                            step.explanation = "In " + line_desc + ", digit " + std::to_string(d) +
                                              " is locked into box " + std::to_string(b + 1) +
                                              ", eliminating it from the rest of the box.";
                            step.primary_cells = in_line;
                            step.secondary_cells = GRID.house_bitsets[line];

                            elim_cells.for_each_cell([&](int elim_c) {
                                step.eliminations.push_back({elim_c, d});
                            });
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

