#include "thread_pool.h"

#include <algorithm>

#include "engine.h"

SearchThreadPool::SearchThreadPool(Engine& engine) : engine_(engine) {}

void SearchThreadPool::set_thread_count(int count) {
    count = std::max(1, count);
    if (count == thread_count_) return;
    stop();
    start(count);
}

void SearchThreadPool::start(int count) {
    uint64_t initial_job_id;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        thread_count_ = std::max(1, count);
        terminate_ = false;
        active_workers_ = 0;
        job_.thread_count = 1;
        initial_job_id = job_id_;
    }

    engine_.tls_data.clear_counters();
    workers_.clear();
    workers_.reserve(static_cast<size_t>(thread_count_ - 1));
    for (int thread_id = 1; thread_id < thread_count_; ++thread_id) {
        workers_.emplace_back(
            [this, thread_id, initial_job_id] { worker_loop(thread_id, initial_job_id); });
    }
}

void SearchThreadPool::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        terminate_ = true;
        ++job_id_;
    }
    start_cv_.notify_all();

    for (std::thread& worker : workers_) {
        if (worker.joinable()) worker.join();
    }
    workers_.clear();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        terminate_ = false;
        thread_count_ = 1;
        active_workers_ = 0;
        job_.thread_count = 1;
    }
    engine_.tls_data.clear_counters();
}

void SearchThreadPool::execute(const Position& position, const SearchLimits& limits,
    int participating_threads, TimeControlDecision& time_control,
    Move& best_move, int& best_score, ThreadLocalData* master_data) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        engine_.stop_search.store(false, std::memory_order_relaxed);
        job_.position = position;
        job_.limits = limits;
        job_.thread_count = participating_threads;
        active_workers_ = std::max(0, participating_threads - 1);
        ++job_id_;
    }

    start_cv_.notify_all();
    engine_.iterative_deepening_new(
        0, true, best_move, best_score, position, time_control, master_data);
    engine_.stop_search.store(true, std::memory_order_relaxed);

    if (participating_threads > 1) {
        std::unique_lock<std::mutex> lock(mutex_);
        done_cv_.wait(lock, [this] { return active_workers_ == 0; });
    }
}

void SearchThreadPool::worker_loop(int thread_id, uint64_t initial_job_id) {
    uint64_t seen_job = initial_job_id;
    Move local_best;
    int local_score = 0;
    ThreadLocalData tls_data;

    while (true) {
        Position position;
        SearchLimits limits;
        uint64_t assigned_job = 0;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            start_cv_.wait(lock, [this, &seen_job] {
                return terminate_ || job_id_ != seen_job;
            });
            if (terminate_) return;

            seen_job = job_id_;
            assigned_job = job_id_;
            if (thread_id >= job_.thread_count) continue;
            position = job_.position;
            limits = job_.limits;
        }

        tls_data.clear_counters();
        Move best_move = local_best;
        int best_score = local_score;
        TimeControlDecision time_control = decide_time_control(position, limits);
        engine_.iterative_deepening_new(
            thread_id, false, best_move, best_score, position, time_control, &tls_data);
        tls_data.flush_counters(&engine_, true);
        local_best = best_move;
        local_score = best_score;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (assigned_job == job_id_ && active_workers_ > 0) {
                --active_workers_;
                if (active_workers_ == 0) done_cv_.notify_one();
            }
        }
    }
}
