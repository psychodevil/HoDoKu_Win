#pragma once

#include "AppTypes.hpp"
#include "StudioModel.hpp"

namespace hodoku::ui {

class GridRenderer {
public:
    GridRenderer() = default;

    void update_layout(int x, int y, int width, int height) {
        int availW = width - 4;
        int availH = height - 4;
        int size = std::min(availW, availH);
        if (size < 100) size = 100;

        m_gridSize = size;
        m_cellSize = static_cast<float>(size) / 9.0f;
        m_offsetX = x + (availW - size) / 2;
        m_offsetY = y + (availH - size) / 2;
    }

    int hit_test_grid(int x, int y) const {
        if (x < m_offsetX || x >= m_offsetX + m_gridSize ||
            y < m_offsetY || y >= m_offsetY + m_gridSize || m_cellSize <= 0.0f) {
            return -1;
        }
        int c = static_cast<int>((x - m_offsetX) / m_cellSize);
        int r = static_cast<int>((y - m_offsetY) / m_cellSize);
        if (r >= 0 && r < 9 && c >= 0 && c < 9) {
            return cell_index(r, c);
        }
        return -1;
    }

    int hit_test_candidate(int x, int y, int cell) const {
        if (cell < 0 || cell >= TOTAL_CELLS || m_cellSize <= 0.0f) return 0;
        int r = cell_row(cell);
        int c = cell_col(cell);
        float cx = m_offsetX + c * m_cellSize;
        float cy = m_offsetY + r * m_cellSize;
        float relX = x - cx;
        float relY = y - cy;
        if (relX < 0.0f || relX >= m_cellSize || relY < 0.0f || relY >= m_cellSize) return 0;

        float sub = m_cellSize / 3.0f;
        int subC = static_cast<int>(relX / sub);
        int subR = static_cast<int>(relY / sub);
        if (subC >= 0 && subC < 3 && subR >= 0 && subR < 3) {
            int digit = subR * 3 + subC + 1;
            return digit;
        }
        return 0;
    }

    void render_grid_canvas(Graphics& g, const HoDoKuStudio& studio, int x, int y, int width, int height) {
        update_layout(x, y, width, height);

        // Background
        SolidBrush bgCanvas(Color(255, 255, 255, 255));
        g.FillRectangle(&bgCanvas, m_offsetX, m_offsetY, m_gridSize, m_gridSize);

        // Cell Background Highlights
        SolidBrush primHintBrush(Color(255, 255, 242, 117));     // HoDoKu Yellow Hint (#fff275)
        SolidBrush secHintBrush(Color(255, 255, 211, 182));      // HoDoKu Peach Secondary (#ffd3b6)
        SolidBrush filterHitBrush(Color(255, 185, 255, 185));    // HoDoKu POSSIBLE_CELL_COLOR from Options.java
        SolidBrush bivalueHitBrush(Color(255, 220, 252, 231));   // Bi-value Green

        StringFormat centerFmt;
        centerFmt.SetAlignment(StringAlignmentCenter);
        centerFmt.SetLineAlignment(StringAlignmentCenter);

        const auto& board = studio.get_board();
        auto selStep = studio.get_selected_step();
        auto hintLvl = studio.get_hint_level();
        int activeFilter = studio.get_active_filter();
        bool filterBivalue = studio.is_bivalue_filter();

        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            int r = cell_row(cell);
            int c = cell_col(cell);
            float cx = m_offsetX + c * m_cellSize;
            float cy = m_offsetY + r * m_cellSize;

            // 1. User cell custom color from palette (if set)
            int userCol = studio.get_cell_color(cell);
            if (userCol >= 0 && userCol < 10) {
                SolidBrush uBrush(HODOKU_PALETTE[userCol]);
                g.FillRectangle(&uBrush, cx, cy, m_cellSize, m_cellSize);
            }

            // 2. Overlays
            if (hintLvl == HintLevel::Concrete && selStep && selStep->primary_cells.test(cell)) {
                g.FillRectangle(&primHintBrush, cx, cy, m_cellSize, m_cellSize);
            } else if (hintLvl == HintLevel::Concrete && selStep && selStep->secondary_cells.test(cell)) {
                g.FillRectangle(&secHintBrush, cx, cy, m_cellSize, m_cellSize);
            } else if (activeFilter > 0 && (board.get_value(cell) == activeFilter || board.has_candidate(cell, activeFilter))) {
                g.FillRectangle(&filterHitBrush, cx, cy, m_cellSize, m_cellSize);
            } else if (filterBivalue && board.is_unfilled(cell) && board.count_candidates(cell) == 2) {
                g.FillRectangle(&bivalueHitBrush, cx, cy, m_cellSize, m_cellSize);
            }
        }

        // Grid Lines
        Pen thinLine(Color(255, 190, 195, 200), 1.0f);
        Pen thickLine(Color(255, 0, 0, 0), 2.5f);

