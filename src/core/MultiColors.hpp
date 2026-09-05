#pragma once

#include <vector>
#include <string>
#include <queue>
#include "BoardState.hpp"
#include "Step.hpp"

namespace hodoku::core {

class MultiColors {
public:
    struct Cluster {
        int id{0};
        std::vector<int> color0;
        std::vector<int> color1;
        BitSet81 mask0;
        BitSet81 mask1;
    };

    static std::vector<Step> find_multi_colors(const BoardState& board) {
        std::vector<Step> steps;

        for (int d = 1; d <= 9; ++d) {
            std::vector<std::vector<int>> adj(TOTAL_CELLS);
            BitSet81 all_nodes;

            for (int h = 0; h < TOTAL_HOUSES; ++h) {
                BitSet81 cands = board.get_candidates_in_house(h, d);
                if (cands.count() == 2) {
                    int u = cands.pop_first_cell();
                    int v = cands.pop_first_cell();
                    adj[u].push_back(v);
                    adj[v].push_back(u);
                    all_nodes.set(u);
                    all_nodes.set(v);
                }
            }

            if (all_nodes.count() < 4) continue;

            // 1. Find all bipartite clusters
            std::vector<int> color(TOTAL_CELLS, -1);
            std::vector<bool> visited(TOTAL_CELLS, false);
            std::vector<Cluster> clusters;

            all_nodes.for_each_cell([&](int start_node) {
                if (visited[start_node]) return;

                Cluster cl;
                cl.id = static_cast<int>(clusters.size());
                std::queue<int> q;

                visited[start_node] = true;
                color[start_node] = 0;
                cl.color0.push_back(start_node);
                cl.mask0.set(start_node);
                q.push(start_node);

                bool is_bipartite = true;
                while (!q.empty()) {
                    int u = q.front();
                    q.pop();

                    for (int v : adj[u]) {
                        if (!visited[v]) {
                            visited[v] = true;
                            color[v] = 1 - color[u];
                            if (color[v] == 0) {
                                cl.color0.push_back(v);
                                cl.mask0.set(v);
                            } else {
                                cl.color1.push_back(v);
                                cl.mask1.set(v);
                            }
                            q.push(v);
                        } else if (color[v] == color[u]) {
                            is_bipartite = false;
                        }
                    }
                }

                if (is_bipartite && (!cl.color0.empty() && !cl.color1.empty())) {
                    clusters.push_back(cl);
                }
            });

            if (clusters.size() < 2) continue;

            // Helper to check if two sets have cells sharing a house (weak link conflict)
            auto has_weak_link = [&](const BitSet81& setA, const BitSet81& setB) -> bool {
                bool conflict = false;
                setA.for_each_cell([&](int u) {
                    if (conflict) return;
                    if ((setB & GRID.peer_bitsets[u]).any()) {
                        conflict = true;
                    }
                });
                return conflict;
            };

            // 2. Multi-Colors Type 2 (Color Wrap between clusters):
            // If cluster A (color cA) conflicts with BOTH color 0 AND color 1 of cluster B:
            // Then cluster A (color cA) cannot be true! Eliminate candidate d from all cells in that set!
            for (size_t i = 0; i < clusters.size(); ++i) {
                for (size_t j = 0; j < clusters.size(); ++j) {
                    if (i == j) continue;

                    for (int cA = 0; cA < 2; ++cA) {
                        const auto& setA = (cA == 0) ? clusters[i].mask0 : clusters[i].mask1;
                        const auto& cellsA = (cA == 0) ? clusters[i].color0 : clusters[i].color1;

                        bool conflict0 = has_weak_link(setA, clusters[j].mask0);
                        bool conflict1 = has_weak_link(setA, clusters[j].mask1);

                        if (conflict0 && conflict1) {
                            Step step;
                            step.type = TechniqueType::MultiColors2;
                            step.difficulty = DifficultyLevel::Unfair;
                            step.score = 190;
                            step.name = "Multi-Colors Type 2";

                            for (int cell : cellsA) {
                                step.eliminations.push_back({cell, d});
                                step.primary_cells.set(cell);
                            }

                            clusters[j].mask0.for_each_cell([&](int c) { step.secondary_cells.set(c); });
                            clusters[j].mask1.for_each_cell([&](int c) { step.secondary_cells.set(c); });

                            step.explanation = "Multi-Colors Type 2 on candidate " + std::to_string(d) +
                                               ": Cluster " + std::to_string(i + 1) + " sees both colors of Cluster " +
                                               std::to_string(j + 1) + ", eliminating candidate " + std::to_string(d) +
                                               " from " + std::to_string(step.eliminations.size()) + " cell(s).";

                            steps.push_back(step);
                            return steps; // Return first step
                        }
                    }
                }
            }

            // 3. Multi-Colors Type 1 (Bridge / Trap):
            // If cluster A (color cA) and cluster B (color cB) share a weak link:
            // Then at least one of oppA or oppB must be true!
            // Any candidate cell seeing a cell in oppA AND a cell in oppB can be eliminated!
            for (size_t i = 0; i < clusters.size(); ++i) {
                for (size_t j = i + 1; j < clusters.size(); ++j) {
                    for (int cA = 0; cA < 2; ++cA) {
                        for (int cB = 0; cB < 2; ++cB) {
                            const auto& linkA = (cA == 0) ? clusters[i].mask0 : clusters[i].mask1;
                            const auto& linkB = (cB == 0) ? clusters[j].mask0 : clusters[j].mask1;

                            if (has_weak_link(linkA, linkB)) {
                                // The opposite colors are true-candidates
                                const auto& oppA = (cA == 0) ? clusters[i].mask1 : clusters[i].mask0;
                                const auto& oppB = (cB == 0) ? clusters[j].mask1 : clusters[j].mask0;

                                // Cells that see both oppA and oppB
                                BitSet81 seenA;
                                oppA.for_each_cell([&](int u) { seenA |= GRID.peer_bitsets[u]; });

                                BitSet81 seenB;
                                oppB.for_each_cell([&](int v) { seenB |= GRID.peer_bitsets[v]; });

                                BitSet81 elimCandidates = seenA & seenB;
                                elimCandidates &= ~clusters[i].mask0;
                                elimCandidates &= ~clusters[i].mask1;
                                elimCandidates &= ~clusters[j].mask0;
                                elimCandidates &= ~clusters[j].mask1;

                                Step step;
                                step.type = TechniqueType::MultiColors1;
                                step.difficulty = DifficultyLevel::Unfair;
                                step.score = 190;
                                step.name = "Multi-Colors Type 1";

                                elimCandidates.for_each_cell([&](int cell) {
                                    if (board.has_candidate(cell, d)) {
                                        step.eliminations.push_back({cell, d});
                                        step.primary_cells.set(cell);
                                    }
                                });

                                if (!step.eliminations.empty()) {
                                    oppA.for_each_cell([&](int c) { step.secondary_cells.set(c); });
                                    oppB.for_each_cell([&](int c) { step.secondary_cells.set(c); });

                                    step.explanation = "Multi-Colors Type 1 on candidate " + std::to_string(d) +
                                                       ": Clusters " + std::to_string(i + 1) + " and " +
                                                       std::to_string(j + 1) + " bridge eliminations for " +
                                                       std::to_string(step.eliminations.size()) + " cell(s).";

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
