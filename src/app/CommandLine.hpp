#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <windows.h>
#include "../core/Types.hpp"
#include "../core/BoardState.hpp"
#include "../core/StepFinder.hpp"
#include "../core/Generator.hpp"
#include "../core/DlxSolver.hpp"

namespace hodoku::ui {

class CommandLine {
public:
    static bool process_command_line(int argc, char* argv[]) {
        if (argc <= 1) return false; // No arguments, proceed to launch GUI

        // Attach to the calling parent console
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            FILE* fp;
            freopen_s(&fp, "CONOUT$", "w", stdout);
            freopen_s(&fp, "CONOUT$", "w", stderr);
            freopen_s(&fp, "CONIN$", "r", stdin);
            std::ios::sync_with_stdio();
        }

        std::string mode;
        std::string inputFile;
        std::string outputFile;
        int count = 1;
        int diffInt = 0;
        std::string symStr = "180";

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-h" || arg == "--help" || arg == "/?") {
                print_help();
                return true;
            } else if (arg == "-b" && i + 1 < argc) {
                mode = "batch";
                inputFile = argv[++i];
            } else if (arg == "-bs" && i + 1 < argc) {
                mode = "batch_steps";
                inputFile = argv[++i];
            } else if (arg == "-sc" && i + 1 < argc) {
                mode = "solve_check";
                inputFile = argv[++i];
            } else if (arg == "-g") {
                mode = "generate";
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    count = std::max(1, std::atoi(argv[++i]));
                }
            } else if (arg == "-d" && i + 1 < argc) {
                diffInt = std::atoi(argv[++i]);
            } else if (arg == "-s" && i + 1 < argc) {
                symStr = argv[++i];
            } else if (arg == "-o" && i + 1 < argc) {
                outputFile = argv[++i];
            } else if (arg.length() == 81) {
                // Direct single-puzzle solve
                mode = "solve_single";
                inputFile = arg;
            }
        }

        if (mode.empty()) {
            print_help();
            return true;
        }

        if (mode == "batch") {
            run_batch_solve(inputFile, false);
        } else if (mode == "batch_steps") {
            run_batch_solve(inputFile, true);
        } else if (mode == "solve_check") {
            run_solve_check(inputFile);
        } else if (mode == "generate") {
            run_generate(count, diffInt, symStr, outputFile);
        } else if (mode == "solve_single") {
            run_single_solve(inputFile);
        }

        std::cout << std::flush;
        return true;
    }

