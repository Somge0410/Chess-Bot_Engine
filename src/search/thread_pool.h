#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

#include "position.h"
#include "search.h"
#include "time_manager.h"

class Engine;

struct SearchJob {
    Position position;
    SearchLimits limits;
    int thread_count = 1;
};

class SearchThreadPool {
public:
    explicit SearchThreadPool(Engine& engine);
    SearchThreadPool(const SearchThreadPool&) = delete;
    SearchThreadPool& operator=(const SearchThreadPool&) = delete;

    void set_thread_count(int count);
    void start(int count);
    void stop();
    void execute(const Position& position, const SearchLimits& limits,
        int participating_threads, TimeControlDecision& time_control,
        Move& best_move, int& best_score, ThreadLocalData* master_data);
    int thread_count() const { return thread_count_; }

private:
    void worker_loop(int thread_id, uint64_t initial_job_id);

    Engine& engine_;
    std::vector<std::thread> workers_;
    int thread_count_ = 1;
    std::mutex mutex_;
    std::condition_variable start_cv_;
    std::condition_variable done_cv_;
    bool terminate_ = false;
    uint64_t job_id_ = 0;
    int active_workers_ = 0;
    SearchJob job_;
};
