#pragma once

struct SearchLimits {
    int depth = -1;
    int movetime = -1;
    int wtime = -1;
    int btime = -1;
    int winc = 0;
    int binc = 0;
    int nodes = -1;
    int mate = -1;
    bool infinite = false;
};
struct ThreadLocalData {
    static constexpr uint32_t TIME_CHECK_INTERVAL = 1024;
    static constexpr int QSEARCH_PLY_CAPACITY = 25;

    void clear_counters() {
        nodes = 0;
        qnodes = 0;
#if ENABLE_QSEARCH_DIAGNOSTICS
        qply_sum = 0;
        quiet_checks_searched = 0;
        qnodes_in_check = 0;
        cycle_cutoffs = 0;
        hard_cap_hits = 0;
        max_qply = 0;
        move_order_nodes = 0;
        moves_searched_sum = 0;
        best_move_index_sum = 0;
        best_move_first = 0;
        beta_cutoffs = 0;
        beta_cutoff_index_sum = 0;
        first_move_beta_cutoffs = 0;
        max_best_move_index = 0;
        detail_diagnostics.fill(0);
        for (TTDiagnostics& diagnostics : tt_diagnostics) {
            diagnostics = {};
        }
#endif
        nodes_until_time_check = TIME_CHECK_INTERVAL;
    }
    void clear_heuristics() {
        std::memset(killer_moves, 0, sizeof(killer_moves));
        std::memset(history_scores, 0, sizeof(history_scores));
        std::memset(counter_moves, 0, sizeof(counter_moves));
    }
    bool should_check_time() {
        if (--nodes_until_time_check != 0) {
            return false;
        }
        nodes_until_time_check = TIME_CHECK_INTERVAL;
        return true;
    }

    MoveList move_lists[MAX_PLY];
    MoveList qmove_lists[QSEARCH_PLY_CAPACITY];
    uint64_t qsearch_hashes[QSEARCH_PLY_CAPACITY] = {};
    int move_scores[MAX_PLY][256] = {};
    Move killer_moves[128][2] = {};
    MoveList searched_quiets[MAX_PLY];
    int history_scores[2][6][64] = {};
    Move counter_moves[2][7][64] = {};
    uint64_t nodes{ 0 };
    uint64_t qnodes{ 0 };
#if ENABLE_QSEARCH_DIAGNOSTICS
    uint64_t qply_sum{ 0 };
    uint64_t quiet_checks_searched{ 0 };
    uint64_t qnodes_in_check{ 0 };
    uint64_t cycle_cutoffs{ 0 };
    uint64_t hard_cap_hits{ 0 };
    uint32_t max_qply{ 0 };
    uint64_t move_order_nodes{ 0 };
    uint64_t moves_searched_sum{ 0 };
    uint64_t best_move_index_sum{ 0 };
    uint64_t best_move_first{ 0 };
    uint64_t beta_cutoffs{ 0 };
    uint64_t beta_cutoff_index_sum{ 0 };
    uint64_t first_move_beta_cutoffs{ 0 };
    uint32_t max_best_move_index{ 0 };
    std::array<TTDiagnostics, TT_DIAGNOSTIC_MODE_COUNT> tt_diagnostics{};
    std::array<uint64_t, SEARCH_DIAG_COUNTER_COUNT> detail_diagnostics{};
    bool last_tt_probe_was_shallow{ false };
    bool current_tt_probe_in_check{ false };
#endif
    uint32_t nodes_until_time_check{ TIME_CHECK_INTERVAL };
    void flush_counters(Engine* engine, bool force = false);
};
SearchLimits job_limits;
