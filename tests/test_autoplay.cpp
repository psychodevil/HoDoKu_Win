#include <iostream>
#include <cassert>
#include "app/AppTypes.hpp"
#include "app/StudioModel.hpp"

using namespace hodoku;
using namespace hodoku::ui;

int main() {
    std::cout << "========================================\n";
    std::cout << " HoDoKu Timed Auto-Play Controller Test \n";
    std::cout << "========================================\n";

    HoDoKuStudio studio;

    // 1. Initial State
    assert(studio.get_auto_play_state() == AutoPlayState::Stopped);
    assert(!studio.is_auto_playing());
    assert(!studio.is_auto_play_paused());
    assert(studio.get_auto_play_delay() == 750);
    std::cout << "[TEST] Initial Auto-Play state... PASSED\n";

    // 2. Delay settings & clamping
    studio.set_auto_play_delay(1200);
    assert(studio.get_auto_play_delay() == 1200);
    studio.set_auto_play_delay(10); // clamped to 50
    assert(studio.get_auto_play_delay() == 50);
    studio.set_auto_play_delay(99999); // clamped to 5000
    assert(studio.get_auto_play_delay() == 5000);
    studio.set_auto_play_delay(500);
    std::cout << "[TEST] Auto-Play delay clamping... PASSED\n";

    // 3. Start auto-play on an unsolved puzzle
    studio.import_from_string(PUZZLE_LIBRARY[0].second); // Easy puzzle
    assert(!studio.get_board().is_solved());
    bool started = studio.start_auto_play(400);
    assert(started);
    assert(studio.is_auto_playing());
    assert(studio.get_auto_play_state() == AutoPlayState::Playing);
    assert(studio.get_auto_play_delay() == 400);
    assert(studio.get_selected_step().has_value());
    assert(studio.get_hint_level() == HintLevel::Concrete);
    std::cout << "[TEST] Start Auto-Play on puzzle... PASSED\n";

    // 4. Step auto-play execution
    size_t initialSteps = studio.get_solution_path().size();
    bool hasMore = studio.step_auto_play();
    assert(hasMore);
    assert(studio.is_auto_playing());
    assert(studio.get_selected_step().has_value());
    assert(studio.get_solution_path().size() <= initialSteps);
    std::cout << "[TEST] Step Auto-Play execution... PASSED\n";

    // 5. Pause and resume auto-play
    studio.pause_auto_play();
    assert(studio.is_auto_play_paused());
    assert(!studio.is_auto_playing());
    assert(studio.get_auto_play_state() == AutoPlayState::Paused);

    // Stepping while paused should not advance auto-play
    bool steppedWhilePaused = studio.step_auto_play();
    assert(!steppedWhilePaused);

    studio.resume_auto_play();
    assert(studio.is_auto_playing());
    assert(!studio.is_auto_play_paused());
    std::cout << "[TEST] Pause & Resume Auto-Play... PASSED\n";

    // 6. Manual step backward and forward
    size_t stepsBeforeBack = studio.get_solution_path().size();
    bool backOk = studio.step_backward();
    assert(backOk);
    assert(studio.get_solution_path().size() >= stepsBeforeBack);

    bool fwdOk = studio.step_forward();
    assert(fwdOk);
    std::cout << "[TEST] Step Backward & Step Forward... PASSED\n";

    // 7. Run auto-play loop until puzzle solved
    int iterations = 0;
    while (studio.is_auto_playing() && iterations < 200) {
        studio.step_auto_play();
        iterations++;
    }
    assert(studio.get_board().is_solved());
    assert(studio.get_auto_play_state() == AutoPlayState::Stopped);
    assert(!studio.is_auto_playing());
    std::cout << "[TEST] Full auto-play solve loop (solved in " << iterations << " steps)... PASSED\n";

    // 8. Starting auto-play on a solved puzzle must fail gracefully
    bool restartOnSolved = studio.start_auto_play();
    assert(!restartOnSolved);
    assert(studio.get_auto_play_state() == AutoPlayState::Stopped);
    std::cout << "[TEST] Auto-Play on solved puzzle rejection... PASSED\n";

    // 9. Dynamic speed slider adjustment during playback
    studio.import_from_string(PUZZLE_LIBRARY[1].second); // Medium puzzle
    bool started2 = studio.start_auto_play(750);
    assert(started2);
    assert(studio.is_auto_playing());
    assert(studio.get_auto_play_delay() == 750);

    // Change speed slider live while playing
    studio.set_auto_play_delay(250);
    assert(studio.get_auto_play_delay() == 250);
    assert(studio.is_auto_playing());
    assert(studio.step_auto_play());

    // Change speed slider live while paused
    studio.pause_auto_play();
    assert(studio.is_auto_play_paused());
    studio.set_auto_play_delay(1500);
    assert(studio.get_auto_play_delay() == 1500);
    studio.resume_auto_play();
    assert(studio.is_auto_playing());
    std::cout << "[TEST] Dynamic speed slider adjustment during playback... PASSED\n";

    // 10. Toolbar Stop action & manual stepping after stop
    studio.stop_auto_play();
    assert(studio.get_auto_play_state() == AutoPlayState::Stopped);
    assert(!studio.is_auto_playing());
    assert(!studio.is_auto_play_paused());
    assert(!studio.step_auto_play()); // Stepping auto-play while stopped must return false

    // Manual step forward and step backward when stopped
    bool manualFwd = studio.step_forward();
    assert(manualFwd);
    bool manualBack = studio.step_backward();
    assert(manualBack);
    std::cout << "[TEST] Toolbar Stop action and manual stepping... PASSED\n";

    // 11. Visual Step Transitions for Eliminated Candidates & Placed Digits (Plan 6.4 Task 3)
    studio.import_from_string(PUZZLE_LIBRARY[0].second);
    assert(!studio.has_transition());

    studio.give_concrete_hint();
    assert(studio.get_selected_step().has_value());
    auto stepToExec = studio.get_selected_step().value();
    studio.execute_hint();

    assert(studio.has_transition());
    const auto& transition = studio.get_last_transition();
    assert(transition.active);
    assert(transition.technique_name == stepToExec.name);

    // Verify placed digits tracking
    if (!stepToExec.assignments.empty()) {
        int placedCell = stepToExec.assignments.front().cell;
        int placedDigit = stepToExec.assignments.front().digit;
        assert(studio.is_recently_placed(placedCell));
        assert(studio.get_recently_placed_digit(placedCell) == placedDigit);
    }

    // Verify eliminated candidates tracking
    if (!stepToExec.eliminations.empty()) {
        int elimCell = stepToExec.eliminations.front().cell;
        int elimDigit = stepToExec.eliminations.front().digit;
        assert(studio.is_recently_eliminated(elimCell, elimDigit));
    }

    // Clear transition cleans up state
    studio.clear_transition();
    assert(!studio.has_transition());
    std::cout << "[TEST] Step Transitions for Eliminated Candidates & Placed Digits... PASSED\n";

    std::cout << "========================================\n";
    std::cout << " ALL Auto-Play Tests PASSED!            \n";
    std::cout << "========================================\n";
    return 0;
}

