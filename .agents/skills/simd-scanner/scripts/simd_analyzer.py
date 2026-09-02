#!/usr/bin/env python3
"""
SIMD Opportunity Analyzer for C++ Sudoku & BitSet Engines
Part of the Antigravity simd-scanner skill.
"""

import sys
import os
import re
from pathlib import Path

PATTERNS = [
    {
        "id": "BITSET_DUAL_WORD",
        "name": "BitSet81 Dual 64-bit Word Vectorization",
        "category": "BitSet Vectorization",
        "priority": "HIGH",
        "regex": re.compile(r"(lo\s*(&=|\|=|\^=|&=~|&|\||\^)\s*[^;]+;\s*hi\s*(&=|\|=|\^=|&=~|&|\||\^))|(std::popcount\s*\(\s*lo\s*\)\s*\+\s*std::popcount\s*\(\s*hi\s*\))"),
        "suggestion": "Replace dual 64-bit scalar operations (lo, hi) with a single 128-bit (__m128i) or 256-bit (__m256i) vector using _mm_and_si128, _mm_or_si128, _mm_andnot_si128, and hardware popcount.",
        "speedup": "2.0x - 3.5x for all BitSet bitwise operations"
    },
    {
        "id": "CANDIDATE_MASK_LOOP_81",
        "name": "Full Grid 81-Cell Candidate Mask Vectorization",
        "category": "Candidate Mask Batch Processing",
        "priority": "HIGH",
        "regex": re.compile(r"for\s*\(\s*int\s+\w+\s*=\s*0\s*;\s*\w+\s*<\s*(TOTAL_CELLS|81)\s*;\s*\+\+\w+\s*\)[\s\S]{0,120}?(m_candidates\[\w+\]|get_candidates\(\w+\))"),
        "suggestion": "Store 81 16-bit candidate masks aligned to 32 bytes. Load 16 masks per __m256i vector (6 vectors total). Vectorize digit presence tests using _mm256_set1_epi16 and _mm256_and_si256.",
        "speedup": "4.0x - 8.0x reduction in grid-wide candidate scans"
    },
    {
        "id": "HOUSE_CANDIDATE_GATHER",
        "name": "House 9-Cell Candidate Mask Reduction",
        "category": "House Aggregation",
        "priority": "MEDIUM",
        "regex": re.compile(r"for\s*\(\s*int\s+\w+\s*:\s*(GRID\.house_cells\[\w+\]|house_cells\[\w+\])\s*\)[\s\S]{0,80}?(get_candidates|m_candidates)"),
        "suggestion": "Gather 9 candidate masks into a single 128-bit/256-bit vector using _mm256_i32gather_epi32 or pre-arranged house candidate vectors for zero-overhead house reductions.",
        "speedup": "2.5x - 4.0x in house constraint checking"
    },
    {
        "id": "SCALAR_POPCOUNT_LOOP",
        "name": "Scalar Array Popcount Aggregation",
        "category": "Popcount Acceleration",
        "priority": "MEDIUM",
        "regex": re.compile(r"for\s*\([^)]*81[^)]*\)[\s\S]{0,100}?(std::popcount|count_candidates)"),
        "suggestion": "Replace scalar popcount iterations with AVX2 vector popcount or parallel bit manipulation routines.",
        "speedup": "3.0x - 5.0x"
    },
    {
        "id": "BITMASK_INTERSECTIONS",
        "name": "Batch Candidate Bitmask Intersections",
        "category": "Candidate Mask Batch Processing",
        "priority": "HIGH",
        "regex": re.compile(r"(common_elims\[c\d*\]\s*&=|branch_elims\[c\d*\])"),
        "suggestion": "Perform array-level bitwise AND operations across 81 elements using 256-bit AVX2 vector chunks (_mm256_and_si256) instead of 81 sequential scalar iterations.",
        "speedup": "5.0x - 8.0x in Forcing Chains & ALS intersection calculations"
    }
]

def scan_file(filepath: Path):
    findings = []
    try:
        content = filepath.read_text(encoding="utf-8", errors="ignore")
    except Exception as e:
        return findings

    lines = content.splitlines()

    for pattern in PATTERNS:
        matches = list(pattern["regex"].finditer(content))
        for match in matches:
            start_pos = match.start()
            line_no = content[:start_pos].count("\n") + 1
            snippet = match.group(0).strip().replace("\r", "").replace("\n", " ")
            if len(snippet) > 80:
                snippet = snippet[:77] + "..."

            findings.append({
                "file": str(filepath.as_posix()),
                "line": line_no,
                "id": pattern["id"],
                "name": pattern["name"],
                "category": pattern["category"],
                "priority": pattern["priority"],
                "snippet": snippet,
                "suggestion": pattern["suggestion"],
                "speedup": pattern["speedup"]
            })

    return findings

def scan_directory(dir_path: Path):
    all_findings = []
    extensions = {".hpp", ".cpp", ".h", ".c"}

    for root, _, files in os.walk(dir_path):
        for file in files:
            p = Path(root) / file
            if p.suffix.lower() in extensions:
                all_findings.extend(scan_file(p))

    return all_findings

def print_report(findings):
    print("=" * 80)
    print("   SIMD VECTORIZATION OPPORTUNITY REPORT")
    print("=" * 80)

    if not findings:
        print("No obvious SIMD opportunities found in target path.")
        return

    # Group by priority
    high = [f for f in findings if f["priority"] == "HIGH"]
    med = [f for f in findings if f["priority"] == "MEDIUM"]
    low = [f for f in findings if f["priority"] == "LOW"]

    print(f"Total SIMD Opportunities Identified: {len(findings)}")
    print(f"  -> High Priority (Major Impact):   {len(high)}")
    print(f"  -> Medium Priority (Good ROI):    {len(med)}")
    print(f"  -> Low Priority (Micro-opts):     {len(low)}")
    print("-" * 80)

    for idx, f in enumerate(findings, 1):
        p_tag = f"[{f['priority']}]"
        print(f"{idx}. {p_tag:<8} {f['name']}")
        print(f"   Location:   {f['file']}:{f['line']}")
        print(f"   Snippet:    {f['snippet']}")
        print(f"   Category:   {f['category']}")
        print(f"   Suggestion: {f['suggestion']}")
        print(f"   Speedup:    {f['speedup']}")
        print("-" * 80)

def main():
    target = sys.argv[1] if len(sys.argv) > 1 else "src/"
    target_path = Path(target)

    if not target_path.exists():
        print(f"Error: Target path '{target}' does not exist.")
        sys.exit(1)

    print(f"Scanning '{target_path.resolve()}' for SIMD vectorization opportunities...")
    findings = scan_directory(target_path)
    print_report(findings)

if __name__ == "__main__":
    main()
