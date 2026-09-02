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

    std::cout << "\n[TEST] All technique modules successfully executed... PASSED\n";
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

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "========================================\n";
    std::cout << " All core tests PASSED in " << elapsed_us << " us ("
              << (elapsed_us / 1000.0) << " ms)!\n";
    std::cout << "========================================\n";

    return 0;
}
