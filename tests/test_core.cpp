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
#include "core/DiagonalBitboards.hpp"
#include "core/HyperSudokuWindows.hpp"

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

void test_advanced_patterns() {
    std::cout << "\n[TEST] Advanced Solver Patterns (Empty Rectangle, Dual ER, Grouped AIC)...";

    // 1. Synthesize Empty Rectangle:
    // Box 0 (r0-2, c0-2): candidates for digit 5 form a cross at r1, c1
    // Box 0 cands: r1c0, r1c1, r0c1, r2c1
    BoardState b;
    for (int i = 0; i < TOTAL_CELLS; ++i) b.set_candidates(i, 0);
    int r1c0 = cell_index(1, 0);
    int r1c1 = cell_index(1, 1);
    int r0c1 = cell_index(0, 1);
    int r2c1 = cell_index(2, 1);

    b.set_candidates(r1c0, digit_to_mask(5) | digit_to_mask(1));
    b.set_candidates(r1c1, digit_to_mask(5) | digit_to_mask(2));
    b.set_candidates(r0c1, digit_to_mask(5) | digit_to_mask(3));
    b.set_candidates(r2c1, digit_to_mask(5) | digit_to_mask(4));

    // Conjugate pair in Column 5: r1c5 and r7c5
    int r1c5 = cell_index(1, 5);
    int r7c5 = cell_index(7, 5);
    b.set_candidates(r1c5, digit_to_mask(5) | digit_to_mask(6));
    b.set_candidates(r7c5, digit_to_mask(5) | digit_to_mask(7));

    // Target elimination: r7c1 has candidate 5
    int r7c1 = cell_index(7, 1);
    b.set_candidates(r7c1, digit_to_mask(5) | digit_to_mask(8));

    auto er_steps = SingleDigitPatterns::find_empty_rectangles(b);
    assert(!er_steps.empty());
    bool found_er = false;
    for (const auto& s : er_steps) {
        if (s.type == TechniqueType::EmptyRectangle) {
            for (const auto& e : s.eliminations) {
                if (e.cell == r7c1 && e.digit == 5) {
                    found_er = true;
                    break;
                }
            }
        }
    }
    assert(found_er);
    std::cout << "\n  -> Empty Rectangle (ER) detection: PASSED";

    // 2. Synthesize Dual Empty Rectangle:
    // Add second conjugate pair in Column 7: r1c7 and r4c7
    int r1c7 = cell_index(1, 7);
    int r4c7 = cell_index(4, 7);
    b.set_candidates(r1c7, digit_to_mask(5) | digit_to_mask(8));
    b.set_candidates(r4c7, digit_to_mask(5) | digit_to_mask(9));

    // Second elimination: r4c1 has candidate 5
    int r4c1 = cell_index(4, 1);
    b.set_candidates(r4c1, digit_to_mask(5) | digit_to_mask(2));

    auto der_steps = SingleDigitPatterns::find_dual_empty_rectangles(b);
    assert(!der_steps.empty());
    bool found_der = false;
    for (const auto& s : der_steps) {
        if (s.type == TechniqueType::DualEmptyRectangle && s.eliminations.size() == 2) {
            bool has_e1 = (s.eliminations[0].cell == r7c1 || s.eliminations[1].cell == r7c1);
            bool has_e2 = (s.eliminations[0].cell == r4c1 || s.eliminations[1].cell == r4c1);
            if (has_e1 && has_e2) {
                found_der = true;
                break;
            }
        }
    }
    assert(found_der);
    std::cout << "\n  -> Dual Empty Rectangle (Dual ER) detection: PASSED";

    // 3. Synthesize Grouped AIC:
    // Digit 3 in Box 0: group node at r0c0 and r0c1 (in row 0, box 0)
    BoardState bg;
    for (int i = 0; i < TOTAL_CELLS; ++i) bg.set_candidates(i, 0);
    int g_r0c0 = cell_index(0, 0);
    int g_r0c1 = cell_index(0, 1);
    int g_r0c8 = cell_index(0, 8);
    int g_r6c8 = cell_index(6, 8);
    int g_r6c1 = cell_index(6, 1);
    int g_target = cell_index(2, 1); // Sees GroupNode in Box 0 and sees r6c1 in Column 1

    bg.set_candidates(g_r0c0, digit_to_mask(3) | digit_to_mask(1));
    bg.set_candidates(g_r0c1, digit_to_mask(3) | digit_to_mask(2));
    // Additional candidate in Box 0 outside row 0 so group is valid
    int g_r2c2 = cell_index(2, 2);
    bg.set_candidates(g_r2c2, digit_to_mask(3) | digit_to_mask(7));

    // Strong link in Row 0: GroupNode (r0c0, r0c1) = r0c8
    bg.set_candidates(g_r0c8, digit_to_mask(3) | digit_to_mask(4));

    // Strong link in Col 8: r0c8 = r6c8
    bg.set_candidates(g_r6c8, digit_to_mask(3) | digit_to_mask(5));

    // Strong link in Row 6: r6c8 = r6c1
    bg.set_candidates(g_r6c1, digit_to_mask(3) | digit_to_mask(6));

    // Target cell has candidate 3
    bg.set_candidates(g_target, digit_to_mask(3) | digit_to_mask(9));

    auto gaic_steps = Chains::find_grouped_aic(bg);
    assert(!gaic_steps.empty());
    bool found_gaic = false;
    for (const auto& s : gaic_steps) {
        if (s.type == TechniqueType::GroupedAIC) {
            for (const auto& e : s.eliminations) {
                if (e.cell == g_target && e.digit == 3) {
                    found_gaic = true;
                    break;
                }
            }
        }
    }
    assert(found_gaic);
    std::cout << "\n  -> Grouped AIC (Alternating Inference Chains) detection: PASSED";

    std::cout << "\n[TEST] Advanced Solver Patterns (ER, Dual ER, Grouped AIC)... PASSED\n";
}

