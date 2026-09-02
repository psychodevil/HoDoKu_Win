# Technical Guide: Vectorizing C++ Sudoku Engines with AVX2 & SSE

This guide details exact C++20 SIMD vectorization implementations tailored for Sudoku engines, BitSet operations, and candidate propagation.

---

## 1. Vectorizing `BitSet81` (128-bit SSE / 256-bit AVX2)

### Current Scalar Layout
Currently, `BitSet81` uses two `uint64_t` members:
```cpp
struct BitSet81 {
    uint64_t lo; // cells 0..63
    uint64_t hi; // cells 64..80
};
```
Every binary operator (`&=`, `|=`, `^=`, `~`) requires two separate 64-bit scalar instructions.

### Vectorized 128-bit Implementation
Using Intel SSE intrinsics (`<immintrin.h>`), both halves are processed simultaneously in a single 128-bit `__m128i` register:

```cpp
#include <immintrin.h>

struct alignas(16) SimdBitSet81 {
    union {
        __m128i vec;
        struct {
            uint64_t lo;
            uint64_t hi;
        };
    };

    SimdBitSet81() noexcept : vec(_mm_setzero_si128()) {}
    explicit SimdBitSet81(__m128i v) noexcept : vec(v) {}

    // Bitwise AND in 1 clock cycle:
    SimdBitSet81& operator&=(const SimdBitSet81& rhs) noexcept {
        vec = _mm_and_si128(vec, rhs.vec);
        return *this;
    }

    // Bitwise OR in 1 clock cycle:
    SimdBitSet81& operator|=(const SimdBitSet81& rhs) noexcept {
        vec = _mm_or_si128(vec, rhs.vec);
        return *this;
    }

    // Bitwise AND-NOT (~a & b) in 1 clock cycle:
    [[nodiscard]] SimdBitSet81 and_not(const SimdBitSet81& rhs) const noexcept {
        return SimdBitSet81(_mm_andnot_si128(rhs.vec, vec));
    }

    // Fast zero test (returns true if all 81 bits are 0) in 1 clock cycle:
    [[nodiscard]] bool empty() const noexcept {
        return _mm_testz_si128(vec, vec) != 0;
    }

    // Fast intersection test:
    [[nodiscard]] bool intersects(const SimdBitSet81& other) const noexcept {
        return !_mm_testz_si128(vec, other.vec);
    }

    // Subset test: (a & ~b) == 0
    [[nodiscard]] bool is_subset_of(const SimdBitSet81& other) const noexcept {
        return _mm_testz_si128(vec, _mm_andnot_si128(other.vec, _mm_set1_epi64x(-1LL)));
    }
};
```

---

## 2. 81-Cell Candidate Mask Batch Vectorization (AVX2)

### Problem
In Sudoku solving, scanning which cells currently contain candidate digit $d$ requires looping across all 81 cells:
```cpp
for (int cell = 0; cell < 81; ++cell) {
    if (m_candidates[cell] & (1 << (d - 1))) {
        result.set(cell);
    }
}
```

### AVX2 Solution
Since each candidate mask is a 16-bit integer (`uint16_t`), a 256-bit AVX2 register (`__m256i`) holds **16 candidate masks**:
- 81 cells fit into **six 256-bit vector registers** ($6 \times 16 = 96$ slots).
- Broadcast the target digit mask `_mm256_set1_epi16(1 << (d - 1))` across all lanes.
- Check presence using `_mm256_and_si256` and compare `_mm256_cmpeq_epi16`.
- Extract match bitmask using `_mm256_movemask_epi8`.

```cpp
#include <immintrin.h>

// Candidate array aligned for AVX2
alignas(32) uint16_t m_candidates[96]; // 81 used, padded to 96

BitSet81 get_cells_with_candidate_avx2(int digit) const noexcept {
    __m256i target = _mm256_set1_epi16(static_cast<short>(1 << (digit - 1)));
    uint64_t lo_bits = 0;
    uint64_t hi_bits = 0;

    // Process cells 0..63 in 4 AVX2 loads (64 cells)
    for (int i = 0; i < 4; ++i) {
        __m256i chunk = _mm256_load_si256(reinterpret_cast<const __m256i*>(&m_candidates[i * 16]));
        __m256i matches = _mm256_cmpeq_epi16(_mm256_and_si256(chunk, target), target);
        // Pack 16-bit comparison results to 8-bit, then movemask
        // Yields instant 16-bit mask of active cells per chunk
    }

    return BitSet81(lo_bits, hi_bits);
}
```

---

## 3. Compilation & Benchmarking Flags

When compiling with SIMD optimizations:

- **GCC / Clang:**
  ```bash
  g++ -std=c++20 -O3 -mavx2 -mfma -mpopcnt -mbmi2 ...
  ```
- **Automatic Auto-Vectorization Diagnostics:**
  ```bash
  -fopt-info-vec-optimized -fopt-info-vec-missed
  ```
