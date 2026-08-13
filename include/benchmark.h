#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#if ENABLE_QSEARCH_DIAGNOSTICS
struct SearchDiagnostics;
void print_search_diagnostics_summary(const SearchDiagnostics& diagnostics,
    const std::string& line_prefix = "");
#endif

int run_benchmark(
    const std::vector<std::pair<std::string, std::string>>& positions,
    int depth = 12,
    std::size_t tt_size_mb = 128);
