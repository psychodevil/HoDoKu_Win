#pragma once

#include <array>
#include <string_view>
#include "Types.hpp"
#include "BitSet81.hpp"
#include "GridConstants.hpp"
#include "DiagonalBitboards.hpp"

namespace hodoku::core {

enum class HyperWindow : uint8_t {
    TopLeft = 0,     // Rows 1..3, Cols 1..3 (r2-r4, c2-c4 in 1-based)
    TopRight = 1,    // Rows 1..3, Cols 5..7 (r2-r4, c6-c8 in 1-based)
    BottomLeft = 2,  // Rows 5..7, Cols 1..3 (r6-r8, c2-c4 in 1-based)
    BottomRight = 3  // Rows 5..7, Cols 5..7 (r6-r8, c6-c8 in 1-based)
};

inline constexpr int HYPER_WINDOWS = 4;
inline constexpr int HYPER_WINDOW_SIZE = 9;

struct HyperSudokuWindows {
    // 1. Bitmasks for each window and combined mask
    std::array<BitSet81, HYPER_WINDOWS> window_masks{};
    BitSet81 all_windows_mask{};

    // 2. Cell lists for each window (9 cells each)
    std::array<std::array<int, HYPER_WINDOW_SIZE>, HYPER_WINDOWS> window_cells{};

    // 3. Per-cell properties
    std::array<bool, TOTAL_CELLS> is_window_cell{};
    std::array<int8_t, TOTAL_CELLS> cell_window_index{}; // 0..3, or -1 if not in any window

    // 4. Peer relationships
    // Window-only peers (other 8 cells in the same window, excluding itself; empty if not in a window)
    std::array<BitSet81, TOTAL_CELLS> window_peer_bitsets{};

    // Complete Hyper-Sudoku peers: standard 20 peers | window peers (excluding itself)
    std::array<BitSet81, TOTAL_CELLS> hyper_peer_bitsets{};
    std::array<uint8_t, TOTAL_CELLS> hyper_peer_counts{};
    std::array<std::array<int, 32>, TOTAL_CELLS> hyper_peer_cells{};

    // Combined Diagonal + Hyper-Sudoku (X-Windoku) peers: standard | diagonal | window peers
    std::array<BitSet81, TOTAL_CELLS> x_hyper_peer_bitsets{};
    std::array<uint8_t, TOTAL_CELLS> x_hyper_peer_counts{};
    std::array<std::array<int, 40>, TOTAL_CELLS> x_hyper_peer_cells{};
};

[[nodiscard]] constexpr HyperSudokuWindows build_hyper_sudoku_windows() noexcept {
    HyperSudokuWindows h{};

    // Window top-left coordinates: (row_start, col_start)
    constexpr int window_row_starts[HYPER_WINDOWS] = {1, 1, 5, 5};
    constexpr int window_col_starts[HYPER_WINDOWS] = {1, 5, 1, 5};

    // Initialize all cells as non-window by default
    for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
        h.is_window_cell[cell] = false;
        h.cell_window_index[cell] = -1;
    }

    // 1. Build the 4 interior 3x3 window regions
    for (int w = 0; w < HYPER_WINDOWS; ++w) {
        int r_start = window_row_starts[w];
        int c_start = window_col_starts[w];
        int idx = 0;

        for (int dr = 0; dr < 3; ++dr) {
            for (int dc = 0; dc < 3; ++dc) {
                int r = r_start + dr;
                int c = c_start + dc;
                int cell = cell_index(r, c);

                h.window_cells[w][idx++] = cell;
                h.window_masks[w].set(cell);
                h.is_window_cell[cell] = true;
                h.cell_window_index[cell] = static_cast<int8_t>(w);
            }
        }
        h.all_windows_mask |= h.window_masks[w];
    }

    // 2. Build peer relationships and lookup tables
    for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
        BitSet81 win_peers{};
        int8_t win_idx = h.cell_window_index[cell];

        if (win_idx >= 0) {
            win_peers = h.window_masks[static_cast<size_t>(win_idx)];
            win_peers.reset(cell);
        }
        h.window_peer_bitsets[cell] = win_peers;

        // Hyper-Sudoku peers: standard 20 peers | window peers
        BitSet81 hyper_peers = GRID.peer_bitsets[cell] | win_peers;
        h.hyper_peer_bitsets[cell] = hyper_peers;
        h.hyper_peer_counts[cell] = static_cast<uint8_t>(hyper_peers.count());

        int peer_idx = 0;
        hyper_peers.for_each_cell([&](int p_cell) {
            h.hyper_peer_cells[cell][peer_idx++] = p_cell;
        });