void test_diagonal_bitboards() {
    std::cout << "[TEST] DiagonalBitboards (X-Sudoku constraints and peer sets)...";

    // 1. Bitmasks and counts
    const auto& main_diag = get_main_diagonal_bitset();
    const auto& anti_diag = get_anti_diagonal_bitset();
    const auto& all_diags = get_all_diagonals_bitset();

    assert(main_diag.count() == 9);
    assert(anti_diag.count() == 9);
    assert(all_diags.count() == 17); // 9 + 9 - 1 (cell 40 intersection)

    // Center cell (40 = r4c4) is the only intersection
    BitSet81 intersection = main_diag & anti_diag;
    assert(intersection.count() == 1);
    assert(intersection.test(40));
    assert(DIAGONALS.intersection_mask == intersection);

    // 2. Cell coordinates & membership
    for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
        int r = cell_row(cell);
        int c = cell_col(cell);

        bool expected_main = (r == c);
        bool expected_anti = (r + c == 8);
        bool expected_any = expected_main || expected_anti;
        int expected_count = (expected_main ? 1 : 0) + (expected_anti ? 1 : 0);

        assert(is_main_diagonal_cell(cell) == expected_main);
        assert(is_anti_diagonal_cell(cell) == expected_anti);
        assert(is_diagonal_cell(cell) == expected_any);
        assert(get_diagonal_membership_count(cell) == expected_count);

        // Diagonal peer bitset validation
        const auto& diag_peers = get_diagonal_peer_bitset(cell);
        assert(!diag_peers.test(cell)); // Cannot be peer to oneself

        if (!expected_any) {
            assert(diag_peers.empty());
            assert(get_x_peer_count(cell) == 20);
            assert(get_x_peer_bitset(cell) == GRID.peer_bitsets[cell]);
        } else if (cell == 40) {
            // Center cell belongs to both diagonals (8 peers on each = 16 diagonal peers)
            assert(diag_peers.count() == 16);
            assert(get_x_peer_count(cell) == 32);
        } else {
            // Non-center diagonal cell belongs to 1 diagonal (8 diagonal peers)
            assert(diag_peers.count() == 8);
            assert(get_x_peer_count(cell) == 26);
        }

        // Full X-Sudoku peers must contain standard peers
        const auto& x_peers = get_x_peer_bitset(cell);
        assert(GRID.peer_bitsets[cell].is_subset_of(x_peers));
        assert(diag_peers.is_subset_of(x_peers));
        assert(!x_peers.test(cell));

        // X-Sudoku peer list consistency
        const auto& peer_list = get_x_peer_cells(cell);
        int count = get_x_peer_count(cell);
        for (int i = 0; i < count; ++i) {
            assert(x_peers.test(peer_list[i]));
        }
    }

    // 3. Symmetry of X-Sudoku peer relationships: A in peers(B) <=> B in peers(A)
    for (int a = 0; a < TOTAL_CELLS; ++a) {
        for (int b = 0; b < TOTAL_CELLS; ++b) {
            assert(get_x_peer_bitset(a).test(b) == get_x_peer_bitset(b).test(a));
        }
    }

    // 4. House indexing and cell arrays
    const auto& main_cells = get_diagonal_house_cells(DiagonalType::Main);
    const auto& anti_cells = get_diagonal_house_cells(DiagonalType::Anti);

    for (int i = 0; i < 9; ++i) {
        assert(main_cells[i] == cell_index(i, i));
        assert(anti_cells[i] == cell_index(i, 8 - i));
    }

    assert(get_diagonal_house_bitset(DiagonalType::Main) == main_diag);
    assert(get_diagonal_house_bitset(DiagonalType::Anti) == anti_diag);

    assert(get_diagonal_name(DiagonalType::Main) == "Main Diagonal (\\)");
    assert(get_diagonal_name(DiagonalType::Anti) == "Anti-Diagonal (/)");

    std::cout << " PASSED\n";
}

