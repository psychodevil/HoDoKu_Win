#include <iostream>
#include <cassert>
#include <chrono>
#include "core/Types.hpp"
#include "core/BoardState.hpp"
#include "core/DlxSolver.hpp"
#include "core/Generator.hpp"

using namespace hodoku::core;

void test_preset_patterns() {
    std::cout << "[Test 1] Testing preset patterns..." << std::endl;
    auto diamond = SudokuGenerator::make_preset_diamond();
    assert(diamond.count() >= 17);
    std::cout << "  Diamond givens: " << diamond.count() << std::endl;

    auto cross = SudokuGenerator::make_preset_cross();
    assert(cross.count() >= 17);
    std::cout << "  Cross givens: " << cross.count() << std::endl;

    auto frame = SudokuGenerator::make_preset_picture_frame();
    assert(frame.count() >= 17);
    std::cout << "  Picture Frame givens: " << frame.count() << std::endl;

    auto checker = SudokuGenerator::make_preset_checkerboard();
    assert(checker.count() >= 17);
    std::cout << "  Checkerboard givens: " << checker.count() << std::endl;

    SudokuGenerator gen(42);
    auto rand180 = gen.make_preset_random_symmetric(28, SymmetryType::Rotational180);
    assert(rand180.count() >= 17);
    std::cout << "  Random 180 symmetric givens: " << rand180.count() << std::endl;
    std::cout << "Preset patterns verified successfully." << std::endl;
}

void test_symmetric_cells() {
    std::cout << "[Test 2] Testing symmetry orbits..." << std::endl;
    int cell = cell_index(1, 2);
    auto sym180 = SudokuGenerator::get_symmetric_cells(cell, SymmetryType::Rotational180);
    assert(sym180.size() == 2);
    assert(sym180[0] == cell || sym180[1] == cell);
    assert(sym180[0] == cell_index(7, 6) || sym180[1] == cell_index(7, 6));

    auto sym90 = SudokuGenerator::get_symmetric_cells(cell, SymmetryType::Rotational90);
    assert(sym90.size() == 4);

    auto symNone = SudokuGenerator::get_symmetric_cells(cell, SymmetryType::None);
    assert(symNone.size() == 1);
    assert(symNone[0] == cell);
    std::cout << "Symmetry orbits verified successfully." << std::endl;
}

void test_clue_count_guards() {
    std::cout << "[Test 3] Testing clue count guards (< 17 rejected)..." << std::endl;
    BitSet81 small_mask;
    for (int i = 0; i < 16; ++i) small_mask.set(i);
    assert(small_mask.count() == 16);

    SudokuGenerator gen(12345);
    BoardState terminal = gen.generate_terminal_grid();

    auto dug = gen.dig_pattern(terminal, small_mask);
    assert(!dug.has_value());

    auto puz = gen.generate_pattern_puzzle(small_mask, 10);
    assert(!puz.has_value());
    std::cout << "Clue count guards verified successfully." << std::endl;
}

void test_exact_pattern_digging_and_generation() {
    std::cout << "[Test 4] Testing pattern-based puzzle generation & strict adherence..." << std::endl;
    SudokuGenerator gen(999);
    
    // Generate a valid terminal grid first
    BoardState terminal = gen.generate_terminal_grid();
    DlxSolver dlx;
    assert(dlx.count_solutions(terminal, 2) == 1);

    // Dig a standard 180 symmetric puzzle to get a guaranteed unique clue mask
    BoardState classic = gen.dig_puzzle(terminal, SymmetryType::Rotational180);
    BitSet81 mask = classic.get_givens();
    assert(mask.count() >= 17);
    std::cout << "  Testing with guaranteed unique pattern (" << mask.count() << " givens)..." << std::endl;

    // Dig pattern from the original terminal grid
    auto dug = gen.dig_pattern(terminal, mask);
    assert(dug.has_value());
    assert(dug->get_givens().count() == mask.count());
    for (int i = 0; i < TOTAL_CELLS; ++i) {
        assert(dug->is_given(i) == mask.test(i));
    }
    assert(dlx.count_solutions(*dug, 2) == 1);

    // Now test generate_pattern_puzzle using that mask
    auto generated = gen.generate_pattern_puzzle(mask, 100);
    assert(generated.has_value());
    assert(generated->get_givens().count() == mask.count());
    for (int i = 0; i < TOTAL_CELLS; ++i) {
        assert(generated->is_given(i) == mask.test(i));
    }
    assert(dlx.count_solutions(*generated, 2) == 1);
    std::cout << "Pattern puzzle generated strictly adhering to mask with unique solution." << std::endl;
}

int main() {
    std::cout << "=== HoDoKu Native: Pattern Generator Test Suite ===" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    test_preset_patterns();
    test_symmetric_cells();
    test_clue_count_guards();
    test_exact_pattern_digging_and_generation();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    std::cout << "\n>>> All Pattern Generator Tests PASSED in " << elapsed << " ms! <<<\n";
    return 0;
}
