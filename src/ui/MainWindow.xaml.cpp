#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "../core/SimpleTechniques.hpp"
#include <winrt/Microsoft.Graphics.Canvas.h>
#include <winrt/Microsoft.Graphics.Canvas.Geometry.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <algorithm>

namespace winrt::HoDoKuNative::UI::implementation
{
    using namespace Microsoft::Graphics::Canvas;
    using namespace Microsoft::Graphics::Canvas::Text;
    using namespace Microsoft::Graphics::Canvas::UI::Xaml;
    using namespace Windows::UI;

    MainWindow::MainWindow()
    {
        InitializeComponent();

        // Load a standard default puzzle
        m_board.from_string("530070000600195000098000060800060003400803001700020006060000280000419005000080079");
        m_initialBoard = m_board;
    }

    void MainWindow::UpdateMetrics(float width, float height)
    {
        float available = std::min(width, height) - 16.0f;
        if (available < 50.0f) available = 50.0f;

        m_gridSize = available;
        m_cellSize = m_gridSize / 9.0f;
        m_offsetX = (width - m_gridSize) / 2.0f;
        m_offsetY = (height - m_gridSize) / 2.0f;
    }

    int32_t MainWindow::HitTestCell(float x, float y) const noexcept
    {
        if (x < m_offsetX || x > m_offsetX + m_gridSize ||
            y < m_offsetY || y > m_offsetY + m_gridSize || m_cellSize <= 0.0f)
        {
            return -1;
        }

        int col = static_cast<int>((x - m_offsetX) / m_cellSize);
        int row = static_cast<int>((y - m_offsetY) / m_cellSize);

        if (row >= 0 && row < 9 && col >= 0 && col < 9) {
            return hodoku::core::cell_index(row, col);
        }
        return -1;
    }

    void MainWindow::RedrawCanvas()
    {
        if (SudokuCanvas()) {
            SudokuCanvas().Invalidate();
        }
    }

    void MainWindow::OnCanvasCreateResources(
        CanvasControl const& sender,
        CanvasCreateResourcesEventArgs const& /*args*/)
    {
        m_digitFormat = CanvasTextFormat();
        m_digitFormat.FontFamily(L"Segoe UI Variable Display");
        m_digitFormat.FontSize(32.0f);
        m_digitFormat.HorizontalAlignment(CanvasHorizontalAlignment::Center);
        m_digitFormat.VerticalAlignment(CanvasVerticalAlignment::Center);

        m_candidateFormat = CanvasTextFormat();
        m_candidateFormat.FontFamily(L"Segoe UI Variable Text");
        m_candidateFormat.FontSize(11.0f);
        m_candidateFormat.HorizontalAlignment(CanvasHorizontalAlignment::Center);
        m_candidateFormat.VerticalAlignment(CanvasVerticalAlignment::Center);
    }

    void MainWindow::OnCanvasSizeChanged(
        Windows::Foundation::IInspectable const& /*sender*/,
        Microsoft::UI::Xaml::SizeChangedEventArgs const& args)
    {
        UpdateMetrics(static_cast<float>(args.NewSize().Width), static_cast<float>(args.NewSize().Height));
    }

