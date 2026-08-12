#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

int run_benchmark(
    const std::vector<std::pair<std::string, std::string>>& positions,
    int depth = 12,
    std::size_t tt_size_mb = 128);
