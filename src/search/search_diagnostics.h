#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

#ifndef ENABLE_QSEARCH_DIAGNOSTICS
#define ENABLE_QSEARCH_DIAGNOSTICS 0
#endif

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

class Position;
struct Move;
struct ThreadLocalData;
enum class TTMode;

enum class TTProbeDiagnosticEvent { Probe, KeyHit, UsableHit, Cutoff };

TTDiagnostics* active_tt_diagnostics(TTMode mode);
void increment_diagnostic(ThreadLocalData* tls, SearchDiagCounter counter, uint64_t value = 1);
SearchDiagCounter qply_bucket(int qply);
void record_tt_probe_categories(TTDiagnostics* diagnostics, int depth, int alpha, int beta,
    bool in_check, TTProbeDiagnosticEvent event);
void record_move_order_diagnostics(ThreadLocalData* tls, uint32_t moves_searched,
    uint32_t best_move_index, uint32_t beta_cutoff_index,
    MoveOrderSource best_source = MoveOrderSource::Unknown,
    MoveOrderSource cutoff_source = MoveOrderSource::Unknown);
MoveOrderSource classify_move_order_source(const Move& move, const Move& tt_move,
    bool tt_depth_0, const Position& pos, ThreadLocalData* tls,
    const Move& previous_move, int ply);

#endif
