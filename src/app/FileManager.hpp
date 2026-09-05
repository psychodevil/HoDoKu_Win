#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include "StudioModel.hpp"

namespace hodoku::ui {

enum class FileFormat {
    Auto,
    Sdk,
    SimpleSudoku,
    HoDoKuSolution,
    PlainText
};

class FileManager {
public:
    static FileFormat detect_format(const std::wstring& path, const std::string& content) {
        std::wstring lowerPath = path;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);

        if (lowerPath.ends_with(L".hsol") || content.find("[HODOKU_SOLUTION]") != std::string::npos || content.find("<sudoku") != std::string::npos) {
            return FileFormat::HoDoKuSolution;
        }
        if (lowerPath.ends_with(L".ss") || (content.find("I") != std::string::npos && content.find("E") != std::string::npos)) {
            return FileFormat::SimpleSudoku;
        }
        if (lowerPath.ends_with(L".sdk")) {
            return FileFormat::Sdk;
        }
        return FileFormat::PlainText;
    }

    static bool load_file(const std::wstring& path, HoDoKuStudio& studio, std::string& outError) {
        std::ifstream file(path.c_str(), std::ios::binary);
        if (!file.is_open()) {
            outError = "Failed to open file for reading.";
            return false;
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        if (content.empty()) {
            outError = "File is empty.";
            return false;
        }

        FileFormat fmt = detect_format(path, content);
        bool success = false;

        switch (fmt) {
            case FileFormat::HoDoKuSolution:
                success = parse_hsol(content, studio, outError);
                break;
            case FileFormat::SimpleSudoku:
                success = parse_simple_sudoku(content, studio, outError);
                break;
            case FileFormat::Sdk:
            case FileFormat::PlainText:
            default:
                success = parse_sdk_or_text(content, studio, outError);
                break;
        }

        if (success) {
            studio.set_current_file_path(path);
        }
        return success;
    }

    static bool save_file(const std::wstring& path, const HoDoKuStudio& studio, FileFormat format, std::string& outError) {
        std::ofstream file(path.c_str(), std::ios::binary);
        if (!file.is_open()) {
            outError = "Failed to open file for writing.";
            return false;
        }

        if (format == FileFormat::Auto) {
            format = detect_format(path, "");
        }

        std::string payload;
        switch (format) {
            case FileFormat::SimpleSudoku:
                payload = export_simple_sudoku(studio);
                break;
            case FileFormat::HoDoKuSolution:
                payload = export_hsol(studio);
                break;
            case FileFormat::Sdk:
            case FileFormat::PlainText:
            default:
                payload = export_sdk(studio);
                break;
        }

        file << payload;
        file.close();
        const_cast<HoDoKuStudio&>(studio).set_current_file_path(path);
        return true;
    }

    static std::string export_sdk(const HoDoKuStudio& studio) {
        return studio.export_givens_string() + "\r\n";
    }

    static std::string export_simple_sudoku(const HoDoKuStudio& studio) {
        std::ostringstream oss;
        const auto& board = studio.get_board();

        // 1. 9x9 ASCII table of givens
        oss << "*-----------*\r\n";
        for (int r = 0; r < 9; ++r) {
            oss << "|";
            for (int c = 0; c < 9; ++c) {
                int cell = cell_index(r, c);
                if (board.is_given(cell)) {
                    oss << static_cast<int>(board.get_value(cell));
                } else {
                    oss << ".";
                }
                if (c == 2 || c == 5) oss << "|";
            }
            oss << "|\r\n";
            if (r == 2 || r == 5) oss << "|---+---+---|\r\n";
        }
        oss << "*-----------*\r\n\r\n";

        // 2. User filled cells: I<index:02d><digit>
        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            if (board.get_value(cell) != 0 && !board.is_given(cell)) {
                oss << "I" << std::setw(2) << std::setfill('0') << cell << static_cast<int>(board.get_value(cell)) << "\r\n";
            }
        }

        // 3. Eliminated candidates: E<index:02d><candidate:03d>
        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            if (board.is_unfilled(cell)) {
                core::CandidateMask mask = board.get_candidates(cell);
                for (int d = 1; d <= 9; ++d) {
                    if (!mask_has_digit(mask, d)) {
                        oss << "E" << std::setw(2) << std::setfill('0') << cell << std::setw(3) << std::setfill('0') << d << "\r\n";
                    }
                }
            }
        }

