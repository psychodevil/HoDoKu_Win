#pragma once

#include <string>
#include <vector>
#include "Types.hpp"
#include "BitSet81.hpp"

namespace hodoku::core {

enum class TechniqueType {
    FullHouse,
    NakedSingle,
    HiddenSingle,
    LockedCandidatesPointing,
    LockedCandidatesClaiming,
    NakedPair,
    HiddenPair,
    NakedTriple,
    HiddenTriple,
    NakedQuadruple,
    HiddenQuadruple,
    XWing,
    Swordfish,
    Jellyfish,
    SimpleColoring,
    SimpleColors,
    XYWing,
    XYZWing,
    WXYZWing,
    UniqueRectangle,
    AvoidableRectangle,
    BUG,
    RemotePair,
    AlsXz,
    SueDeCoq,
    DeathBlossom,
    BruteForce,
    Custom
};

struct CandidateElimination {
    int cell{0};
    int digit{0};
};

struct CandidateAssignment {
    int cell{0};
    int digit{0};
};

struct Step {
    TechniqueType type{TechniqueType::FullHouse};
    std::string name;
    DifficultyLevel difficulty{DifficultyLevel::Easy};
    int score{0};
    std::string explanation;

    // Visual highlights for UI rendering
    BitSet81 primary_cells;
    BitSet81 secondary_cells;
    std::vector<CandidateAssignment> assignments;
    std::vector<CandidateElimination> eliminations;

    // Helper to check if step actually does anything
    [[nodiscard]] bool has_effect() const noexcept {
        return !assignments.empty() || !eliminations.empty();
    }
};

} // namespace hodoku::core

