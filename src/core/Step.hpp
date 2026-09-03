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
    Skyscraper,
    TwoStringKite,
    TurbotFish,
    SimpleColoring,
    SimpleColors,
    MultiColors1,
    MultiColors2,
    XYWing,
    XYZWing,
    WWing,
    WXYZWing,
    UniqueRectangle,
    AvoidableRectangle,
    BUG,
    RemotePair,
    AlsXz,
    SueDeCoq,
    DeathBlossom,
    FrankenFish,
    MutantFish,
    BruteForce,
    Custom
};

inline std::string technique_name(TechniqueType t) {
    switch (t) {
        case TechniqueType::FullHouse: return "Full House";
        case TechniqueType::NakedSingle: return "Naked Single";
        case TechniqueType::HiddenSingle: return "Hidden Single";
        case TechniqueType::LockedCandidatesPointing: return "Locked Candidates (Pointing)";
        case TechniqueType::LockedCandidatesClaiming: return "Locked Candidates (Claiming)";
        case TechniqueType::NakedPair: return "Naked Pair";
        case TechniqueType::HiddenPair: return "Hidden Pair";
        case TechniqueType::NakedTriple: return "Naked Triple";
        case TechniqueType::HiddenTriple: return "Hidden Triple";
        case TechniqueType::NakedQuadruple: return "Naked Quadruple";
        case TechniqueType::HiddenQuadruple: return "Hidden Quadruple";
        case TechniqueType::XWing: return "X-Wing";
        case TechniqueType::Swordfish: return "Swordfish";
        case TechniqueType::Jellyfish: return "Jellyfish";
        case TechniqueType::Skyscraper: return "Skyscraper";
        case TechniqueType::TwoStringKite: return "2-String Kite";
        case TechniqueType::TurbotFish: return "Turbot Fish";
        case TechniqueType::SimpleColoring: return "Simple Coloring";
        case TechniqueType::SimpleColors: return "Simple Colors";
        case TechniqueType::MultiColors1: return "Multi-Colors Type 1";
        case TechniqueType::MultiColors2: return "Multi-Colors Type 2";
        case TechniqueType::XYWing: return "XY-Wing";
        case TechniqueType::XYZWing: return "XYZ-Wing";
        case TechniqueType::WWing: return "W-Wing";
        case TechniqueType::WXYZWing: return "WXYZ-Wing";
        case TechniqueType::UniqueRectangle: return "Unique Rectangle";
        case TechniqueType::AvoidableRectangle: return "Avoidable Rectangle";
        case TechniqueType::BUG: return "Bivalue Universal Grave +1";
        case TechniqueType::RemotePair: return "Remote Pair";
        case TechniqueType::AlsXz: return "Almost Locked Set XZ";
        case TechniqueType::SueDeCoq: return "Sue de Coq";
        case TechniqueType::DeathBlossom: return "Death Blossom";
        case TechniqueType::FrankenFish: return "Franken Fish";
        case TechniqueType::MutantFish: return "Mutant Fish";
        case TechniqueType::BruteForce: return "Brute Force";
        default: return "Custom Technique";
    }
}

struct CandidateElimination {
    int cell{0};
    int digit{0};
};

struct CandidateAssignment {
    int cell{0};
    int digit{0};
};

struct StepLink {
    int from_cell{0};
    int from_digit{0};
    int to_cell{0};
    int to_digit{0};
    bool is_strong{true};
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
    std::vector<StepLink> links;

    // Helper to check if step actually does anything
    [[nodiscard]] bool has_effect() const noexcept {
        return !assignments.empty() || !eliminations.empty();
    }
};

} // namespace hodoku::core

