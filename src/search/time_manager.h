#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>

#include "search.h"

class Position;

struct TimeControlDecision {
    int time_ms;
    int max_depth;
    int max_time_ms;
};

TimeControlDecision decide_time_control(const Position& position, const SearchLimits& limits);

class TimeManager {
public:
    void start(int total_time_ms);
    void set_budget_ms(int total_time_ms);
    bool is_time_up() const;
    int64_t remaining_ms(int64_t current_time_ns) const;
    std::chrono::steady_clock::time_point start_time() const { return start_time_; }
    static int64_t now_ns();

private:
    std::chrono::steady_clock::time_point start_time_{};
    std::atomic<int64_t> search_start_ns_{ 0 };
    std::atomic<int64_t> search_deadline_ns_{ std::numeric_limits<int64_t>::max() };
};
