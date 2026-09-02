#pragma once

#include <array>
#include "Types.hpp"
#include "BitSet81.hpp"

namespace hodoku::core {

struct GridTables {
    std::array<std::array<int, 9>, 9> row_cells{};
    std::array<std::array<int, 9>, 9> col_cells{};
    std::array<std::array<int, 9>, 9> box_cells{};
    std::array<std::array<int, 9>, TOTAL_HOUSES> house_cells{};

    std::array<BitSet81, 9> row_bitsets{};
    std::array<BitSet81, 9> col_bitsets{};
    std::array<BitSet81, 9> box_bitsets{};
    std::array<BitSet81, TOTAL_HOUSES> house_bitsets{};

    std::array<int, TOTAL_CELLS> cell_row{};
    std::array<int, TOTAL_CELLS> cell_col{};
    std::array<int, TOTAL_CELLS> cell_box{};
    std::array<std::array<int, 3>, TOTAL_CELLS> cell_houses{};

    std::array<BitSet81, TOTAL_CELLS> peer_bitsets{};
    std::array<std::array<int, 20>, TOTAL_CELLS> peer_cells{};
};

[[nodiscard]] constexpr GridTables build_grid_tables() noexcept {
    GridTables t{};

    // 1. Build basic cell row/col/box
    for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
        int r = cell / GRID_SIZE;
        int c = cell % GRID_SIZE;
        int b = (r / BOX_SIZE) * BOX_SIZE + (c / BOX_SIZE);

        t.cell_row[cell] = r;
        t.cell_col[cell] = c;
        t.cell_box[cell] = b;

        t.cell_houses[cell][0] = r;             // Row house (0..8)
        t.cell_houses[cell][1] = 9 + c;         // Col house (9..17)
        t.cell_houses[cell][2] = 18 + b;        // Box house (18..26)
    }

    // 2. Build rows
    for (int r = 0; r < GRID_SIZE; ++r) {
        for (int c = 0; c < GRID_SIZE; ++c) {
            int cell = cell_index(r, c);
            t.row_cells[r][c] = cell;
            t.row_bitsets[r].set(cell);
            t.house_cells[r][c] = cell;
            t.house_bitsets[r].set(cell);
        }
    }

    // 3. Build cols
    for (int c = 0; c < GRID_SIZE; ++c) {
        for (int r = 0; r < GRID_SIZE; ++r) {
            int cell = cell_index(r, c);
            t.col_cells[c][r] = cell;
            t.col_bitsets[c].set(cell);
            t.house_cells[9 + c][r] = cell;
            t.house_bitsets[9 + c].set(cell);
        }
    }

    // 4. Build boxes
    for (int b = 0; b < GRID_SIZE; ++b) {
        int start_r = (b / BOX_SIZE) * BOX_SIZE;
        int start_c = (b % BOX_SIZE) * BOX_SIZE;
        int idx = 0;
        for (int dr = 0; dr < BOX_SIZE; ++dr) {
            for (int dc = 0; dc < BOX_SIZE; ++dc) {
                int cell = cell_index(start_r + dr, start_c + dc);
                t.box_cells[b][idx] = cell;
                t.box_bitsets[b].set(cell);
                t.house_cells[18 + b][idx] = cell;
                t.house_bitsets[18 + b].set(cell);
                ++idx;
            }
        }
    }

    // 5. Build peer bitsets and peer lists
    for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
        int r = t.cell_row[cell];
        int c = t.cell_col[cell];
        int b = t.cell_box[cell];

        BitSet81 peers = t.row_bitsets[r] | t.col_bitsets[c] | t.box_bitsets[b];
        peers.reset(cell);
        t.peer_bitsets[cell] = peers;

        int peer_idx = 0;
        peers.for_each_cell([&](int peer_cell) {
            t.peer_cells[cell][peer_idx++] = peer_cell;
        });
    }

    return t;
}

// Compile-time precomputed constant tables
inline constexpr GridTables GRID = build_grid_tables();

// Fast inline accessors
[[nodiscard]] constexpr const BitSet81& get_peer_bitset(int cell) noexcept {
    return GRID.peer_bitsets[cell];
}

[[nodiscard]] constexpr const std::array<int, 20>& get_peer_cells(int cell) noexcept {
    return GRID.peer_cells[cell];
}

[[nodiscard]] constexpr const BitSet81& get_house_bitset(int house_index) noexcept {
    return GRID.house_bitsets[house_index];
}

[[nodiscard]] constexpr const std::array<int, 9>& get_house_cells(int house_index) noexcept {
    return GRID.house_cells[house_index];
}

[[nodiscard]] constexpr int get_house_index(HouseType type, int index) noexcept {
    switch (type) {
        case HouseType::Row: return index;
        case HouseType::Col: return 9 + index;
        case HouseType::Box: return 18 + index;
    }
    return 0;
}

} // namespace hodoku::core

