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
#include <thread>
#include <cstring>
#include <condition_variable>
#include <limits>
#include <array>

#ifndef ENABLE_QSEARCH_DIAGNOSTICS
#define ENABLE_QSEARCH_DIAGNOSTICS 0
#endif

enum TTFlag {
    EXACT,
    LOWERBOUND,
    UPPERBOUND,
    TEMPERED,
};
struct TTEntry {
    alignas(8) uint64_t entry;
    static constexpr uint64_t GENERATION_MASK = 0x3Full << 26;

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
    uint8_t generation() const {
        return static_cast<uint8_t>((entry>>26)& 0x3Full);
    }
    uint64_t with_generation(uint8_t new_generation) const {
        return (entry & ~GENERATION_MASK)
            | ((uint64_t(new_generation) & 0x3Full) << 26);
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

constexpr int score_to_tt(int score, int ply) {
    if (score >= MATE_THRESHOLD) return score + ply;
    if (score <= -MATE_THRESHOLD) return score - ply;
    return score;
}

constexpr int score_from_tt(int score, int ply) {
    if (score >= MATE_THRESHOLD) return score - ply;
    if (score <= -MATE_THRESHOLD) return score + ply;
    return score;
}

static_assert(score_to_tt(MATE_SCORE - 7, 7) == MATE_SCORE);
static_assert(score_from_tt(MATE_SCORE, 3) == MATE_SCORE - 3);
static_assert(score_to_tt(-MATE_SCORE + 7, 7) == -MATE_SCORE);
static_assert(score_from_tt(-MATE_SCORE, 3) == -MATE_SCORE + 3);

struct alignas(32) TTCluster {
	TTEntry entries[4];
};
static_assert(sizeof(TTCluster) == 32);
static_assert(alignof(TTCluster) == 32);
inline uint64_t tt_load(TTEntry& entry) {
    return std::atomic_ref<uint64_t>(entry.entry).load(std::memory_order_relaxed);
}
inline void tt_store(TTEntry& entry, uint64_t value) {
    std::atomic_ref<uint64_t>(entry.entry).store(value, std::memory_order_relaxed);
}
inline void tt_refresh_generation(TTEntry& entry, uint64_t observed, uint8_t generation) {
    TTEntry current(observed);
    const uint64_t refreshed = current.with_generation(generation);
    if (refreshed == observed) return;

    std::atomic_ref<uint64_t>(entry.entry).compare_exchange_strong(
        observed, refreshed, std::memory_order_relaxed, std::memory_order_relaxed);
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
    int max_time_ms;
};
enum class TTMode {Negamax, Quiescence, PrincipalVariation};
struct SearchResult {
    int score;
    Move best_move;
    bool is_tempered=false;
};
#if ENABLE_QSEARCH_DIAGNOSTICS
constexpr std::size_t TT_DIAGNOSTIC_MODE_COUNT = 2;
constexpr std::size_t TT_PROBE_CATEGORY_COUNT = 5;
constexpr std::size_t MOVE_ORDER_SOURCE_COUNT = 7;
constexpr std::size_t MOVE_INDEX_BUCKET_COUNT = 7;
constexpr std::size_t QPLY_BUCKET_COUNT = 7;
constexpr std::size_t TT_CLUSTER_OCCUPANCY_BUCKET_COUNT = 5;
constexpr std::size_t DIAGNOSTIC_ITERATION_DEPTH_COUNT = 64;

enum class MoveOrderSource : std::size_t {
    TT,
    WinningCapture,
    Promotion,
    Killer,
    Countermove,
    History,
    LosingCapture,
    Unknown
};

enum class SearchDiagCounter : std::size_t {
    RfpAttempts,
    RfpCutoffs,
    NmpCandidates,
    NmpSearches,
    NmpCutoffs,
    IirCandidates,
    IirReductions,
    IirPvReductions,
    IirNonPvReductions,
    IirInCheckSkips,
    FutilityChecks,
    FutilityPrunes,
    LmrReductions,
    LmrResearches,
    LmrResearchImproved,
    PvsZeroWindowSearches,
    PvsFullWindowResearches,
    CheckExtensions,

    BestSourceTT,
    BestSourceWinningCapture,
    BestSourcePromotion,
    BestSourceKiller,
    BestSourceCountermove,
    BestSourceHistory,
    BestSourceLosingCapture,
    CutoffSourceTT,
    CutoffSourceWinningCapture,
    CutoffSourcePromotion,
    CutoffSourceKiller,
    CutoffSourceCountermove,
    CutoffSourceHistory,
    CutoffSourceLosingCapture,

    BestIndex1,
    BestIndex2,
    BestIndex3To4,
    BestIndex5To8,
    BestIndex9To16,
    BestIndex17To32,
    BestIndex33Plus,
    CutoffIndex1,
    CutoffIndex2,
    CutoffIndex3To4,
    CutoffIndex5To8,
    CutoffIndex9To16,
    CutoffIndex17To32,
    CutoffIndex33Plus,

    QPly0,
    QPly1,
    QPly2,
    QPly3To4,
    QPly5To8,
    QPly9To12,
    QPly13Plus,
    QStandPatCutoffs,
    QSoftCapStaticReturns,
    QHardCapStaticReturns,
    QHardCapDrawReturns,
    QFiftyMoveDraws,
    QRepetitionDraws,
    QCheckmates,
    QNoTacticalMoves,
    QSeePrunes,
    QCaptureBetaCutoffs,
    QQuietCheckBetaCutoffs,
    QPromotionBetaCutoffs,
    QEvasionBetaCutoffs,
    QMovesGenerated,
    QMovesSearched,

    TTRepetitionRejectedReturns,
    ShallowTTMoveFirst,
    ShallowTTMoveBest,
    ShallowTTMoveCutoff,

    MainMovesGenerated,
    MainMovesSearched,
    PvNodes,
    PvMovesGenerated,
    PvMovesSearched,
    NonPvNodes,
    NonPvMovesGenerated,
    NonPvMovesSearched,
    InCheckNodes,
    InCheckMovesGenerated,
    InCheckMovesSearched,
    SearchFiftyMoveDraws,
    SearchRepetitionDraws,
    TerminalCheckmates,
    TerminalStalemates,

    AspirationAttempts,
    AspirationFailLows,
    AspirationFailHighs,
    IterationsCompleted,
    BestMoveChanges,
    ScoreSignFlips,
    InstabilityTimeExtensions,
    ScoreTimeExtensions,
    InstabilityTimeAddedMs,
    ScoreTimeAddedMs,
    PredictionSamples,
    PredictedIterationMs,
    ActualIterationMs,
    AbsolutePredictionErrorMs,
    HardSafetyStops,
    PredictedIterationStops,
    Count
};

constexpr std::size_t SEARCH_DIAG_COUNTER_COUNT =
    static_cast<std::size_t>(SearchDiagCounter::Count);

struct TTDiagnostics {
    uint64_t probes = 0;
    uint64_t slots_examined = 0;
    uint64_t empty_terminations = 0;
    uint64_t key_hits = 0;
    uint64_t shallow_hits = 0;
    uint64_t tempered_rejections = 0;
    uint64_t invalid_move_rejections = 0;
    uint64_t exact_hits = 0;
    uint64_t bound_hits = 0;
    uint64_t bound_cutoffs = 0;

    uint64_t stores = 0;
    uint64_t exact_stores = 0;
    uint64_t lowerbound_stores = 0;
    uint64_t upperbound_stores = 0;
    uint64_t tempered_stores = 0;
    uint64_t same_key_updates = 0;
    uint64_t deeper_entries_kept = 0;
    uint64_t empty_inserts = 0;
    uint64_t replacements = 0;
    uint64_t dropped_stores = 0;
    uint64_t replaced_depth_sum = 0;
    uint64_t replacement_depth_sum = 0;
    uint64_t replaced_age_sum = 0;

    std::array<uint64_t, TT_PROBE_CATEGORY_COUNT> category_probes{};
    std::array<uint64_t, TT_PROBE_CATEGORY_COUNT> category_key_hits{};
    std::array<uint64_t, TT_PROBE_CATEGORY_COUNT> category_usable_hits{};
    std::array<uint64_t, TT_PROBE_CATEGORY_COUNT> category_cutoffs{};

    void add(const TTDiagnostics& other);
};

struct AtomicTTDiagnostics {
    std::atomic<uint64_t> probes{ 0 };
    std::atomic<uint64_t> slots_examined{ 0 };
    std::atomic<uint64_t> empty_terminations{ 0 };
    std::atomic<uint64_t> key_hits{ 0 };
    std::atomic<uint64_t> shallow_hits{ 0 };
    std::atomic<uint64_t> tempered_rejections{ 0 };
    std::atomic<uint64_t> invalid_move_rejections{ 0 };
    std::atomic<uint64_t> exact_hits{ 0 };
    std::atomic<uint64_t> bound_hits{ 0 };
    std::atomic<uint64_t> bound_cutoffs{ 0 };

    std::atomic<uint64_t> stores{ 0 };
    std::atomic<uint64_t> exact_stores{ 0 };
    std::atomic<uint64_t> lowerbound_stores{ 0 };
    std::atomic<uint64_t> upperbound_stores{ 0 };
    std::atomic<uint64_t> tempered_stores{ 0 };
    std::atomic<uint64_t> same_key_updates{ 0 };
    std::atomic<uint64_t> deeper_entries_kept{ 0 };
    std::atomic<uint64_t> empty_inserts{ 0 };
    std::atomic<uint64_t> replacements{ 0 };
    std::atomic<uint64_t> dropped_stores{ 0 };
    std::atomic<uint64_t> replaced_depth_sum{ 0 };
    std::atomic<uint64_t> replacement_depth_sum{ 0 };
    std::atomic<uint64_t> replaced_age_sum{ 0 };

    std::array<std::atomic<uint64_t>, TT_PROBE_CATEGORY_COUNT> category_probes{};
    std::array<std::atomic<uint64_t>, TT_PROBE_CATEGORY_COUNT> category_key_hits{};
    std::array<std::atomic<uint64_t>, TT_PROBE_CATEGORY_COUNT> category_usable_hits{};
    std::array<std::atomic<uint64_t>, TT_PROBE_CATEGORY_COUNT> category_cutoffs{};

    void add(const TTDiagnostics& diagnostics);
    void reset();
    TTDiagnostics snapshot() const;
};

struct SearchDiagnostics {
    uint64_t main_nodes = 0;
    uint64_t qnodes = 0;
    uint64_t qply_sum = 0;
    uint64_t quiet_checks_searched = 0;
    uint64_t qnodes_in_check = 0;
    uint64_t cycle_cutoffs = 0;
    uint64_t hard_cap_hits = 0;
    uint32_t max_qply = 0;
    uint64_t move_order_nodes = 0;
    uint64_t moves_searched_sum = 0;
    uint64_t best_move_index_sum = 0;
    uint64_t best_move_first = 0;
    uint64_t beta_cutoffs = 0;
    uint64_t beta_cutoff_index_sum = 0;
    uint64_t first_move_beta_cutoffs = 0;
    uint32_t max_best_move_index = 0;
    std::array<TTDiagnostics, TT_DIAGNOSTIC_MODE_COUNT> tt{};
    uint64_t tt_capacity_entries = 0;
    uint64_t tt_occupied_entries = 0;
    uint64_t tt_current_generation_entries = 0;
    std::array<uint64_t, TT_CLUSTER_OCCUPANCY_BUCKET_COUNT> tt_cluster_occupancy{};
    std::array<uint64_t, SEARCH_DIAG_COUNTER_COUNT> detail{};
    std::array<uint64_t, DIAGNOSTIC_ITERATION_DEPTH_COUNT> iteration_nodes{};
    std::array<uint64_t, DIAGNOSTIC_ITERATION_DEPTH_COUNT> iteration_time_ms{};
};
#endif
class Engine;
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
    void flush_counters(Engine* engine,bool force=false);
};
class Engine {
    public:
		Engine(size_t tt_size_mb = MAX_MEMORY_TT_MB);
        void set_threads(int n);
		void resize_tt(size_t tt_size_mb);
        ~Engine();
        void shutdown();
        void flush_node_counters();
        uint64_t get_total_nodes();
        uint64_t get_qnodes();
#if ENABLE_QSEARCH_DIAGNOSTICS
        SearchDiagnostics get_search_diagnostics(bool include_tt_occupancy = true);
#endif
        int checks_count;
        int ep_count;
        int capture_count;
        int checkmate_count;
        int rev_fut_count = 0;
        std::atomic<uint64_t> nodes{ 0 };
        std::atomic<uint64_t>  qnodes{ 0 };
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
        uint8_t generation=0;