        return oss.str();
    }

    static std::string export_hsol(const HoDoKuStudio& studio) {
        std::ostringstream oss;
        const auto& board = studio.get_board();

        oss << "[HODOKU_SOLUTION_V2]\r\n";
        oss << "GIVENS=" << studio.export_givens_string() << "\r\n";
        oss << "CURRENT=" << board.to_string() << "\r\n";

        // Non-given filled cells
        oss << "FILLED=";
        bool first = true;
        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            if (board.get_value(cell) != 0 && !board.is_given(cell)) {
                if (!first) oss << ";";
                oss << cell << ":" << static_cast<int>(board.get_value(cell));
                first = false;
            }
        }
        oss << "\r\n";

        // Eliminated candidates
        oss << "ELIMINATIONS=";
        first = true;
        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            if (board.is_unfilled(cell)) {
                core::CandidateMask mask = board.get_candidates(cell);
                for (int d = 1; d <= 9; ++d) {
                    if (!mask_has_digit(mask, d)) {
                        if (!first) oss << ";";
                        oss << cell << ":" << d;
                        first = false;
                    }
                }
            }
        }
        oss << "\r\n";

        // Cell Colors
        oss << "CELL_COLORS=";
        first = true;
        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            int col = studio.get_cell_color(cell);
            if (col > 0) {
                if (!first) oss << ";";
                oss << cell << ":" << col;
                first = false;
            }
        }
        oss << "\r\n";

        // Candidate Colors
        oss << "CAND_COLORS=";
        first = true;
        for (int cell = 0; cell < TOTAL_CELLS; ++cell) {
            for (int d = 1; d <= 9; ++d) {
                int8_t col = studio.get_candidate_color(cell, d);
                if (col >= 0) {
                    if (!first) oss << ";";
                    oss << cell << ":" << d << ":" << static_cast<int>(col);
                    first = false;
                }
            }
        }
        oss << "\r\n";

        // User Manual Inference Links (Plan 6.3)
        const auto& links = studio.get_user_links();
        oss << "USER_LINKS=";
        first = true;
        for (const auto& link : links) {
            if (!first) oss << ";";
            oss << link.from_cell << "," << link.from_digit << ","
                << link.to_cell << "," << link.to_digit << ","
                << (link.is_strong ? 1 : 0) << ","
                << static_cast<int>(link.color_index);
            first = false;
        }
        oss << "\r\n";

        // Solution Path Steps Count
        const auto& path = studio.get_solution_path();
        oss << "STEPS_COUNT=" << path.size() << "\r\n";

        return oss.str();
    }