void test_hyper_sudoku_windows() {
    std::cout << "[TEST] HyperSudokuWindows (Windoku 4 interior window constraints)...";

    // 1. Bitmasks, counts, and disjointness
    const auto& all_windows = get_all_hyper_windows_bitset();
    assert(all_windows.count() == 36);

    for (int w = 0; w < HYPER_WINDOWS; ++w) {
        const auto& win_mask = get_hyper_window_bitset(w);
        assert(win_mask.count() == 9);
        assert(win_mask.is_subset_of(all_windows));

        // Each window must be mutually disjoint with all other windows
        for (int other_w = 0; other_w < HYPER_WINDOWS; ++other_w) {
            if (w != other_w) {
                BitSet81 overlap = win_mask & get_hyper_window_bitset(other_w);
                assert(overlap.empty());
                assert(overlap.count() == 0);
            }
        }
    }

    // 2. Cell coordinates & window membership
    for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
        int r = cell_row(cell);
        int c = cell_col(cell);

        bool in_row_band = (r >= 1 && r <= 3) || (r >= 5 && r <= 7);
        bool in_col_band = (c >= 1 && c <= 3) || (c >= 5 && c <= 7);
        bool expected_window_cell = in_row_band && in_col_band;

        int expected_win_idx = -1;
        if (r >= 1 && r <= 3 && c >= 1 && c <= 3) expected_win_idx = 0;
        else if (r >= 1 && r <= 3 && c >= 5 && c <= 7) expected_win_idx = 1;
        else if (r >= 5 && r <= 7 && c >= 1 && c <= 3) expected_win_idx = 2;
        else if (r >= 5 && r <= 7 && c >= 5 && c <= 7) expected_win_idx = 3;

        assert(is_hyper_window_cell(cell) == expected_window_cell);
        assert(get_hyper_window_index(cell) == expected_win_idx);

        // Window-only peer bitsets
        const auto& win_peers = get_hyper_window_peer_bitset(cell);
        assert(!win_peers.test(cell));

        if (!expected_window_cell) {
            assert(win_peers.empty());
            assert(get_hyper_peer_count(cell) == 20);
            assert(get_hyper_peer_bitset(cell) == GRID.peer_bitsets[cell]);
        } else {
            assert(win_peers.count() == 8);
            assert(win_peers.is_subset_of(get_hyper_window_bitset(expected_win_idx)));
            assert(get_hyper_peer_count(cell) > 20);
        }

        // Full Hyper-Sudoku peers must contain standard peers & window peers
        const auto& hyper_peers = get_hyper_peer_bitset(cell);
        assert(GRID.peer_bitsets[cell].is_subset_of(hyper_peers));
        assert(win_peers.is_subset_of(hyper_peers));
        assert(!hyper_peers.test(cell));
        assert(hyper_peers.count() == get_hyper_peer_count(cell));

        // Hyper peer list consistency
        const auto& peer_list = get_hyper_peer_cells(cell);
        int count = get_hyper_peer_count(cell);
        for (int i = 0; i < count; ++i) {
            assert(hyper_peers.test(peer_list[i]));
        }

        // Combined X-Windoku peers
        const auto& x_hyper_peers = get_x_hyper_peer_bitset(cell);
        assert(hyper_peers.is_subset_of(x_hyper_peers));
        assert(get_x_peer_bitset(cell).is_subset_of(x_hyper_peers));
        assert(!x_hyper_peers.test(cell));
        assert(x_hyper_peers.count() == get_x_hyper_peer_count(cell));
    }

    // 3. Peer symmetry: A in peers(B) <=> B in peers(A)
    for (int a = 0; a < TOTAL_CELLS; ++a) {
        for (int b = 0; b < TOTAL_CELLS; ++b) {
            assert(get_hyper_peer_bitset(a).test(b) == get_hyper_peer_bitset(b).test(a));
            assert(get_x_hyper_peer_bitset(a).test(b) == get_x_hyper_peer_bitset(b).test(a));
        }
    }

    // 4. Window cell indexing and names
    assert(get_hyper_window_bitset(HyperWindow::TopLeft) == get_hyper_window_bitset(0));
    assert(get_hyper_window_bitset(HyperWindow::TopRight) == get_hyper_window_bitset(1));
    assert(get_hyper_window_bitset(HyperWindow::BottomLeft) == get_hyper_window_bitset(2));
    assert(get_hyper_window_bitset(HyperWindow::BottomRight) == get_hyper_window_bitset(3));

    const auto& tl_cells = get_hyper_window_cells(HyperWindow::TopLeft);
    assert(tl_cells[0] == cell_index(1, 1));
    assert(tl_cells[4] == cell_index(2, 2));
    assert(tl_cells[8] == cell_index(3, 3));

    assert(!get_hyper_window_name(HyperWindow::TopLeft).empty());
    assert(!get_hyper_window_name(0).empty());
    assert(get_hyper_window_name(-1) == "Unknown Hyper Window");

    std::cout << " PASSED\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << " HoDoKu Native Core Engine Test Suite   \n";
    std::cout << "========================================\n";

    auto start = std::chrono::high_resolution_clock::now();

    test_bitset81();
    test_grid_constants();
    test_diagonal_bitboards();
    test_hyper_sudoku_windows();
    test_board_state();
    test_dlx_solver();
    test_advanced_techniques();
    test_uniqueness();
    test_advanced_patterns();
    test_generator();

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "========================================\n";
    std::cout << " All core tests PASSED in " << elapsed_us << " us ("
              << (elapsed_us / 1000.0) << " ms)!\n";
    std::cout << "========================================\n";

    return 0;
}