    void MainWindow::OnCanvasDraw(
        CanvasControl const& sender,
        CanvasDrawEventArgs const& args)
    {
        auto ds = args.DrawingSession();

        // 1. Clear background
        Color bg = Color{255, 255, 255, 255};
        ds.Clear(bg);

        if (m_gridSize <= 0.0f || m_cellSize <= 0.0f) {
            return;
        }

        // Color definitions
        Color selectionColor{255, 186, 230, 253};     // Light Sky Blue
        Color peerHighlightColor{255, 240, 249, 255}; // Very subtle Blue
        Color stepPrimaryColor{255, 254, 240, 138};   // Soft Yellow
        Color stepSecondaryColor{255, 254, 215, 170}; // Soft Peach
        Color thinLineColor{255, 203, 213, 225};      // Slate 300
        Color thickLineColor{255, 15, 23, 42};        // Slate 900
        Color givenDigitColor{255, 15, 23, 42};       // Bold Dark Navy
        Color userDigitColor{255, 2, 132, 199};       // Blue 600
        Color candidateColor{255, 100, 116, 139};     // Slate 500
        Color elimCandidateColor{255, 239, 68, 68};   // Red 500

        // 2. Draw cell backgrounds (Highlights & Selection)
        const auto& peerBitset = (m_selectedCell >= 0) ? hodoku::core::get_peer_bitset(m_selectedCell) : hodoku::core::BitSet81();

        for (int cell = 0; cell < hodoku::core::TOTAL_CELLS; ++cell) {
            int r = hodoku::core::cell_row(cell);
            int c = hodoku::core::cell_col(cell);
            float cellX = m_offsetX + c * m_cellSize;
            float cellY = m_offsetY + r * m_cellSize;

            // Highlight priority: Step Primary > Step Secondary > Selected > Peer
            if (m_selectedStep && m_selectedStep->primary_cells.test(cell)) {
                ds.FillRectangle(cellX, cellY, m_cellSize, m_cellSize, stepPrimaryColor);
            } else if (m_selectedStep && m_selectedStep->secondary_cells.test(cell)) {
                ds.FillRectangle(cellX, cellY, m_cellSize, m_cellSize, stepSecondaryColor);
            } else if (cell == m_selectedCell) {
                ds.FillRectangle(cellX, cellY, m_cellSize, m_cellSize, selectionColor);
            } else if (peerBitset.test(cell)) {
                ds.FillRectangle(cellX, cellY, m_cellSize, m_cellSize, peerHighlightColor);
            }
        }

        // 3. Draw grid lines
        // Thin cell lines
        for (int i = 0; i <= 9; ++i) {
            float pos = i * m_cellSize;
            if (i % 3 != 0) {
                ds.DrawLine(m_offsetX + pos, m_offsetY, m_offsetX + pos, m_offsetY + m_gridSize, thinLineColor, 1.0f);
                ds.DrawLine(m_offsetX, m_offsetY + pos, m_offsetX + m_gridSize, m_offsetY + pos, thinLineColor, 1.0f);
            }
        }

        // Thick box lines & outer frame
        for (int i = 0; i <= 9; i += 3) {
            float pos = i * m_cellSize;
            ds.DrawLine(m_offsetX + pos, m_offsetY, m_offsetX + pos, m_offsetY + m_gridSize, thickLineColor, 2.5f);
            ds.DrawLine(m_offsetX, m_offsetY + pos, m_offsetX + m_gridSize, m_offsetY + pos, thickLineColor, 2.5f);
        }

        // 4. Render cell values and pencilmark candidates
        m_digitFormat.FontSize(m_cellSize * 0.65f);
        float subCellSize = m_cellSize / 3.0f;
        m_candidateFormat.FontSize(subCellSize * 0.75f);

        for (int cell = 0; cell < hodoku::core::TOTAL_CELLS; ++cell) {
            int r = hodoku::core::cell_row(cell);
            int c = hodoku::core::cell_col(cell);
            float cellX = m_offsetX + c * m_cellSize;
            float cellY = m_offsetY + r * m_cellSize;

            uint8_t val = m_board.get_value(cell);
            if (val != 0) {
                // Large Digit
                Color digitCol = m_board.is_given(cell) ? givenDigitColor : userDigitColor;
                std::wstring text = std::to_wstring(val);
                ds.DrawText(text, cellX, cellY, m_cellSize, m_cellSize, digitCol, m_digitFormat);
            } else {
                // 3x3 Candidate Mini-Grid
                hodoku::core::CandidateMask mask = m_board.get_candidates(cell);
                for (int d = 1; d <= 9; ++d) {
                    if (hodoku::core::mask_has_digit(mask, d)) {
                        int dr = (d - 1) / 3;
                        int dc = (d - 1) % 3;
                        float candX = cellX + dc * subCellSize;
                        float candY = cellY + dr * subCellSize;

                        // Check if candidate is being eliminated in selected step
                        bool isEliminated = false;
                        if (m_selectedStep) {
                            for (const auto& elim : m_selectedStep->eliminations) {
                                if (elim.cell == cell && elim.digit == d) {
                                    isEliminated = true;
                                    break;
                                }
                            }
                        }

                        Color cCol = isEliminated ? elimCandidateColor : candidateColor;
                        std::wstring candText = std::to_wstring(d);
                        ds.DrawText(candText, candX, candY, subCellSize, subCellSize, cCol, m_candidateFormat);

                        if (isEliminated) {
                            // Strike-through line over eliminated candidate
                            ds.DrawLine(candX + 2.0f, candY + 2.0f, candX + subCellSize - 2.0f, candY + subCellSize - 2.0f, elimCandidateColor, 1.5f);
                        }
                    }
                }
            }
        }
    }

    void MainWindow::OnCanvasPointerPressed(
        Windows::Foundation::IInspectable const& /*sender*/,
        Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args)
    {
        auto pt = args.GetCurrentPoint(SudokuCanvas()).Position();
        int32_t cell = HitTestCell(pt.X, pt.Y);
        if (cell != m_selectedCell) {
            m_selectedCell = cell;
            RedrawCanvas();
        }
    }

