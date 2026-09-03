#pragma once

#include <array>
#include <string>
#include <string_view>
#include <optional>
#include <sstream>
#include <vector>
#include "Types.hpp"
#include "BitSet81.hpp"
#include "GridConstants.hpp"

#if defined(__AVX2__)
#include <immintrin.h>
#endif

namespace hodoku::core {

class BoardState {
public:
#if defined(__AVX2__)
    static inline uint32_t extract_mask16(__m256i cmp) noexcept {
#if defined(__BMI2__)
        return _pext_u32(static_cast<uint32_t>(_mm256_movemask_epi8(cmp)), 0x55555555U);
#else
        __m128i lo_lane = _mm256_castsi256_si128(cmp);
        __m128i hi_lane = _mm256_extracti128_si256(cmp, 1);
        __m128i packed = _mm_packs_epi16(lo_lane, hi_lane);
        return static_cast<uint32_t>(_mm_movemask_epi8(packed));
#endif
    }
#endif

    BoardState() noexcept {
        clear();
    }

    explicit BoardState(std::string_view puzzle_str) {
        clear();
        from_string(puzzle_str);
    }

    void clear() noexcept {
        m_values.fill(0);
        m_candidates.fill(0);
        for (int i = 0; i < TOTAL_CELLS; ++i) m_candidates[i] = ALL_CANDIDATES_MASK;
        m_givens.clear();
        m_unfilled_cells = BitSet81::all();
        m_contradiction = false;

        for (int h = 0; h < TOTAL_HOUSES; ++h) {
            m_house_candidate_counts[h].fill(GRID_SIZE);
            m_house_candidate_counts[h][0] = 0; // digit 0 is unused
        }
    }

    [[nodiscard]] uint8_t get_value(int cell) const noexcept {
        return (cell >= 0 && cell < TOTAL_CELLS) ? m_values[cell] : 0;
    }

    [[nodiscard]] CandidateMask get_candidates(int cell) const noexcept {
        return (cell >= 0 && cell < TOTAL_CELLS) ? m_candidates[cell] : EMPTY_MASK;
    }

    [[nodiscard]] bool is_given(int cell) const noexcept {
        return m_givens.test(cell);
    }

    [[nodiscard]] bool is_unfilled(int cell) const noexcept {
        return m_unfilled_cells.test(cell);
    }

    [[nodiscard]] int unfilled_count() const noexcept {
        return m_unfilled_cells.count();
    }

    [[nodiscard]] bool has_contradiction() const noexcept {
        return m_contradiction;
    }

    [[nodiscard]] bool has_candidate(int cell, int digit) const noexcept {
        return mask_has_digit(get_candidates(cell), digit);
    }

    [[nodiscard]] int count_candidates(int cell) const noexcept {
        return hodoku::core::count_candidates(get_candidates(cell));
    }

    [[nodiscard]] const BitSet81& get_givens() const noexcept {
        return m_givens;
    }

    [[nodiscard]] const BitSet81& get_unfilled_cells() const noexcept {
        return m_unfilled_cells;
    }

    [[nodiscard]] uint8_t get_house_candidate_count(int house_idx, int digit) const noexcept {
        if (house_idx >= 0 && house_idx < TOTAL_HOUSES && digit >= 1 && digit <= 9) {
            return m_house_candidate_counts[house_idx][digit];
        }
        return 0;
    }

    [[nodiscard]] BitSet81 get_cells_with_candidate(int digit) const noexcept {
#if defined(__AVX2__)
        __m256i tgt = _mm256_set1_epi16(static_cast<short>(digit_to_mask(digit)));
        const __m256i* ptr = reinterpret_cast<const __m256i*>(m_candidates.data());

        uint64_t m0 = extract_mask16(_mm256_cmpeq_epi16(_mm256_and_si256(_mm256_load_si256(ptr + 0), tgt), tgt));
        uint64_t m1 = extract_mask16(_mm256_cmpeq_epi16(_mm256_and_si256(_mm256_load_si256(ptr + 1), tgt), tgt));
        uint64_t m2 = extract_mask16(_mm256_cmpeq_epi16(_mm256_and_si256(_mm256_load_si256(ptr + 2), tgt), tgt));
        uint64_t m3 = extract_mask16(_mm256_cmpeq_epi16(_mm256_and_si256(_mm256_load_si256(ptr + 3), tgt), tgt));
        uint64_t lo_bits = m0 | (m1 << 16) | (m2 << 32) | (m3 << 48);

        uint64_t m4 = extract_mask16(_mm256_cmpeq_epi16(_mm256_and_si256(_mm256_load_si256(ptr + 4), tgt), tgt));
        uint64_t m5 = extract_mask16(_mm256_cmpeq_epi16(_mm256_and_si256(_mm256_load_si256(ptr + 5), tgt), tgt));
        uint64_t hi_bits = (m4 | (m5 << 16)) & BitSet81::HI_MASK;

        return BitSet81(lo_bits, hi_bits) & m_unfilled_cells;
#else
        BitSet81 result;
        CandidateMask target = digit_to_mask(digit);
        m_unfilled_cells.for_each_cell([&](int cell) {
            if ((m_candidates[cell] & target) != 0) {
                result.set(cell);
            }
        });
        return result;
#endif
    }

    [[nodiscard]] BitSet81 get_candidates_in_house(int house_idx, int digit) const noexcept {
        return get_cells_with_candidate(digit) & GRID.house_bitsets[house_idx];
    }

