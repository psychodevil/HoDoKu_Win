#pragma once

#include <cmath>
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

    void get_candidate_snap_point(int cell, int digit, float& outX, float& outY) const {
        float subCell = m_cellSize / 3.0f;
        int r = cell_row(cell);
        int c = cell_col(cell);
        if (digit >= 1 && digit <= 9) {
            int dr = (digit - 1) / 3;
            int dc = (digit - 1) % 3;
            outX = m_offsetX + c * m_cellSize + (dc + 0.5f) * subCell;
            outY = m_offsetY + r * m_cellSize + (dr + 0.5f) * subCell;
        } else {
            outX = m_offsetX + (c + 0.5f) * m_cellSize;
            outY = m_offsetY + (r + 0.5f) * m_cellSize;
        }
    }

    void draw_directed_link(Graphics& g, float x1, float y1, float x2, float y2,
                            bool fromCand, bool toCand, float candHeight, Pen& pen) const {
        float dx = x2 - x1;
        float dy = y2 - y1;
        float dist = std::hypot(dx, dy);
        if (dist < 2.0f) return;

        // Inset start and end points outside candidate circles (matching HoDoKu adjustEndPoints)
        float offset1 = fromCand ? (candHeight * 0.5f + 2.0f) : (m_cellSize * 0.2f);
        float offset2 = toCand ? (candHeight * 0.5f + 2.0f) : (m_cellSize * 0.2f);

        if (offset1 + offset2 >= dist - 4.0f) {
            float factor = (dist > 6.0f) ? ((dist - 6.0f) / (offset1 + offset2)) : 0.0f;
            offset1 *= factor;
            offset2 *= factor;
        }

        float ux = dx / dist;
        float uy = dy / dist;
        float sx = x1 + ux * offset1;
        float sy = y1 + uy * offset1;
        float ex = x2 - ux * offset2;
        float ey = y2 - uy * offset2;

        g.DrawLine(&pen, sx, sy, ex, ey);
    }

    void render_grid_canvas(Graphics& g, const HoDoKuStudio& studio, int x, int y, int width, int height) {
        update_layout(x, y, width, height);

        // 1. Background fill
        SolidBrush bgCanvas(Color(255, 255, 255, 255)); // HoDoKu DEFAULT_CELL_COLOR = Color.WHITE
        g.FillRectangle(&bgCanvas, m_offsetX, m_offsetY, m_gridSize, m_gridSize);

        // HoDoKu Official Color Palette from Options.java
        SolidBrush aktCellBrush(Color(255, 255, 255, 150));     // HoDoKu AKT_CELL_COLOR = new Color(255, 255, 150)
        SolidBrush possibleCellBrush(Color(255, 185, 255, 185)); // HoDoKu POSSIBLE_CELL_COLOR = new Color(185, 255, 185)
        SolidBrush invalidCellBrush(Color(255, 255, 185, 185));  // HoDoKu INVALID_CELL_COLOR = new Color(255, 185, 185)

        const auto& board = studio.get_board();
        auto activeStep = studio.get_hovered_step() ? studio.get_hovered_step() : studio.get_selected_step();
        bool showStepOverlays = (studio.get_hint_level() == HintLevel::Concrete && studio.get_selected_step()) || studio.get_hovered_step().has_value();

        int selectedCell = studio.get_selected_cell();
        int activeFilter = studio.get_active_filter();
        uint16_t filterMask = studio.get_filter_mask();
        bool filterBivalue = studio.is_bivalue_filter();
        bool filterExcluded = studio.is_filter_excluded_mode();
        BitSet81 conflictCells = board.get_invalid_conflict_cells();

        // 2. Cell Background Highlights (Exact HoDoKu priority order from SudokuPanel.java lines 2160-2220)
        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            int r = cell_row(cell);
            int c = cell_col(cell);
            float cx = m_offsetX + c * m_cellSize;
            float cy = m_offsetY + r * m_cellSize;

            bool isSelected = studio.is_cell_selected(cell);
            Brush* cellBrush = nullptr;

            // Priority 1: Default selected cell background
            if (isSelected) {
                cellBrush = &aktCellBrush;
            }

            // Priority 2: Filter mode (Possible vs Excluded)
            if (activeFilter > 0 || filterMask != 0) {
                bool candidateValid = false;
                if (activeFilter > 0) {
                    candidateValid = board.has_candidate(cell, activeFilter);
                } else if (filterMask != 0) {
                    for (int d = 1; d <= 9; ++d) {
                        if ((filterMask & (1u << d)) && board.has_candidate(cell, d)) {
                            candidateValid = true;
                            break;
                        }
                    }
                }
                if (!filterExcluded) {
                    // Green mode: ONLY unfilled cells with the candidate are highlighted green (SudokuPanel.java line 2197)
                    if (board.is_unfilled(cell) && candidateValid) {
                        cellBrush = &possibleCellBrush;
                    }
                } else {
                    // Red mode: unfilled cells where candidate is excluded, or cells with conflicts (SudokuPanel.java line 2192)
                    if (board.is_unfilled(cell) && !candidateValid) {
                        cellBrush = &invalidCellBrush;
                    } else if (conflictCells.test(cell)) {
                        cellBrush = &invalidCellBrush;
                    }
                }
            } else if (filterBivalue) {
                if (board.is_unfilled(cell) && board.count_candidates(cell) == 2) {
                    cellBrush = &possibleCellBrush;
                }
            } else if (filterExcluded && conflictCells.test(cell)) {
                // Red mode with no active filter: highlight conflicting cells
                cellBrush = &invalidCellBrush;
            }

            // Priority 3: Custom user palette coloring (0..9)
            int8_t userCol = studio.get_cell_color(cell);
            SolidBrush uBrush(Color(255, 255, 255, 255));
            if (userCol >= 0 && userCol < 10) {
                uBrush.SetColor(HODOKU_PALETTE[userCol]);
                cellBrush = &uBrush;
            }

            // Fill cell if colored
            if (cellBrush != nullptr) {
                g.FillRectangle(cellBrush, cx, cy, m_cellSize, m_cellSize);
            }

            // If selected cell has a background other than aktCellColor, draw HoDoKu cursor frame (SudokuPanel.java line 2208-2220)
            if (isSelected && cellBrush != &aktCellBrush) {
                float frameSize = m_cellSize * 0.08f; // CURSOR_FRAME_SIZE = 0.08 from Options.java
                g.FillRectangle(&aktCellBrush, cx, cy, m_cellSize, frameSize);
                g.FillRectangle(&aktCellBrush, cx, cy, frameSize, m_cellSize);
                g.FillRectangle(&aktCellBrush, cx + m_cellSize - frameSize, cy, frameSize, m_cellSize);
                g.FillRectangle(&aktCellBrush, cx, cy + m_cellSize - frameSize, m_cellSize, frameSize);
                Pen aktBdr(Color(255, 205, 195, 75), 1.0f);
                g.DrawRectangle(&aktBdr, cx + 0.5f, cy + 0.5f, m_cellSize - 1.0f, m_cellSize - 1.0f);
            } else if (cell == selectedCell && !studio.get_selected_cells().empty()) {
                // In multi-selection, emphasize the primary active anchor cell
                Pen anchorBdr(Color(255, 205, 195, 75), 1.5f);
                g.DrawRectangle(&anchorBdr, cx + 0.5f, cy + 0.5f, m_cellSize - 1.0f, m_cellSize - 1.0f);
            }
        }

        // 3. Grid Lines (HoDoKu INNER_GRID_COLOR = LIGHT_GRAY, GRID_COLOR = BLACK)
        Pen thinLine(Color(255, 200, 200, 200), 1.0f);
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

        // 4. Digits & Candidates (Fonts and Brushes from Options.java)
        FontFamily ff(L"Segoe UI");
        Font digitFont(&ff, m_cellSize * 0.58f, FontStyleBold, UnitPixel);
        Font candFont(&ff, m_cellSize * 0.22f, FontStyleRegular, UnitPixel);
        Font candBoldFont(&ff, m_cellSize * 0.22f, FontStyleBold, UnitPixel);

        SolidBrush givenBrush(Color(255, 0, 0, 0));             // CELL_FIXED_VALUE_COLOR = Color.BLACK
        SolidBrush userBrush(Color(255, 0, 34, 204));           // CELL_VALUE_COLOR = Color.BLUE (#0022cc)
        SolidBrush wrongBrush(Color(255, 235, 0, 0));           // WRONG_VALUE_COLOR = Color.RED
        SolidBrush candNormalBrush(Color(255, 100, 100, 100));  // CANDIDATE_COLOR = new Color(100, 100, 100)
        SolidBrush candHighlightBrush(Color(255, 0, 0, 0));     // Active/Key Candidate Color = Color.BLACK

        // Hint candidate background colors from Options.java lines 366-370
        SolidBrush hintCandBackBrush(Color(255, 63, 218, 101));       // HINT_CANDIDATE_BACK_COLOR = new Color(63, 218, 101)
        SolidBrush hintDeleteBackBrush(Color(255, 255, 118, 132));    // HINT_CANDIDATE_DELETE_BACK_COLOR = new Color(255, 118, 132)
        SolidBrush hintFinBackBrush(Color(255, 127, 187, 255));       // HINT_CANDIDATE_FIN_BACK_COLOR = new Color(127, 187, 255)
        SolidBrush hintAlsBackBrush(Color(255, 197, 232, 140));       // HINT_CANDIDATE_ALS_BACK_COLORS[0] = new Color(197, 232, 140)
        Pen elimStrikePen(Color(255, 220, 38, 38), 1.5f);

        StringFormat centerFmt;
        centerFmt.SetAlignment(StringAlignmentCenter);
        centerFmt.SetLineAlignment(StringAlignmentCenter);

        float subCell = m_cellSize / 3.0f;
        float candHeight = subCell * 0.88f;

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
                    Brush* b = board.is_given(cell) ? static_cast<Brush*>(&givenBrush)
                             : conflictCells.test(cell) ? static_cast<Brush*>(&wrongBrush)
                             : static_cast<Brush*>(&userBrush);
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
                        bool isFin = false;
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

                        if (studio.is_colorku_mode()) {
                            float beadRadius = subCell * 0.32f;
                            draw_colorku_marble(g, kx + subCell * 0.5f, ky + subCell * 0.5f, beadRadius, d);
                            if (isElim) {
                                g.DrawLine(&elimStrikePen, kx + 2.0f, ky + 2.0f, kx + subCell - 2.0f, ky + subCell - 2.0f);
                                g.DrawLine(&elimStrikePen, kx + subCell - 2.0f, ky + 2.0f, kx + 2.0f, ky + subCell - 2.0f);
                            }
                        } else {
                            // Candidate background highlighting (HoDoKu fillOval under candidate digit)
                            float ovalOffset = (subCell - candHeight) / 2.0f;
                            if (isElim) {
                                g.FillEllipse(&hintDeleteBackBrush, kx + ovalOffset, ky + ovalOffset, candHeight, candHeight);
                            } else if (isAssign) {
                                g.FillEllipse(&hintCandBackBrush, kx + ovalOffset, ky + ovalOffset, candHeight, candHeight);
                            } else if (isFin) {
                                g.FillEllipse(&hintFinBackBrush, kx + ovalOffset, ky + ovalOffset, candHeight, candHeight);
                            } else {
                                int8_t candCol = studio.get_candidate_color(cell, d);
                                if (candCol >= 0 && candCol < 10) {
                                    SolidBrush cBg(HODOKU_PALETTE[candCol]);
                                    g.FillEllipse(&cBg, kx + ovalOffset, ky + ovalOffset, candHeight, candHeight);
                                }
                            }

                            bool isFilterMatch = (activeFilter == d) || ((filterMask & (1u << d)) != 0);
                            Brush* cBrush = (isElim || isAssign || isFilterMatch) ? static_cast<Brush*>(&candHighlightBrush)
                                                                                  : static_cast<Brush*>(&candNormalBrush);
                            Font* f = (isFilterMatch || isAssign) ? &candBoldFont : &candFont;

                            std::wstring candStr = std::to_wstring(d);
                            g.DrawString(candStr.c_str(), -1, f, candRect, &centerFmt, cBrush);

                            // Highlight active link start candidate
                            if (studio.has_link_start() && studio.get_link_start_cell() == cell && studio.get_link_start_digit() == d) {
                                Pen linkStartPen(studio.is_drawing_strong_link() ? Color(255, 37, 99, 235) : Color(255, 234, 88, 12), 2.0f);
                                g.DrawEllipse(&linkStartPen, kx + ovalOffset - 1.0f, ky + ovalOffset - 1.0f, candHeight + 2.0f, candHeight + 2.0f);
                            }

                            if (isElim) {
                                g.DrawLine(&elimStrikePen, kx + 2.0f, ky + 2.0f, kx + subCell - 2.0f, ky + subCell - 2.0f);
                                g.DrawLine(&elimStrikePen, kx + subCell - 2.0f, ky + 2.0f, kx + 2.0f, ky + subCell - 2.0f);
                            }
                        }
                    }
                }
            }
        }

        // 5. Chain Links from Hint / Solution Step (HoDoKu ARROW_COLOR = Color.RED / Green for strong)
        if (showStepOverlays && activeStep && !activeStep->links.empty()) {
            AdjustableArrowCap arrowCap(3.5f, 3.5f, true);

            Pen strongPen(Color(230, 34, 197, 94), 2.2f);
            strongPen.SetCustomEndCap(&arrowCap);

            Pen weakPen(Color(230, 239, 68, 68), 1.8f);
            weakPen.SetDashStyle(DashStyleDash);
            weakPen.SetCustomEndCap(&arrowCap);

            for (const auto& link : activeStep->links) {
                float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
                bool fromCand = (link.from_digit >= 1 && link.from_digit <= 9);
                bool toCand = (link.to_digit >= 1 && link.to_digit <= 9);
                get_candidate_snap_point(link.from_cell, link.from_digit, x1, y1);
                get_candidate_snap_point(link.to_cell, link.to_digit, x2, y2);

                Pen* p = link.is_strong ? &strongPen : &weakPen;
                draw_directed_link(g, x1, y1, x2, y2, fromCand, toCand, candHeight, *p);
            }
        }

        // 6. User Manual Inference Links (Plan 6.3)
        const auto& userLinks = studio.get_user_links();
        if (!userLinks.empty() || studio.has_link_start()) {
            AdjustableArrowCap arrowCap(3.5f, 3.5f, true);

            // Strong link: solid line with arrow cap (HoDoKu royal blue)
            Pen userStrongPen(Color(240, 37, 99, 235), 2.2f);
            userStrongPen.SetCustomEndCap(&arrowCap);

            // Weak link: dashed line with arrow cap (HoDoKu amber / orange)
            Pen userWeakPen(Color(240, 234, 88, 12), 2.0f);
            userWeakPen.SetDashStyle(DashStyleDash);
            userWeakPen.SetCustomEndCap(&arrowCap);

            for (const auto& link : userLinks) {
                float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
                bool fromCand = (link.from_digit >= 1 && link.from_digit <= 9);
                bool toCand = (link.to_digit >= 1 && link.to_digit <= 9);
                get_candidate_snap_point(link.from_cell, link.from_digit, x1, y1);
                get_candidate_snap_point(link.to_cell, link.to_digit, x2, y2);

                Pen& p = link.is_strong ? userStrongPen : userWeakPen;
                draw_directed_link(g, x1, y1, x2, y2, fromCand, toCand, candHeight, p);
            }

            // Live preview arrow while actively drawing a link
            if (studio.has_link_start() && studio.get_hovered_cell() >= 0 && studio.get_hovered_candidate() > 0) {
                int startCell = studio.get_link_start_cell();
                int startDigit = studio.get_link_start_digit();
                int hoverCell = studio.get_hovered_cell();
                int hoverDigit = studio.get_hovered_candidate();

                if (startCell != hoverCell || startDigit != hoverDigit) {
                    float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
                    get_candidate_snap_point(startCell, startDigit, x1, y1);
                    get_candidate_snap_point(hoverCell, hoverDigit, x2, y2);

                    Color previewColor = studio.is_drawing_strong_link()
                        ? Color(180, 37, 99, 235)
                        : Color(180, 234, 88, 12);
                    Pen previewPen(previewColor, 2.0f);
                    previewPen.SetDashStyle(DashStyleDash);
                    previewPen.SetCustomEndCap(&arrowCap);

                    draw_directed_link(g, x1, y1, x2, y2, true, true, candHeight, previewPen);
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
