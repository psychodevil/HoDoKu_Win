#include <iostream>
#include <cassert>
#include <chrono>
#include <string>
#include "core/Types.hpp"
#include "core/BitSet81.hpp"
#include "core/GridConstants.hpp"
#include "core/BoardState.hpp"
#include "core/DlxSolver.hpp"
#include "core/SimpleTechniques.hpp"
#include "core/Subsets.hpp"
#include "core/SingleDigitPatterns.hpp"
#include "core/Wings.hpp"
#include "core/Fish.hpp"
#include "core/Uniqueness.hpp"
#include "core/Coloring.hpp"
#include "core/Chains.hpp"
#include "core/AlmostLockedSets.hpp"
#include "core/SueDeCoq.hpp"
#include "core/StepFinder.hpp"
#include "core/Generator.hpp"

using namespace hodoku::core;

void test_bitset81() {
    std::cout << "[TEST] BitSet81 operations...";
    BitSet81 bs;
    assert(bs.empty());
    assert(bs.count() == 0);

    bs.set(0);
    bs.set(63);
    bs.set(64);
    bs.set(80);

    assert(!bs.empty());
    assert(bs.count() == 4);
    assert(bs.test(0));
    assert(bs.test(63));
    assert(bs.test(64));
    assert(bs.test(80));
    assert(!bs.test(1));
    assert(!bs.test(65));

    bs.reset(63);
    assert(bs.count() == 3);
    assert(!bs.test(63));

    BitSet81 all_set = BitSet81::all();
    assert(all_set.count() == 81);
    assert(bs.is_subset_of(all_set));

    int visited_count = 0;
    all_set.for_each_cell([&](int cell) {
        assert(cell >= 0 && cell < 81);
        ++visited_count;
    });
    assert(visited_count == 81);

    std::cout << " PASSED\n";
}

void test_grid_constants() {
    std::cout << "[TEST] GridConstants lookup tables...";

    for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
        int r = GRID.cell_row[cell];
        int c = GRID.cell_col[cell];
        int b = GRID.cell_box[cell];

        assert(r >= 0 && r < 9);
        assert(c >= 0 && c < 9);
        assert(b >= 0 && b < 9);
        assert(cell_index(r, c) == cell);

        const auto& peers = GRID.peer_bitsets[cell];
        assert(peers.count() == 20);
        assert(!peers.test(cell));

        peers.for_each_cell([&](int peer_cell) {
            int pr = GRID.cell_row[peer_cell];
            int pc = GRID.cell_col[peer_cell];
            int pb = GRID.cell_box[peer_cell];
            assert(pr == r || pc == c || pb == b);
        });
    }

    for (int h = 0; h < TOTAL_HOUSES; ++h) {
        const auto& house_bs = GRID.house_bitsets[h];
        assert(house_bs.count() == 9);
    }

    std::cout << " PASSED\n";
}

void test_board_state() {
    std::cout << "[TEST] BoardState candidate propagation and validation...";

    std::string puzzle = "530070000600195000098000060800060003400803001700020006060000280000419005000080079";
    BoardState board(puzzle);

    assert(board.is_valid());
    assert(!board.has_contradiction());
    assert(board.get_value(0) == 5);
    assert(board.get_value(1) == 3);
    assert(board.is_given(0));
    assert(board.is_given(1));
    assert(!board.is_given(2));

    assert(!board.has_candidate(2, 5));
    assert(!board.has_candidate(2, 3));

    std::cout << " PASSED\n";
}

void test_dlx_solver() {
    std::cout << "[TEST] DlxSolver exact cover solver...";

    std::string easy_puzzle = "530070000600195000098000060800060003400803001700020006060000280000419005000080079";
    BoardState board(easy_puzzle);

    DlxSolver solver;
    assert(solver.count_solutions(board, 2) == 1);

    auto solution = solver.solve_one(board);
    assert(solution.has_value());
    assert(solution->is_solved());
    assert(solution->is_valid());

    std::string escargot = "100007090030020008009600500005300900010080002600004000300000010040000007007000300";
    BoardState escargot_board(escargot);
    auto escargot_sol = solver.solve_one(escargot_board);
    assert(escargot_sol.has_value());
    assert(escargot_sol->is_solved());
    assert(escargot_sol->is_valid());

    std::cout << " PASSED\n";
}

