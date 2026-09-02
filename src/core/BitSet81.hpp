#pragma once

#include <cstdint>
#include <bit>
#include <string>
#include <concepts>
#include "Types.hpp"

namespace hodoku::core {

class BitSet81 {
public:
    static constexpr uint64_t HI_MASK = (1ULL << (TOTAL_CELLS - 64)) - 1ULL; // 17 bits: 0x1FFFF

    uint64_t lo{0};
    uint64_t hi{0};

    constexpr BitSet81() noexcept = default;

    constexpr BitSet81(uint64_t l, uint64_t h) noexcept : lo(l), hi(h & HI_MASK) {}

    [[nodiscard]] static constexpr BitSet81 all() noexcept {
        return BitSet81(~0ULL, HI_MASK);
    }

    [[nodiscard]] static constexpr BitSet81 from_cell(int cell) noexcept {
        BitSet81 bs;
        bs.set(cell);
        return bs;
    }

    constexpr void set(int cell) noexcept {
        if (cell >= 0 && cell < 64) {
            lo |= (1ULL << cell);
        } else if (cell >= 64 && cell < TOTAL_CELLS) {
            hi |= (1ULL << (cell - 64));
        }
    }

    constexpr void reset(int cell) noexcept {
        if (cell >= 0 && cell < 64) {
            lo &= ~(1ULL << cell);
        } else if (cell >= 64 && cell < TOTAL_CELLS) {
            hi &= ~(1ULL << (cell - 64));
        }
    }

    constexpr void clear() noexcept {
        lo = 0;
        hi = 0;
    }

    constexpr void toggle(int cell) noexcept {
        if (cell >= 0 && cell < 64) {
            lo ^= (1ULL << cell);
        } else if (cell >= 64 && cell < TOTAL_CELLS) {
            hi ^= (1ULL << (cell - 64));
        }
    }

    [[nodiscard]] constexpr bool test(int cell) const noexcept {
        if (cell >= 0 && cell < 64) {
            return (lo & (1ULL << cell)) != 0;
        } else if (cell >= 64 && cell < TOTAL_CELLS) {
            return (hi & (1ULL << (cell - 64))) != 0;
        }
        return false;
    }

    [[nodiscard]] constexpr bool contains(int cell) const noexcept {
        return test(cell);
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return lo == 0 && hi == 0;
    }

    [[nodiscard]] constexpr bool any() const noexcept {
        return !empty();
    }

    [[nodiscard]] constexpr int count() const noexcept {
        return std::popcount(lo) + std::popcount(hi);
    }

    [[nodiscard]] constexpr bool is_subset_of(const BitSet81& other) const noexcept {
        return ((lo & ~other.lo) == 0) && ((hi & ~other.hi) == 0);
    }

    [[nodiscard]] constexpr bool intersects(const BitSet81& other) const noexcept {
        return ((lo & other.lo) != 0) || ((hi & other.hi) != 0);
    }

    [[nodiscard]] constexpr int first_cell() const noexcept {
        if (lo != 0) {
            return std::countr_zero(lo);
        }
        if (hi != 0) {
            return std::countr_zero(hi) + 64;
        }
        return -1;
    }

    constexpr int pop_first_cell() noexcept {
        if (lo != 0) {
            int bit = std::countr_zero(lo);
            lo &= (lo - 1);
            return bit;
        }
        if (hi != 0) {
            int bit = std::countr_zero(hi) + 64;
            hi &= (hi - 1);
            return bit;
        }
        return -1;
    }

    template <typename Func>
    constexpr void for_each_cell(Func&& fn) const {
        uint64_t temp_lo = lo;
        while (temp_lo != 0) {
            int bit = std::countr_zero(temp_lo);
            fn(bit);
            temp_lo &= (temp_lo - 1);
        }
        uint64_t temp_hi = hi;
        while (temp_hi != 0) {
            int bit = std::countr_zero(temp_hi);
            fn(bit + 64);
            temp_hi &= (temp_hi - 1);
        }
    }

    constexpr BitSet81& operator&=(const BitSet81& rhs) noexcept {
        lo &= rhs.lo;
        hi &= rhs.hi;
        return *this;
    }

    constexpr BitSet81& operator|=(const BitSet81& rhs) noexcept {
        lo |= rhs.lo;
        hi |= rhs.hi;
        return *this;
    }

    constexpr BitSet81& operator^=(const BitSet81& rhs) noexcept {
        lo ^= rhs.lo;
        hi ^= rhs.hi;
        return *this;
    }

    [[nodiscard]] friend constexpr BitSet81 operator&(BitSet81 lhs, const BitSet81& rhs) noexcept {
        return BitSet81(lhs.lo & rhs.lo, lhs.hi & rhs.hi);
    }

    [[nodiscard]] friend constexpr BitSet81 operator|(BitSet81 lhs, const BitSet81& rhs) noexcept {
        return BitSet81(lhs.lo | rhs.lo, lhs.hi | rhs.hi);
    }

    [[nodiscard]] friend constexpr BitSet81 operator^(BitSet81 lhs, const BitSet81& rhs) noexcept {
        return BitSet81(lhs.lo ^ rhs.lo, lhs.hi ^ rhs.hi);
    }

    [[nodiscard]] friend constexpr BitSet81 operator~(BitSet81 bs) noexcept {
        return BitSet81(~bs.lo, (~bs.hi) & HI_MASK);
    }

    [[nodiscard]] friend constexpr bool operator==(const BitSet81& lhs, const BitSet81& rhs) noexcept {
        return lhs.lo == rhs.lo && lhs.hi == rhs.hi;
    }

    [[nodiscard]] friend constexpr bool operator!=(const BitSet81& lhs, const BitSet81& rhs) noexcept {
        return !(lhs == rhs);
    }
};

} // namespace hodoku::core

