#pragma once

#include "Types.hpp"
#include "BitSet81.hpp"
#include "GridConstants.hpp"
#include "BoardState.hpp"
#include "Step.hpp"
#include <vector>
#include <string>
#include <algorithm>

namespace hodoku::core {

class Uniqueness {
public:
    static std::vector<Step> find_all_steps(const BoardState& board) {
        std::vector<Step> steps;
        auto ur = find_unique_rectangles(board);
        steps.insert(steps.end(), ur.begin(), ur.end());
        auto bug = find_bug_plus_one(board);
        steps.insert(steps.end(), bug.begin(), bug.end());
        auto ar = find_avoidable_rectangles(board);
        steps.insert(steps.end(), ar.begin(), ar.end());
        return steps;
    }

    // 1. BUG + 1 (Bivalue Universal Grave Type 1)
    static std::vector<Step> find_bug_plus_one(const BoardState& board) {
        std::vector<Step> steps;
        int trivalueCell = -1;
        int trivalueCount = 0;

        for (int c = 0; c < TOTAL_CELLS; ++c) {
            if (!board.is_unfilled(c)) continue;
            int nCands = board.count_candidates(c);
            if (nCands < 2) return steps; // Invalid / already broken
            if (nCands > 3) return steps; // Not a BUG+1 if any cell has > 3 candidates
            if (nCands == 3) {
                trivalueCount++;
                if (trivalueCount > 1) return steps; // Only 1 trivalue cell allowed
                trivalueCell = c;
            }
        }

        if (trivalueCount != 1 || trivalueCell == -1) return steps;

        int targetDigit = -1;
        int r = cell_row(trivalueCell);
        int c = cell_col(trivalueCell);
        int b = cell_box(trivalueCell);

        CandidateMask mask = board.get_candidates(trivalueCell);

        for (int d = 1; d <= 9; ++d) {
            if (!mask_has_digit(mask, d)) continue;

            int countR = 0, countC = 0, countB = 0;
            for (int cell : GRID.row_cells[r]) {
                if (board.is_unfilled(cell) && board.has_candidate(cell, d)) countR++;
            }
            for (int cell : GRID.col_cells[c]) {
                if (board.is_unfilled(cell) && board.has_candidate(cell, d)) countC++;
            }
            for (int cell : GRID.box_cells[b]) {
                if (board.is_unfilled(cell) && board.has_candidate(cell, d)) countB++;
            }

            if (countR == 3 && countC == 3 && countB == 3) {
                targetDigit = d;
                break;
            }
        }

        if (targetDigit != -1) {
            Step s;
            s.type = TechniqueType::BUG;
            s.name = "Bivalue Universal Grave +1";
            s.difficulty = DifficultyLevel::Hard;
            s.score = 100;
            s.explanation = "All unfilled cells are bivalue except " + format_cell(trivalueCell) +
                            ". Digit " + std::to_string(targetDigit) + " appears 3 times in its row, column, and box; placing it prevents deadly multi-solution pattern.";

            s.assignments.push_back({trivalueCell, targetDigit});
            for (int d = 1; d <= 9; ++d) {
                if (d != targetDigit && mask_has_digit(mask, d)) {
                    s.eliminations.push_back({trivalueCell, d});
                }
            }
            s.primary_cells.set(trivalueCell);
            steps.push_back(s);
        }

        return steps;
    }

