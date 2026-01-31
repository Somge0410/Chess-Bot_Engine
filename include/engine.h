#pragma once
#include "board.h"
#include "Move.h"
#include <vector>
#include <map>
#include <exception>
#include <chrono>
#include <utility>
#include "MoveGenerator.h"
#include "constants.h"
#include <atomic>
#include <mutex>
enum TTFlag {
    EXACT,
    LOWERBOUND,
    UPPERBOUND,
    TEMPERED,
};
struct TTEntry {
    alignas(8) uint64_t entry;

    TTEntry() : entry(uint64_t(0xFF << 16)) {};
    bool empty() const {
		return ((entry >> 16) &0xFFull) == 0xFFull;
    }
    TTEntry(int16_t score, uint8_t depth, TTFlag bound, uint8_t generation, const Move& move, uint16_t key) {
        // Optional debug checks

        entry =
            (uint64_t(score) & 0xFFFFull)
            | (uint64_t(depth) & 0xFFull) << 16
            | (uint64_t(bound) & 0x3ull) << 24
            | (uint64_t(generation) & 0x3Full) << 26
            | (uint64_t(move.get_int()) & 0xFFFFull) << 32
            | (uint64_t(key) & 0xFFFFull) << 48;
    }
	TTEntry(uint64_t raw_entry) : entry(raw_entry) {}
    int16_t score() const {
        return static_cast<int16_t>(entry & 0xFFFFull);
    }
    uint16_t depth() const {
        return static_cast<uint8_t>((entry>>16) & 0xFFull);
    }
    TTFlag flag() const {
        return static_cast<TTFlag>((entry >> 24) & 0x3ull);
    }
    int8_t generation() const {
        return static_cast<uint8_t>((entry>>26)& 0x3Full);
    }
    uint16_t move_packed() const {
        return static_cast<uint16_t>( (entry>>32) & 0xFFFFull);
    }
    Move move() const {
        return recover_move_from_int(move_packed());
    }
    uint16_t key() const {
        return static_cast<uint16_t>((entry>> 48) & 0xFFFFull);
    }

};

struct TTCluster {
	TTEntry entries[4];
};
inline uint64_t tt_load(TTEntry& entry) {
    return std::atomic_ref<uint64_t>(entry.entry).load(std::memory_order_relaxed);
}
inline void tt_store(TTEntry& entry, uint64_t value) {
    std::atomic_ref<uint64_t>(entry.entry).store(value, std::memory_order_relaxed);
}
struct PerftRes {
    double duration;
    uint64_t nodes;
};
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
struct TimeControlDecision {
    int time_ms;
    int max_depth;
};
enum class TTMode {Negamax, Quiescence};
struct SearchResult {
    int score;
    Move best_move;
    bool is_tempered=false;
};
class Engine;
struct ThreadLocalData {
    void clear_counters() {
        nodes = 0;
        qnodes = 0;
    }
    void clear_heuristics(){
        std::memset(killer_moves, 0, sizeof(killer_moves));
        std::memset(history_scores, 0, sizeof(history_scores));
    }

    Move killer_moves[128][2] = {};
    int history_scores[2][6][64] = {};
    std::atomic<uint64_t> nodes{ 0 };
    std::atomic<uint64_t> qnodes{ 0 };
    void flush_counters(Engine* engine);
};
class Engine {
    public:
		Engine(size_t tt_size_mb = MAX_MEMORY_TT_MB);
        void set_threads(int n);
        Move search(const Board& board, const SearchLimits& limits);
        PerftRes perft_test(Board& board, int depth);
        int checks_count;
        int ep_count;
        int capture_count;
        int checkmate_count;
        std::atomic<uint64_t> nodes{ 0 };
        std::atomic<uint64_t>  qnodes{ 0 };
        // --- Transposition table statistics ---
        std::atomic<uint64_t>  tt_probes_nm{ 0 };
        std::atomic<uint64_t>  tt_hits_nm{ 0 };
        std::atomic<uint64_t>  tt_misses_nm{ 0 };
        std::atomic<uint64_t>  tt_stores_nm{ 0 };
        std::atomic<uint64_t>  tt_overwrites_nm{ 0 };
        std::atomic<uint64_t>  tt_updates_nm{ 0 };   // same key updated
        std::atomic<uint64_t>  tt_skip_same_gen_nm{ 0 };
        std::atomic<uint64_t>  tt_tempered_flag_found{ 0 };;
        std::atomic<uint64_t>  tt_temp_not_exact{ 0 };
        std::atomic<uint64_t>  tt_temp_flag_stored{ 0 };
        std::atomic<uint64_t>  tt_filled_slots{ 0 }; // count non-empty entries at the end
        std::atomic<uint64_t>  tt_probes_qs{ 0 };
        std::atomic<uint64_t>  tt_hits_qs{ 0 };
        std::atomic<uint64_t>  tt_misses_qs{ 0 };
        std::atomic<uint64_t>  tt_stores_qs{ 0 };
        std::atomic<uint64_t>  tt_updates_qs{ 0 };
        std::atomic<uint64_t>  tt_overwrites_qs{ 0 };
        std::atomic<uint64_t>  tt_skip_same_gen_qs{ 0 };
        uint64_t generation=0;
        std::atomic<uint64_t>  redo_window_search{ 0 };
        std::atomic<uint64_t>  could_result_in_rep{ 0 };
        std::atomic<uint64_t>  have_to_make_move_count{ 0 };
        std::atomic<uint64_t>  cutoff_count{ 0 };
        std::atomic<uint64_t>  moves_before_cutoff{ 0 };
        void compute_tt_fill_rate() {
            tt_filled_slots = 0;
            for (TTCluster& c : tt) {
                for (int i = 0; i < 4; i++) {
                    if (!TTEntry(tt_load(c.entries[i])).empty()) tt_filled_slots++;
                }
            }
        }