    void MainWindow::OnCanvasPointerMoved(
        Windows::Foundation::IInspectable const& /*sender*/,
        Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args)
    {
        auto pt = args.GetCurrentPoint(SudokuCanvas()).Position();
        int32_t cell = HitTestCell(pt.X, pt.Y);
        if (cell != m_hoveredCell) {
            m_hoveredCell = cell;
        }
    }

    void MainWindow::OnCanvasPointerReleased(
        Windows::Foundation::IInspectable const& /*sender*/,
        Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& /*args*/)
    {
    }

    void MainWindow::OnNewPuzzleClicked(
        Windows::Foundation::IInspectable const& /*sender*/,
        Microsoft::UI::Xaml::RoutedEventArgs const& /*args*/)
    {
        // Set a fresh sample puzzle
        m_board.from_string("000000010400000000020000000000050407008000300001090000300400200050100000000806000");
        m_initialBoard = m_board;
        m_selectedCell = -1;
        m_selectedStep.reset();
        m_activeSteps.clear();
        RedrawCanvas();
    }

    void MainWindow::OnHintClicked(
        Windows::Foundation::IInspectable const& /*sender*/,
        Microsoft::UI::Xaml::RoutedEventArgs const& /*args*/)
    {
        m_activeSteps.clear();

        // 1. Try Naked Singles
        auto ns = hodoku::core::SimpleTechniques::find_naked_singles(m_board);
        m_activeSteps.insert(m_activeSteps.end(), ns.begin(), ns.end());

        // 2. Try Hidden Singles
        if (m_activeSteps.empty()) {
            auto hs = hodoku::core::SimpleTechniques::find_hidden_singles(m_board);
            m_activeSteps.insert(m_activeSteps.end(), hs.begin(), hs.end());
        }

        // 3. Try Locked Candidates
        if (m_activeSteps.empty()) {
            auto lc = hodoku::core::SimpleTechniques::find_locked_candidates(m_board);
            m_activeSteps.insert(m_activeSteps.end(), lc.begin(), lc.end());
        }

        if (!m_activeSteps.empty()) {
            m_selectedStep = m_activeSteps.front();
        } else {
            m_selectedStep.reset();
        }

        RedrawCanvas();
    }

    void MainWindow::OnSolveClicked(
        Windows::Foundation::IInspectable const& /*sender*/,
        Microsoft::UI::Xaml::RoutedEventArgs const& /*args*/)
    {
        auto solution = m_solver.solve_one(m_board);
        if (solution) {
            m_board = *solution;
            m_selectedStep.reset();
            m_activeSteps.clear();
            RedrawCanvas();
        }
    }

    void MainWindow::OnResetClicked(
        Windows::Foundation::IInspectable const& /*sender*/,
        Microsoft::UI::Xaml::RoutedEventArgs const& /*args*/)
    {
        m_board = m_initialBoard;
        m_selectedStep.reset();
        m_activeSteps.clear();
        RedrawCanvas();
    }

    void MainWindow::OnClearClicked(
        Windows::Foundation::IInspectable const& /*sender*/,
        Microsoft::UI::Xaml::RoutedEventArgs const& /*args*/)
    {
        m_board.clear();
        m_initialBoard.clear();
        m_selectedCell = -1;
        m_selectedStep.reset();
        m_activeSteps.clear();
        RedrawCanvas();
    }

    void MainWindow::OnDigitClicked(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& /*args*/)
    {
        if (m_selectedCell >= 0 && !m_board.is_given(m_selectedCell)) {
            auto button = sender.as<Microsoft::UI::Xaml::Controls::Button>();
            auto tagStr = unbox_value<hstring>(button.Tag());
            int digit = std::stoi(to_string(tagStr));
            m_board.set_value(m_selectedCell, digit);
            RedrawCanvas();
        }
    }

    void MainWindow::OnEraseClicked(
        Windows::Foundation::IInspectable const& /*sender*/,
        Microsoft::UI::Xaml::RoutedEventArgs const& /*args*/)
    {
        if (m_selectedCell >= 0 && !m_board.is_given(m_selectedCell)) {
            // Clear value or toggle
            RedrawCanvas();
        }
    }

    void MainWindow::OnStepSelectionChanged(
        Windows::Foundation::IInspectable const& /*sender*/,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& /*args*/)
    {
        // Update m_selectedStep and redraw
        RedrawCanvas();
    }
}

