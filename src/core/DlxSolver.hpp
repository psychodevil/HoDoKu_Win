#pragma once

#include <vector>
#include <array>
#include <limits>
#include <span>
#include "Types.hpp"
#include "BoardState.hpp"

namespace hodoku::core {

class DlxSolver {
public:
    static constexpr int NUM_COLS = 324; // 81 cell + 81 row + 81 col + 81 box
    static constexpr int MAX_CANDIDATE_ROWS = 729; // 9 * 9 * 9

    struct Node {
        int left{0};
        int right{0};
        int up{0};
        int down{0};
        int col{0};
        int row_id{-1};
    };

    struct CandidateChoice {
        int cell{0};
        int digit{0};
    };

    DlxSolver() {
        init_matrix();
    }

    // Counts solutions up to max_limit (e.g., max_limit=2 to check uniqueness)
    int count_solutions(const BoardState& board, int max_limit = 2) {
        std::vector<BoardState> solutions;
        solve(board, solutions, max_limit);
        return static_cast<int>(solutions.size());
    }

    // Solves the board and returns the first solution if found
    std::optional<BoardState> solve_one(const BoardState& board) {
        std::vector<BoardState> solutions;
        solve(board, solutions, 1);
        if (!solutions.empty()) {
            return solutions.front();
        }
        return std::nullopt;
    }

    // Finds up to max_solutions
    void solve(const BoardState& board, std::vector<BoardState>& solutions, int max_solutions = 1) {
        init_matrix();

        // Apply board constraints (givens / values already placed)
        // Also respect eliminated candidates in board
        std::vector<int> rows_to_cover;
        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            int val = board.get_value(cell);
            if (val != 0) {
                int row_id = cell * 9 + (val - 1);
                rows_to_cover.push_back(row_id);
            } else {
                // If candidate is not present in board, remove that row choice from the DLX matrix
                CandidateMask mask = board.get_candidates(cell);
                for (int d = 1; d <= 9; ++d) {
                    if (!mask_has_digit(mask, d)) {
                        int row_id = cell * 9 + (d - 1);
                        remove_row(row_id);
                    }
                }
            }
        }

        // Apply given choices
        for (int row_id : rows_to_cover) {
            cover_by_row(row_id);
        }

        std::vector<int> solution_rows;
        search(solution_rows, solutions, board, max_solutions);
    }

private:
    std::vector<Node> m_nodes;
    std::array<int, NUM_COLS + 1> m_col_sizes{};
    std::array<int, MAX_CANDIDATE_ROWS> m_row_first_node{};
    std::array<CandidateChoice, MAX_CANDIDATE_ROWS> m_row_to_choice{};

    static constexpr int ROOT = 0;

