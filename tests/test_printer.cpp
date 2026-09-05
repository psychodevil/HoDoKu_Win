#include <iostream>
#include <cassert>
#include <windows.h>
#include <gdiplus.h>

#include "app/PrintEngine.hpp"
#include "core/BoardState.hpp"
#include "core/Generator.hpp"

using namespace hodoku::ui;
using namespace hodoku::core;

void test_slot_calculations() {
    std::cout << "[Test 1] Testing page slot geometry for 1, 2, 4, 6 layouts..." << std::endl;
    int pW = 1700;
    int pH = 2200;

    // Layout 1
    auto slots1 = PrintEngine::compute_page_slots(pW, pH, PrintLayout::One);
    assert(slots1.size() == 1);
    assert(slots1[0].gridSize > 500);
    assert(slots1[0].gridX >= 0 && slots1[0].gridX + slots1[0].gridSize <= pW);
    assert(slots1[0].gridY >= 0 && slots1[0].gridY + slots1[0].gridSize <= pH);
    std::cout << "  Layout 1: 1 slot, grid size: " << slots1[0].gridSize << "px -> PASSED" << std::endl;

    // Layout 2
    auto slots2 = PrintEngine::compute_page_slots(pW, pH, PrintLayout::Two);
    assert(slots2.size() == 2);
    assert(slots2[0].y + slots2[0].height <= slots2[1].y + 1); // Stacked vertically
    for (const auto& s : slots2) {
        assert(s.gridX >= 0 && s.gridX + s.gridSize <= pW);
        assert(s.gridY >= 0 && s.gridY + s.gridSize <= pH);
        assert(s.gridSize > 300);
    }
    std::cout << "  Layout 2: 2 slots stacked vertically -> PASSED" << std::endl;

    // Layout 4
    auto slots4 = PrintEngine::compute_page_slots(pW, pH, PrintLayout::Four);
    assert(slots4.size() == 4);
    for (size_t i = 0; i < slots4.size(); ++i) {
        assert(slots4[i].gridSize > 250);
        for (size_t j = i + 1; j < slots4.size(); ++j) {
            // Ensure no grid overlap
            RECT r1 = { slots4[i].gridX, slots4[i].gridY, slots4[i].gridX + slots4[i].gridSize, slots4[i].gridY + slots4[i].gridSize };
            RECT r2 = { slots4[j].gridX, slots4[j].gridY, slots4[j].gridX + slots4[j].gridSize, slots4[j].gridY + slots4[j].gridSize };
            RECT inter;
            assert(!IntersectRect(&inter, &r1, &r2));
        }
    }
    std::cout << "  Layout 4: 4 slots (2x2 grid), zero overlaps -> PASSED" << std::endl;

    // Layout 6
    auto slots6 = PrintEngine::compute_page_slots(pW, pH, PrintLayout::Six);
    assert(slots6.size() == 6);
    for (size_t i = 0; i < slots6.size(); ++i) {
        assert(slots6[i].gridSize > 200);
        for (size_t j = i + 1; j < slots6.size(); ++j) {
            RECT r1 = { slots6[i].gridX, slots6[i].gridY, slots6[i].gridX + slots6[i].gridSize, slots6[i].gridY + slots6[i].gridSize };
            RECT r2 = { slots6[j].gridX, slots6[j].gridY, slots6[j].gridX + slots6[j].gridSize, slots6[j].gridY + slots6[j].gridSize };
            RECT inter;
            assert(!IntersectRect(&inter, &r1, &r2));
        }
    }
    std::cout << "  Layout 6: 6 slots (2x3 grid), zero overlaps -> PASSED" << std::endl;
}

