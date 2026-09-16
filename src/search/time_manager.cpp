#include "time_manager.h"

#include <algorithm>
#include <chrono>

#include "position.h"
#include "search_parameters.h"

double interpolate_phase(double phase, double endgame_value, double midgame_value, double opening_value) {
    phase = std::clamp(phase, 0.0, 24.0);
    if (phase <= 12) {
        const double t = phase / 12.0;
		return endgame_value + t * (midgame_value - endgame_value);
    }

	const double t = (phase - 12.0) / 12.0;
	return midgame_value + t * (opening_value - midgame_value);
}
TimeControlDecision decide_time_control(const Position& position, const SearchLimits& limits) {
    TimeControlDecision tc{};
    tc.max_depth = limits.depth > 0 ? limits.depth : INFINITE_DEPTH;
    if (limits.movetime > 0) {
        tc.time_ms = limits.movetime;
		tc.max_time_ms = limits.movetime;
    }
    else if (limits.wtime > 0 || limits.btime > 0) {
		const bool white_to_move = position.get_turn() == Color::White;

        const int time_left = (position.get_turn() == Color::White) ? limits.wtime : limits.btime;
        const int inc = (position.get_turn() == Color::White) ? limits.winc : limits.binc;

        const int usable_time = std::max(1, time_left - MOVE_OVERHEAD_MS);

        const double phase = position.get_game_phase();

		double moves_to_go = interpolate_phase(phase, MOVES_TO_GO_EG, MOVES_TO_GO_MG, MOVES_TO_GO);

		const int moves_after_threshold = std::max(0, position.get_move_count() - MOVE_COUNT_THRESHOLD);

        moves_to_go -= std::min(MAX_MOVE_COUNT_REDUCTION,MOVE_COUNT_WEIGHT * moves_after_threshold);
		// SPSA updates related options through separate setoption commands. Keep the
		// effective interval valid even while, or if, the two endpoints cross.
		const double min_moves_to_go = std::min(MIN_MOVES_TO_GO, MAX_MOVES_TO_GO);
		const double max_moves_to_go = std::max(MIN_MOVES_TO_GO, MAX_MOVES_TO_GO);
		moves_to_go = std::clamp(moves_to_go, min_moves_to_go, max_moves_to_go);

        const double increment_contribution = inc * INC_USAGE_FACTOR;
        
        const double base_time = usable_time / moves_to_go + increment_contribution;

        //Estimate the clock resources avalable over the expected remainng number of moves

        const double effective_time = usable_time + inc * moves_to_go;

        const double time_scale = std::clamp(effective_time / REFERENCE_TIME, 0.0, 1.0);

        double max_multiplier = MAX_MULTIPLIER_FAST + time_scale * (MAX_MULTIPLIER_SLOW - MAX_MULTIPLIER_FAST);
        max_multiplier=std::max(1.0,max_multiplier);

        tc.time_ms = std::clamp(static_cast<int>(base_time), 1, usable_time);
		tc.max_time_ms = std::clamp(static_cast<int>(base_time * max_multiplier), tc.time_ms, usable_time);

    }
    else if (limits.depth > 0) {
        tc.time_ms = INFINITE_TIME_MS;
        tc.max_depth = limits.depth;
    }
    else if (limits.infinite) {
        tc.time_ms = INFINITE_TIME_MS;
    }
    else {
        tc.time_ms = DEFAULT_TIME_MS;
    }
    return tc;
}

int64_t TimeManager::now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void TimeManager::start(int total_time_ms) {
    start_time_ = std::chrono::steady_clock::now();
    const int64_t start_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        start_time_.time_since_epoch()).count();
    search_start_ns_.store(start_ns, std::memory_order_release);
    search_deadline_ns_.store(
        start_ns + static_cast<int64_t>(total_time_ms) * 1000000LL,
        std::memory_order_release);
}

void TimeManager::set_budget_ms(int total_time_ms) {
    const int64_t start_ns = search_start_ns_.load(std::memory_order_acquire);
    search_deadline_ns_.store(
        start_ns + static_cast<int64_t>(total_time_ms) * 1000000LL,
        std::memory_order_release);
}

bool TimeManager::is_time_up() const {
    return now_ns() >= search_deadline_ns_.load(std::memory_order_acquire);
}

int64_t TimeManager::remaining_ms(int64_t current_time_ns) const {
    const int64_t deadline_ns = search_deadline_ns_.load(std::memory_order_acquire);
    return std::max<int64_t>(0, (deadline_ns - current_time_ns) / 1000000LL);
}