        // Combined X-Windoku peers: standard | diagonal | window peers
        BitSet81 x_hyper_peers = DIAGONALS.x_peer_bitsets[cell] | win_peers;
        h.x_hyper_peer_bitsets[cell] = x_hyper_peers;
        h.x_hyper_peer_counts[cell] = static_cast<uint8_t>(x_hyper_peers.count());

        int x_hyper_idx = 0;
        x_hyper_peers.for_each_cell([&](int p_cell) {
            h.x_hyper_peer_cells[cell][x_hyper_idx++] = p_cell;
        });
    }

    return h;
}

// Precomputed constexpr lookup tables for Hyper-Sudoku (Windoku)
inline constexpr HyperSudokuWindows HYPER_WINDOWS_DATA = build_hyper_sudoku_windows();
inline constexpr const HyperSudokuWindows& WINDOKU = HYPER_WINDOWS_DATA;

// Fast inline accessors
[[nodiscard]] constexpr bool is_hyper_window_cell(int cell) noexcept {
    return (cell >= 0 && cell < TOTAL_CELLS) && HYPER_WINDOWS_DATA.is_window_cell[cell];
}

[[nodiscard]] constexpr int8_t get_hyper_window_index(int cell) noexcept {
    return (cell >= 0 && cell < TOTAL_CELLS) ? HYPER_WINDOWS_DATA.cell_window_index[cell] : static_cast<int8_t>(-1);
}

[[nodiscard]] constexpr const BitSet81& get_hyper_window_bitset(HyperWindow win) noexcept {
    return HYPER_WINDOWS_DATA.window_masks[static_cast<size_t>(win)];
}

[[nodiscard]] constexpr const BitSet81& get_hyper_window_bitset(int window_idx) noexcept {
    return HYPER_WINDOWS_DATA.window_masks[static_cast<size_t>(window_idx)];
}

[[nodiscard]] constexpr const BitSet81& get_all_hyper_windows_bitset() noexcept {
    return HYPER_WINDOWS_DATA.all_windows_mask;
}

[[nodiscard]] constexpr const std::array<int, HYPER_WINDOW_SIZE>& get_hyper_window_cells(HyperWindow win) noexcept {
    return HYPER_WINDOWS_DATA.window_cells[static_cast<size_t>(win)];
}

[[nodiscard]] constexpr const std::array<int, HYPER_WINDOW_SIZE>& get_hyper_window_cells(int window_idx) noexcept {
    return HYPER_WINDOWS_DATA.window_cells[static_cast<size_t>(window_idx)];
}

[[nodiscard]] constexpr const BitSet81& get_hyper_window_peer_bitset(int cell) noexcept {
    return HYPER_WINDOWS_DATA.window_peer_bitsets[cell];
}

[[nodiscard]] constexpr const BitSet81& get_hyper_peer_bitset(int cell) noexcept {
    return HYPER_WINDOWS_DATA.hyper_peer_bitsets[cell];
}

[[nodiscard]] constexpr uint8_t get_hyper_peer_count(int cell) noexcept {
    return HYPER_WINDOWS_DATA.hyper_peer_counts[cell];
}

[[nodiscard]] constexpr const std::array<int, 32>& get_hyper_peer_cells(int cell) noexcept {
    return HYPER_WINDOWS_DATA.hyper_peer_cells[cell];
}

[[nodiscard]] constexpr const BitSet81& get_x_hyper_peer_bitset(int cell) noexcept {
    return HYPER_WINDOWS_DATA.x_hyper_peer_bitsets[cell];
}

[[nodiscard]] constexpr uint8_t get_x_hyper_peer_count(int cell) noexcept {
    return HYPER_WINDOWS_DATA.x_hyper_peer_counts[cell];
}

[[nodiscard]] constexpr const std::array<int, 40>& get_x_hyper_peer_cells(int cell) noexcept {
    return HYPER_WINDOWS_DATA.x_hyper_peer_cells[cell];
}

[[nodiscard]] constexpr std::string_view get_hyper_window_name(HyperWindow win) noexcept {
    switch (win) {
        case HyperWindow::TopLeft: return "Top-Left Window (r2-4, c2-4)";
        case HyperWindow::TopRight: return "Top-Right Window (r2-4, c6-8)";
        case HyperWindow::BottomLeft: return "Bottom-Left Window (r6-8, c2-4)";
        case HyperWindow::BottomRight: return "Bottom-Right Window (r6-8, c6-8)";
    }
    return "Unknown Hyper Window";
}

[[nodiscard]] constexpr std::string_view get_hyper_window_name(int window_idx) noexcept {
    if (window_idx >= 0 && window_idx < HYPER_WINDOWS) {
        return get_hyper_window_name(static_cast<HyperWindow>(window_idx));
    }
    return "Unknown Hyper Window";
}

} // namespace hodoku::core