void test_gdiplus_page_rendering() {
    std::cout << "[Test 2] Testing GDI+ page rendering for multi-grid sheets..." << std::endl;

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    int pW = 850;
    int pH = 1100;

    SudokuGenerator gen(42);
    BoardState b1 = gen.generate_puzzle(DifficultyLevel::Easy, SymmetryType::Rotational180, 5);
    BoardState b2 = gen.generate_puzzle(DifficultyLevel::Medium, SymmetryType::Rotational180, 5);
    BoardState b3 = gen.generate_puzzle(DifficultyLevel::Hard, SymmetryType::Rotational180, 5);
    BoardState b4 = gen.generate_puzzle(DifficultyLevel::Hard, SymmetryType::Rotational180, 5);
    BoardState b5 = gen.generate_puzzle(DifficultyLevel::Unfair, SymmetryType::Rotational180, 5);
    BoardState b6 = gen.generate_puzzle(DifficultyLevel::Extreme, SymmetryType::Rotational180, 5);

    std::vector<BoardState> puzzles = { b1, b2, b3, b4, b5, b6 };
    std::vector<std::wstring> titles = {
        L"Puzzle #1 (Easy)", L"Puzzle #2 (Medium)", L"Puzzle #3 (Hard)",
        L"Puzzle #4 (Hard)", L"Puzzle #5 (Unfair)", L"Puzzle #6 (Extreme)"
    };

    // Render 1-puzzle
    {
        Gdiplus::Bitmap bmp(pW, pH, PixelFormat32bppARGB);
        Gdiplus::Graphics g(&bmp);
        std::vector<BoardState> p1 = { b1 };
        std::vector<std::wstring> t1 = { titles[0] };
        PrintEngine::render_page(g, p1, t1, 1, 1, pW, pH, PrintLayout::One, false, false, false);
    }
    std::cout << "  Rendered 1-puzzle page cleanly." << std::endl;

    // Render 2-puzzle
    {
        Gdiplus::Bitmap bmp(pW, pH, PixelFormat32bppARGB);
        Gdiplus::Graphics g(&bmp);
        std::vector<BoardState> p2 = { b1, b2 };
        std::vector<std::wstring> t2 = { titles[0], titles[1] };
        PrintEngine::render_page(g, p2, t2, 1, 1, pW, pH, PrintLayout::Two, false, false, false);
    }
    std::cout << "  Rendered 2-puzzle page cleanly." << std::endl;

    // Render 4-puzzle
    {
        Gdiplus::Bitmap bmp(pW, pH, PixelFormat32bppARGB);
        Gdiplus::Graphics g(&bmp);
        std::vector<BoardState> p4 = { b1, b2, b3, b4 };
        std::vector<std::wstring> t4 = { titles[0], titles[1], titles[2], titles[3] };
        PrintEngine::render_page(g, p4, t4, 1, 1, pW, pH, PrintLayout::Four, false, false, false);
    }
    std::cout << "  Rendered 4-puzzle page cleanly." << std::endl;

    // Render 6-puzzle
    {
        Gdiplus::Bitmap bmp(pW, pH, PixelFormat32bppARGB);
        Gdiplus::Graphics g(&bmp);
        PrintEngine::render_page(g, puzzles, titles, 1, 1, pW, pH, PrintLayout::Six, false, false, false);
    }
    std::cout << "  Rendered 6-puzzle page cleanly." << std::endl;

    // Render solution page (6 solutions)
    {
        Gdiplus::Bitmap bmp(pW, pH, PixelFormat32bppARGB);
        Gdiplus::Graphics g(&bmp);
        DlxSolver dlx;
        std::vector<BoardState> solutions;
        for (const auto& puz : puzzles) {
            auto sol = dlx.solve_one(puz);
            solutions.push_back(sol.has_value() ? *sol : puz);
        }
        PrintEngine::render_page(g, solutions, titles, 2, 2, pW, pH, PrintLayout::Six, false, false, true);
    }
    std::cout << "  Rendered 6-solution answer key page cleanly." << std::endl;

    Gdiplus::GdiplusShutdown(gdiplusToken);
}

int main() {
    std::cout << "=== HoDoKu Native: Multi-Puzzle Print Layout Test Suite ===" << std::endl;
    test_slot_calculations();
    test_gdiplus_page_rendering();
    std::cout << "\n>>> All Multi-Puzzle Print Layout Tests PASSED! <<<\n";
    return 0;
}
