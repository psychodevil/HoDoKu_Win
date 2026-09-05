#pragma once

#include <vector>
#include <array>
#include <limits>
#include <span>
#include <optional>
#include "Types.hpp"
#include "BoardState.hpp"
#include "DiagonalBitboards.hpp"
#include "HyperSudokuWindows.hpp"

namespace hodoku::core {

class DlxSolver {
public:
    static constexpr int NUM_COLS_BASE = 324; // 81 cell + 81 row + 81 col + 81 box
    static constexpr int NUM_COLS_DIAG = 18;  // 9 main diagonal + 9 anti-diagonal
    static constexpr int NUM_COLS_HYPER = 36; // 4 windows * 9 digits
    static constexpr int NUM_COLS = NUM_COLS_BASE; // Standard backwards-compatibility alias
    static constexpr int MAX_COLS = NUM_COLS_BASE + NUM_COLS_DIAG + NUM_COLS_HYPER; // 378
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

    explicit DlxSolver(SudokuVariant variant = SudokuVariant::Standard)
        : m_variant(variant) {
        init_matrix();
    }

    void set_variant(SudokuVariant variant) {
        if (m_variant != variant) {
            m_variant = variant;
            init_matrix();
        }
    }

    [[nodiscard]] SudokuVariant get_variant() const noexcept {
        return m_variant;
    }

    [[nodiscard]] int active_columns() const noexcept {
        return m_active_cols;
    }

    [[nodiscard]] bool has_diagonal() const noexcept {
        return has_diagonal_constraint(m_variant);
    }

    [[nodiscard]] bool has_hyper() const noexcept {
        return has_hyper_constraint(m_variant);
    }

    // Counts solutions up to max_limit (e.g., max_limit=2 to check uniqueness)
    int count_solutions(const BoardState& board, int max_limit = 2, std::optional<SudokuVariant> variant = std::nullopt) {
        if (variant.has_value() && *variant != m_variant) {
            set_variant(*variant);
        }
        std::vector<BoardState> solutions;
        solve(board, solutions, max_limit);
        return static_cast<int>(solutions.size());
    }

    // Solves the board and returns the first solution if found
    std::optional<BoardState> solve_one(const BoardState& board, std::optional<SudokuVariant> variant = std::nullopt) {
        if (variant.has_value() && *variant != m_variant) {
            set_variant(*variant);
        }
        std::vector<BoardState> solutions;
        solve(board, solutions, 1);
        if (!solutions.empty()) {
            return solutions.front();
        }
        return std::nullopt;
    }

    // Finds up to max_solutions
    void solve(const BoardState& board, std::vector<BoardState>& solutions, int max_solutions = 1, std::optional<SudokuVariant> variant = std::nullopt) {
        if (variant.has_value() && *variant != m_variant) {
            set_variant(*variant);
        }
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
            if (!cover_by_row(row_id)) {
                // Given conflict detected (e.g., house/diagonal/window duplicate)
                return;
            }
        }

        std::vector<int> solution_rows;
        search(solution_rows, solutions, board, max_solutions);
    }

private:
    std::vector<Node> m_nodes;
    std::array<int, MAX_COLS + 1> m_col_sizes{};
    std::array<bool, MAX_COLS + 1> m_col_covered{};
    std::array<int, MAX_CANDIDATE_ROWS> m_row_first_node{};
    std::array<CandidateChoice, MAX_CANDIDATE_ROWS> m_row_to_choice{};
    int m_active_cols{NUM_COLS_BASE};
    SudokuVariant m_variant{SudokuVariant::Standard};

    static constexpr int ROOT = 0;

    void init_matrix() {
        m_nodes.clear();
        m_nodes.reserve(5000);
        m_col_sizes.fill(0);
        m_col_covered.fill(false);
        m_row_first_node.fill(-1);

        int num_cols = NUM_COLS_BASE;
        int diag_main_start = -1;
        int diag_anti_start = -1;
        int hyper_start = -1;

        bool with_diag = has_diagonal_constraint(m_variant);
        bool with_hyper = has_hyper_constraint(m_variant);

        if (with_diag) {
            diag_main_start = 1 + num_cols;
            diag_anti_start = 1 + num_cols + 9;
            num_cols += NUM_COLS_DIAG;
        }

        if (with_hyper) {
            hyper_start = 1 + num_cols;
            num_cols += NUM_COLS_HYPER;
        }

        m_active_cols = num_cols;

        // Node 0 is ROOT
        m_nodes.push_back(Node{ROOT, ROOT, ROOT, ROOT, ROOT, -1});

        // Add column headers for 1 .. m_active_cols
        for (int c = 1; c <= m_active_cols; ++c) {
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

                std::array<int, 6> cols{};
                int col_count = 0;
                cols[col_count++] = col_cell;
                cols[col_count++] = col_row;
                cols[col_count++] = col_col;
                cols[col_count++] = col_box;

                if (with_diag) {
                    if (is_main_diagonal_cell(cell)) {
                        cols[col_count++] = diag_main_start + (d - 1);
                    }
                    if (is_anti_diagonal_cell(cell)) {
                        cols[col_count++] = diag_anti_start + (d - 1);
                    }
                }

                if (with_hyper) {
                    if (is_hyper_window_cell(cell)) {
                        int w = get_hyper_window_index(cell);
                        cols[col_count++] = hyper_start + w * 9 + (d - 1);
                    }
                }

                int first_node = -1;

                for (int i = 0; i < col_count; ++i) {
                    int col_idx = cols[i];
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
        m_col_covered[c] = true;
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
        m_col_covered[c] = false;
    }

    bool cover_by_row(int row_id) {
        int first = m_row_first_node[row_id];
        if (first == -1) return false;

        // Check if any column for this row is already covered
        int curr = first;
        do {
            int col = m_nodes[curr].col;
            if (m_col_covered[col]) {
                return false;
            }
            curr = m_nodes[curr].right;
        } while (curr != first);

        curr = first;
        do {
            cover_column(m_nodes[curr].col);
            curr = m_nodes[curr].right;
        } while (curr != first);

        return true;
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

