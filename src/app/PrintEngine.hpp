#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <cmath>
#include <windows.h>
#include <gdiplus.h>

#include "../core/Types.hpp"
#include "../core/BoardState.hpp"
#include "../core/DlxSolver.hpp"
#include "../core/Generator.hpp"
#include "StudioModel.hpp"
#include "GridRenderer.hpp"

namespace hodoku::ui {

enum class PrintLayout {
    One = 1,
    Two = 2,
    Four = 4,
    Six = 6
};

struct PrintSlot {
    int index;
    int x;
    int y;
    int width;
    int height;
    int gridX;
    int gridY;
    int gridSize;
    RECT headerRect;
};

struct PrintConfig {
    PrintLayout layout{PrintLayout::Four};
    int puzzleCount{4};
    hodoku::core::DifficultyLevel difficulty{hodoku::core::DifficultyLevel::Hard};
    bool useCurrentBoard{true};
    bool mixedDifficulty{false};
    bool showCandidates{false};
    bool allBlack{false};
    bool printRating{true};
    bool includeSolutions{true};
};

class PrintEngine {
public:
    static std::vector<PrintSlot> compute_page_slots(int pageWidth, int pageHeight, PrintLayout layout) {
        std::vector<PrintSlot> slots;
        int marginX = static_cast<int>(pageWidth * 0.05f);
        int marginY = static_cast<int>(pageHeight * 0.04f);
        int headerH = static_cast<int>(pageHeight * 0.035f);
        int footerH = static_cast<int>(pageHeight * 0.035f);

        int availW = pageWidth - 2 * marginX;
        int availH = pageHeight - marginY - headerH - footerH - marginY;
        int startY = marginY + headerH;

        int cols = 1;
        int rows = 1;
        switch (layout) {
        case PrintLayout::One:  cols = 1; rows = 1; break;
        case PrintLayout::Two:  cols = 1; rows = 2; break;
        case PrintLayout::Four: cols = 2; rows = 2; break;
        case PrintLayout::Six:  cols = 2; rows = 3; break;
        }

        int slotW = availW / cols;
        int slotH = availH / rows;

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                int slotIdx = r * cols + c;
                PrintSlot s{};
                s.index = slotIdx;
                s.x = marginX + c * slotW;
                s.y = startY + r * slotH;
                s.width = slotW;
                s.height = slotH;

                int slotHeaderH = static_cast<int>(slotH * 0.12f);
                s.headerRect = { s.x, s.y, s.x + slotW, s.y + slotHeaderH };

                int maxGridW = static_cast<int>(slotW * 0.88f);
                int maxGridH = static_cast<int>((slotH - slotHeaderH) * 0.90f);
                s.gridSize = (std::min)(maxGridW, maxGridH);
                s.gridX = s.x + (slotW - s.gridSize) / 2;
                s.gridY = s.y + slotHeaderH + ((slotH - slotHeaderH) - s.gridSize) / 2;

                slots.push_back(s);
            }
        }
        return slots;
    }

    static void render_page(
        Gdiplus::Graphics& g,
        const std::vector<hodoku::core::BoardState>& puzzles,
        const std::vector<std::wstring>& titles,
        int pageNumber,
        int totalPages,
        int pageWidth,
        int pageHeight,
        PrintLayout layout,
        bool showCandidates,
        bool allBlack,
        bool isSolutionPage
    ) {
        using namespace Gdiplus;

        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

        // Page Header
        FontFamily fontFamily(L"Segoe UI");
        Font pageHeaderFont(&fontFamily, (std::max)(8.0f, pageHeight * 0.015f), FontStyleBold, UnitPixel);
        Font pageFooterFont(&fontFamily, (std::max)(6.0f, pageHeight * 0.011f), FontStyleRegular, UnitPixel);
        SolidBrush textBrush(Color(255, 15, 23, 42));

        StringFormat centerFmt;
        centerFmt.SetAlignment(StringAlignmentCenter);
        centerFmt.SetLineAlignment(StringAlignmentCenter);

        StringFormat leftFmt;
        leftFmt.SetAlignment(StringAlignmentNear);
        leftFmt.SetLineAlignment(StringAlignmentCenter);

        std::wstring pageTitle = isSolutionPage ? L"HoDoKu Sudoku Booklet — Solutions & Answer Key"
                                               : L"HoDoKu Sudoku Booklet";
        RectF pageHeaderRect(pageWidth * 0.05f, pageHeight * 0.015f, pageWidth * 0.90f, pageHeight * 0.025f);
        g.DrawString(pageTitle.c_str(), -1, &pageHeaderFont, pageHeaderRect, &leftFmt, &textBrush);

        // Header separator line
        Pen sepPen(Color(255, 203, 213, 225), 1.0f);
        g.DrawLine(&sepPen, pageWidth * 0.05f, pageHeight * 0.042f, pageWidth * 0.95f, pageHeight * 0.042f);

        auto slots = compute_page_slots(pageWidth, pageHeight, layout);
        GridRenderer renderer;

        float titlePixelSize = (layout == PrintLayout::One) ? (pageHeight * 0.020f) :
                               (layout == PrintLayout::Two) ? (pageHeight * 0.016f) : (pageHeight * 0.013f);
        Font slotTitleFont(&fontFamily, (std::max)(7.0f, titlePixelSize), FontStyleBold, UnitPixel);

        for (size_t i = 0; i < slots.size() && i < puzzles.size(); ++i) {
            const auto& slot = slots[i];
            const auto& board = puzzles[i];
            const auto& title = titles[i];

            RectF slotTitleRc(static_cast<float>(slot.headerRect.left),
                              static_cast<float>(slot.headerRect.top),
                              static_cast<float>(slot.headerRect.right - slot.headerRect.left),
                              static_cast<float>(slot.headerRect.bottom - slot.headerRect.top));
            g.DrawString(title.c_str(), -1, &slotTitleFont, slotTitleRc, &centerFmt, &textBrush);

            renderer.render_board_clean(g, board, slot.gridX, slot.gridY, slot.gridSize, slot.gridSize, showCandidates, allBlack);
        }

        // Page Footer
        std::wstring footer = L"Page " + std::to_wstring(pageNumber) + L" of " + std::to_wstring(totalPages) +
                              L"  •  Printed with HoDoKu Native (C++20 Windows Edition)";
        RectF pageFooterRect(pageWidth * 0.05f, pageHeight * 0.965f, pageWidth * 0.90f, pageHeight * 0.025f);
        g.DrawString(footer.c_str(), -1, &pageFooterFont, pageFooterRect, &centerFmt, &textBrush);
    }

    static bool execute_print(HWND hwndOwner, const HoDoKuStudio& studio, const PrintConfig& config) {
        PRINTDLGW pd = {};
        pd.lStructSize = sizeof(pd);
        pd.hwndOwner = hwndOwner;
        pd.Flags = PD_RETURNDC | PD_USEDEVMODECOPIESANDCOLLATE | PD_NOSELECTION;

        if (!PrintDlgW(&pd)) {
            return false;
        }

        // Prepare puzzles
        std::vector<hodoku::core::BoardState> puzzles;
        std::vector<std::wstring> titles;
        std::vector<hodoku::core::BoardState> solutions;

        hodoku::core::SudokuGenerator generator;
        hodoku::core::DlxSolver dlx;

        const wchar_t* diffNames[] = { L"Easy", L"Medium", L"Hard", L"Unfair", L"Extreme" };

        for (int i = 0; i < config.puzzleCount; ++i) {
            hodoku::core::BoardState puz;
            hodoku::core::DifficultyLevel lvl = config.difficulty;

            if (i == 0 && config.useCurrentBoard) {
                puz = studio.get_board();
                lvl = studio.get_hardest_level();
            } else {
                if (config.mixedDifficulty) {
                    lvl = static_cast<hodoku::core::DifficultyLevel>(i % 5);
                }
                puz = generator.generate_puzzle(lvl, hodoku::core::SymmetryType::Rotational180, 10);
            }

            puzzles.push_back(puz);

            int lvlIdx = std::clamp(static_cast<int>(lvl), 0, 4);
            std::wstring title = L"Puzzle #" + std::to_wstring(i + 1) + L" (" + diffNames[lvlIdx] + L")";
            if (config.printRating) {
                title += L"  •  " + std::to_wstring(puz.get_givens().count()) + L" clues";
            }
            titles.push_back(title);

            if (config.includeSolutions) {
                auto sol = dlx.solve_one(puz);
                if (sol.has_value()) solutions.push_back(*sol);
                else solutions.push_back(puz);
            }
        }

        int perPage = static_cast<int>(config.layout);
        int puzzlePages = (static_cast<int>(puzzles.size()) + perPage - 1) / perPage;
        int solPerPage = 6;
        int solPages = config.includeSolutions ? ((static_cast<int>(solutions.size()) + solPerPage - 1) / solPerPage) : 0;
        int totalPages = puzzlePages + solPages;

        DOCINFOW di = {};
        di.cbSize = sizeof(DOCINFOW);
        di.lpszDocName = L"HoDoKu Sudoku Booklet";

        if (StartDocW(pd.hDC, &di) > 0) {
            int pWidth = GetDeviceCaps(pd.hDC, HORZRES);
            int pHeight = GetDeviceCaps(pd.hDC, VERTRES);

            int curPage = 1;

            // 1. Puzzle Pages
            for (int pg = 0; pg < puzzlePages; ++pg) {
                if (StartPage(pd.hDC) > 0) {
                    Gdiplus::Graphics g(pd.hDC);
                    int startIdx = pg * perPage;
                    int count = (std::min)(perPage, static_cast<int>(puzzles.size()) - startIdx);

                    std::vector<hodoku::core::BoardState> pagePuzzles(puzzles.begin() + startIdx, puzzles.begin() + startIdx + count);
                    std::vector<std::wstring> pageTitles(titles.begin() + startIdx, titles.begin() + startIdx + count);

                    render_page(g, pagePuzzles, pageTitles, curPage, totalPages, pWidth, pHeight,
                                config.layout, config.showCandidates, config.allBlack, false);
                    EndPage(pd.hDC);
                    curPage++;
                }
            }

            // 2. Solution Pages (6 per page)
            if (config.includeSolutions) {
                for (int spg = 0; spg < solPages; ++spg) {
                    if (StartPage(pd.hDC) > 0) {
                        Gdiplus::Graphics g(pd.hDC);
                        int startIdx = spg * solPerPage;
                        int count = (std::min)(solPerPage, static_cast<int>(solutions.size()) - startIdx);

                        std::vector<hodoku::core::BoardState> pageSolutions(solutions.begin() + startIdx, solutions.begin() + startIdx + count);
                        std::vector<std::wstring> pageSolTitles;
                        for (int k = 0; k < count; ++k) {
                            pageSolTitles.push_back(L"Solution #" + std::to_wstring(startIdx + k + 1));
                        }

                        render_page(g, pageSolutions, pageSolTitles, curPage, totalPages, pWidth, pHeight,
                                    PrintLayout::Six, false, config.allBlack, true);
                        EndPage(pd.hDC);
                        curPage++;
                    }
                }
            }

            EndDoc(pd.hDC);
        }

        DeleteDC(pd.hDC);
        if (pd.hDevMode) GlobalFree(pd.hDevMode);
        if (pd.hDevNames) GlobalFree(pd.hDevNames);

        return true;
    }
};

} // namespace hodoku::ui
