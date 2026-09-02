#pragma once

#include <vector>
#include <array>
#include "Types.hpp"
#include "BitSet81.hpp"
#include "BoardState.hpp"
#include "Step.hpp"

namespace hodoku::core {

class Templates {
public:
    static std::vector<Step> find_template_steps(const BoardState& board) {
        std::vector<Step> steps;

        for (int d = 1; d <= 9; ++d) {
            BitSet81 current_d = board.get_cells_with_candidate(d);
            // Include cells that already have value d
            BitSet81 fixed_d;
            for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
                if (board.get_value(cell) == d) {
                    fixed_d.set(cell);
                    current_d.set(cell);
                }
            }

            if (current_d.count() == 0) continue;

            // Find all valid templates for digit d
            std::vector<BitSet81> valid_templates;
            BitSet81 current_template;
            uint16_t used_cols = 0;
            uint16_t used_boxes = 0;

            auto search_templates = [&](auto& self, int r) -> void {
                if (valid_templates.size() >= 500) return; // upper limit for safety
                if (r == 9) {
                    valid_templates.push_back(current_template);
                    return;
                }

                // Check if row r already has a fixed value d
                int fixed_c = -1;
                for (int c = 0; c < 9; ++c) {
                    if (fixed_d.test(cell_index(r, c))) {
                        fixed_c = c;
                        break;
                    }
                }

                if (fixed_c != -1) {
                    int b = cell_box(cell_index(r, fixed_c));
                    if ((used_cols & (1 << fixed_c)) == 0 && (used_boxes & (1 << b)) == 0) {
                        used_cols |= (1 << fixed_c);
                        used_boxes |= (1 << b);
                        current_template.set(cell_index(r, fixed_c));

                        self(self, r + 1);

                        current_template.reset(cell_index(r, fixed_c));
                        used_boxes &= ~(1 << b);
                        used_cols &= ~(1 << fixed_c);
                    }
                    return;
                }

                // Try each available column in row r having candidate d
                for (int c = 0; c < 9; ++c) {
                    int cell = cell_index(r, c);
                    if (board.has_candidate(cell, d)) {
                        int b = cell_box(cell);
                        if ((used_cols & (1 << c)) == 0 && (used_boxes & (1 << b)) == 0) {
                            used_cols |= (1 << c);
                            used_boxes |= (1 << b);
                            current_template.set(cell);

                            self(self, r + 1);

                            current_template.reset(cell);
                            used_boxes &= ~(1 << b);
                            used_cols &= ~(1 << c);
                        }
                    }
                }
            };

            search_templates(search_templates, 0);

            if (valid_templates.empty()) continue;

            // Template Delete: candidate d exists in current_d but not in any valid template
            BitSet81 union_templates;
            for (const auto& t : valid_templates) {
                union_templates |= t;
            }

            BitSet81 elim_cells = current_d & ~union_templates & ~fixed_d;
            if (!elim_cells.empty()) {
                Step step;
                step.type = TechniqueType::Custom;
                step.name = "Template Delete";
                step.difficulty = DifficultyLevel::Extreme;
                step.score = 400;
                step.explanation = "Template Delete for digit " + std::to_string(d) + ": cannot be placed in cells outside any valid template.";
                elim_cells.for_each_cell([&](int c) {
                    step.eliminations.push_back(CandidateElimination{c, d});
                    step.primary_cells.set(c);
                });
                steps.push_back(std::move(step));
            }

            // Template Set: cell is present in EVERY valid template
            BitSet81 intersect_templates = valid_templates.front();
            for (size_t i = 1; i < valid_templates.size(); ++i) {
                intersect_templates &= valid_templates[i];
            }

            BitSet81 forced_cells = intersect_templates & ~fixed_d;
            if (!forced_cells.empty()) {
                Step step;
                step.type = TechniqueType::Custom;
                step.name = "Template Set";
                step.difficulty = DifficultyLevel::Extreme;
                step.score = 400;
                step.explanation = "Template Set for digit " + std::to_string(d) + ": digit is present in all valid templates.";
                forced_cells.for_each_cell([&](int c) {
                    step.assignments.push_back(CandidateAssignment{c, d});
                    step.primary_cells.set(c);
                });
                steps.push_back(std::move(step));
            }
        }

        return steps;
    }
};

} // namespace hodoku::core