private:
    static bool parse_sdk_or_text(const std::string& content, HoDoKuStudio& studio, std::string& outError) {
        std::string digits;
        digits.reserve(TOTAL_CELLS);
        for (char ch : content) {
            if (ch >= '1' && ch <= '9') {
                digits.push_back(ch);
            } else if (ch == '.' || ch == '0') {
                digits.push_back('.');
            }
            if (digits.size() == TOTAL_CELLS) break;
        }

        if (digits.size() < TOTAL_CELLS) {
            outError = "File does not contain 81 valid Sudoku digits or blank markers.";
            return false;
        }

        studio.import_from_string(digits);
        return true;
    }

    static bool parse_simple_sudoku(const std::string& content, HoDoKuStudio& studio, std::string& outError) {
        std::istringstream iss(content);
        std::string line;
        std::string gridText;

        // 1. Read header grid up to blank line or 'I' / 'E'
        while (std::getline(iss, line)) {
            std::string trimmed = line;
            trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
            trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

            if (trimmed.empty() || trimmed.starts_with("I") || trimmed.starts_with("E")) {
                if (!gridText.empty()) break;
                continue;
            }
            gridText += line + "\n";
        }

        if (!parse_sdk_or_text(gridText, studio, outError)) {
            return false;
        }

        // 2. Read 'I' and 'E' records
        do {
            std::string trimmed = line;
            trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
            trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

            if (trimmed.length() >= 4 && trimmed[0] == 'I') {
                int cell = std::stoi(trimmed.substr(1, 2));
                int val = trimmed[3] - '0';
                studio.set_cell_digit(cell, val);
            } else if (trimmed.length() >= 4 && trimmed[0] == 'E') {
                int cell = std::stoi(trimmed.substr(1, 2));
                int cand = std::stoi(trimmed.substr(3));
                if (cand >= 1 && cand <= 9) {
                    if (studio.get_board().has_candidate(cell, cand)) {
                        studio.toggle_cell_candidate(cell, cand);
                    }
                }
            }
        } while (std::getline(iss, line));

        return true;
    }

    static bool parse_hsol(const std::string& content, HoDoKuStudio& studio, std::string& outError) {
        // If it's HoDoKu native key-value format:
        if (content.find("[HODOKU_SOLUTION") != std::string::npos) {
            std::istringstream iss(content);
            std::string line;
            std::string givens;
            std::string filled;
            std::string elims;
            std::string cellColors;
            std::string candColors;
            std::string userLinks;

            while (std::getline(iss, line)) {
                if (line.starts_with("GIVENS=")) {
                    givens = line.substr(7);
                    givens.erase(givens.find_last_not_of(" \t\r\n") + 1);
                } else if (line.starts_with("FILLED=")) {
                    filled = line.substr(7);
                } else if (line.starts_with("ELIMINATIONS=")) {
                    elims = line.substr(13);
                } else if (line.starts_with("CELL_COLORS=")) {
                    cellColors = line.substr(12);
                } else if (line.starts_with("CAND_COLORS=")) {
                    candColors = line.substr(12);
                } else if (line.starts_with("USER_LINKS=")) {
                    userLinks = line.substr(11);
                }
            }

            if (givens.empty()) {
                outError = "Invalid HoDoKu solution file: missing GIVENS.";
                return false;
            }

            studio.import_from_string(givens);

            // Apply filled cells
            if (!filled.empty()) {
                std::istringstream fss(filled);
                std::string item;
                while (std::getline(fss, item, ';')) {
                    size_t colon = item.find(':');
                    if (colon != std::string::npos) {
                        int cell = std::stoi(item.substr(0, colon));
                        int val = std::stoi(item.substr(colon + 1));
                        studio.set_cell_digit(cell, val);
                    }
                }
            }

            // Apply eliminations
            if (!elims.empty()) {
                std::istringstream ess(elims);
                std::string item;
                while (std::getline(ess, item, ';')) {
                    size_t colon = item.find(':');
                    if (colon != std::string::npos) {
                        int cell = std::stoi(item.substr(0, colon));
                        int cand = std::stoi(item.substr(colon + 1));
                        if (studio.get_board().has_candidate(cell, cand)) {
                            studio.toggle_cell_candidate(cell, cand);
                        }
                    }
                }
            }

            // Apply cell colors
            if (!cellColors.empty()) {
                std::istringstream css(cellColors);
                std::string item;
                while (std::getline(css, item, ';')) {
                    size_t colon = item.find(':');
                    if (colon != std::string::npos) {
                        int cell = std::stoi(item.substr(0, colon));
                        int col = std::stoi(item.substr(colon + 1));
                        studio.set_cell_color(cell, col);
                    }
                }
            }

            // Apply candidate colors
            if (!candColors.empty()) {
                std::istringstream cdss(candColors);
                std::string item;
                while (std::getline(cdss, item, ';')) {
                    size_t c1 = item.find(':');
                    size_t c2 = (c1 != std::string::npos) ? item.find(':', c1 + 1) : std::string::npos;
                    if (c1 != std::string::npos && c2 != std::string::npos) {
                        int cell = std::stoi(item.substr(0, c1));
                        int digit = std::stoi(item.substr(c1 + 1, c2 - c1 - 1));
                        int col = std::stoi(item.substr(c2 + 1));
                        studio.set_candidate_color(cell, digit, static_cast<int8_t>(col));
                    }
                }
            }

            // Apply user manual links
            studio.clear_user_links();
            if (!userLinks.empty()) {
                std::istringstream lss(userLinks);
                std::string item;
                while (std::getline(lss, item, ';')) {
                    if (item.empty()) continue;
                    std::istringstream issLink(item);
                    std::string fCellStr, fDigitStr, tCellStr, tDigitStr, strongStr, colStr;
                    if (std::getline(issLink, fCellStr, ',') &&
                        std::getline(issLink, fDigitStr, ',') &&
                        std::getline(issLink, tCellStr, ',') &&
                        std::getline(issLink, tDigitStr, ',') &&
                        std::getline(issLink, strongStr, ',')) {
                        ManualLink link;
                        link.from_cell = std::stoi(fCellStr);
                        link.from_digit = std::stoi(fDigitStr);
                        link.to_cell = std::stoi(tCellStr);
                        link.to_digit = std::stoi(tDigitStr);
                        link.is_strong = (std::stoi(strongStr) != 0);
                        if (std::getline(issLink, colStr, ',')) {
                            link.color_index = static_cast<int8_t>(std::stoi(colStr));
                        } else {
                            link.color_index = COLOR_NONE;
                        }
                        studio.add_user_link(link);
                    }
                }
            }

            return true;
        }

        // If it's an original HoDoKu Java XML file, extract 81-character puzzle string
        return parse_sdk_or_text(content, studio, outError);
    }
};

} // namespace hodoku::ui
