---
name: simd-scanner
description: >-
  Scans C++ source code to identify loops, bitwise operations, array reductions, and candidate
  manipulations that can be accelerated using SIMD instructions (SSE4.2, AVX2, AVX-512, NEON).
  Use this skill when analyzing codebase performance, profiling data-parallel operations, or optimizing
  Sudoku / BitSet algorithms with vector intrinsics.
---

# SIMD Scanner & Vectorization Optimizer Skill

This skill provides an automated workflow and static analysis tools to identify sections of C++ code that can benefit from Single Instruction Multiple Data (SIMD) vectorization, specifically focusing on bitboard, grid, and bitmask routines.

## When to Use This Skill

Activate this skill when:
- The user asks to profile, optimize, or accelerate C++ algorithmic code using SIMD (SSE, AVX2, AVX-512, ARM NEON).
- Looking for vectorization opportunities in 9x9 grids, 81-cell bitsets, or 9-bit candidate masks.
- Reviewing loops and bitwise operations for parallel instruction throughput.

---

## Vectorization Target Areas in Sudoku & Grid Engines

1. **BitSet81 Vectorization (`lo`, `hi` 64-bit words):**
   - **Current:** Separate operations on two `uint64_t` words (`lo`, `hi`).
   - **SIMD Target:** A single 128-bit `__m128i` vector or 256-bit `__m256i` vector:
     - Bitwise AND: `_mm_and_si128` / `_mm256_and_si256`
     - Bitwise OR: `_mm_or_si128` / `_mm256_or_si256`
     - Bitwise AND-NOT: `_mm_andnot_si128` / `_mm256_andnot_si256`
     - Zero check: `_mm_testz_si128` / `_mm256_testz_si256`

2. **Candidate Mask Batch Operations (`TOTAL_CELLS = 81`):**
   - **Current:** Scalar loops checking `m_candidates[cell]` one cell at a time.
   - **SIMD Target:** Storing `uint16_t m_candidates[81]` aligned to 32 bytes:
     - 16 candidate masks fit into a single `__m256i` register!
     - 6 AVX2 vectors cover the entire 81-cell board.
     - Single digit search: broadcast digit mask `_mm256_set1_epi16(1 << (d - 1))` and `_mm256_and_si256`.
     - Non-zero test generates an 81-cell presence bitmask in just 6 clock cycles!

3. **House Constraint Checking (9 cells per House):**
   - **Current:** Iterating over 9 cell indices in a house to count digit frequency.
   - **SIMD Target:** AVX2 gather `_mm256_i32gather_epi32` or pre-gathered 16-bit lane comparisons.

4. **Batch Bit Counts (Popcounts):**
   - Hardware popcount via `_mm_popcnt_u64` or AVX-512 `_mm512_popcnt_epi16`.

---

## Workflow Steps

### Step 1: Run the Automated Scanner Script
Execute the bundled Python analyzer on the target directory:
```bash
python .agents/skills/simd-scanner/scripts/simd_analyzer.py src/
```

### Step 2: Review Findings
The analyzer generates a report categorized by:
- **Priority 1 (High Impact):** Multi-word bitset operations, candidate array reductions.
- **Priority 2 (Medium Impact):** Loops of size 9, 27, 81 that perform bitwise operations or popcounts.
- **Priority 3 (Micro-Optimizations):** Scalar conditionals that can be replaced with vector masks.

### Step 3: Implement Optimizations Using the Reference Guide
Consult the technical reference guide:
[SIMD Optimization Reference Guide](./references/simd_guide.md)

### Step 4: Verify with Compiler Flags
Compile with target instruction sets enabled:
- AVX2 + FMA: `-mavx2 -mfma -mpopcnt`
- AVX-512 (if hardware supports): `-mavx512f -mavx512bw -mavx512vpopcntdq`

Always measure before and after using the test suite benchmarks.