        Move search(const Board& position, const SearchLimits& limits);
        void stop_search_and_wait() {
            stop_search.store(true, std::memory_order_release);
        }

    private:
        std::atomic<bool> stop_search{ false };
        void start_thread_pool(int n);
		void stop_thread_pool();

        void worker_loop(int thread_id, uint64_t initial_job_id);
        std::vector<std::thread> workers;
		int thread_count = 1;
        std::mutex pool_mtx;
		std::condition_variable cv_start;
		std::condition_variable cv_done;

        bool terminate_pool = false;

        uint64_t job_id = 0;
        int active_workers = 0;
        int job_thread_count = 1;

        Board job_position;
		SearchLimits job_limits;

        static constexpr uint64_t CHECKERS_UNKNOWN = std::numeric_limits<uint64_t>::max();
        static constexpr int LMR_DEPTH_COUNT = 64;
        static constexpr int LMR_MOVE_COUNT = 218;

        SearchResult negamax(Board & board, int depth, int alpha, int beta, int ply,ThreadLocalData* tls,
            const Move& previous_move=Move(), uint64_t checkers=CHECKERS_UNKNOWN, bool null_move_allowed=true);
        int quiescence_search(Board& board, int alpha, int beta, int search_ply, int qply,
            ThreadLocalData* tls, uint64_t checkers=CHECKERS_UNKNOWN, bool after_check_invasion=false);
        Move best_move_this_iteration;
        std::vector<TTCluster> tt;/*
        Move killer_moves[128][2];
        int history_scores[2][6][64]={};*/
        std::chrono::steady_clock::time_point start_time;
        std::atomic<int64_t> search_start_ns{ 0 };
		std::atomic<int64_t> search_deadline_ns{ std::numeric_limits<int64_t>::max() };

