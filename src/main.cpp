#include <chrono>
#include <iostream>
#include <vector>

#include "Squares.h"
#include "board.h"
#include "constants.h"
#include "evaluation.h"
#include "engine.h"
#include "prepare_data.h"
#include "see.h"
#include "uci.h"
#include "uci_helpers.h"
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

static int run_main_bench() {
    constexpr int bench_depth = 12;
    const auto positions = get_default_positions();

    SearchLimits limits;
    limits.depth = bench_depth;

    uint64_t total_nodes = 0;
    uint64_t total_time_ms = 0;

    std::cout << "info string bench start positions " << positions.size()
              << " depth " << bench_depth << "\n";
    std::cout.flush();

    for (const auto& [name, fen] : positions) {
        Board board(fen);

        // Neuer Engine pro Position => neues TT + frische Heuristiken
        Engine engine(128);

        const auto start = std::chrono::steady_clock::now();
        const Move best = engine.search(board, limits);
        const auto end = std::chrono::steady_clock::now();

        const uint64_t elapsed_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

        const uint64_t nodes = engine.get_total_nodes();

        total_nodes += nodes;
        total_time_ms += elapsed_ms;

        std::cout << "info string bench pos " << name
                  << " bestmove " << move_to_uci(best)
                  << " time " << elapsed_ms
                  << " nodes " << 1 << "\n";
        std::cout.flush();

        engine.shutdown();
    }

    const uint64_t nps = (total_time_ms > 0) ? (total_nodes * 1000ULL) / total_time_ms : 0;

    std::cout << "info string bench total positions " << positions.size()
              << " depth " << bench_depth
              << " time " << total_time_ms
              << " nodes " << 1
              << " nps " << nps << "\n";
    std::cout.flush();
    std::cout << "Nodes searched: 1\n";
    std::cout << "Nodes/second: " << nps << '\n';
    std::cout.flush();
    return 0;
}

int main(int argc, char* argv[]) {
    Zobrist::initialize_keys();

    if (argc >= 2 && std::string(argv[1]) == "bench") {
        return run_main_bench();
    }

    Board board("k7/8/4P3/8/8/8/8/K3b3 w - - 0 1");
    std::cout << "Evaluating board: " << evaluate(board) << std::endl;
    trace_eval_agree(board, EvalWeights);
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    uci_loop();
    return 0;
}