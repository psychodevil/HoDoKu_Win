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
            // 1. Build adjacency list of strong links for digit d
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

            if (nodes.count() < 4) continue;

            // 2. Find connected components and 2-color them
            std::vector<int> color(TOTAL_CELLS, -1);
            std::vector<bool> visited(TOTAL_CELLS, false);

            nodes.for_each_cell([&](int start_node) {
                if (visited[start_node]) return;

                std::vector<int> comp_color0;
                std::vector<int> comp_color1;
                bool is_bipartite = true;
                int conflict_u = -1, conflict_v = -1;

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
                        } else if (color[v] == color[u]) {
                            is_bipartite = false;
                            conflict_u = u;
                            conflict_v = v;
                        }
                    }
                }

                // Rule 2: Color Wrap (Two cells of the same color share a house)
                int wrap_bad_color = -1;
                if (!is_bipartite) {
                    wrap_bad_color = color[conflict_u];
                } else {
                    // Check if any two cells of color 0 or color 1 see each other
                    for (int c = 0; c < 2; ++c) {
                        const auto& list = (c == 0) ? comp_color0 : comp_color1;
                        size_t sz = list.size();
                        for (size_t i = 0; i < sz && wrap_bad_color == -1; ++i) {
                            for (size_t j = i + 1; j < sz; ++j) {
                                if (get_peer_bitset(list[i]).test(list[j])) {
                                    wrap_bad_color = c;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (wrap_bad_color != -1) {
                    const auto& bad_cells = (wrap_bad_color == 0) ? comp_color0 : comp_color1;
                    std::vector<CandidateElimination> elims;
                    for (int cell : bad_cells) {
                        if (board.is_unfilled(cell) && board.has_candidate(cell, d)) {
                            elims.push_back({cell, d});
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
                    for (int c : comp_color0) seen_by_c0 |= get_peer_bitset(c);

                    BitSet81 seen_by_c1;
                    for (int c : comp_color1) seen_by_c1 |= get_peer_bitset(c);

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