    bool remove_candidate(int cell, int digit) noexcept {
        if (cell < 0 || cell >= TOTAL_CELLS || digit < 1 || digit > 9) return false;
        if (!m_unfilled_cells.test(cell)) return false;

        CandidateMask d_mask = digit_to_mask(digit);
        if ((m_candidates[cell] & d_mask) == 0) return false;

        m_candidates[cell] &= ~d_mask;

        // Update house candidate counts
        for (int h : GRID.cell_houses[cell]) {
            if (m_house_candidate_counts[h][digit] > 0) {
                --m_house_candidate_counts[h][digit];
            }
        }

        if (m_candidates[cell] == EMPTY_MASK) {
            m_contradiction = true;
        }

        return true;
    }

    bool add_candidate(int cell, int digit) noexcept {
        if (cell < 0 || cell >= TOTAL_CELLS || digit < 1 || digit > 9) return false;
        if (!m_unfilled_cells.test(cell)) return false;

        CandidateMask d_mask = digit_to_mask(digit);
        if ((m_candidates[cell] & d_mask) != 0) return false;

        m_candidates[cell] |= d_mask;

        for (int h : GRID.cell_houses[cell]) {
            ++m_house_candidate_counts[h][digit];
        }

        return true;
    }

    void set_candidates(int cell, CandidateMask mask) noexcept {
        if (cell < 0 || cell >= TOTAL_CELLS) return;
        m_unfilled_cells.set(cell);
        m_values[cell] = 0;
        for (int d = 1; d <= 9; ++d) {
            bool had = (m_candidates[cell] & digit_to_mask(d)) != 0;
            bool will_have = (mask & digit_to_mask(d)) != 0;
            if (had && !will_have) {
                for (int h : GRID.cell_houses[cell]) {
                    if (m_house_candidate_counts[h][d] > 0) --m_house_candidate_counts[h][d];
                }
            } else if (!had && will_have) {
                for (int h : GRID.cell_houses[cell]) {
                    ++m_house_candidate_counts[h][d];
                }
            }
        }
        m_candidates[cell] = mask;
    }

    bool set_value(int cell, int digit) noexcept {
        if (cell < 0 || cell >= TOTAL_CELLS || digit < 1 || digit > 9) return false;

        if (!m_unfilled_cells.test(cell)) {
            return m_values[cell] == digit;
        }

        // Remove all current candidates of this cell from house counts
        CandidateMask cur_mask = m_candidates[cell];
        for (int d = 1; d <= 9; ++d) {
            if (mask_has_digit(cur_mask, d)) {
                for (int h : GRID.cell_houses[cell]) {
                    if (m_house_candidate_counts[h][d] > 0) {
                        --m_house_candidate_counts[h][d];
                    }
                }
            }
        }

        m_values[cell] = static_cast<uint8_t>(digit);
        m_candidates[cell] = EMPTY_MASK;
        m_unfilled_cells.reset(cell);

        // Eliminate this digit from all 20 peers
        for (int peer : GRID.peer_cells[cell]) {
            if (m_unfilled_cells.test(peer)) {
                remove_candidate(peer, digit);
            }
        }

        return !m_contradiction;
    }

    bool set_given(int cell, int digit) noexcept {
        if (cell < 0 || cell >= TOTAL_CELLS || digit < 1 || digit > 9) return false;
        m_givens.set(cell);
        return set_value(cell, digit);
    }

    [[nodiscard]] bool is_valid() const noexcept {
        if (m_contradiction) return false;

        // Check each house for duplicate values
        for (int h = 0; h < TOTAL_HOUSES; ++h) {
            uint16_t seen = 0;
            for (int cell : GRID.house_cells[h]) {
                uint8_t val = m_values[cell];
                if (val != 0) {
                    uint16_t mask = digit_to_mask(val);
                    if ((seen & mask) != 0) {
                        return false; // duplicate in house
                    }
                    seen |= mask;
                }
            }
        }

        // Check unfilled cells have at least one candidate
        bool valid_candidates = true;
        m_unfilled_cells.for_each_cell([&](int cell) {
            if (m_candidates[cell] == EMPTY_MASK) {
                valid_candidates = false;
            }
        });

        return valid_candidates;
    }

    [[nodiscard]] bool is_solved() const noexcept {
        return m_unfilled_cells.empty() && is_valid();
    }

    [[nodiscard]] std::string to_string() const {
        std::string result;
        result.reserve(TOTAL_CELLS);
        for (int i = 0; i < TOTAL_CELLS; ++i) {
            if (m_values[i] != 0) {
                result.push_back(static_cast<char>('0' + m_values[i]));
            } else {
                result.push_back('.');
            }
        }
        return result;
    }

    bool from_string(std::string_view puzzle_str) {
        clear();
        int cell_idx = 0;
        for (char ch : puzzle_str) {
            if (cell_idx >= TOTAL_CELLS) break;

            if (ch >= '1' && ch <= '9') {
                int digit = ch - '0';
                if (!set_given(cell_idx, digit)) {
                    return false;
                }
                ++cell_idx;
            } else if (ch == '.' || ch == '0') {
                ++cell_idx;
            }
            // Ignore whitespaces, newlines, dividers
        }
        return !m_contradiction && (cell_idx == TOTAL_CELLS);
    }

private:
    std::array<uint8_t, TOTAL_CELLS> m_values{};
    alignas(32) std::array<CandidateMask, 96> m_candidates{};
    BitSet81 m_givens{};
    BitSet81 m_unfilled_cells{};
    std::array<std::array<uint8_t, 10>, TOTAL_HOUSES> m_house_candidate_counts{};
    bool m_contradiction{false};
};

} // namespace hodoku::core