    // 2. Unique Rectangles (Types 1-6)
    static std::vector<Step> find_unique_rectangles(const BoardState& board) {
        std::vector<Step> steps;

        for (int r1 = 0; r1 < 8; ++r1) {
            for (int r2 = r1 + 1; r2 < 9; ++r2) {
                for (int c1 = 0; c1 < 8; ++c1) {
                    for (int c2 = c1 + 1; c2 < 9; ++c2) {
                        int p11 = cell_index(r1, c1);
                        int p12 = cell_index(r1, c2);
                        int p21 = cell_index(r2, c1);
                        int p22 = cell_index(r2, c2);

                        if (!board.is_unfilled(p11) || !board.is_unfilled(p12) ||
                            !board.is_unfilled(p21) || !board.is_unfilled(p22)) {
                            continue;
                        }

                        int b11 = cell_box(p11);
                        int b12 = cell_box(p12);
                        int b21 = cell_box(p21);
                        int b22 = cell_box(p22);

                        bool validBoxes = ((b11 == b12 && b21 == b22 && b11 != b21) ||
                                           (b11 == b21 && b12 == b22 && b11 != b12));
                        if (!validBoxes) continue;

                        int cells[4] = {p11, p12, p21, p22};
                        CandidateMask masks[4] = {
                            board.get_candidates(p11),
                            board.get_candidates(p12),
                            board.get_candidates(p21),
                            board.get_candidates(p22)
                        };

                        for (int d1 = 1; d1 < 9; ++d1) {
                            for (int d2 = d1 + 1; d2 <= 9; ++d2) {
                                if (!mask_has_digit(masks[0], d1) || !mask_has_digit(masks[0], d2) ||
                                    !mask_has_digit(masks[1], d1) || !mask_has_digit(masks[1], d2) ||
                                    !mask_has_digit(masks[2], d1) || !mask_has_digit(masks[2], d2) ||
                                    !mask_has_digit(masks[3], d1) || !mask_has_digit(masks[3], d2)) {
                                    continue;
                                }

                                check_ur_patterns(board, cells, masks, d1, d2, steps);
                            }
                        }
                    }
                }
            }
        }

        return steps;
    }

