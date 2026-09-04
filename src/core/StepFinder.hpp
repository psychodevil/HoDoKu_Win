#pragma once

#include <vector>
#include <optional>
#include "BoardState.hpp"
#include "Step.hpp"
#include "SimpleTechniques.hpp"
#include "Subsets.hpp"
#include "SingleDigitPatterns.hpp"
#include "Wings.hpp"
#include "Fish.hpp"
#include "Uniqueness.hpp"
#include "Coloring.hpp"
#include "Chains.hpp"
#include "AlmostLockedSets.hpp"
#include "SueDeCoq.hpp"
#include "MultiColors.hpp"
#include "FrankenMutantFish.hpp"
#include "ForcingChains.hpp"
#include "Templates.hpp"
#include "DlxSolver.hpp"

namespace hodoku::core {

class StepFinder {
public:
    // Finds the next logical deduction step following HoDoKu's strict difficulty hierarchy
    static std::optional<Step> find_next_step(const BoardState& board) {
        // 1. Singles
        auto ns = SimpleTechniques::find_naked_singles(board);
        if (!ns.empty()) return ns.front();

        auto hs = SimpleTechniques::find_hidden_singles(board);
        if (!hs.empty()) return hs.front();

        // 2. Intersections (Locked Candidates)
        auto lc = SimpleTechniques::find_locked_candidates(board);
        if (!lc.empty()) return lc.front();

        // 3. Subsets (Pairs, Triples, Quads)
        auto np = Subsets::find_naked_pairs(board);
        if (!np.empty()) return np.front();

        auto hp = Subsets::find_hidden_pairs(board);
        if (!hp.empty()) return hp.front();

        auto nt = Subsets::find_naked_triples(board);
        if (!nt.empty()) return nt.front();

        auto ht = Subsets::find_hidden_triples(board);
        if (!ht.empty()) return ht.front();

        auto nq = Subsets::find_naked_quads(board);
        if (!nq.empty()) return nq.front();

        auto hq = Subsets::find_hidden_quads(board);
        if (!hq.empty()) return hq.front();

        // 4. Basic Fish
        auto xw = Fish::find_x_wings(board);
        if (!xw.empty()) return xw.front();

        auto sf = Fish::find_swordfish(board);
        if (!sf.empty()) return sf.front();

        auto jf = Fish::find_jellyfish(board);
        if (!jf.empty()) return jf.front();

        // 5. Single Digit Patterns
        auto sky = SingleDigitPatterns::find_skyscrapers(board);
        if (!sky.empty()) return sky.front();

        auto kite = SingleDigitPatterns::find_two_string_kites(board);
        if (!kite.empty()) return kite.front();

        auto tf = SingleDigitPatterns::find_turbot_fish(board);
        if (!tf.empty()) return tf.front();

        auto er = SingleDigitPatterns::find_empty_rectangles(board);
        if (!er.empty()) return er.front();

        auto der = SingleDigitPatterns::find_dual_empty_rectangles(board);
        if (!der.empty()) return der.front();

        // 6. Wings
        auto xy = Wings::find_xy_wings(board);
        if (!xy.empty()) return xy.front();

        auto xyz = Wings::find_xyz_wings(board);
        if (!xyz.empty()) return xyz.front();

        auto ww = Wings::find_w_wings(board);
        if (!ww.empty()) return ww.front();

        // 7. Uniqueness
        auto ur = Uniqueness::find_unique_rectangles(board);
        if (!ur.empty()) return ur.front();

        auto bug = Uniqueness::find_bug_plus_one(board);
        if (!bug.empty()) return bug.front();

        auto ar = Uniqueness::find_avoidable_rectangles(board);
        if (!ar.empty()) return ar.front();

        // 8. Finned Fish
        auto fxw = Fish::find_finned_x_wings(board);
        if (!fxw.empty()) return fxw.front();

        // 9. Simple Colors & Multi-Colors
        auto sc = Coloring::find_simple_colors(board);
        if (!sc.empty()) return sc.front();

        auto mc = MultiColors::find_multi_colors(board);
        if (!mc.empty()) return mc.front();

        // 10. Chains
        auto rp = Chains::find_remote_pairs(board);
        if (!rp.empty()) return rp.front();

        auto xyc = Chains::find_xy_chains(board);
        if (!xyc.empty()) return xyc.front();

        auto gaic = Chains::find_grouped_aic(board);
        if (!gaic.empty()) return gaic.front();

        // 11. Almost Locked Sets & Miscellaneous
        auto als = AlmostLockedSets::find_als_xz(board);
        if (!als.empty()) return als.front();

        auto sdc = SueDeCoq::find_sue_de_coq(board);
        if (!sdc.empty()) return sdc.front();

        auto ff = FrankenMutantFish::find_franken_fish(board);
        if (!ff.empty()) return ff.front();

        // 12. Forcing Chains (Cell, Region, Contradiction)
        auto cfc = ForcingChains::find_cell_forcing_chains(board);
        if (!cfc.empty()) return cfc.front();

        auto rfc = ForcingChains::find_house_forcing_chains(board);
        if (!rfc.empty()) return rfc.front();

        auto cdc = ForcingChains::find_contradiction_chains(board);
        if (!cdc.empty()) return cdc.front();

        // 13. Templates (Template Delete, Template Set)
        auto tmpl = Templates::find_template_steps(board);
        if (!tmpl.empty()) return tmpl.front();

        // 14. Brute Force (Last Resort Fallback - HoDoKu BruteForceSolver.java, Score: 10000)
        return find_brute_force(board);
    }