        Move search_new(const Board& position, const SearchLimits& limits);


    private:
        std::atomic<bool> stop_search{ false };
        void start_thread_pool(int n);
		void stop_thread_pool();

        void worker_loop(int thread_id);
        std::vector<std::thread> workers;
		int thread_count = 1;
        std::mutex pool_mtx;
		std::condition_variable cv_start;
		std::condition_variable cv_done;

        bool terminate_pool = false;

        uint64_t job_id = 0;
        int active_workers = 0;

        Board job_position;
		SearchLimits job_limits;

        SearchResult negamax(Board & board, int depth, int alpha, int beta, int ply,ThreadLocalData* tls);
        int quiescence_search(Board& board, int alpha, int beta, int ply, ThreadLocalData* tls);
        Move best_move_this_iteration;
        std::vector<TTCluster> tt;/*
        Move killer_moves[128][2];
        int history_scores[2][6][64]={};*/
        std::chrono::steady_clock::time_point start_time;
        std::chrono::duration<double> time_limit;
        void sort_moves(MoveList& moves, const Board& board, int ply,const Move& tt_move, bool tt_depth_0 = false,ThreadLocalData* tls={});
        int score_move(const Move& move, int ply,const Move& tt_move, bool depth_0,const Board& board,ThreadLocalData* tls);
        uint64_t perft_driver(Board& board, int depth, int orignal_depth);
        TimeControlDecision decide_time_control(const Board& position, const SearchLimits& limits);
        bool probe_tt(uint64_t hash, int depth, int alpha, int beta, int& out_score, Move& out_move, bool is_depth_0 = false, TTMode mode = TTMode::Negamax);
        bool store_tt(uint64_t hash, int depth, int original_alpha, int beta, int best_score, Move& best_move,bool is_best_tempered,bool is_any_tempered = false, TTMode mode = TTMode::Negamax);
		bool should_futility_prune(int depth, int eval, int alpha, bool in_check,const Move& move);
		int late_move_reduction(int depth, int moves_searched, const Move& move, int ply, ThreadLocalData* tls);
		bool try_null_move_pruning(Board& board,bool is_in_check, int depth, int alpha, int beta, int ply, int& out_score,ThreadLocalData* tls);
		SearchResult terminal_eval(const Board& board, bool king_is_in_check);
		void update_history_killer(const Move& move, int depth, int ply,ThreadLocalData* tls);
        void init_tt(size_t tt_size_mb = MAX_MEMORY_TT_MB);
        bool move_could_result_in_repetition(Board& board, Move& move, int count=3);
        void recover_move_fully(Move& move,const Board& board);
        void score_moves(const MoveList& moves, int* scores, 
		int ply, const Move& tt_move, bool depth_0,const Board& board, ThreadLocalData* tls);
        void score_quiet_moves(const MoveList& moves, int* scores,const Board& board,bool evade_check);
		int relevant_pawn_push(const Board& board, const Move& move);
		void iterative_deepening_new(int thread_id, bool is_master,Move& out_best_move ,int& io_best_score,const Board& board, const TimeControlDecision& tc,ThreadLocalData* tls);
		void perturb_root_order(MoveList& moves, int thread_id, int current_depth, uint64_t hash);
        void root_pvs(const Board& pos,
            MoveList& root_moves,
            int current_depth,
            int alpha,
            int beta,
            int& out_best_score,
            Move& out_best_move);

};
inline uint8_t dist_mod64_fast(uint8_t a, uint8_t b) {
    uint8_t d = (a - b) & 63;          // in 0..63 (mod 64)
    return (d <= 32) ? d : (64 - d);   // shortest way around
}