#include "benchmark.h"

#include <chrono>
#include <algorithm>
#include <cstdint>
#include <iomanip>
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
#if ENABLE_QSEARCH_DIAGNOSTICS
    SearchDiagnostics total_diagnostics{};
#endif
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
#if ENABLE_QSEARCH_DIAGNOSTICS
        const SearchDiagnostics diagnostics = engine.get_search_diagnostics();
        total_diagnostics.main_nodes += diagnostics.main_nodes;
        total_diagnostics.qnodes += diagnostics.qnodes;
        total_diagnostics.qply_sum += diagnostics.qply_sum;
        total_diagnostics.quiet_checks_searched += diagnostics.quiet_checks_searched;
        total_diagnostics.qnodes_in_check += diagnostics.qnodes_in_check;
        total_diagnostics.cycle_cutoffs += diagnostics.cycle_cutoffs;
        total_diagnostics.hard_cap_hits += diagnostics.hard_cap_hits;
        total_diagnostics.max_qply = std::max(total_diagnostics.max_qply, diagnostics.max_qply);
#endif
        total_time_ms += elapsed_ms;

#if ENABLE_QSEARCH_DIAGNOSTICS
        const double qnode_percentage = nodes > 0
            ? 100.0 * static_cast<double>(diagnostics.qnodes) / static_cast<double>(nodes)
            : 0.0;
        const double average_qply = diagnostics.qnodes > 0
            ? static_cast<double>(diagnostics.qply_sum) / static_cast<double>(diagnostics.qnodes)
            : 0.0;
#endif

        std::cout << "info string bench pos " << name
                  << " bestmove " << move_to_uci(best)
                  << " time " << elapsed_ms
                  << " nodes " << nodes;
#if ENABLE_QSEARCH_DIAGNOSTICS
        std::cout
                  << " mainnodes " << diagnostics.main_nodes
                  << " qnodes " << diagnostics.qnodes
                  << " qpercent " << std::fixed << std::setprecision(2) << qnode_percentage
                  << " maxqply " << diagnostics.max_qply
                  << " avgqply " << average_qply
                  << " quietchecks " << diagnostics.quiet_checks_searched
                  << " inchecknodes " << diagnostics.qnodes_in_check
                  << " cyclecutoffs " << diagnostics.cycle_cutoffs
                  << " hardcaphits " << diagnostics.hard_cap_hits;
#endif
        std::cout << "\n";
        std::cout.flush();

        engine.shutdown();
    }

    const uint64_t nps = total_time_ms > 0
        ? (total_nodes * 1000ULL) / total_time_ms
        : 0;
#if ENABLE_QSEARCH_DIAGNOSTICS
    const double qnode_percentage = total_nodes > 0
        ? 100.0 * static_cast<double>(total_diagnostics.qnodes) / static_cast<double>(total_nodes)
        : 0.0;
    const double average_qply = total_diagnostics.qnodes > 0
        ? static_cast<double>(total_diagnostics.qply_sum) / static_cast<double>(total_diagnostics.qnodes)
        : 0.0;
#endif

    std::cout << "info string bench total positions " << positions.size()
              << " depth " << depth
              << " time " << total_time_ms
              << " nodes " << total_nodes
              << " nps " << nps << "\n";
    std::cout << "Nodes searched: " << total_nodes << "\n";
    std::cout << "Nodes/second: " << nps << '\n';
#if ENABLE_QSEARCH_DIAGNOSTICS
    std::cout << "Main nodes: " << total_diagnostics.main_nodes << '\n';
    std::cout << "QNodes: " << total_diagnostics.qnodes << '\n';
    std::cout << "QNode percentage: " << std::fixed << std::setprecision(2)
              << qnode_percentage << "%\n";
    std::cout << "Maximum qply: " << total_diagnostics.max_qply << '\n';
    std::cout << "Average qply: " << average_qply << '\n';
    std::cout << "Quiet checks searched: " << total_diagnostics.quiet_checks_searched << '\n';
    std::cout << "QNodes while in check: " << total_diagnostics.qnodes_in_check << '\n';
    std::cout << "Cycle cutoffs: " << total_diagnostics.cycle_cutoffs << '\n';
    std::cout << "Hard-cap hits: " << total_diagnostics.hard_cap_hits << '\n';
#endif
    std::cout.flush();
    return 0;
}