    static std::optional<Step> find_brute_force(const BoardState& board) {
        DlxSolver dlx;
        auto sol = dlx.solve_one(board);
        if (!sol) return std::nullopt;

        std::vector<int> unfilled;
        for (int i = 0; i < 81; ++i) {
            if (board.is_unfilled(i)) unfilled.push_back(i);
        }
        if (unfilled.empty()) return std::nullopt;

        int target_cell = unfilled[unfilled.size() / 2];
        int correct_val = sol->get_value(target_cell);

        Step step;
        step.type = TechniqueType::BruteForce;
        step.name = "Brute Force";
        step.difficulty = DifficultyLevel::Extreme;
        step.score = 10000;
        step.primary_cells.set(target_cell);
        step.assignments.push_back({target_cell, correct_val});
        step.explanation = "Brute Force (Exact Cover solver) determines cell r" +
                          std::to_string(cell_row(target_cell) + 1) + "c" +
                          std::to_string(cell_col(target_cell) + 1) + " must be " +
                          std::to_string(correct_val) + ".";
        return step;
    }

    // Finds all deduction steps applicable in the current state (FAS)
    static std::vector<Step> find_all_steps(const BoardState& board) {
        std::vector<Step> all;

        auto append = [&](std::vector<Step>&& steps) {
            all.insert(all.end(), steps.begin(), steps.end());
        };

        append(SimpleTechniques::find_naked_singles(board));
        append(SimpleTechniques::find_hidden_singles(board));
        append(SimpleTechniques::find_locked_candidates(board));
        append(Subsets::find_naked_pairs(board));
        append(Subsets::find_hidden_pairs(board));
        append(Subsets::find_naked_triples(board));
        append(Subsets::find_hidden_triples(board));
        append(Subsets::find_naked_quads(board));
        append(Subsets::find_hidden_quads(board));
        append(Fish::find_x_wings(board));
        append(Fish::find_swordfish(board));
        append(Fish::find_jellyfish(board));
        append(SingleDigitPatterns::find_skyscrapers(board));
        append(SingleDigitPatterns::find_two_string_kites(board));
        append(SingleDigitPatterns::find_turbot_fish(board));
        append(SingleDigitPatterns::find_empty_rectangles(board));
        append(SingleDigitPatterns::find_dual_empty_rectangles(board));
        append(Wings::find_xy_wings(board));
        append(Wings::find_xyz_wings(board));
        append(Wings::find_w_wings(board));
        append(Uniqueness::find_unique_rectangles(board));
        append(Uniqueness::find_bug_plus_one(board));
        append(Uniqueness::find_avoidable_rectangles(board));
        append(Fish::find_finned_x_wings(board));
        append(Coloring::find_simple_colors(board));
        append(MultiColors::find_multi_colors(board));
        append(Chains::find_remote_pairs(board));
        append(Chains::find_xy_chains(board));
        append(Chains::find_grouped_aic(board));
        append(AlmostLockedSets::find_als_xz(board));
        append(SueDeCoq::find_sue_de_coq(board));
        append(FrankenMutantFish::find_franken_fish(board));
        append(ForcingChains::find_cell_forcing_chains(board));
        append(ForcingChains::find_house_forcing_chains(board));
        append(ForcingChains::find_contradiction_chains(board));
        append(Templates::find_template_steps(board));

        return all;
    }
};

} // namespace hodoku::core
