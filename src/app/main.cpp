#include <chrono>
#include <iostream>
#include <vector>

#include "Squares.h"
#include "benchmark.h"
#include "board.h"
#include "constants.h"
#include "evaluation.h"
#include "prepare_data.h"
#include "see.h"
#include "uci.h"
#include "utils.h"
#include "zobrist.h"
#include "attack_rays.h"
#include "bitboard_masks.h"
#include "MoveGenerator.h"
#include "notation_utils.h"
#include "rook_tables.h"
#include "bishop_tables.h"

void print_2d_array(const std::string& name, const std::vector<std::vector<uint64_t>>& arr) {
    if (arr.empty() || arr[0].empty())
    {
        std::cout << "const uint64_t " << name << "[0][0] = {};" << std::endl;
        return;
    }
    size_t rows = arr.size();
    size_t cols = arr[0].size();

    std::cout << "const uint64_t " << name << "[" << rows << "][" << cols << "] = {" << std::endl;
    for (size_t i = 0; i < rows; ++i)
    {
        std::cout << "    { ";
        for (size_t j = 0; j < cols; ++j)
        {
            std::cout << "0x" << std::hex << arr[i][j] << "ULL," << (j % 8 == 7 ? "\n      " : " ");
        }
        std::cout << "}," << std::endl;
    }
    std::cout << "};" << std::endl << std::endl;
}

int main(int argc, char* argv[]) {
    Zobrist::initialize_keys();

    if (argc >= 2 && std::string(argv[1]) == "bench") {
        return run_benchmark(get_bench_positions());
    }
    if (argc >= 2 && std::string(argv[1]) == "bench_full") {
        return run_benchmark(get_full_bench_positions());
    }
    if (argc >= 2 && std::string(argv[1]) == "bench_tt") {
        return run_benchmark(get_full_bench_positions(), true);
    }
    if (argc >= 2 && std::string(argv[1]) == "bench_game") {
        return run_benchmark_game();
    }

    Board board("k7/8/4P3/8/8/8/8/K3b3 w - - 0 1");
    std::cout << "Evaluating board: " << evaluate(board) << std::endl;
    trace_eval_agree(board, EvalWeights);
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    uci_loop();
    return 0;
}
