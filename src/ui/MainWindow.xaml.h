#pragma once

#include "MainWindow.g.h"
#include "../core/BoardState.hpp"
#include "../core/DlxSolver.hpp"
#include "../core/Step.hpp"
#include <winrt/Microsoft.Graphics.Canvas.UI.Xaml.h>
#include <winrt/Microsoft.Graphics.Canvas.Text.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <winrt/Windows.UI.h>
#include <vector>
#include <optional>

namespace winrt::HoDoKuNative::UI::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        int32_t SelectedCell() const noexcept { return m_selectedCell; }

        // Win2D Canvas Callbacks
        void OnCanvasCreateResources(
            Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender,
            Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesEventArgs const& args);

        void OnCanvasDraw(
            Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender,
            Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args);

        void OnCanvasPointerPressed(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args);

        void OnCanvasPointerMoved(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args);

        void OnCanvasPointerReleased(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args);

        void OnCanvasSizeChanged(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::SizeChangedEventArgs const& args);

        // UI Event Handlers
        void OnNewPuzzleClicked(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::RoutedEventArgs const& args);

        void OnHintClicked(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::RoutedEventArgs const& args);

        void OnSolveClicked(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::RoutedEventArgs const& args);

        void OnResetClicked(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::RoutedEventArgs const& args);

        void OnClearClicked(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::RoutedEventArgs const& args);

        void OnDigitClicked(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::RoutedEventArgs const& args);

        void OnEraseClicked(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::RoutedEventArgs const& args);

        void OnStepSelectionChanged(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

    private:
        // Pure C++20 Core Board State (Zero WinRT in Core)
        hodoku::core::BoardState m_board;
        hodoku::core::BoardState m_initialBoard;
        hodoku::core::DlxSolver m_solver;
        std::vector<hodoku::core::Step> m_activeSteps;
        std::optional<hodoku::core::Step> m_selectedStep;

        int32_t m_selectedCell{-1};
        int32_t m_hoveredCell{-1};

        // Win2D DirectWrite Typography
        Microsoft::Graphics::Canvas::Text::CanvasTextFormat m_digitFormat{nullptr};
        Microsoft::Graphics::Canvas::Text::CanvasTextFormat m_candidateFormat{nullptr};

        // Grid Metric Calculations
        float m_gridSize{0.0f};
        float m_cellSize{0.0f};
        float m_offsetX{0.0f};
        float m_offsetY{0.0f};

        void UpdateMetrics(float width, float height);
        [[nodiscard]] int32_t HitTestCell(float x, float y) const noexcept;
        void RedrawCanvas();
    };
}

namespace winrt::HoDoKuNative::UI::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}

