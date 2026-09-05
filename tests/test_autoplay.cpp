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

    std::cout << "========================================\n";
    std::cout << " ALL Auto-Play Tests PASSED!            \n";
    std::cout << "========================================\n";
    return 0;
}
