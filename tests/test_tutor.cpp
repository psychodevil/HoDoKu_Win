#include <iostream>
#include <cassert>
#include <string>
#include "app/StudioModel.hpp"

using namespace hodoku;
using namespace hodoku::ui;

int main() {
    std::cout << "[TEST] Learning Mode (Tutor) Validator...\n";

    HoDoKuStudio studio;
    std::string sampleGivens = "38.4.6...9..2..7...4.3.......2...6...9.7.1...5...3.......8.4...7..9..2...6.1.58";
    studio.import_from_string(sampleGivens);

    // Unfilled cell 2 is row 0, col 2.
    // Givens in row 0 are: r0c0=3, r0c1=8, r0c3=4, r0c5=6.
    // In block 0, givens are 3, 8, 9, 4.

    // 1. Test Rule Violation: entering 8 at cell 2 (duplicate with r0c1=8)
    auto resViolation = studio.validate_move(2, 8);
    assert(resViolation == MoveValidation::RuleViolation);
    std::cout << "  -> Rule violation detection (house duplicate): PASSED\n";

    // 2. Test Solution Deviation:
    // Cell 2 has candidates. Let's find one that does not violate immediate rules but differs from true solution.
    DlxSolver solver;
    auto sol = solver.solve_one(studio.get_board());
    assert(sol.has_value());
    int trueDigit = sol->get_value(2);

    int wrongDigit = -1;
    for (int d = 1; d <= 9; ++d) {
        if (d != trueDigit && studio.validate_move(2, d) != MoveValidation::RuleViolation) {
            wrongDigit = d;
            break;
        }
    }
    assert(wrongDigit != -1);

    auto resDev = studio.validate_move(2, wrongDigit);
    assert(resDev == MoveValidation::SolutionDeviation);
    std::cout << "  -> Solution deviation detection: PASSED\n";

    // 3. Test Valid Move:
    auto resValid = studio.validate_move(2, trueDigit);
    assert(resValid == MoveValidation::Valid);
    std::cout << "  -> Valid solution move: PASSED\n";

    // 4. Test Audit Progress:
    // No errors initially
    auto errors = studio.audit_progress();
    assert(errors.empty());

    // Enter wrong digit
    studio.set_cell_digit(2, wrongDigit);
    errors = studio.audit_progress();
    assert(errors.size() == 1);
    assert(errors[0] == 2);
    std::cout << "  -> Audit progress error detection: PASSED\n";

    // Correct the digit
    studio.set_cell_digit(2, trueDigit);
    errors = studio.audit_progress();
    assert(errors.empty());
    std::cout << "  -> Audit progress correction: PASSED\n";

    std::cout << "ALL Tutor Tests PASSED!\n";
    return 0;
}

