#include <iostream>
#include <cassert>
#include "app/AppTypes.hpp"
#include "app/StudioModel.hpp"

using namespace hodoku::ui;

int main() {
    std::cout << "========================================\n";
    std::cout << " HoDoKu Manual Inference Links Test    \n";
    std::cout << "========================================\n";

    HoDoKuStudio studio;

    // 1. Initial link state
    assert(!studio.is_link_mode());
    assert(studio.is_drawing_strong_link());
    assert(!studio.has_link_start());
    assert(studio.get_user_links().empty());
    std::cout << "[TEST] Initial link state... PASSED\n";

    // 2. Toggle link mode
    studio.toggle_link_mode();
    assert(studio.is_link_mode());
    studio.toggle_link_type();
    assert(!studio.is_drawing_strong_link()); // Now weak link
    studio.toggle_link_type();
    assert(studio.is_drawing_strong_link()); // Now strong link
    std::cout << "[TEST] Toggle link mode & type... PASSED\n";

    // 3. Link creation: r1c1:1 to r1c2:2
    int c1 = 0;  // r1c1
    int c2 = 1;  // r1c2
    studio.handle_candidate_link_click(c1, 1);
    assert(studio.has_link_start());
    assert(studio.get_link_start_cell() == c1);
    assert(studio.get_link_start_digit() == 1);

    studio.handle_candidate_link_click(c2, 2);
    assert(!studio.has_link_start());
    assert(studio.get_user_links().size() == 1);
    const auto& link1 = studio.get_user_links()[0];
    assert(link1.from_cell == c1 && link1.from_digit == 1);
    assert(link1.to_cell == c2 && link1.to_digit == 2);
    assert(link1.is_strong == true);
    std::cout << "[TEST] Strong link creation... PASSED\n";

    // 4. Toggle off existing link
    studio.handle_candidate_link_click(c1, 1);
    studio.handle_candidate_link_click(c2, 2);
    assert(studio.get_user_links().empty());
    std::cout << "[TEST] Toggle off existing link... PASSED\n";

    // 5. Create weak link
    studio.set_drawing_strong_link(false);
    studio.handle_candidate_link_click(c1, 3);
    studio.handle_candidate_link_click(c2, 4);
    assert(studio.get_user_links().size() == 1);
    assert(studio.get_user_links()[0].is_strong == false);
    std::cout << "[TEST] Weak link creation... PASSED\n";

    // 6. Undo link creation
    assert(studio.can_undo());
    studio.undo();
    assert(studio.get_user_links().empty());
    assert(studio.can_redo());
    studio.redo();
    assert(studio.get_user_links().size() == 1);
    assert(studio.get_user_links()[0].from_digit == 3);
    std::cout << "[TEST] Undo / Redo for links... PASSED\n";

    // 7. Savepoints / Bookmark preservation
    studio.add_savepoint("Test Bookmark with Link");
    studio.clear_user_links();
    assert(studio.get_user_links().empty());
    studio.restore_savepoint(studio.get_savepoints().size() - 1);
    assert(studio.get_user_links().size() == 1);
    std::cout << "[TEST] Savepoint bookmark preservation... PASSED\n";

    // 8. Cancel link start
    studio.handle_candidate_link_click(c1, 9);
    assert(studio.has_link_start());
    studio.cancel_link_start();
    assert(!studio.has_link_start());
    std::cout << "[TEST] Cancel link start... PASSED\n";

    // 9. Candidate Snap Points and Directed Link Geometry
    auto calc_snap = [](int cell, int digit, float cellSize, float offsetX, float offsetY) {
        float subCell = cellSize / 3.0f;
        int r = cell / 9;
        int c = cell % 9;
        float x, y;
        if (digit >= 1 && digit <= 9) {
            int dr = (digit - 1) / 3;
            int dc = (digit - 1) % 3;
            x = offsetX + c * cellSize + (dc + 0.5f) * subCell;
            y = offsetY + r * cellSize + (dr + 0.5f) * subCell;
        } else {
            x = offsetX + (c + 0.5f) * cellSize;
            y = offsetY + (r + 0.5f) * cellSize;
        }
        return std::make_pair(x, y);
    };

    // Test r1c1 candidates
    auto [x1_1, y1_1] = calc_snap(0, 1, 60.0f, 0.0f, 0.0f);
    assert(x1_1 == 10.0f && y1_1 == 10.0f); // Top-left candidate 1
    auto [x1_5, y1_5] = calc_snap(0, 5, 60.0f, 0.0f, 0.0f);
    assert(x1_5 == 30.0f && y1_5 == 30.0f); // Center candidate 5
    auto [x1_9, y1_9] = calc_snap(0, 9, 60.0f, 0.0f, 0.0f);
    assert(x1_9 == 50.0f && y1_9 == 50.0f); // Bottom-right candidate 9

    // Test snap points inside candidate cells for all 81 cells
    for (int cell = 0; cell < 81; ++cell) {
        int r = cell / 9;
        int c = cell % 9;
        float cellMinX = c * 60.0f;
        float cellMaxX = (c + 1) * 60.0f;
        float cellMinY = r * 60.0f;
        float cellMaxY = (r + 1) * 60.0f;

        for (int d = 1; d <= 9; ++d) {
            auto [sx, sy] = calc_snap(cell, d, 60.0f, 0.0f, 0.0f);
            assert(sx > cellMinX && sx < cellMaxX);
            assert(sy > cellMinY && sy < cellMaxY);
        }
    }
    std::cout << "[TEST] Candidate snap points geometry... PASSED\n";

    std::cout << "========================================\n";
    std::cout << " ALL Manual Link Tests PASSED!          \n";
    std::cout << "========================================\n";
    return 0;
}