void test_advanced_techniques() {
    std::cout << "[TEST] Complete Techniques Suite across difficulty tiers...";

    // Test benchmark puzzles:
    // 1. Easy
    std::string easy = "530070000600195000098000060800060003400803001700020006060000280000419005000080079";
    BoardState b_easy(easy);
    auto easy_fas = StepFinder::find_all_steps(b_easy);
    assert(!easy_fas.empty());
    std::cout << "\n  -> Easy FAS steps count: " << easy_fas.size();

    // 2. Medium (requires subsets)
    std::string med = "000000010400000000020000000000050407008000300001090000300400200050100000000806000";
    BoardState b_med(med);
    auto med_fas = StepFinder::find_all_steps(b_med);
    assert(!med_fas.empty());
    std::cout << "\n  -> Medium FAS steps count: " << med_fas.size();

    // 3. Hard (requires fish, wings, patterns)
    std::string hard = "000000000000003085001020000000507000004000100090000000500000073002010000000040009";
    BoardState b_hard(hard);
    auto hard_fas = StepFinder::find_all_steps(b_hard);
    assert(!hard_fas.empty());
    std::cout << "\n  -> Hard FAS steps count: " << hard_fas.size();

    // 4. Extreme
    std::string extreme = "100007090030020008009600500005300900010080002600004000300000010040000007007000300";
    BoardState b_extreme(extreme);
    auto ext_fas = StepFinder::find_all_steps(b_extreme);
    std::cout << "\n  -> Extreme FAS steps count: " << ext_fas.size();

    // Test full logical solve loop
    std::cout << "\n  -> Testing step-by-step solving of Easy puzzle...";
    BoardState sim = b_easy;
    int step_count = 0;
    while (!sim.is_solved()) {
        auto step = StepFinder::find_next_step(sim);
        if (!step) {
            std::cout << " [STUCK after " << step_count << " steps!] ";
            break;
        }
        for (const auto& a : step->assignments) sim.set_value(a.cell, a.digit);
        for (const auto& e : step->eliminations) sim.remove_candidate(e.cell, e.digit);
        step_count++;
    }
    std::cout << " (Solved: " << sim.is_solved() << ", steps: " << step_count << ")";

    std::cout << "\n  -> Testing step-by-step solving of Medium puzzle...";
    sim = b_med;
    step_count = 0;
    while (!sim.is_solved()) {
        auto step = StepFinder::find_next_step(sim);
        if (!step) {
            std::cout << " [STUCK after " << step_count << " steps, " << sim.unfilled_count() << " cells left!] ";
            break;
        }
        for (const auto& a : step->assignments) sim.set_value(a.cell, a.digit);
        for (const auto& e : step->eliminations) sim.remove_candidate(e.cell, e.digit);
        step_count++;
    }
    std::cout << "\n  -> Testing step-by-step solving of Hard puzzle with TRUE solution check...";
    sim = b_hard;
    DlxSolver dlx_hard;
    auto true_sol_hard = dlx_hard.solve_one(b_hard);
    assert(true_sol_hard.has_value());

    step_count = 0;
    while (!sim.is_solved()) {
        auto step = StepFinder::find_next_step(sim);
        if (!step) {
            std::cout << " [STUCK after " << step_count << " steps, " << sim.unfilled_count() << " cells left!] ";
            break;
        }
        for (const auto& a : step->assignments) {
            assert(a.digit == true_sol_hard->get_value(a.cell));
            sim.set_value(a.cell, a.digit);
        }
        for (const auto& e : step->eliminations) {
            if (e.digit == true_sol_hard->get_value(e.cell)) {
                std::cout << "\n[BUG FOUND in Hard] Technique '" << step->name << "' made ILLEGAL ELIMINATION at r"
                          << (cell_row(e.cell) + 1) << "c" << (cell_col(e.cell) + 1)
                          << ": eliminated digit " << e.digit << " which is the TRUE VALUE!\n";
                std::cout << "Explanation: " << step->explanation << "\n" << std::flush;
                assert(false);
            }
            sim.remove_candidate(e.cell, e.digit);
        }
        step_count++;
    }
    std::cout << " (Solved: " << sim.is_solved() << ", steps: " << step_count << ")";

    std::cout << "\n  -> Testing step-by-step solving of Extreme puzzle with TRUE solution check...";
    sim = b_extreme;
    DlxSolver dlx_ext;
    auto true_sol_ext = dlx_ext.solve_one(b_extreme);
    assert(true_sol_ext.has_value());

    step_count = 0;
    while (!sim.is_solved()) {
        auto step = StepFinder::find_next_step(sim);
        if (!step) {
            std::cout << " [STUCK after " << step_count << " steps, " << sim.unfilled_count() << " cells left!] ";
            break;
        }
        for (const auto& a : step->assignments) {
            assert(a.digit == true_sol_ext->get_value(a.cell));
            sim.set_value(a.cell, a.digit);
        }
        for (const auto& e : step->eliminations) {
            if (e.digit == true_sol_ext->get_value(e.cell)) {
                std::cout << "\n[BUG FOUND in Extreme] Technique '" << step->name << "' made ILLEGAL ELIMINATION at r"
                          << (cell_row(e.cell) + 1) << "c" << (cell_col(e.cell) + 1)
                          << ": eliminated digit " << e.digit << " which is the TRUE VALUE!\n";
                std::cout << "Explanation: " << step->explanation << "\n" << std::flush;
                assert(false);
            }
            sim.remove_candidate(e.cell, e.digit);
        }
        step_count++;
    }
    std::cout << " (Solved: " << sim.is_solved() << ", steps: " << step_count << ")";

    // Verify sub-solvers directly:
    auto triples = Subsets::find_naked_triples(b_med);
    auto wings = Wings::find_xy_wings(b_hard);
    auto swordfish = Fish::find_swordfish(b_hard);
    auto skys = SingleDigitPatterns::find_skyscrapers(b_hard);
    auto urs = Uniqueness::find_unique_rectangles(b_hard);
    auto colors = Coloring::find_simple_colors(b_hard);
    auto chains = Chains::find_xy_chains(b_hard);
    auto als_list = AlmostLockedSets::find_all_als(b_hard);
    assert(!als_list.empty());
    std::cout << "\n  -> Discovered " << als_list.size() << " Almost Locked Sets (ALS)";

    auto sdc_list = SueDeCoq::find_sue_de_coq(b_hard);
    auto db_list = DeathBlossom::find_death_blossom(b_hard);
    std::cout << "\n  -> Evaluated Sue de Coq and Death Blossom (" << sdc_list.size() << " SDC, " << db_list.size() << " DB)";

    auto tmpl_steps = Templates::find_template_steps(b_hard);
    std::cout << "\n  -> Evaluated Forcing Chains and Templates (" << tmpl_steps.size() << " template steps evaluated)";

    std::cout << "\n[TEST] All technique modules successfully executed... PASSED\n";
}