    // 3. Avoidable Rectangles (AR Type 1 & 2)
    static std::vector<Step> find_avoidable_rectangles(const BoardState& board) {
        std::vector<Step> steps;

        for (int r1 = 0; r1 < 8; ++r1) {
            for (int r2 = r1 + 1; r2 < 9; ++r2) {
                for (int c1 = 0; c1 < 8; ++c1) {
                    for (int c2 = c1 + 1; c2 < 9; ++c2) {
                        int p11 = cell_index(r1, c1);
                        int p12 = cell_index(r1, c2);
                        int p21 = cell_index(r2, c1);
                        int p22 = cell_index(r2, c2);

                        int b11 = cell_box(p11);
                        int b12 = cell_box(p12);
                        int b21 = cell_box(p21);
                        int b22 = cell_box(p22);
                        bool validBoxes = ((b11 == b12 && b21 == b22 && b11 != b21) ||
                                           (b11 == b21 && b12 == b22 && b11 != b12));
                        if (!validBoxes) continue;

                        int cells[4] = {p11, p12, p21, p22};
                        int solvedCount = 0;
                        int unfilledCell = -1;
                        bool hasGivens = false;

                        for (int c : cells) {
                            if (board.is_given(c)) { hasGivens = true; break; }
                            if (board.get_value(c) != 0) solvedCount++;
                            else unfilledCell = c;
                        }

                        if (hasGivens || solvedCount != 3 || unfilledCell == -1) continue;

                        std::vector<int> vals;
                        for (int c : cells) {
                            if (c != unfilledCell) vals.push_back(board.get_value(c));
                        }

                        int d1 = vals[0], d2 = -1;
                        if (vals[1] == d1) d2 = vals[2];
                        else if (vals[2] == d1) d2 = vals[1];
                        else if (vals[1] == vals[2]) { d2 = vals[1]; }
                        else continue;

                        int deadlyDigit = (std::count(vals.begin(), vals.end(), d1) == 1) ? d1 : d2;

                        if (board.has_candidate(unfilledCell, deadlyDigit)) {
                            Step s;
                            s.type = TechniqueType::AvoidableRectangle;
                            s.name = "Avoidable Rectangle Type 1";
                            s.difficulty = DifficultyLevel::Hard;
                            s.score = 100;
                            s.explanation = "Cells " + format_cell(cells[0]) + ", " + format_cell(cells[1]) + ", " +
                                            format_cell(cells[2]) + ", " + format_cell(cells[3]) +
                                            " form an Avoidable Rectangle on digits " + std::to_string(d1) + " and " +
                                            std::to_string(d2) + ". Candidate " + std::to_string(deadlyDigit) +
                                            " is eliminated from " + format_cell(unfilledCell) + " to preserve single solution.";
                            s.eliminations.push_back({unfilledCell, deadlyDigit});
                            for (int c : cells) s.primary_cells.set(c);

                            steps.push_back(s);
                        }
                    }
                }
            }
        }

        return steps;
    }

private:
    static void check_ur_patterns(const BoardState& board,
                                  const int cells[4],
                                  const CandidateMask masks[4],
                                  int d1, int d2,
                                  std::vector<Step>& steps) {
        CandidateMask baseMask = digit_to_mask(d1) | digit_to_mask(d2);

        std::vector<int> pureCells;
        std::vector<int> extraCells;

        for (int i = 0; i < 4; ++i) {
            if (masks[i] == baseMask) {
                pureCells.push_back(cells[i]);
            } else {
                extraCells.push_back(cells[i]);
            }
        }

        // --- UR Type 1 ---
        if (pureCells.size() == 3 && extraCells.size() == 1) {
            int extraCell = extraCells[0];
            Step s;
            s.type = TechniqueType::UniqueRectangle;
            s.name = "Unique Rectangle Type 1";
            s.difficulty = DifficultyLevel::Hard;
            s.score = 100;
            s.explanation = "Cells " + format_cell(cells[0]) + ", " + format_cell(cells[1]) + ", " +
                            format_cell(cells[2]) + ", " + format_cell(cells[3]) + " form a UR on {" +
                            std::to_string(d1) + ", " + std::to_string(d2) + "}. Candidates " +
                            std::to_string(d1) + " and " + std::to_string(d2) + " are eliminated from " +
                            format_cell(extraCell) + " to avoid deadly pattern.";

            s.eliminations.push_back({extraCell, d1});
            s.eliminations.push_back({extraCell, d2});
            for (int i = 0; i < 4; ++i) s.primary_cells.set(cells[i]);

            steps.push_back(s);
            return;
        }

        // --- UR Type 2 & Type 5 ---
        if (pureCells.size() == 2 && extraCells.size() == 2) {
            int e1 = extraCells[0];
            int e2 = extraCells[1];
            CandidateMask extra1 = masks_diff(board.get_candidates(e1), baseMask);
            CandidateMask extra2 = masks_diff(board.get_candidates(e2), baseMask);

            if (count_candidates(extra1) == 1 && extra1 == extra2) {
                int extraDigit = get_single_digit(extra1);

                BitSet81 commonPeers = GRID.peer_bitsets[e1] & GRID.peer_bitsets[e2];
                std::vector<CandidateElimination> elims;

                commonPeers.for_each_cell([&](int p) {
                    if (board.is_unfilled(p) && board.has_candidate(p, extraDigit)) {
                        elims.push_back({p, extraDigit});
                    }
                });

                if (!elims.empty()) {
                    bool sameLine = (cell_row(e1) == cell_row(e2) || cell_col(e1) == cell_col(e2));
                    std::string urName = sameLine ? "Unique Rectangle Type 2" : "Unique Rectangle Type 5";

                    Step s;
                    s.type = TechniqueType::UniqueRectangle;
                    s.name = urName;
                    s.difficulty = DifficultyLevel::Hard;
                    s.score = 100;
                    s.explanation = "UR on {" + std::to_string(d1) + ", " + std::to_string(d2) + "} in cells " +
                                    format_cell(cells[0]) + ", " + format_cell(cells[1]) + ", " +
                                    format_cell(cells[2]) + ", " + format_cell(cells[3]) + " with extra candidate " +
                                    std::to_string(extraDigit) + ". Eliminates " + std::to_string(extraDigit) + " from common peers.";

                    s.eliminations = elims;
                    for (int i = 0; i < 4; ++i) s.primary_cells.set(cells[i]);

                    steps.push_back(s);
                    return;
                }
            }
        }

        // --- UR Type 4 ---
        if (pureCells.size() == 2 && extraCells.size() == 2) {
            int e1 = extraCells[0];
            int e2 = extraCells[1];
            bool sameRow = (cell_row(e1) == cell_row(e2));
            bool sameCol = (cell_col(e1) == cell_col(e2));

            if (sameRow || sameCol) {
                int house = sameRow ? cell_row(e1) : (9 + cell_col(e1));
                const auto& houseCells = GRID.house_cells[house];

                for (int testDigit : {d1, d2}) {
                    int otherDigit = (testDigit == d1) ? d2 : d1;
                    int countInHouse = 0;
                    for (int hc : houseCells) {
                        if (board.is_unfilled(hc) && board.has_candidate(hc, testDigit)) {
                            countInHouse++;
                        }
                    }

                    if (countInHouse == 2) {
                        std::vector<CandidateElimination> elims;
                        if (board.has_candidate(e1, otherDigit)) elims.push_back({e1, otherDigit});
                        if (board.has_candidate(e2, otherDigit)) elims.push_back({e2, otherDigit});

                        if (!elims.empty()) {
                            Step s;
                            s.type = TechniqueType::UniqueRectangle;
                            s.name = "Unique Rectangle Type 4";
                            s.difficulty = DifficultyLevel::Hard;
                            s.score = 100;
                            s.explanation = "UR Type 4 on {" + std::to_string(d1) + ", " + std::to_string(d2) + "}. " +
                                            "Candidate " + std::to_string(testDigit) + " is locked in " +
                                            format_cell(e1) + " and " + format_cell(e2) + ", eliminating " +
                                            std::to_string(otherDigit) + " from both cells.";
                            s.eliminations = elims;
                            for (int i = 0; i < 4; ++i) s.primary_cells.set(cells[i]);

                            steps.push_back(s);
                            return;
                        }
                    }
                }
            }
        }

        // --- UR Type 3 (Subset) ---
        if (pureCells.size() == 2 && extraCells.size() == 2) {
            int e1 = extraCells[0];
            int e2 = extraCells[1];
            bool sameRow = (cell_row(e1) == cell_row(e2));
            bool sameCol = (cell_col(e1) == cell_col(e2));

            if (sameRow || sameCol) {
                int house = sameRow ? cell_row(e1) : (9 + cell_col(e1));
                CandidateMask extraMask = masks_diff(board.get_candidates(e1) | board.get_candidates(e2), baseMask);

                if (count_candidates(extraMask) == 2) {
                    for (int otherCell : GRID.house_cells[house]) {
                        if (otherCell == e1 || otherCell == e2 || !board.is_unfilled(otherCell)) continue;
                        CandidateMask om = board.get_candidates(otherCell);
                        if (count_candidates(om) >= 1 && (om & ~extraMask) == 0) {
                            std::vector<CandidateElimination> elims;
                            for (int target : GRID.house_cells[house]) {
                                if (target == e1 || target == e2 || target == otherCell || !board.is_unfilled(target)) continue;
                                CandidateMask tm = board.get_candidates(target) & extraMask;
                                for (int d = 1; d <= 9; ++d) {
                                    if (mask_has_digit(tm, d)) {
                                        elims.push_back({target, d});
                                    }
                                }
                            }

                            if (!elims.empty()) {
                                Step s;
                                s.type = TechniqueType::UniqueRectangle;
                                s.name = "Unique Rectangle Type 3";
                                s.difficulty = DifficultyLevel::Hard;
                                s.score = 100;
                                s.explanation = "UR Type 3 on {" + std::to_string(d1) + ", " + std::to_string(d2) +
                                                "} with extra candidates forming a locked subset with " +
                                                format_cell(otherCell) + ".";
                                s.eliminations = elims;
                                for (int i = 0; i < 4; ++i) s.primary_cells.set(cells[i]);
                                s.primary_cells.set(otherCell);

                                steps.push_back(s);
                                return;
                            }
                        }
                    }
                }
            }
        }
    }

    static CandidateMask masks_diff(CandidateMask m1, CandidateMask m2) {
        return m1 & ~m2;
    }
};

} // namespace hodoku::core
