#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <limits>
#include <string>

#include "constants.h"
#include "position.h"
#include "search.h"
#include "search_diagnostics.h"
#include "thread_pool.h"
#include "time_manager.h"
#include "transposition_table.h"

class Engine {
public:
    explicit Engine(size_t tt_size_mb = MAX_MEMORY_TT_MB);
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    Move search(const Position& position, const SearchLimits& limits);
    void stop_search_and_wait() { stop_search.store(true, std::memory_order_release); }
    void set_threads(int count);
    void resize_tt(size_t tt_size_mb);
    void shutdown();
    void flush_node_counters();
    uint64_t get_total_nodes();
    uint64_t get_qnodes();
#if ENABLE_QSEARCH_DIAGNOSTICS
    SearchDiagnostics get_search_diagnostics(bool include_tt_occupancy = true);
#endif

private:
    friend struct ThreadLocalData;
    friend class SearchThreadPool;

    static constexpr uint64_t CHECKERS_UNKNOWN = std::numeric_limits<uint64_t>::max();
    static constexpr int LMR_DEPTH_COUNT = 64;
    static constexpr int LMR_MOVE_COUNT = 218;

    SearchResult negamax(Position& pos, int depth, int alpha, int beta, int ply,
        ThreadLocalData* tls, const Move& previous_move = Move(),
        uint64_t checkers = CHECKERS_UNKNOWN, bool null_move_allowed = true);
    int quiescence_search(Position& pos, int alpha, int beta, int search_ply,
        int qply, ThreadLocalData* tls, uint64_t checkers = CHECKERS_UNKNOWN,
        bool after_check_invasion = false);
    bool should_futility_prune(int depth, int eval, int alpha, bool in_check,
        const Move& move);
    int late_move_reduction(int depth, int moves_searched, const Move& move,
        int ply, ThreadLocalData* tls, const Move& previous_move);
    void initialize_lmr_tables();
    bool try_null_move_pruning(Position& pos, bool is_in_check, int depth,
        int alpha, int beta, int ply, int static_eval, int& out_score,
        ThreadLocalData* tls);
    SearchResult terminal_eval(const Position& pos, bool king_is_in_check, int ply);
    bool move_could_result_in_repetition(Position& pos, Move& move, int count = 3);
    void recover_move_fully(Move& move, const Position& pos);
    void iterative_deepening_new(int thread_id, bool is_master, Move& out_best_move,
        int& io_best_score, const Position& pos, TimeControlDecision& time_control,
        ThreadLocalData* tls);
    void root_pvs(const Position& pos, MoveList& root_moves, int current_depth,
        int alpha, int beta, int& out_best_score, Move& out_best_move,
        int& out_second_best_score, Move& second_best_move, ThreadLocalData* tls);
    std::string create_pv_string(const Position& pos, const Move& best_move, int depth);

    std::atomic<bool> stop_search{ false };
    SearchThreadPool thread_pool;
    TranspositionTable tt;
    TimeManager time_manager;
    std::atomic<uint64_t> nodes{ 0 };
    std::atomic<uint64_t> qnodes{ 0 };
#if ENABLE_QSEARCH_DIAGNOSTICS
    std::atomic<uint64_t> qply_sum{ 0 };
    std::atomic<uint64_t> quiet_checks_searched{ 0 };
    std::atomic<uint64_t> qnodes_in_check{ 0 };
    std::atomic<uint64_t> cycle_cutoffs{ 0 };
    std::atomic<uint64_t> hard_cap_hits{ 0 };
    std::atomic<uint32_t> max_qply{ 0 };
    std::atomic<uint64_t> move_order_nodes{ 0 };
    std::atomic<uint64_t> moves_searched_sum{ 0 };
    std::atomic<uint64_t> best_move_index_sum{ 0 };
    std::atomic<uint64_t> best_move_first{ 0 };
    std::atomic<uint64_t> beta_cutoffs{ 0 };
    std::atomic<uint64_t> beta_cutoff_index_sum{ 0 };
    std::atomic<uint64_t> first_move_beta_cutoffs{ 0 };
    std::atomic<uint32_t> max_best_move_index{ 0 };
    std::array<AtomicTTDiagnostics, TT_DIAGNOSTIC_MODE_COUNT> tt_diagnostics{};
    std::array<std::atomic<uint64_t>, SEARCH_DIAG_COUNTER_COUNT> detail_diagnostics{};
    std::array<uint64_t, DIAGNOSTIC_ITERATION_DEPTH_COUNT> diagnostic_iteration_nodes{};
    std::array<uint64_t, DIAGNOSTIC_ITERATION_DEPTH_COUNT> diagnostic_iteration_time_ms{};
#endif
    std::array<std::array<uint8_t, LMR_MOVE_COUNT>, LMR_DEPTH_COUNT> quiet_lmr{};
    std::array<std::array<uint8_t, LMR_MOVE_COUNT>, LMR_DEPTH_COUNT> tactical_lmr{};
};
