#include <iostream>
#include <cassert>
#include <string>
#include <filesystem>
#include "app/StudioModel.hpp"
#include "app/FileManager.hpp"

using namespace hodoku;
using namespace hodoku::ui;

int main() {
    std::cout << "[TEST] FileManager formats (.sdk, .ss, .hsol)...\n";

    HoDoKuStudio studio;
    std::string sampleGivens = "38.4.6...9..2..7...4.3.......2...6...9.7.1...5...3.......8.4...7..9..2...6.1.58";
    studio.import_from_string(sampleGivens);

    std::filesystem::create_directories("test_tmp");
    std::wstring tmpSdk = L"test_tmp/test_puzzle.sdk";
    std::wstring tmpSs = L"test_tmp/test_puzzle.ss";
    std::wstring tmpHsol = L"test_tmp/test_puzzle.hsol";

    std::string err;

    // 1. Test SDK export and reload
    bool ok = FileManager::save_file(tmpSdk, studio, FileFormat::Sdk, err);
    assert(ok);
    HoDoKuStudio studioSdk;
    ok = FileManager::load_file(tmpSdk, studioSdk, err);
    assert(ok);
    assert(studioSdk.export_givens_string() == studio.export_givens_string());
    std::cout << "  -> SDK format save/load: PASSED\n";

    // 2. Test Simple Sudoku (SS) export and reload
    ok = FileManager::save_file(tmpSs, studio, FileFormat::SimpleSudoku, err);
    assert(ok);
    HoDoKuStudio studioSs;
    ok = FileManager::load_file(tmpSs, studioSs, err);
    assert(ok);
    assert(studioSs.export_givens_string() == studio.export_givens_string());
    std::cout << "  -> Simple Sudoku (SS) format save/load: PASSED\n";

    // 3. Test HoDoKu Solution (HSOL) export and reload
    studio.set_cell_color(0, 3);
    studio.set_candidate_color(5, 7, 2);
    studio.add_user_link(ManualLink{0, 1, 1, 2, true, 4});
    studio.add_user_link(ManualLink{10, 4, 20, 8, false, 7});

    ok = FileManager::save_file(tmpHsol, studio, FileFormat::HoDoKuSolution, err);
    assert(ok);
    HoDoKuStudio studioHsol;
    ok = FileManager::load_file(tmpHsol, studioHsol, err);
    assert(ok);
    assert(studioHsol.export_givens_string() == studio.export_givens_string());
    assert(studioHsol.get_cell_color(0) == 3);
    assert(studioHsol.get_candidate_color(5, 7) == 2);
    assert(studioHsol.get_user_links().size() == 2);
    assert(studioHsol.get_user_links()[0].from_cell == 0 && studioHsol.get_user_links()[0].from_digit == 1);
    assert(studioHsol.get_user_links()[0].to_cell == 1 && studioHsol.get_user_links()[0].to_digit == 2);
    assert(studioHsol.get_user_links()[0].is_strong == true);
    assert(studioHsol.get_user_links()[0].color_index == 4);
    assert(studioHsol.get_user_links()[1].from_cell == 10 && studioHsol.get_user_links()[1].from_digit == 4);
    assert(studioHsol.get_user_links()[1].to_cell == 20 && studioHsol.get_user_links()[1].to_digit == 8);
    assert(studioHsol.get_user_links()[1].is_strong == false);
    assert(studioHsol.get_user_links()[1].color_index == 7);
    std::cout << "  -> HoDoKu Solution (HSOL) format save/load with custom colored links: PASSED\n";

    // Clean up temporary files
    std::filesystem::remove_all("test_tmp");

    std::cout << "ALL File I/O Tests PASSED!\n";
    return 0;
}

