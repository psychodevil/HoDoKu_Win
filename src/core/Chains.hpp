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
                                for (size_t ci = 0; ci + 1 < chain.size(); ++ci) {
                                    step.links.push_back({chain[ci], 0, chain[ci + 1], 0, (ci % 2 == 0)});
                                }

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

    // 3. Grouped Alternating Inference Chains (Grouped AIC / Grouped X-Chain)
    // Matches HoDoKu GroupNode.java & TablingSolver.java (GROUPED_AIC, Unfair, Score: 300)
    struct GroupChainNode {
        int id{0};
        bool is_group{false};
        int digit{0};
        int rep_cell{0};
        BitSet81 cells;
        BitSet81 buddies;
    };

    static std::vector<Step> find_grouped_aic(const BoardState& board) {
        std::vector<Step> steps;

        for (int d = 1; d <= 9; ++d) {
            std::vector<GroupChainNode> nodes;

            // 1. Single cell nodes for digit d
            for (int c = 0; c < TOTAL_CELLS; ++c) {
                if (board.is_unfilled(c) && board.has_candidate(c, d)) {
                    GroupChainNode node;
                    node.id = static_cast<int>(nodes.size());
                    node.is_group = false;
                    node.digit = d;
                    node.rep_cell = c;
                    node.cells.set(c);
                    node.buddies = get_peer_bitset(c);
                    nodes.push_back(node);
                }
            }

            // 2. GroupNodes (2 or 3 cells in a line/col within a box)
            for (int b = 0; b < 9; ++b) {
                int r_start = (b / 3) * 3;
                int c_start = (b % 3) * 3;

                // Box-row groups
                for (int r = r_start; r < r_start + 3; ++r) {
                    BitSet81 r_cands = board.get_candidates_in_house(r, d);
                    BitSet81 b_cands = board.get_candidates_in_house(18 + b, d);
                    BitSet81 inter = r_cands & b_cands;
                    int count = inter.count();
                    if (count >= 2 && count <= 3) {
                        if (r_cands.count() > count || b_cands.count() > count) {
                            GroupChainNode node;
                            node.id = static_cast<int>(nodes.size());
                            node.is_group = true;
                            node.digit = d;
                            node.rep_cell = inter.first_cell();
                            node.cells = inter;
                            BitSet81 common = BitSet81::all();
                            inter.for_each_cell([&](int c) {
                                common &= get_peer_bitset(c);
                            });
                            node.buddies = common;
                            nodes.push_back(node);
                        }
                    }
                }

                // Box-col groups
                for (int c = c_start; c < c_start + 3; ++c) {
                    BitSet81 c_cands = board.get_candidates_in_house(9 + c, d);
                    BitSet81 b_cands = board.get_candidates_in_house(18 + b, d);
                    BitSet81 inter = c_cands & b_cands;
                    int count = inter.count();
                    if (count >= 2 && count <= 3) {
                        if (c_cands.count() > count || b_cands.count() > count) {
                            GroupChainNode node;
                            node.id = static_cast<int>(nodes.size());
                            node.is_group = true;
                            node.digit = d;
                            node.rep_cell = inter.first_cell();
                            node.cells = inter;
                            BitSet81 common = BitSet81::all();
                            inter.for_each_cell([&](int cell) {
                                common &= get_peer_bitset(cell);
                            });
                            node.buddies = common;
                            nodes.push_back(node);
                        }
                    }
                }
            }

            size_t num_nodes = nodes.size();
            if (num_nodes < 4) continue;

            // 3. Build Strong & Weak links
            std::vector<std::vector<int>> strong_links(num_nodes);
            std::vector<std::vector<int>> weak_links(num_nodes);

            for (size_t i = 0; i < num_nodes; ++i) {
                for (size_t j = i + 1; j < num_nodes; ++j) {
                    // Nodes must be mutually disjoint
                    if (!(nodes[i].cells & nodes[j].cells).empty()) continue;

                    // Check weak link: can all cells in j see i's cells?
                    bool sees_all = true;
                    nodes[j].cells.for_each_cell([&](int c) {
                        if (!nodes[i].buddies.test(c)) sees_all = false;
                    });

                    if (sees_all) {
                        weak_links[i].push_back(static_cast<int>(j));
                        weak_links[j].push_back(static_cast<int>(i));
                    }

                    // Check strong link: do they together exhaust all candidates in some house H?
                    for (int h = 0; h < TOTAL_HOUSES; ++h) {
                        BitSet81 h_cands = board.get_candidates_in_house(h, d);
                        int total_in_house = h_cands.count();
                        if (total_in_house == nodes[i].cells.count() + nodes[j].cells.count()) {
                            if (nodes[i].cells.is_subset_of(h_cands) && nodes[j].cells.is_subset_of(h_cands)) {
                                strong_links[i].push_back(static_cast<int>(j));
                                strong_links[j].push_back(static_cast<int>(i));
                                break;
                            }
                        }
                    }
                }
            }

            // 4. Search alternating chains (Strong - Weak - Strong ... - Strong)
            constexpr int MAX_NODES = 6;
            std::vector<int> chain;
            BitSet81 used_cells;

            auto dfs = [&](auto& self, int curr_node, bool need_strong) -> void {
                if (need_strong) {
                    // Just completed a weak link; next link must be STRONG
                    for (int next_node : strong_links[curr_node]) {
                        if (!(nodes[next_node].cells & used_cells).empty()) continue;

                        chain.push_back(next_node);
                        BitSet81 next_cells = nodes[next_node].cells;
                        used_cells |= next_cells;

                        if (chain.size() >= 4 && (chain.size() % 2 == 0)) {
                            bool has_group = false;
                            for (int nid : chain) {
                                if (nodes[nid].is_group) {
                                    has_group = true;
                                    break;
                                }
                            }

                            if (has_group) {
                                int first_node = chain.front();
                                int last_node = chain.back();

                                BitSet81 common = nodes[first_node].buddies & nodes[last_node].buddies;
                                std::vector<CandidateElimination> elims;

                                common.for_each_cell([&](int elim_cell) {
                                    if (board.is_unfilled(elim_cell) && board.has_candidate(elim_cell, d) && !used_cells.test(elim_cell)) {
                                        elims.push_back({elim_cell, d});
                                    }
                                });

                                if (!elims.empty()) {
                                    Step step;
                                    step.type = TechniqueType::GroupedAIC;
                                    step.name = "Grouped AIC";
                                    step.difficulty = DifficultyLevel::Unfair;
                                    step.score = 300;
                                    for (int nid : chain) {
                                        step.primary_cells |= nodes[nid].cells;
                                    }
                                    step.eliminations = elims;

                                    for (size_t ci = 0; ci + 1 < chain.size(); ++ci) {
                                        bool is_s = (ci % 2 == 0);
                                        step.links.push_back({
                                            nodes[chain[ci]].rep_cell, d,
                                            nodes[chain[ci + 1]].rep_cell, d,
                                            is_s
                                        });
                                    }

                                    step.explanation = "Grouped AIC on digit " + std::to_string(d) +
                                                      " of length " + std::to_string(chain.size()) +
                                                      " with " + std::to_string(chain.size() - 1) + " alternating links" +
                                                      " eliminates candidate " + std::to_string(d) + " from common peers.";
                                    steps.push_back(step);
                                }
                            }
                        }

                        if (chain.size() < MAX_NODES) {
                            self(self, next_node, false);
                        }

                        used_cells &= ~next_cells;
                        chain.pop_back();
                    }
                } else {
                    // Just completed a strong link; next link must be WEAK
                    for (int next_node : weak_links[curr_node]) {
                        if (!(nodes[next_node].cells & used_cells).empty()) continue;

                        chain.push_back(next_node);
                        BitSet81 next_cells = nodes[next_node].cells;
                        used_cells |= next_cells;

                        self(self, next_node, true);

                        used_cells &= ~next_cells;
                        chain.pop_back();
                    }
                }
            };

            for (size_t start_idx = 0; start_idx < num_nodes; ++start_idx) {
                chain.push_back(static_cast<int>(start_idx));
                used_cells = nodes[start_idx].cells;

                dfs(dfs, static_cast<int>(start_idx), true);

                chain.pop_back();
                used_cells.clear();
            }
        }

        // Deduplicate steps with identical eliminations
        std::vector<Step> unique_steps;
        for (auto& s : steps) {
            bool dup = false;
            for (const auto& u : unique_steps) {
                if (u.eliminations.size() == s.eliminations.size()) {
                    bool match = true;
                    for (size_t k = 0; k < u.eliminations.size(); ++k) {
                        if (u.eliminations[k].cell != s.eliminations[k].cell ||
                            u.eliminations[k].digit != s.eliminations[k].digit) {
                            match = false;
                            break;
                        }
                    }
                    if (match) { dup = true; break; }
                }
            }
            if (!dup) unique_steps.push_back(std::move(s));
        }

        return unique_steps;
    }
};

} // namespace hodoku::core

