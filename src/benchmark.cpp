#include "benchmark.h"

#include <chrono>
#include <cstdint>
#include <iostream>

#include "board.h"
#include "engine.h"
#include "uci_helpers.h"

int run_benchmark(
    const std::vector<std::pair<std::string, std::string>>& positions,
    int depth,
    std::size_t tt_size_mb) {
    SearchLimits limits;
    limits.depth = depth;

    uint64_t total_nodes = 0;
    uint64_t total_qnodes = 0;
    uint64_t total_time_ms = 0;

    std::cout << "info string bench start positions " << positions.size()
              << " depth " << depth << "\n";
    std::cout.flush();

    for (const auto& [name, fen] : positions) {
        Board board(fen);
        Engine engine(tt_size_mb);

        const auto start = std::chrono::steady_clock::now();
        const Move best = engine.search(board, limits);
        const auto end = std::chrono::steady_clock::now();

        const uint64_t elapsed_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
        const uint64_t nodes = engine.get_total_nodes();
		const uint64_t qnodes = engine.get_qnodes();
        total_nodes += nodes;
		total_qnodes += qnodes;
        total_time_ms += elapsed_ms;

        std::cout << "info string bench pos " << name
                  << " bestmove " << move_to_uci(best)
                  << " time " << elapsed_ms
                  << " nodes " << nodes << "\n";
        std::cout.flush();

        engine.shutdown();
    }

    const uint64_t nps = total_time_ms > 0
        ? (total_nodes * 1000ULL) / total_time_ms
        : 0;

    std::cout << "info string bench total positions " << positions.size()
              << " depth " << depth
              << " time " << total_time_ms
              << " nodes " << total_nodes
              << " nps " << nps << "\n";
    std::cout << "Nodes searched: " << total_nodes << "\n";
    std::cout << "Nodes/second: " << nps << '\n';
	std::cout << "q-nodes percentage " << (total_qnodes*100)/total_nodes << "%\n";
    std::cout.flush();
    return 0;
}
