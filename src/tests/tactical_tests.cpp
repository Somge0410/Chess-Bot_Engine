#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include "Default_positions.h"
#include "engine.h"
#include "position.h"
#include "search.h"

namespace {
constexpr int TACTICAL_MOVETIME_MS = 1'500;
constexpr std::size_t TACTICAL_TT_SIZE_MB = 16;

class ScopedCoutCapture {
public:
    ScopedCoutCapture() : previous_buffer_(std::cout.rdbuf(output_.rdbuf())) {}

    ~ScopedCoutCapture() {
        std::cout.rdbuf(previous_buffer_);
    }

    std::string str() const {
        return output_.str();
    }

private:
    std::ostringstream output_;
    std::streambuf* previous_buffer_;
};

struct SearchResultSummary {
    std::string last_best_move;
    int first_match_depth = 0;
};

SearchResultSummary summarize_search_output(std::string_view output,
    std::string_view expected_move) {
    SearchResultSummary summary;
    std::istringstream lines{std::string(output)};
    std::string line;
    while (std::getline(lines, line)) {
        std::istringstream tokens(line);
        std::string token;
        int depth = 0;
        std::string pv_move;
        while (tokens >> token) {
            if (token == "depth") {
                tokens >> depth;
            }
            else if (token == "pv") {
                tokens >> pv_move;
                break;
            }
        }

        if (pv_move.empty()) {
            continue;
        }
        summary.last_best_move = pv_move;
        if (summary.first_match_depth == 0 && pv_move == expected_move) {
            summary.first_match_depth = depth;
        }
    }
    return summary;
}
}

int run_tactical_tests() {
    std::size_t found_count = 0;
    std::size_t failure_count = 0;
    std::size_t total_found_depth = 0;

    std::cout << "Tactical search: " << tactical_defaults.size()
              << " positions, " << TACTICAL_MOVETIME_MS << " ms each\n";

    for (const DefaultPosition& reference : tactical_defaults) {
        Position position{std::string(reference.fen)};
        Engine engine(TACTICAL_TT_SIZE_MB);
        engine.set_threads(1);

        SearchLimits limits;
        limits.movetime = TACTICAL_MOVETIME_MS;

        std::string search_output;
        {
            ScopedCoutCapture capture;
            engine.search(position, limits);
            search_output = capture.str();
        }

        const SearchResultSummary result = summarize_search_output(
            search_output, reference.best_move);
        engine.shutdown();

        if (result.first_match_depth != 0) {
            ++found_count;
            total_found_depth += static_cast<std::size_t>(result.first_match_depth);
            std::cout << "[PASS] " << reference.name << " bestmove "
                      << reference.best_move << " found at depth "
                      << result.first_match_depth << '\n';
        }
        else {
            ++failure_count;
            std::cout << "[FAIL] " << reference.name << " bestmove "
                      << reference.best_move << " not found in "
                      << TACTICAL_MOVETIME_MS << " ms (last move: "
                      << (result.last_best_move.empty() ? "none" : result.last_best_move)
                      << ")\n";
        }
    }

    const std::size_t total_positions = tactical_defaults.size();
    const double found_percentage = total_positions == 0
        ? 0.0
        : 100.0 * static_cast<double>(found_count) / static_cast<double>(total_positions);
    std::cout << "Tactical result: " << found_count << '/' << total_positions
              << " best moves found (" << found_percentage << "%)";
    if (found_count != 0) {
        std::cout << "; average first-found depth: "
                  << static_cast<double>(total_found_depth)
                        / static_cast<double>(found_count)
                  << " plies";
    }
    std::cout << '\n';

    if (failure_count != 0) {
        std::cerr << "Tactical search regression failed: " << failure_count
                  << " position(s) did not find the reference move\n";
        return 1;
    }

    return 0;
}
