#pragma once

#include <vector>
#include <string>
#include <queue>
#include "BoardState.hpp"
#include "Step.hpp"

namespace hodoku::core {

class Coloring {
public:
    static std::vector<Step> find_simple_colors(const BoardState& board) {
        std::vector<Step> steps;

        for (int d = 1; d <= 9; ++d) {
            // 1. Build conjugate pairs (strong links where house candidate count == 2)
            std::vector<std::vector<int>> adj(TOTAL_CELLS);
            BitSet81 nodes;

            for (int h = 0; h < TOTAL_HOUSES; ++h) {
                BitSet81 cands = board.get_candidates_in_house(h, d);
                if (cands.count() == 2) {
                    int u = cands.pop_first_cell();
                    int v = cands.pop_first_cell();
                    adj[u].push_back(v);
                    adj[v].push_back(u);
                    nodes.set(u);
                    nodes.set(v);
                }
            }

            if (nodes.empty()) continue;

            // 2. Find connected components of strong links and 2-color them
            std::vector<int> color(TOTAL_CELLS, -1);
            std::vector<bool> visited(TOTAL_CELLS, false);

            nodes.for_each_cell([&](int start_node) {
                if (visited[start_node]) return;

                std::vector<int> comp_color0;
                std::vector<int> comp_color1;
                std::queue<int> q;

                visited[start_node] = true;
                color[start_node] = 0;
                comp_color0.push_back(start_node);
                q.push(start_node);

                while (!q.empty()) {
                    int u = q.front();
                    q.pop();

                    for (int v : adj[u]) {
                        if (!visited[v]) {
                            visited[v] = true;
                            color[v] = 1 - color[u];
                            if (color[v] == 0) comp_color0.push_back(v);
                            else comp_color1.push_back(v);
                            q.push(v);
                        }
                    }
                }

                if (comp_color0.empty() || comp_color1.empty()) return;

                // Rule 2: Color Wrap (Two cells of the same color share a house)
                bool wrap0 = false;
                for (size_t i = 0; i < comp_color0.size() && !wrap0; ++i) {
                    for (size_t j = i + 1; j < comp_color0.size(); ++j) {
                        if (GRID.peer_bitsets[comp_color0[i]].test(comp_color0[j])) {
                            wrap0 = true;
                            break;
                        }
                    }
                }

                bool wrap1 = false;
                for (size_t i = 0; i < comp_color1.size() && !wrap1; ++i) {
                    for (size_t j = i + 1; j < comp_color1.size(); ++j) {
                        if (GRID.peer_bitsets[comp_color1[i]].test(comp_color1[j])) {
                            wrap1 = true;
                            break;
                        }
                    }
                }

                if (wrap0 || wrap1) {
                    std::vector<CandidateElimination> elims;
                    if (wrap0) {
                        for (int cell : comp_color0) {
                            if (board.is_unfilled(cell) && board.has_candidate(cell, d)) {
                                elims.push_back({cell, d});
                            }
                        }
                    }
                    if (wrap1) {
                        for (int cell : comp_color1) {
                            if (board.is_unfilled(cell) && board.has_candidate(cell, d)) {
                                elims.push_back({cell, d});
                            }
                        }
                    }

                    if (!elims.empty()) {
                        Step step;
                        step.type = TechniqueType::SimpleColors;
                        step.name = "Simple Colors (Color Wrap)";
                        step.difficulty = DifficultyLevel::Unfair;
                        step.score = 150;
                        for (int c : comp_color0) step.primary_cells.set(c);
                        for (int c : comp_color1) step.secondary_cells.set(c);
                        step.eliminations = elims;

                        step.explanation = "Simple Colors (Color Wrap) on digit " + std::to_string(d) +
                                          ": two cells of the same color see each other, eliminating " +
                                          std::to_string(d) + " from that entire color.";
                        steps.push_back(step);
                    }
                } else {
                    // Rule 1: Color Trap (Uncolored cell sees both Color 0 and Color 1)
                    BitSet81 seen_by_c0;
                    for (int c : comp_color0) seen_by_c0 |= GRID.peer_bitsets[c];

                    BitSet81 seen_by_c1;
                    for (int c : comp_color1) seen_by_c1 |= GRID.peer_bitsets[c];

                    BitSet81 common_seen = seen_by_c0 & seen_by_c1;
                    for (int c : comp_color0) common_seen.reset(c);
                    for (int c : comp_color1) common_seen.reset(c);

                    std::vector<CandidateElimination> elims;
                    common_seen.for_each_cell([&](int cell) {
                        if (board.is_unfilled(cell) && board.has_candidate(cell, d)) {
                            elims.push_back({cell, d});
                        }
                    });

                    if (!elims.empty()) {
                        Step step;
                        step.type = TechniqueType::SimpleColors;
                        step.name = "Simple Colors (Color Trap)";
                        step.difficulty = DifficultyLevel::Unfair;
                        step.score = 150;
                        for (int c : comp_color0) step.primary_cells.set(c);
                        for (int c : comp_color1) step.secondary_cells.set(c);
                        step.eliminations = elims;

                        step.explanation = "Simple Colors (Color Trap) on digit " + std::to_string(d) +
                                          ": uncolored cells see both colors, eliminating candidate " +
                                          std::to_string(d) + ".";
                        steps.push_back(step);
                    }
                }
            });
        }

        return steps;
    }
};

} // namespace hodoku::core