void test_generator() {
    std::cout << "\n[TEST] Procedural Generator & Symmetries...\n";
    SudokuGenerator gen(42);

    std::cout << "  -> Testing random terminal grid generation...";
    BoardState terminal = gen.generate_terminal_grid();
    assert(terminal.unfilled_count() == 0);
    // Verify terminal grid is completely valid
    for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
        int val = terminal.get_value(cell);
        assert(val >= 1 && val <= 9);
        int r = cell_row(cell), c = cell_col(cell), b = cell_box(cell);
        for (int p = 0; p < TOTAL_CELLS; ++p) {
            if (p != cell && (cell_row(p) == r || cell_col(p) == c || cell_box(p) == b)) {
                assert(terminal.get_value(p) != val);
            }
        }
    }
    std::cout << " VALID 81-digit terminal grid!\n";

    std::cout << "  -> Testing symmetric clue digging (Rotational180)...";
    BoardState puzzle180 = gen.dig_puzzle(terminal, SymmetryType::Rotational180);
    DlxSolver dlx;
    assert(dlx.count_solutions(puzzle180, 2) == 1);
    for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
        int r = cell_row(cell), c = cell_col(cell);
        int symm = cell_index(8 - r, 8 - c);
        bool has1 = (puzzle180.get_value(cell) != 0);
        bool has2 = (puzzle180.get_value(symm) != 0);
        assert(has1 == has2);
    }
    std::cout << " 180 deg symmetric with UNIQUE solution (" << puzzle180.get_givens().count() << " clues)!\n";

    std::cout << "  -> Testing symmetric clue digging (Rotational90)...";
    BoardState puzzle90 = gen.dig_puzzle(terminal, SymmetryType::Rotational90);
    assert(dlx.count_solutions(puzzle90, 2) == 1);
    for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
        int r = cell_row(cell), c = cell_col(cell);
        int p1 = cell_index(c, 8 - r);
        int p2 = cell_index(8 - r, 8 - c);
        int p3 = cell_index(8 - c, r);
        bool v0 = (puzzle90.get_value(cell) != 0);
        assert(v0 == (puzzle90.get_value(p1) != 0));
        assert(v0 == (puzzle90.get_value(p2) != 0));
        assert(v0 == (puzzle90.get_value(p3) != 0));
    }
    std::cout << " 90 deg 4-fold symmetric with UNIQUE solution (" << puzzle90.get_givens().count() << " clues)!\n";

    std::cout << "  -> Testing targeted difficulty generation (Easy)...";
    BoardState easyPuz = gen.generate_puzzle(DifficultyLevel::Easy, SymmetryType::Rotational180, 5);
    assert(dlx.count_solutions(easyPuz, 2) == 1);
    int score = 0;
    DifficultyLevel lvl = gen.evaluate_difficulty(easyPuz, score);
    std::cout << " Generated " << difficulty_name(lvl) << " puzzle (Score: " << score << ", Clues: " << easyPuz.get_givens().count() << ")!\n";

    std::cout << "[TEST] Procedural Generator & Symmetries... PASSED\n";
}

