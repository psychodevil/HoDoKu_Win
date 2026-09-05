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

enum class SudokuVariant : uint8_t {
    Standard = 0,
    Diagonal = 1 << 0,     // X-Sudoku (+18 columns: main and anti diagonals)
    Hyper = 1 << 1,        // Windoku (+36 columns: 4 interior 3x3 windows)
    DiagonalHyper = Diagonal | Hyper // X-Windoku (+54 columns)
};

[[nodiscard]] constexpr bool has_diagonal_constraint(SudokuVariant v) noexcept {
    return (static_cast<uint8_t>(v) & static_cast<uint8_t>(SudokuVariant::Diagonal)) != 0;
}

[[nodiscard]] constexpr bool has_hyper_constraint(SudokuVariant v) noexcept {
    return (static_cast<uint8_t>(v) & static_cast<uint8_t>(SudokuVariant::Hyper)) != 0;
}

[[nodiscard]] constexpr SudokuVariant operator|(SudokuVariant a, SudokuVariant b) noexcept {
    return static_cast<SudokuVariant>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

[[nodiscard]] constexpr SudokuVariant operator&(SudokuVariant a, SudokuVariant b) noexcept {
    return static_cast<SudokuVariant>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

[[nodiscard]] constexpr std::string_view variant_name(SudokuVariant v) noexcept {
    switch (v) {
        case SudokuVariant::Standard: return "Standard";
        case SudokuVariant::Diagonal: return "Diagonal (X-Sudoku)";
        case SudokuVariant::Hyper: return "Hyper-Sudoku (Windoku)";
        case SudokuVariant::DiagonalHyper: return "Diagonal Hyper-Sudoku (X-Windoku)";
    }
    return "Unknown";
}

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

[[nodiscard]] constexpr std::string_view difficulty_name(DifficultyLevel level) noexcept {
    switch (level) {
        case DifficultyLevel::Easy: return "Easy";
        case DifficultyLevel::Medium: return "Medium";
        case DifficultyLevel::Hard: return "Hard";
        case DifficultyLevel::Unfair: return "Unfair";
        case DifficultyLevel::Extreme: return "Extreme";
    }
    return "Unknown";
}

inline std::string format_cell(int cell) {
    return "r" + std::to_string(cell_row(cell) + 1) + "c" + std::to_string(cell_col(cell) + 1);
}

} // namespace hodoku::core

