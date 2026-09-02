#pragma once

#include <vector>
#include <string>
#include <queue>
#include <algorithm>
#include "BoardState.hpp"
#include "Step.hpp"

namespace hodoku::core {

class Chains {
public:
    // 1. Remote Pairs (Unfair, Score: 110)
    static std::vector<Step> find_remote_pairs(const BoardState& board) {
        std::vector<Step> steps;

        // Group bivalue cells by their candidate pair
        for (int d1 = 1; d1 <= 8; ++d1) {
            for (int d2 = d1 + 1; d2 <= 9; ++d2) {
                CandidateMask pair_mask = digit_to_mask(d1) | digit_to_mask(d2);
                std::vector<int> cells;

                for (int c = 0; c < TOTAL_CELLS; ++c) {
                    if (board.is_unfilled(c) && board.get_candidates(c) == pair_mask) {
                        cells.push_back(c);
                    }
                }

                size_t n = cells.size();
                if (n < 4) continue;

                // Build adjacency graph for cells that see each other
                std::vector<std::vector<int>> adj(n);
                for (size_t i = 0; i < n; ++i) {
                    for (size_t j = i + 1; j < n; ++j) {
                        if (get_peer_bitset(cells[i]).test(cells[j])) {
                            adj[i].push_back(static_cast<int>(j));
                            adj[j].push_back(static_cast<int>(i));
                        }
                    }
                }

                // Run BFS from each cell to find odd-length paths >= 3 edges (>= 4 nodes)
                for (size_t start = 0; start < n; ++start) {
                    std::vector<int> dist(n, -1);
                    std::queue<int> q;
                    dist[start] = 0;
                    q.push(static_cast<int>(start));

                    while (!q.empty()) {
                        int u = q.front();
                        q.pop();

                        for (int v : adj[u]) {
                            if (dist[v] == -1) {
                                dist[v] = dist[u] + 1;
                                q.push(v);
                            }
                        }
                    }

                    // For each node v with odd distance >= 3:
                    for (size_t target = start + 1; target < n; ++target) {
                        if (dist[target] >= 3 && (dist[target] % 2 == 1)) {
                            int c_start = cells[start];
                            int c_end = cells[target];

                            BitSet81 common = get_peer_bitset(c_start) & get_peer_bitset(c_end);
                            std::vector<CandidateElimination> elims;

                            common.for_each_cell([&](int elim_cell) {
                                if (board.is_unfilled(elim_cell)) {
                                    if (board.has_candidate(elim_cell, d1)) elims.push_back({elim_cell, d1});
                                    if (board.has_candidate(elim_cell, d2)) elims.push_back({elim_cell, d2});
                                }
                            });

                            if (!elims.empty()) {
                                Step step;
                                step.type = TechniqueType::RemotePair;
                                step.name = "Remote Pair";
                                step.difficulty = DifficultyLevel::Unfair;
                                step.score = 110;
                                step.primary_cells.set(c_start);
                                step.primary_cells.set(c_end);
                                step.eliminations = elims;

                                step.explanation = "Remote Pair (" + std::to_string(d1) + "/" + std::to_string(d2) +
                                                  ") of length " + std::to_string(dist[target] + 1) +
                                                  " between r" + std::to_string(cell_row(c_start) + 1) + "c" + std::to_string(cell_col(c_start) + 1) +
                                                  " and r" + std::to_string(cell_row(c_end) + 1) + "c" + std::to_string(cell_col(c_end) + 1) +
                                                  " eliminates candidates from common peers.";
                                steps.push_back(step);
                            }
                        }
                    }
                }
            }
        }

        return steps;
    }

    // 2. XY-Chains (Unfair, Score: 160)
    static std::vector<Step> find_xy_chains(const BoardState& board) {
        std::vector<Step> steps;

        // Collect all bivalue cells
        std::vector<int> bivalue_cells;
        for (int c = 0; c < TOTAL_CELLS; ++c) {
            if (board.is_unfilled(c) && board.count_candidates(c) == 2) {
                bivalue_cells.push_back(c);
            }
        }

        size_t n = bivalue_cells.size();
        if (n < 3) return steps;

        // DFS for alternating bivalue chains up to depth 8
        constexpr int MAX_DEPTH = 8;

        for (size_t start_idx = 0; start_idx < n; ++start_idx) {
            int start_cell = bivalue_cells[start_idx];
            CandidateMask m_start = board.get_candidates(start_cell);
            int d_start1 = std::countr_zero(static_cast<unsigned int>(m_start)) + 1;
            int d_start2 = std::countr_zero(static_cast<unsigned int>(m_start & ~digit_to_mask(d_start1))) + 1;

            // Try both candidates as the chain's starting digit
            for (int start_d : {d_start1, d_start2}) {
                int link_d = (start_d == d_start1) ? d_start2 : d_start1;

                std::vector<int> chain = {start_cell};
                std::vector<bool> in_chain(TOTAL_CELLS, false);
                in_chain[start_cell] = true;

                auto dfs = [&](auto& self, int current_cell, int current_link_d, int depth) -> void {
                    if (depth > MAX_DEPTH) return;

                    const auto& peers = get_peer_bitset(current_cell);

                    for (size_t next_idx = 0; next_idx < n; ++next_idx) {
                        int next_cell = bivalue_cells[next_idx];
                        if (in_chain[next_cell] || !peers.test(next_cell)) continue;

                        CandidateMask m_next = board.get_candidates(next_cell);
                        if (!mask_has_digit(m_next, current_link_d)) continue;

                        int other_d = (std::countr_zero(static_cast<unsigned int>(m_next)) + 1 == current_link_d)
                                    ? std::countr_zero(static_cast<unsigned int>(m_next & ~digit_to_mask(current_link_d))) + 1
                                    : std::countr_zero(static_cast<unsigned int>(m_next)) + 1;

                        chain.push_back(next_cell);
                        in_chain[next_cell] = true;

                        // If other_d matches start_d, we found an XY-Chain!
                        if (other_d == start_d && chain.size() >= 3) {
                            BitSet81 common = get_peer_bitset(start_cell) & get_peer_bitset(next_cell);
                            std::vector<CandidateElimination> elims;

                            common.for_each_cell([&](int target) {
                                if (board.is_unfilled(target) && board.has_candidate(target, start_d) && !in_chain[target]) {
                                    elims.push_back({target, start_d});
                                }
                            });

                            if (!elims.empty()) {
                                Step step;
                                step.type = TechniqueType::Custom;
                                step.name = "XY-Chain";
                                step.difficulty = DifficultyLevel::Unfair;
                                step.score = 160;
                                for (int c : chain) step.primary_cells.set(c);
                                step.eliminations = elims;

                                step.explanation = "XY-Chain on digit " + std::to_string(start_d) +
                                                  " of length " + std::to_string(chain.size()) +
                                                  " eliminates " + std::to_string(start_d) + " from common peers.";
                                steps.push_back(step);
                            }
                        }

                        // Continue recursion
                        self(self, next_cell, other_d, depth + 1);

                        in_chain[next_cell] = false;
                        chain.pop_back();
                    }
                };

                dfs(dfs, start_cell, link_d, 1);
            }
        }

        return steps;
    }
};

} // namespace hodoku::core