private:
    static void print_help() {
        std::cout << "\n=======================================================\n";
        std::cout << "  HoDoKu 2.2 - Native C++20 High-Performance Edition   \n";
        std::cout << "=======================================================\n";
        std::cout << "Usage: hodoku_native.exe [options] [puzzle]\n\n";
        std::cout << "Options:\n";
        std::cout << "  -b <file>                   Batch solve puzzles from file and output scores\n";
        std::cout << "  -bs <file>                  Batch solve with full step-by-step solution paths\n";
        std::cout << "  -g [count] -d <lvl> -s <sym> Batch generate puzzles to stdout or file\n";
        std::cout << "                                -d <lvl>: 0=Easy, 1=Medium, 2=Hard, 3=Unfair, 4=Extreme\n";
        std::cout << "                                -s <sym>: 180, 90, diag, anti, full, orth\n";
        std::cout << "  -sc <file>                  Solve check: verify puzzle validity & uniqueness (DLX)\n";
        std::cout << "  -o <file>                   Output destination file for generated puzzles\n";
        std::cout << "  -h, --help                  Display this help message\n\n";
        std::cout << "Examples:\n";
        std::cout << "  hodoku_native.exe -b puzzles.txt\n";
        std::cout << "  hodoku_native.exe -g 10 -d 2 -s 180 -o hard_puzzles.txt\n";
        std::cout << "  hodoku_native.exe 530070000600195000098000060800060003400803001700020006060000280000419005000080079\n\n";
    }

    static void run_batch_solve(const std::string& path, bool printSteps) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Error: Unable to open puzzle file: " << path << "\n";
            return;
        }

        std::cout << "\nStarting Batch Solver for file: " << path << "\n";
        std::cout << "-------------------------------------------------------\n";

        std::string line;
        int index = 0;
        int solvedCount = 0;
        long long totalUs = 0;
        int totalScore = 0;

        while (std::getline(file, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
                line.pop_back();
            }
            if (line.empty() || line[0] == '#') continue;

            index++;
            core::BoardState board(line);
            auto t0 = std::chrono::high_resolution_clock::now();

            int score = 0;
            core::DifficultyLevel hardest = core::DifficultyLevel::Easy;
            std::vector<core::Step> pathSteps;

            core::BoardState sim = board;
            while (!sim.is_solved()) {
                auto nextStep = core::StepFinder::find_next_step(sim);
                if (!nextStep) break;

                score += nextStep->score;
                if (nextStep->difficulty > hardest) hardest = nextStep->difficulty;
                pathSteps.push_back(*nextStep);

                for (const auto& a : nextStep->assignments) sim.set_value(a.cell, a.digit);
                for (const auto& e : nextStep->eliminations) sim.remove_candidate(e.cell, e.digit);
            }

            auto t1 = std::chrono::high_resolution_clock::now();
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
            totalUs += us;
            totalScore += score;

            bool solved = sim.is_solved();
            if (solved) solvedCount++;

            std::cout << "Puzzle #" << index << ": " << (solved ? "SOLVED" : "INCOMPLETE")
                      << " | Difficulty: " << core::difficulty_name(hardest)
                      << " | Score: " << score
                      << " | Steps: " << pathSteps.size()
                      << " | Time: " << us << " us\n";

            if (printSteps) {
                for (size_t si = 0; si < pathSteps.size(); ++si) {
                    std::cout << "   " << (si + 1) << ". " << pathSteps[si].name << ": "
                              << pathSteps[si].explanation << "\n";
                }
                std::cout << "\n";
            }
        }

        std::cout << "-------------------------------------------------------\n";
        std::cout << "Batch Summary: " << solvedCount << "/" << index << " puzzles solved.\n";
        if (index > 0) {
            std::cout << "Average Score: " << (totalScore / index) << "\n";
            std::cout << "Average Solve Time: " << (totalUs / index) << " us ("
                      << (totalUs / 1000.0 / index) << " ms)\n";
        }
    }

    static void run_solve_check(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Error: Unable to open file: " << path << "\n";
            return;
        }

        std::cout << "\nStarting DLX Validity & Uniqueness Check: " << path << "\n";
        std::cout << "-------------------------------------------------------\n";

        core::DlxSolver dlx;
        std::string line;
        int index = 0;
        int uniqueCount = 0;
        int multiCount = 0;
        int invalidCount = 0;

        while (std::getline(file, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
                line.pop_back();
            }
            if (line.empty() || line[0] == '#') continue;

            index++;
            core::BoardState board(line);
            int solCount = dlx.count_solutions(board, 2);

            if (solCount == 1) {
                uniqueCount++;
                std::cout << "Puzzle #" << index << ": UNIQUE (1 solution)\n";
            } else if (solCount > 1) {
                multiCount++;
                std::cout << "Puzzle #" << index << ": MULTI-SOLUTION (>1 solution)\n";
            } else {
                invalidCount++;
                std::cout << "Puzzle #" << index << ": INVALID (0 solutions)\n";
            }
        }

        std::cout << "-------------------------------------------------------\n";
        std::cout << "Check Summary: Total: " << index
                  << " | Unique: " << uniqueCount
                  << " | Multi: " << multiCount
                  << " | Invalid: " << invalidCount << "\n";
    }

    static void run_generate(int count, int diffInt, const std::string& symStr, const std::string& outPath) {
        core::DifficultyLevel diff = static_cast<core::DifficultyLevel>(std::clamp(diffInt, 0, 4));
        core::SymmetryType sym = core::SymmetryType::Rotational180;
        if (symStr == "90") sym = core::SymmetryType::Rotational90;
        else if (symStr == "diag") sym = core::SymmetryType::Diagonal;
        else if (symStr == "anti") sym = core::SymmetryType::AntiDiagonal;
        else if (symStr == "horiz") sym = core::SymmetryType::Horizontal;
        else if (symStr == "vert") sym = core::SymmetryType::Vertical;
        else if (symStr == "none") sym = core::SymmetryType::None;

        std::cout << "\nGenerating " << count << " " << core::difficulty_name(diff) << " puzzle(s)...\n";

        core::SudokuGenerator gen(static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count()));
        std::vector<std::string> results;

        for (int i = 0; i < count; ++i) {
            core::BoardState puz = gen.generate_puzzle(diff, sym, 8);
            std::string puzStr = puz.to_string();
            results.push_back(puzStr);
            std::cout << " [" << (i + 1) << "] " << puzStr << " (" << puz.get_givens().count() << " clues)\n";
        }

        if (!outPath.empty()) {
            std::ofstream out(outPath);
            if (out.is_open()) {
                for (const auto& p : results) out << p << "\n";
                std::cout << "\nPuzzles successfully saved to: " << outPath << "\n";
            } else {
                std::cerr << "Error writing output file: " << outPath << "\n";
            }
        }
    }

    static void run_single_solve(const std::string& puzStr) {
        core::BoardState board(puzStr);
        std::cout << "\nSolving Single Puzzle:\n" << puzStr << "\n\n";

        core::BoardState sim = board;
        int stepIdx = 0;
        int totalScore = 0;
        core::DifficultyLevel hardest = core::DifficultyLevel::Easy;

        while (!sim.is_solved()) {
            auto next = core::StepFinder::find_next_step(sim);
            if (!next) {
                std::cout << "Solver reached a deadlock / requires brute force.\n";
                break;
            }
            stepIdx++;
            totalScore += next->score;
            if (next->difficulty > hardest) hardest = next->difficulty;

            std::cout << "Step #" << stepIdx << " [" << core::difficulty_name(next->difficulty) << "] "
                      << next->name << " (" << next->score << " pts): " << next->explanation << "\n";

            for (const auto& a : next->assignments) sim.set_value(a.cell, a.digit);
            for (const auto& e : next->eliminations) sim.remove_candidate(e.cell, e.digit);
        }

        if (sim.is_solved()) {
            std::cout << "\nPuzzle Solved!\n";
            std::cout << "Difficulty: " << core::difficulty_name(hardest)
                      << " | Total Score: " << totalScore
                      << " | Total Steps: " << stepIdx << "\n";
            std::cout << "Result: " << sim.to_string() << "\n";
        }
    }
};

} // namespace hodoku::ui