        static int64_t now_ns();
        void set_time_budget_ms(int total_time_ms);
        bool is_time_up() const;
        void initialize_lmr_tables();
        void sort_moves(MoveList& moves, const Board& board, int ply,const Move& tt_move, bool tt_depth_0 = false,ThreadLocalData* tls={}, const Move& previous_move=Move());
        int score_move(const Move& move, int ply,const Move& tt_move, bool depth_0,const Board& board,ThreadLocalData* tls, const Move& previous_move);
        TimeControlDecision decide_time_control(const Board& position, const SearchLimits& limits);
        bool probe_tt(uint64_t hash, int depth, int alpha, int beta, int& out_score, Move& out_move, int ply, bool is_depth_0 = false, TTMode mode = TTMode::Negamax);
        bool store_tt(uint64_t hash, int depth, int original_alpha, int beta, int best_score, Move& best_move, int ply, bool is_best_tempered, bool is_any_tempered = false, TTMode mode = TTMode::Negamax);
		bool should_futility_prune(int depth, int eval, int alpha, bool in_check,const Move& move);
		int late_move_reduction(int depth, int moves_searched, const Move& move, int ply, ThreadLocalData* tls,const Move& previous_move);
		bool try_null_move_pruning(Board& board,bool is_in_check, int depth, int alpha, int beta, int ply,
            int static_eval, int& out_score,ThreadLocalData* tls);
		SearchResult terminal_eval(const Board& board, bool king_is_in_check,int ply);
		void update_history_killer(const Move& move, int depth, int ply,ThreadLocalData* tls, const Move& previous_move=Move(),const MoveList& searched_quiets=MoveList());
        void init_tt(size_t tt_size_mb = MAX_MEMORY_TT_MB);
        bool move_could_result_in_repetition(Board& board, Move& move, int count=3);
        void recover_move_fully(Move& move,const Board& board);
        void score_moves(const MoveList& moves, int* scores, 
		int ply, const Move& tt_move, bool depth_0,const Board& board, ThreadLocalData* tls,
            const Move& previous_move, bool lazy_see=false);
        void pick_next_staged(MoveList& moves, int* scores, int start, const Board& board);
        void score_qsearch_moves(const MoveList& moves, int* scores);
		int relevant_pawn_push(const Board& board, const Move& move);
		void iterative_deepening_new(int thread_id, bool is_master,Move& out_best_move ,int& io_best_score,const Board& board, TimeControlDecision& tc,ThreadLocalData* tls);
		void perturb_root_order(MoveList& moves, int thread_id, int current_depth, uint64_t hash);
        void root_pvs(const Board& pos,
            MoveList& root_moves,
            int current_depth,
            int alpha,
            int beta,
            int& out_best_score,
            Move& out_best_move,
            int& out_second_best_score,
            Move& second_best_move,
			ThreadLocalData* tls);
		std::string create_pv_string(const Board& board,const Move& best_move, int depth);
		void add_history(ThreadLocalData* tls, const Move& move, int bonus);
        std::array<std::array<uint8_t, LMR_MOVE_COUNT>, LMR_DEPTH_COUNT> quiet_lmr{};
        std::array<std::array<uint8_t, LMR_MOVE_COUNT>, LMR_DEPTH_COUNT> tactical_lmr{};
};
constexpr uint8_t generation_age(uint8_t entry_generation, uint8_t current_generation) {
    return static_cast<uint8_t>((current_generation - entry_generation) & 63);
}
static_assert(generation_age(63, 0) == 1);
static_assert(generation_age(0, 1) == 1);
static_assert(generation_age(32, 1) == 33);
static_assert(generation_age(0, 63) == 63);