        for (int i = 0; i <= 9; ++i) {
            float pos = i * m_cellSize;
            if (i % 3 != 0) {
                g.DrawLine(&thinLine, m_offsetX + pos, static_cast<float>(m_offsetY), m_offsetX + pos, static_cast<float>(m_offsetY + m_gridSize));
                g.DrawLine(&thinLine, static_cast<float>(m_offsetX), m_offsetY + pos, static_cast<float>(m_offsetX + m_gridSize), m_offsetY + pos);
            }
        }

        for (int i = 0; i <= 9; i += 3) {
            float pos = i * m_cellSize;
            g.DrawLine(&thickLine, m_offsetX + pos, static_cast<float>(m_offsetY), m_offsetX + pos, static_cast<float>(m_offsetY + m_gridSize));
            g.DrawLine(&thickLine, static_cast<float>(m_offsetX), m_offsetY + pos, static_cast<float>(m_offsetX + m_gridSize), m_offsetY + pos);
        }

        // Focus Cursor Accent Border (Exact Yellow outline from screenshot)
        int selectedCell = studio.get_selected_cell();
        if (selectedCell >= 0) {
            int r = cell_row(selectedCell);
            int c = cell_col(selectedCell);
            Pen cursorPen(Color(255, 234, 179, 8), 2.5f); // Gold/Yellow border
            g.DrawRectangle(&cursorPen, m_offsetX + c * m_cellSize + 1.0f, m_offsetY + r * m_cellSize + 1.0f, m_cellSize - 2.0f, m_cellSize - 2.0f);
        }

        // Digits & Candidates
        FontFamily ff(L"Segoe UI");
        Font digitFont(&ff, m_cellSize * 0.58f, FontStyleBold, UnitPixel);
        Font candFont(&ff, m_cellSize * 0.22f, FontStyleRegular, UnitPixel);
        Font candBoldFont(&ff, m_cellSize * 0.22f, FontStyleBold, UnitPixel);

        SolidBrush givenBrush(Color(255, 0, 0, 0));             // Black Given
        SolidBrush userBrush(Color(255, 0, 34, 204));           // Bold Blue User Value (#0022cc)
        SolidBrush candNormalBrush(Color(255, 35, 35, 35));     // Crisp Dark Candidate
        SolidBrush candHighlightBrush(Color(255, 0, 0, 0));     // Filtered Candidate
        SolidBrush candElimBrush(Color(255, 220, 38, 38));      // Red Eliminated
        Pen elimStrikePen(Color(255, 220, 38, 38), 1.5f);

        float subCell = m_cellSize / 3.0f;

        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            int r = cell_row(cell);
            int c = cell_col(cell);
            float cx = m_offsetX + c * m_cellSize;
            float cy = m_offsetY + r * m_cellSize;

            uint8_t val = board.get_value(cell);
            if (val != 0) {
                RectF cellRect(cx, cy, m_cellSize, m_cellSize);
                std::wstring text = std::to_wstring(val);
                Brush* b = board.is_given(cell) ? static_cast<Brush*>(&givenBrush) : static_cast<Brush*>(&userBrush);
                g.DrawString(text.c_str(), -1, &digitFont, cellRect, &centerFmt, b);
            } else {
                CandidateMask mask = board.get_candidates(cell);
                for (int d = 1; d <= 9; ++d) {
                    if (mask_has_digit(mask, d)) {
                        int dr = (d - 1) / 3;
                        int dc = (d - 1) % 3;
                        float kx = cx + dc * subCell;
                        float ky = cy + dr * subCell;
                        RectF candRect(kx, ky, subCell, subCell);

                        // Candidate-level custom coloring background
                        int candCol = studio.get_candidate_color(cell, d);
                        if (candCol > 0 && candCol < 10) {
                            SolidBrush candBg(HODOKU_PALETTE[candCol]);
                            g.FillRectangle(&candBg, kx + 1.0f, ky + 1.0f, subCell - 2.0f, subCell - 2.0f);
                        }

                        bool isElim = false;
                        if (hintLvl == HintLevel::Concrete && selStep) {
                            for (const auto& elim : selStep->eliminations) {
                                if (elim.cell == cell && elim.digit == d) {
                                    isElim = true;
                                    break;
                                }
                            }
                        }

                        bool isFilterMatch = (activeFilter == d);
                        Brush* cBrush = isElim ? static_cast<Brush*>(&candElimBrush)
                                      : isFilterMatch ? static_cast<Brush*>(&candHighlightBrush)
                                      : static_cast<Brush*>(&candNormalBrush);

                        Font* f = isFilterMatch ? &candBoldFont : &candFont;
                        std::wstring candStr = std::to_wstring(d);
                        g.DrawString(candStr.c_str(), -1, f, candRect, &centerFmt, cBrush);

                        if (isElim) {
                            g.DrawLine(&elimStrikePen, kx + 2, ky + 2, kx + subCell - 2, ky + subCell - 2);
                        }
                    }
                }
            }
        }
    }

    int get_grid_size() const { return m_gridSize; }
    float get_cell_size() const { return m_cellSize; }
    int get_offset_x() const { return m_offsetX; }
    int get_offset_y() const { return m_offsetY; }

private:
    int m_offsetX{0};
    int m_offsetY{0};
    int m_gridSize{540};
    float m_cellSize{60.0f};
};

} // namespace hodoku::ui

