#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#if ENABLE_QSEARCH_DIAGNOSTICS
struct SearchDiagnostics;
void print_search_diagnostics_summary(const SearchDiagnostics& diagnostics,
    const std::string& line_prefix = "",
    bool tt_snapshot_is_final = false);
#endif

int run_benchmark(
    const std::vector<std::pair<std::string, std::string>>& positions,
    bool tt_bench = false,
    int depth = 12,
    std::size_t tt_size_mb = 128);
int run_benchmark_game(
    int movetime_ms = 200,
    std::size_t tt_size_mb = 16,
    int max_moves = 30);
