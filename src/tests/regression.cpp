#include <iostream>

#include "make_undo_test.h"
#include <string_view>
#include "pawn_table_tests.h"
#include "perft_tests.h"
#include "tactical_tests.h"
#include "zobrist.h"

int main(int argc, char* argv[]) {
    Zobrist::initialize_keys();

    if (argc == 2 && std::string_view(argv[1]) == "pawn-table") {
        return run_pawn_table_tests();
    }

    int failed_suites = 0;

    std::cout << "=== Pawn-table regression ===\n";
    if (run_pawn_table_tests() != 0) {
        ++failed_suites;
    }

    std::cout << "=== Make/undo regression ===\n";
    if (run_make_undo_tests() != 0) {
        ++failed_suites;
    }

    std::cout << "=== Perft regression ===\n";
    if (run_perft_tests() != 0) {
        ++failed_suites;
    }

    std::cout << "=== Tactical search regression ===\n";
    if (run_tactical_tests() != 0) {
        ++failed_suites;
    }

    if (failed_suites != 0) {
        std::cerr << "Regression failed: " << failed_suites
                  << " suite(s) failed\n";
        return 1;
    }

    std::cout << "All regression suites passed\n";
    return 0;
}
