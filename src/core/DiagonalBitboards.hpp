#pragma once

#include <array>
#include <string_view>
#include "Types.hpp"
#include "BitSet81.hpp"
#include "GridConstants.hpp"

namespace hodoku::core {

enum class DiagonalType : uint8_t {
    Main = 0, // Top-left to bottom-right (\): cells (0,0) to (8,8)
    Anti = 1  // Top-right to bottom-left (/): cells (0,8) to (8,0)
};

inline constexpr int DIAGONAL_HOUSES = 2;

struct DiagonalBitboards {
    // 1. Bitmasks for main, anti, and combined diagonals
    BitSet81 main_diagonal_mask{};
    BitSet81 anti_diagonal_mask{};
    BitSet81 all_diagonal_mask{};
    BitSet81 intersection_mask{}; // Cell 40 (r4c4)

    // 2. Cell arrays for each diagonal (9 cells each)
    std::array<int, 9> main_diagonal_cells{};
    std::array<int, 9> anti_diagonal_cells{};

    // 3. House arrays for indexing: [0] = Main, [1] = Anti
    std::array<std::array<int, 9>, DIAGONAL_HOUSES> house_cells{};
    std::array<BitSet81, DIAGONAL_HOUSES> house_bitsets{};

    // 4. Per-cell properties
    std::array<bool, TOTAL_CELLS> is_main_diagonal{};
    std::array<bool, TOTAL_CELLS> is_anti_diagonal{};
    std::array<bool, TOTAL_CELLS> is_any_diagonal{};
    std::array<uint8_t, TOTAL_CELLS> diagonal_membership_count{}; // 0, 1, or 2

    // 5. Diagonal peer relationships
    // Diagonal-only peers of a cell (other cells on same diagonal(s), excluding itself)
    std::array<BitSet81, TOTAL_CELLS> diagonal_peer_bitsets{};
    // Complete X-Sudoku peers: standard 20 peers | diagonal peers (excluding itself)
    std::array<BitSet81, TOTAL_CELLS> x_peer_bitsets{};
    std::array<uint8_t, TOTAL_CELLS> x_peer_counts{};
    std::array<std::array<int, 32>, TOTAL_CELLS> x_peer_cells{};
};

[[nodiscard]] constexpr DiagonalBitboards build_diagonal_bitboards() noexcept {
    DiagonalBitboards d{};

    // 1. Build main diagonal: r == c, cell = r * 9 + r
    for (int r = 0; r < GRID_SIZE; ++r) {
        int cell = cell_index(r, r);
        d.main_diagonal_cells[r] = cell;
        d.main_diagonal_mask.set(cell);
        d.house_cells[0][r] = cell;
    }
    d.house_bitsets[0] = d.main_diagonal_mask;

    // 2. Build anti-diagonal: r + c == 8, c = 8 - r, cell = r * 9 + (8 - r)
    for (int r = 0; r < GRID_SIZE; ++r) {
        int c = GRID_SIZE - 1 - r;
        int cell = cell_index(r, c);
        d.anti_diagonal_cells[r] = cell;
        d.anti_diagonal_mask.set(cell);
        d.house_cells[1][r] = cell;
    }
    d.house_bitsets[1] = d.anti_diagonal_mask;

    // 3. Combined masks
    d.all_diagonal_mask = d.main_diagonal_mask | d.anti_diagonal_mask;
    d.intersection_mask = d.main_diagonal_mask & d.anti_diagonal_mask; // Exactly cell 40

    // 4. Per-cell attributes and peer masks
    for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
        int r = cell / GRID_SIZE;
        int c = cell % GRID_SIZE;

        bool on_main = (r == c);
        bool on_anti = (r + c == (GRID_SIZE - 1));

        d.is_main_diagonal[cell] = on_main;
        d.is_anti_diagonal[cell] = on_anti;
        d.is_any_diagonal[cell] = (on_main || on_anti);
        d.diagonal_membership_count[cell] = static_cast<uint8_t>((on_main ? 1 : 0) + (on_anti ? 1 : 0));

        BitSet81 diag_peers{};
        if (on_main) {
            diag_peers |= d.main_diagonal_mask;
        }
        if (on_anti) {
            diag_peers |= d.anti_diagonal_mask;
        }
        diag_peers.reset(cell);
        d.diagonal_peer_bitsets[cell] = diag_peers;

        BitSet81 x_peers = GRID.peer_bitsets[cell] | diag_peers;
        d.x_peer_bitsets[cell] = x_peers;
        d.x_peer_counts[cell] = static_cast<uint8_t>(x_peers.count());

        int idx = 0;
        x_peers.for_each_cell([&](int peer_cell) {
            d.x_peer_cells[cell][idx++] = peer_cell;
        });
    }

