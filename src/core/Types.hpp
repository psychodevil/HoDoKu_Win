#pragma once

#include <cstdint>
#include <bit>
#include <string_view>
#include <array>

namespace hodoku::core {

// Grid constants
inline constexpr int GRID_SIZE = 9;
inline constexpr int BOX_SIZE = 3;
inline constexpr int TOTAL_CELLS = 81;
inline constexpr int TOTAL_HOUSES = 27; // 9 rows, 9 cols, 9 boxes

// Candidate bitmasks (bits 0..8 correspond to digits 1..9)
using CandidateMask = uint16_t;
inline constexpr CandidateMask EMPTY_MASK = 0x0000;
inline constexpr CandidateMask ALL_CANDIDATES_MASK = 0x01FF; // 9 bits (digits 1-9)

enum class HouseType : uint8_t {
    Row = 0,
    Col = 1,
    Box = 2
};

enum class DifficultyLevel : uint8_t {
    Easy = 0,
    Medium = 1,
    Hard = 2,
    Unfair = 3,
    Extreme = 4
};

enum class CellStatus : uint8_t {
    Empty = 0,
    Given = 1,
    Solved = 2,
    UserSet = 3
};

// Bitwise helper functions
[[nodiscard]] constexpr CandidateMask digit_to_mask(int digit) noexcept {
    if (digit >= 1 && digit <= 9) {
        return static_cast<CandidateMask>(1u << (digit - 1));
    }
    return 0;
}

[[nodiscard]] constexpr bool mask_has_digit(CandidateMask mask, int digit) noexcept {
    return (mask & digit_to_mask(digit)) != 0;
}

[[nodiscard]] constexpr int count_candidates(CandidateMask mask) noexcept {
    return std::popcount(static_cast<unsigned int>(mask & ALL_CANDIDATES_MASK));
}

[[nodiscard]] constexpr int get_single_digit(CandidateMask mask) noexcept {
    if (count_candidates(mask) == 1) {
        return std::countr_zero(static_cast<unsigned int>(mask)) + 1;
    }
    return 0;
}

[[nodiscard]] constexpr CandidateMask remove_digit_from_mask(CandidateMask mask, int digit) noexcept {
    return mask & static_cast<CandidateMask>(~digit_to_mask(digit));
}

[[nodiscard]] constexpr CandidateMask add_digit_to_mask(CandidateMask mask, int digit) noexcept {
    return mask | digit_to_mask(digit);
}

// Coordinate helpers
[[nodiscard]] constexpr int cell_index(int row, int col) noexcept {
    return row * GRID_SIZE + col;
}

[[nodiscard]] constexpr int cell_row(int cell) noexcept {
    return cell / GRID_SIZE;
}

[[nodiscard]] constexpr int cell_col(int cell) noexcept {
    return cell % GRID_SIZE;
}

[[nodiscard]] constexpr int cell_box(int cell) noexcept {
    return (cell / 27) * 3 + (cell % 9) / 3;
}

[[nodiscard]] constexpr int box_row_col(int row, int col) noexcept {
    return (row / 3) * 3 + (col / 3);
}

} // namespace hodoku::core

