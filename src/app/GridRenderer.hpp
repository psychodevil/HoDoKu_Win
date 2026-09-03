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

    static void draw_colorku_marble(Graphics& g, float cx, float cy, float radius, int digit) {
        if (digit < 1 || digit > 9) return;
        Color baseCol = COLORKU_PALETTE[digit];

        // 1. Soft drop shadow behind marble
        SolidBrush shadowBrush(Color(70, 0, 0, 0));
        g.FillEllipse(&shadowBrush, cx - radius + 1.5f, cy - radius + 2.5f, radius * 2.0f, radius * 2.0f);

        // 2. Base 3D sphere body
        SolidBrush bodyBrush(baseCol);
        g.FillEllipse(&bodyBrush, cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

        // 3. Shaded rim arc for 3D depth
        Color darkCol(80, 0, 0, 0);
        Pen rimPen(darkCol, std::max(1.0f, radius * 0.15f));
        g.DrawArc(&rimPen, cx - radius + 1.0f, cy - radius + 1.0f, radius * 2.0f - 2.0f, radius * 2.0f - 2.0f, 15.0f, 120.0f);

        // 4. Upper-left specular highlight (polished glass / lacquer reflection)
        float hlRadius = radius * 0.42f;
        float hx = cx - radius * 0.35f;
        float hy = cy - radius * 0.35f;
        SolidBrush hlBrush(Color(180, 255, 255, 255));
        g.FillEllipse(&hlBrush, hx - hlRadius * 0.5f, hy - hlRadius * 0.5f, hlRadius, hlRadius);

        // 5. Pinpoint white sparkle glint
        if (radius >= 10.0f) {
            SolidBrush glintBrush(Color(240, 255, 255, 255));
            g.FillEllipse(&glintBrush, hx - 1.0f, hy - 1.0f, 2.5f, 2.5f);
        }
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
        auto activeStep = studio.get_hovered_step() ? studio.get_hovered_step() : studio.get_selected_step();
        bool showStepOverlays = (studio.get_hint_level() == HintLevel::Concrete && studio.get_selected_step()) || studio.get_hovered_step().has_value();

        int hoveredCell = studio.get_hovered_cell();
        int hoveredCand = studio.get_hovered_candidate();
        int selectedCell = studio.get_selected_cell();
        int activeFilter = studio.get_active_filter();
        bool filterBivalue = studio.is_bivalue_filter();

        // Crosshair Guide (Subtle Row, Column, Box alignment guides)
        if (hoveredCell >= 0 && hoveredCell < TOTAL_CELLS) {
            int hr = cell_row(hoveredCell);
            int hc = cell_col(hoveredCell);
            int hb = cell_box(hoveredCell);
            SolidBrush crosshairBrush(Color(20, 59, 130, 246)); // 8% alpha cool blue
            for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
                if (cell != hoveredCell && (cell_row(cell) == hr || cell_col(cell) == hc || cell_box(cell) == hb)) {
                    int r = cell_row(cell);
                    int c = cell_col(cell);
                    float cx = m_offsetX + c * m_cellSize;
                    float cy = m_offsetY + r * m_cellSize;
                    g.FillRectangle(&crosshairBrush, cx, cy, m_cellSize, m_cellSize);
                }
            }
        }

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
            if (showStepOverlays && activeStep && activeStep->primary_cells.test(cell)) {
                g.FillRectangle(&primHintBrush, cx, cy, m_cellSize, m_cellSize);
            } else if (showStepOverlays && activeStep && activeStep->secondary_cells.test(cell)) {
                g.FillRectangle(&secHintBrush, cx, cy, m_cellSize, m_cellSize);
            } else if (activeFilter > 0 && (board.get_value(cell) == activeFilter || board.has_candidate(cell, activeFilter))) {
                g.FillRectangle(&filterHitBrush, cx, cy, m_cellSize, m_cellSize);
            } else if (filterBivalue && board.is_unfilled(cell) && board.count_candidates(cell) == 2) {
                g.FillRectangle(&bivalueHitBrush, cx, cy, m_cellSize, m_cellSize);
            } else if (cell == hoveredCell && cell != selectedCell) {
                SolidBrush hoverCellBrush(Color(50, 59, 130, 246)); // Soft blue hover
                g.FillRectangle(&hoverCellBrush, cx, cy, m_cellSize, m_cellSize);
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
        if (selectedCell >= 0) {
            int r = cell_row(selectedCell);
            int c = cell_col(selectedCell);
            Pen cursorPen(Color(255, 234, 179, 8), 2.5f); // Gold/Yellow border
            g.DrawRectangle(&cursorPen, m_offsetX + c * m_cellSize + 1.0f, m_offsetY + r * m_cellSize + 1.0f, m_cellSize - 2.0f, m_cellSize - 2.0f);
        }

        // Hovered Cell Accent Border
        if (hoveredCell >= 0 && hoveredCell != selectedCell) {
            int r = cell_row(hoveredCell);
            int c = cell_col(hoveredCell);
            Pen hoverPen(Color(180, 59, 130, 246), 1.8f); // Soft blue border
            g.DrawRectangle(&hoverPen, m_offsetX + c * m_cellSize + 1.0f, m_offsetY + r * m_cellSize + 1.0f, m_cellSize - 2.0f, m_cellSize - 2.0f);
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
                if (studio.is_colorku_mode()) {
                    float marbleRadius = m_cellSize * 0.38f;
                    draw_colorku_marble(g, cx + m_cellSize * 0.5f, cy + m_cellSize * 0.5f, marbleRadius, val);
                } else {
                    RectF cellRect(cx, cy, m_cellSize, m_cellSize);
                    std::wstring text = std::to_wstring(val);
                    Brush* b = board.is_given(cell) ? static_cast<Brush*>(&givenBrush) : static_cast<Brush*>(&userBrush);
                    g.DrawString(text.c_str(), -1, &digitFont, cellRect, &centerFmt, b);
                }
            } else {
                CandidateMask mask = board.get_candidates(cell);
                for (int d = 1; d <= 9; ++d) {
                    if (mask_has_digit(mask, d)) {
                        int dr = (d - 1) / 3;
                        int dc = (d - 1) % 3;
                        float kx = cx + dc * subCell;
                        float ky = cy + dr * subCell;
                        RectF candRect(kx, ky, subCell, subCell);

                        bool isElim = false;
                        bool isAssign = false;
                        if (showStepOverlays && activeStep) {
                            for (const auto& elim : activeStep->eliminations) {
                                if (elim.cell == cell && elim.digit == d) {
                                    isElim = true;
                                    break;
                                }
                            }
                            for (const auto& asgn : activeStep->assignments) {
                                if (asgn.cell == cell && asgn.digit == d) {
                                    isAssign = true;
                                    break;
                                }
                            }
                        }

                        bool isHoveredCand = (cell == hoveredCell && d == hoveredCand);

                        if (studio.is_colorku_mode()) {
                            float beadRadius = subCell * 0.32f;
                            if (isHoveredCand) {
                                SolidBrush hoverRing(Color(180, 59, 130, 246));
                                g.FillEllipse(&hoverRing, kx + 1.0f, ky + 1.0f, subCell - 2.0f, subCell - 2.0f);
                            }
                            draw_colorku_marble(g, kx + subCell * 0.5f, ky + subCell * 0.5f, beadRadius, d);
                            if (isElim) {
                                g.DrawLine(&elimStrikePen, kx + 2.0f, ky + 2.0f, kx + subCell - 2.0f, ky + subCell - 2.0f);
                                g.DrawLine(&elimStrikePen, kx + subCell - 2.0f, ky + 2.0f, kx + 2.0f, ky + subCell - 2.0f);
                            }
                        } else {
                            // Candidate-level custom coloring background
                            int candCol = studio.get_candidate_color(cell, d);
                            if (candCol > 0 && candCol < 10) {
                                SolidBrush candBg(HODOKU_PALETTE[candCol]);
                                g.FillRectangle(&candBg, kx + 1.0f, ky + 1.0f, subCell - 2.0f, subCell - 2.0f);
                            }

                            if (isHoveredCand) {
                                SolidBrush candHoverPill(Color(200, 59, 130, 246));
                                g.FillEllipse(&candHoverPill, kx + 1.0f, ky + 1.0f, subCell - 2.0f, subCell - 2.0f);
                            } else if (isAssign) {
                                SolidBrush assignGlow(Color(180, 187, 247, 208));
                                g.FillEllipse(&assignGlow, kx + 1.0f, ky + 1.0f, subCell - 2.0f, subCell - 2.0f);
                                Pen assignBdr(Color(255, 34, 197, 94), 1.2f);
                                g.DrawEllipse(&assignBdr, kx + 1.0f, ky + 1.0f, subCell - 2.0f, subCell - 2.0f);
                            }

                            bool isFilterMatch = (activeFilter == d);
                            SolidBrush candHoverTextBrush(Color(255, 255, 255, 255));
                            Brush* cBrush = isElim ? static_cast<Brush*>(&candElimBrush)
                                          : isHoveredCand ? static_cast<Brush*>(&candHoverTextBrush)
                                          : isFilterMatch ? static_cast<Brush*>(&candHighlightBrush)
                                          : isAssign ? static_cast<Brush*>(&candHighlightBrush)
                                          : static_cast<Brush*>(&candNormalBrush);

                            Font* f = (isFilterMatch || isAssign || isHoveredCand) ? &candBoldFont : &candFont;
                            std::wstring candStr = std::to_wstring(d);
                            g.DrawString(candStr.c_str(), -1, f, candRect, &centerFmt, cBrush);

                            if (isElim) {
                                g.DrawLine(&elimStrikePen, kx + 2.0f, ky + 2.0f, kx + subCell - 2.0f, ky + subCell - 2.0f);
                                g.DrawLine(&elimStrikePen, kx + subCell - 2.0f, ky + 2.0f, kx + 2.0f, ky + subCell - 2.0f);
                            }
                        }
                    }
                }
            }
        }

        // Render Graphical Step Link Overlays (Strong & Weak Chain Links)
        if (showStepOverlays && activeStep && !activeStep->links.empty()) {
            AdjustableArrowCap arrowCap(3.5f, 3.5f, true);

            Pen strongPen(Color(230, 34, 197, 94), 2.2f);
            strongPen.SetCustomEndCap(&arrowCap);

            Pen weakPen(Color(230, 239, 68, 68), 1.8f);
            weakPen.SetDashStyle(DashStyleDash);
            weakPen.SetCustomEndCap(&arrowCap);

            for (const auto& link : activeStep->links) {
                float x1, y1, x2, y2;
                if (link.from_digit >= 1 && link.from_digit <= 9) {
                    x1 = m_offsetX + cell_col(link.from_cell) * m_cellSize + ((link.from_digit - 1) % 3 + 0.5f) * subCell;
                    y1 = m_offsetY + cell_row(link.from_cell) * m_cellSize + ((link.from_digit - 1) / 3 + 0.5f) * subCell;
                } else {
                    x1 = m_offsetX + (cell_col(link.from_cell) + 0.5f) * m_cellSize;
                    y1 = m_offsetY + (cell_row(link.from_cell) + 0.5f) * m_cellSize;
                }

                if (link.to_digit >= 1 && link.to_digit <= 9) {
                    x2 = m_offsetX + cell_col(link.to_cell) * m_cellSize + ((link.to_digit - 1) % 3 + 0.5f) * subCell;
                    y2 = m_offsetY + cell_row(link.to_cell) * m_cellSize + ((link.to_digit - 1) / 3 + 0.5f) * subCell;
                } else {
                    x2 = m_offsetX + (cell_col(link.to_cell) + 0.5f) * m_cellSize;
                    y2 = m_offsetY + (cell_row(link.to_cell) + 0.5f) * m_cellSize;
                }

                Pen* p = link.is_strong ? &strongPen : &weakPen;
                g.DrawLine(p, x1, y1, x2, y2);
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