    return d;
}

// Precomputed constexpr lookup tables for Diagonal Sudoku (X-Sudoku)
inline constexpr DiagonalBitboards DIAGONALS = build_diagonal_bitboards();

// Fast inline accessors
[[nodiscard]] constexpr bool is_main_diagonal_cell(int cell) noexcept {
    return (cell >= 0 && cell < TOTAL_CELLS) && DIAGONALS.is_main_diagonal[cell];
}

[[nodiscard]] constexpr bool is_anti_diagonal_cell(int cell) noexcept {
    return (cell >= 0 && cell < TOTAL_CELLS) && DIAGONALS.is_anti_diagonal[cell];
}

[[nodiscard]] constexpr bool is_diagonal_cell(int cell) noexcept {
    return (cell >= 0 && cell < TOTAL_CELLS) && DIAGONALS.is_any_diagonal[cell];
}

[[nodiscard]] constexpr uint8_t get_diagonal_membership_count(int cell) noexcept {
    return (cell >= 0 && cell < TOTAL_CELLS) ? DIAGONALS.diagonal_membership_count[cell] : 0;
}

[[nodiscard]] constexpr const BitSet81& get_main_diagonal_bitset() noexcept {
    return DIAGONALS.main_diagonal_mask;
}

[[nodiscard]] constexpr const BitSet81& get_anti_diagonal_bitset() noexcept {
    return DIAGONALS.anti_diagonal_mask;
}

[[nodiscard]] constexpr const BitSet81& get_all_diagonals_bitset() noexcept {
    return DIAGONALS.all_diagonal_mask;
}

[[nodiscard]] constexpr const BitSet81& get_diagonal_house_bitset(DiagonalType type) noexcept {
    return DIAGONALS.house_bitsets[static_cast<size_t>(type)];
}

[[nodiscard]] constexpr const std::array<int, 9>& get_diagonal_house_cells(DiagonalType type) noexcept {
    return DIAGONALS.house_cells[static_cast<size_t>(type)];
}

[[nodiscard]] constexpr const BitSet81& get_diagonal_peer_bitset(int cell) noexcept {
    return DIAGONALS.diagonal_peer_bitsets[cell];
}

[[nodiscard]] constexpr const BitSet81& get_x_peer_bitset(int cell) noexcept {
    return DIAGONALS.x_peer_bitsets[cell];
}

[[nodiscard]] constexpr uint8_t get_x_peer_count(int cell) noexcept {
    return DIAGONALS.x_peer_counts[cell];
}

[[nodiscard]] constexpr const std::array<int, 32>& get_x_peer_cells(int cell) noexcept {
    return DIAGONALS.x_peer_cells[cell];
}

[[nodiscard]] constexpr std::string_view get_diagonal_name(DiagonalType type) noexcept {
    switch (type) {
        case DiagonalType::Main: return "Main Diagonal (\\)";
        case DiagonalType::Anti: return "Anti-Diagonal (/)";
    }
    return "Unknown Diagonal";
}

} // namespace hodoku::core
