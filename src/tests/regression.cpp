#include <iostream>

#include "make_undo_test.h"
#include "perft_tests.h"
#include "zobrist.h"

int main() {
    Zobrist::initialize_keys();

    int failed_suites = 0;

    std::cout << "=== Make/undo regression ===\n";
    if (run_make_undo_tests() != 0) {
        ++failed_suites;
    }

    std::cout << "=== Perft regression ===\n";
    if (run_perft_tests() != 0) {
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
 