    void init_matrix() {
        m_nodes.clear();
        m_nodes.reserve(4000);
        m_col_sizes.fill(0);
        m_row_first_node.fill(-1);

        // Node 0 is ROOT
        m_nodes.push_back(Node{ROOT, ROOT, ROOT, ROOT, ROOT, -1});

        // Add 324 column headers
        for (int c = 1; c <= NUM_COLS; ++c) {
            int prev = c - 1;
            m_nodes.push_back(Node{prev, ROOT, c, c, c, -1});
            m_nodes[prev].right = c;
            m_nodes[ROOT].left = c;
        }

        // Build 729 rows (choices)
        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            int r = cell_row(cell);
            int c = cell_col(cell);
            int b = cell_box(cell);

            for (int d = 1; d <= 9; ++d) {
                int row_id = cell * 9 + (d - 1);
                m_row_to_choice[row_id] = {cell, d};

                int col_cell = 1 + cell;
                int col_row = 1 + 81 + r * 9 + (d - 1);
                int col_col = 1 + 162 + c * 9 + (d - 1);
                int col_box = 1 + 243 + b * 9 + (d - 1);

                std::array<int, 4> cols = {col_cell, col_row, col_col, col_box};
                int first_node = -1;

                for (int col_idx : cols) {
                    int node_idx = static_cast<int>(m_nodes.size());
                    int up = m_nodes[col_idx].up;

                    m_nodes.push_back(Node{node_idx, node_idx, up, col_idx, col_idx, row_id});
                    m_nodes[up].down = node_idx;
                    m_nodes[col_idx].up = node_idx;
                    ++m_col_sizes[col_idx];

                    if (first_node == -1) {
                        first_node = node_idx;
                    } else {
                        int left = m_nodes[first_node].left;
                        m_nodes[node_idx].left = left;
                        m_nodes[node_idx].right = first_node;
                        m_nodes[left].right = node_idx;
                        m_nodes[first_node].left = node_idx;
                    }
                }
                m_row_first_node[row_id] = first_node;
            }
        }
    }

    void cover_column(int c) noexcept {
        m_nodes[m_nodes[c].right].left = m_nodes[c].left;
        m_nodes[m_nodes[c].left].right = m_nodes[c].right;

        for (int row_node = m_nodes[c].down; row_node != c; row_node = m_nodes[row_node].down) {
            for (int right_node = m_nodes[row_node].right; right_node != row_node; right_node = m_nodes[right_node].right) {
                m_nodes[m_nodes[right_node].down].up = m_nodes[right_node].up;
                m_nodes[m_nodes[right_node].up].down = m_nodes[right_node].down;
                --m_col_sizes[m_nodes[right_node].col];
            }
        }
    }

    void uncover_column(int c) noexcept {
        for (int row_node = m_nodes[c].up; row_node != c; row_node = m_nodes[row_node].up) {
            for (int left_node = m_nodes[row_node].left; left_node != row_node; left_node = m_nodes[left_node].left) {
                ++m_col_sizes[m_nodes[left_node].col];
                m_nodes[m_nodes[left_node].down].up = left_node;
                m_nodes[m_nodes[left_node].up].down = left_node;
            }
        }
        m_nodes[m_nodes[c].right].left = c;
        m_nodes[m_nodes[c].left].right = c;
    }

    void cover_by_row(int row_id) {
        int first = m_row_first_node[row_id];
        if (first == -1) return;

        int curr = first;
        do {
            cover_column(m_nodes[curr].col);
            curr = m_nodes[curr].right;
        } while (curr != first);
    }

    void remove_row(int row_id) {
        int first = m_row_first_node[row_id];
        if (first == -1) return;

        int curr = first;
        do {
            m_nodes[m_nodes[curr].down].up = m_nodes[curr].up;
            m_nodes[m_nodes[curr].up].down = m_nodes[curr].down;
            --m_col_sizes[m_nodes[curr].col];
            curr = m_nodes[curr].right;
        } while (curr != first);
    }

    void search(std::vector<int>& solution_rows, std::vector<BoardState>& solutions, const BoardState& orig_board, int max_solutions) {
        if (static_cast<int>(solutions.size()) >= max_solutions) {
            return;
        }

        if (m_nodes[ROOT].right == ROOT) {
            // All constraints covered!
            BoardState solved = orig_board;
            for (int row_id : solution_rows) {
                const auto& choice = m_row_to_choice[row_id];
                solved.set_value(choice.cell, choice.digit);
            }
            solutions.push_back(solved);
            return;
        }

        // Choose column with lowest size (heuristic)
        int best_col = m_nodes[ROOT].right;
        int min_size = m_col_sizes[best_col];

        for (int c = m_nodes[ROOT].right; c != ROOT; c = m_nodes[c].right) {
            if (m_col_sizes[c] < min_size) {
                min_size = m_col_sizes[c];
                best_col = c;
            }
        }

        if (min_size == 0) {
            return; // Contradiction: dead branch
        }

        cover_column(best_col);

        for (int row_node = m_nodes[best_col].down; row_node != best_col; row_node = m_nodes[row_node].down) {
            solution_rows.push_back(m_nodes[row_node].row_id);

            for (int right_node = m_nodes[row_node].right; right_node != row_node; right_node = m_nodes[right_node].right) {
                cover_column(m_nodes[right_node].col);
            }

            search(solution_rows, solutions, orig_board, max_solutions);

            solution_rows.pop_back();

            for (int left_node = m_nodes[row_node].left; left_node != row_node; left_node = m_nodes[left_node].left) {
                uncover_column(m_nodes[left_node].col);
            }

            if (static_cast<int>(solutions.size()) >= max_solutions) {
                break;
            }
        }

        uncover_column(best_col);
    }
};

} // namespace hodoku::core

