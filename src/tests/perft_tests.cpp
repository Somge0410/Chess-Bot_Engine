#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Default_positions.h"
#include "perft.h"
#include "perft_tests.h"
#include "position.h"

namespace {
constexpr std::uint64_t NODE_LIMIT = 200'000'000ULL;

// The halfmove and fullmove counters do not affect legal move generation.
// Using only the first four FEN fields deduplicates positions that differ only
// in those counters.
std::string perft_position_key(std::string_view fen) {
    std::istringstream input{std::string(fen)};
    std::string key;
    std::string field;
    for (int field_index = 0; field_index < 4; ++field_index) {
        if (!(input >> field)) {
            return std::string(fen);
        }
        if (!key.empty()) {
            key += ' ';
        }
        key += field;
    }
    return key;
}
}

int run_perft_tests() {
    const std::array<std::reference_wrapper<const std::vector<DefaultPosition>>, 4>
        position_groups = {
            std::cref(tactical_defaults),
            std::cref(perft_defaults),
            std::cref(bench_defaults),
            std::cref(full_bench_defaults),
        };

    std::vector<const DefaultPosition*> unique_positions;
    std::unordered_map<std::string, const DefaultPosition*> positions_by_key;
    std::size_t duplicate_count = 0;
    std::size_t failure_count = 0;

    for (const auto& group_reference : position_groups) {
        for (const DefaultPosition& position : group_reference.get()) {
            const std::string key = perft_position_key(position.fen);
            const auto [existing, inserted] = positions_by_key.emplace(key, &position);
            if (inserted) {
                unique_positions.push_back(&position);
                continue;
            }

            ++duplicate_count;
            if (existing->second->expected_perft_counts
                != position.expected_perft_counts) {
                std::cerr << "[FAIL] conflicting perft references for "
                          << existing->second->name << " and " << position.name
                          << '\n';
                ++failure_count;
            }
        }
    }

    std::size_t test_count = 0;
    std::uint64_t expected_node_total = 0;
    for (const DefaultPosition* position : unique_positions) {
        for (std::uint64_t expected : position->expected_perft_counts) {
            if (expected < NODE_LIMIT) {
                ++test_count;
                expected_node_total += expected;
            }
        }
    }

    std::cout << "Perft regression: " << unique_positions.size()
              << " unique positions, " << duplicate_count
              << " duplicates removed, " << test_count
              << " depth checks below " << NODE_LIMIT << " nodes"
              << " (approximately " << expected_node_total
              << " generated nodes total)\n";

    std::size_t completed_count = 0;
    for (const DefaultPosition* reference : unique_positions) {
        for (std::size_t index = 0;
             index < reference->expected_perft_counts.size(); ++index) {
            const std::uint64_t expected = reference->expected_perft_counts[index];
            if (expected >= NODE_LIMIT) {
                continue;
            }

            const int depth = static_cast<int>(index + 1);
            Position position(reference->fen);
            const auto start = std::chrono::steady_clock::now();
            const std::uint64_t actual = perft(position, depth);
            const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
            ++completed_count;

            if (actual != expected) {
                std::cerr << "[FAIL] " << reference->name
                          << " depth " << depth
                          << ": expected " << expected
                          << ", got " << actual << '\n';
                ++failure_count;
            }
            else {
                std::cout << "[PASS] " << reference->name
                          << " depth " << depth
                          << " nodes " << actual
                          << " time " << elapsed_ms << " ms\n";
            }
        }
    }

    if (failure_count != 0) {
        std::cerr << "Perft regression failed: " << failure_count
                  << " failure(s) across " << completed_count
                  << " completed checks\n";
        return 1;
    }

    std::cout << "Perft regression passed: " << completed_count
              << " checks\n";
    return 0;
}