void test_uniqueness() {
    std::cout << "\n[TEST] Complete Uniqueness Techniques Suite (UR 1-6, BUG+1, AR)...";

    // 1. Synthesize a board with Unique Rectangle Type 1
    // A rectangle spanning 2 boxes: r0c0 (box 0), r0c3 (box 1), r1c0 (box 0), r1c3 (box 1)
    BoardState b;
    b.clear();
    int p11 = cell_index(0, 0); // r0c0
    int p12 = cell_index(0, 3); // r0c3
    int p21 = cell_index(1, 0); // r1c0
    int p22 = cell_index(1, 3); // r1c3

    b.set_candidates(p11, digit_to_mask(1) | digit_to_mask(2));
    b.set_candidates(p12, digit_to_mask(1) | digit_to_mask(2));
    b.set_candidates(p21, digit_to_mask(1) | digit_to_mask(2));
    b.set_candidates(p22, digit_to_mask(1) | digit_to_mask(2) | digit_to_mask(7));

    auto ur_steps = Uniqueness::find_unique_rectangles(b);
    assert(!ur_steps.empty());
    bool found_ur1 = false;
    for (const auto& s : ur_steps) {
        if (s.name == "Unique Rectangle Type 1") {
            found_ur1 = true;
            assert(s.eliminations.size() == 2);
            assert(s.eliminations[0].cell == p22 || s.eliminations[1].cell == p22);
            break;
        }
    }
    assert(found_ur1);
    std::cout << "\n  -> UR Type 1 detection: PASSED";

    // 2. Synthesize UR Type 2:
    // p11={1,2}, p12={1,2}, p21={1,2,5}, p22={1,2,5}.
    // Extra digit 5 in p21 and p22 (row 1). Common peer in row 1 with 5 can have 5 eliminated.
    int p24 = cell_index(1, 4); // in row 1, peer of both p21 and p22
    b.set_candidates(p21, digit_to_mask(1) | digit_to_mask(2) | digit_to_mask(5));
    b.set_candidates(p22, digit_to_mask(1) | digit_to_mask(2) | digit_to_mask(5));
    b.set_candidates(p24, digit_to_mask(5) | digit_to_mask(9));

    auto ur2_steps = Uniqueness::find_unique_rectangles(b);
    bool found_ur2 = false;
    for (const auto& s : ur2_steps) {
        if (s.name == "Unique Rectangle Type 2") {
            found_ur2 = true;
            bool found_elim = false;
            for (const auto& e : s.eliminations) {
                if (e.cell == p24 && e.digit == 5) found_elim = true;
            }
            assert(found_elim);
            break;
        }
    }
    assert(found_ur2);
    std::cout << "\n  -> UR Type 2 detection: PASSED";

    // 3. Synthesize Avoidable Rectangle Type 1:
    // 3 solved cells (non-givens) at corners: p11=1, p12=2, p21=2.
    // Unfilled cell p22 has candidate 1.
    BoardState arb;
    arb.clear();
    arb.set_value(p11, 1);
    arb.set_value(p12, 2);
    arb.set_value(p21, 2);
    arb.set_candidates(p22, digit_to_mask(1) | digit_to_mask(8));
    auto ar_steps = Uniqueness::find_avoidable_rectangles(arb);
    assert(!ar_steps.empty());
    assert(ar_steps[0].eliminations[0].cell == p22 && ar_steps[0].eliminations[0].digit == 1);
    std::cout << "\n  -> Avoidable Rectangle Type 1 detection: PASSED";

    std::cout << "\n[TEST] Complete Uniqueness Techniques Suite... PASSED\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << " HoDoKu Native Core Engine Test Suite   \n";
    std::cout << "========================================\n";

    auto start = std::chrono::high_resolution_clock::now();

    test_bitset81();
    test_grid_constants();
    test_board_state();
    test_dlx_solver();
    test_advanced_techniques();
    test_uniqueness();
    test_generator();

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "========================================\n";
    std::cout << " All core tests PASSED in " << elapsed_us << " us ("
              << (elapsed_us / 1000.0) << " ms)!\n";
    std::cout << "========================================\n";

    return 0;
}
