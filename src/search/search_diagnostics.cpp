#include "search_diagnostics.h"

#include <algorithm>

#include "engine.h"
#include "search.h"
#include "see.h"

#if ENABLE_QSEARCH_DIAGNOSTICS
#define TT_DIAGNOSTIC_FIELDS(X) \
    X(probes) \
    X(slots_examined) \
    X(empty_terminations) \
    X(key_hits) \
    X(shallow_hits) \
    X(tempered_rejections) \
    X(invalid_move_rejections) \
    X(exact_hits) \
    X(bound_hits) \
    X(bound_cutoffs) \
    X(stores) \
    X(exact_stores) \
    X(lowerbound_stores) \
    X(upperbound_stores) \
    X(tempered_stores) \
    X(same_key_updates) \
    X(deeper_entries_kept) \
    X(empty_inserts) \
    X(replacements) \
    X(dropped_stores) \
    X(replaced_depth_sum) \
    X(replacement_depth_sum) \
    X(replaced_age_sum)

void TTDiagnostics::add(const TTDiagnostics& other) {
#define ADD_TT_DIAGNOSTIC(field) field += other.field;
    TT_DIAGNOSTIC_FIELDS(ADD_TT_DIAGNOSTIC)
#undef ADD_TT_DIAGNOSTIC
    for (std::size_t i = 0; i < TT_PROBE_CATEGORY_COUNT; ++i) {
        category_probes[i] += other.category_probes[i];
        category_key_hits[i] += other.category_key_hits[i];
        category_usable_hits[i] += other.category_usable_hits[i];
        category_cutoffs[i] += other.category_cutoffs[i];
    }
}

void AtomicTTDiagnostics::add(const TTDiagnostics& diagnostics) {
#define ADD_ATOMIC_TT_DIAGNOSTIC(field) field.fetch_add(diagnostics.field, std::memory_order_relaxed);
    TT_DIAGNOSTIC_FIELDS(ADD_ATOMIC_TT_DIAGNOSTIC)
#undef ADD_ATOMIC_TT_DIAGNOSTIC
    for (std::size_t i = 0; i < TT_PROBE_CATEGORY_COUNT; ++i) {
        category_probes[i].fetch_add(diagnostics.category_probes[i], std::memory_order_relaxed);
        category_key_hits[i].fetch_add(diagnostics.category_key_hits[i], std::memory_order_relaxed);
        category_usable_hits[i].fetch_add(diagnostics.category_usable_hits[i], std::memory_order_relaxed);
        category_cutoffs[i].fetch_add(diagnostics.category_cutoffs[i], std::memory_order_relaxed);
    }
}

void AtomicTTDiagnostics::reset() {
#define RESET_ATOMIC_TT_DIAGNOSTIC(field) field.store(0, std::memory_order_relaxed);
    TT_DIAGNOSTIC_FIELDS(RESET_ATOMIC_TT_DIAGNOSTIC)
#undef RESET_ATOMIC_TT_DIAGNOSTIC
    for (std::size_t i = 0; i < TT_PROBE_CATEGORY_COUNT; ++i) {
        category_probes[i].store(0, std::memory_order_relaxed);
        category_key_hits[i].store(0, std::memory_order_relaxed);
        category_usable_hits[i].store(0, std::memory_order_relaxed);
        category_cutoffs[i].store(0, std::memory_order_relaxed);
    }
}

TTDiagnostics AtomicTTDiagnostics::snapshot() const {
    TTDiagnostics diagnostics{};
#define SNAPSHOT_ATOMIC_TT_DIAGNOSTIC(field) diagnostics.field = field.load(std::memory_order_relaxed);
    TT_DIAGNOSTIC_FIELDS(SNAPSHOT_ATOMIC_TT_DIAGNOSTIC)
#undef SNAPSHOT_ATOMIC_TT_DIAGNOSTIC
    for (std::size_t i = 0; i < TT_PROBE_CATEGORY_COUNT; ++i) {
        diagnostics.category_probes[i] = category_probes[i].load(std::memory_order_relaxed);
        diagnostics.category_key_hits[i] = category_key_hits[i].load(std::memory_order_relaxed);
        diagnostics.category_usable_hits[i] = category_usable_hits[i].load(std::memory_order_relaxed);
        diagnostics.category_cutoffs[i] = category_cutoffs[i].load(std::memory_order_relaxed);
    }
    return diagnostics;
}

#undef TT_DIAGNOSTIC_FIELDS
#endif

#if ENABLE_QSEARCH_DIAGNOSTICS
static_assert(static_cast<std::size_t>(TTMode::Negamax) == 0);
static_assert(static_cast<std::size_t>(TTMode::Quiescence) == 1);

TTDiagnostics* active_tt_diagnostics(TTMode mode) {
    if (mode == TTMode::PrincipalVariation) {
        return nullptr;
    }
    return &tls_data.tt_diagnostics[static_cast<std::size_t>(mode)];
}

constexpr std::size_t diagnostic_index(SearchDiagCounter counter) {
    return static_cast<std::size_t>(counter);
}

void increment_diagnostic(ThreadLocalData* tls, SearchDiagCounter counter, uint64_t value) {
    if (tls) {
        tls->detail_diagnostics[diagnostic_index(counter)] += value;
    }
}

SearchDiagCounter move_index_bucket(uint32_t index, bool cutoff) {
    const SearchDiagCounter base = cutoff ? SearchDiagCounter::CutoffIndex1 : SearchDiagCounter::BestIndex1;
    std::size_t offset = 0;
    if (index == 1) offset = 0;
    else if (index == 2) offset = 1;
    else if (index <= 4) offset = 2;
    else if (index <= 8) offset = 3;
    else if (index <= 16) offset = 4;
    else if (index <= 32) offset = 5;
    else offset = 6;
    return static_cast<SearchDiagCounter>(diagnostic_index(base) + offset);
}

SearchDiagCounter qply_bucket(int qply) {
    std::size_t offset = 0;
    if (qply <= 0) offset = 0;
    else if (qply == 1) offset = 1;
    else if (qply == 2) offset = 2;
    else if (qply <= 4) offset = 3;
    else if (qply <= 8) offset = 4;
    else if (qply <= 12) offset = 5;
    else offset = 6;
    return static_cast<SearchDiagCounter>(diagnostic_index(SearchDiagCounter::QPly0) + offset);
}

void increment_move_source(ThreadLocalData* tls, MoveOrderSource source, bool cutoff) {
    if (!tls || source == MoveOrderSource::Unknown) {
        return;
    }
    const SearchDiagCounter base = cutoff
        ? SearchDiagCounter::CutoffSourceTT
        : SearchDiagCounter::BestSourceTT;
    increment_diagnostic(tls, static_cast<SearchDiagCounter>(
        diagnostic_index(base) + static_cast<std::size_t>(source)));
}

void record_tt_probe_categories(TTDiagnostics* diagnostics, int depth, int alpha, int beta,
    bool in_check, TTProbeDiagnosticEvent event) {
    if (!diagnostics) {
        return;
    }
    std::array<bool, TT_PROBE_CATEGORY_COUNT> categories{
        depth == 0,
        depth > 0,
        beta - alpha > 1,
        beta - alpha <= 1,
        in_check
    };
    for (std::size_t i = 0; i < categories.size(); ++i) {
        if (!categories[i]) continue;
        switch (event) {
        case TTProbeDiagnosticEvent::Probe:
            diagnostics->category_probes[i]++;
            break;
        case TTProbeDiagnosticEvent::KeyHit:
            diagnostics->category_key_hits[i]++;
            break;
        case TTProbeDiagnosticEvent::UsableHit:
            diagnostics->category_usable_hits[i]++;
            break;
        case TTProbeDiagnosticEvent::Cutoff:
            diagnostics->category_cutoffs[i]++;
            break;
        }
    }
}

void record_move_order_diagnostics(ThreadLocalData* tls, uint32_t moves_searched,
    uint32_t best_move_index, uint32_t beta_cutoff_index,
    MoveOrderSource best_source,
    MoveOrderSource cutoff_source) {
    if (!tls || moves_searched == 0 || best_move_index == 0) {
        return;
    }
    tls->move_order_nodes++;
    tls->moves_searched_sum += moves_searched;
    tls->best_move_index_sum += best_move_index;
    tls->best_move_first += best_move_index == 1;
    tls->max_best_move_index = std::max(tls->max_best_move_index, best_move_index);
    increment_diagnostic(tls, move_index_bucket(best_move_index, false));
    increment_move_source(tls, best_source, false);
    if (beta_cutoff_index > 0) {
        tls->beta_cutoffs++;
        tls->beta_cutoff_index_sum += beta_cutoff_index;
        tls->first_move_beta_cutoffs += beta_cutoff_index == 1;
        increment_diagnostic(tls, move_index_bucket(beta_cutoff_index, true));
        increment_move_source(tls, cutoff_source, true);
    }
}
#endif

#if ENABLE_QSEARCH_DIAGNOSTICS
MoveOrderSource classify_move_order_source(const Move& move, const Move& tt_move, bool tt_depth_0,
    const Position& pos, ThreadLocalData* tls, const Move& previous_move, int ply) {
    if (move == tt_move && !tt_depth_0) return MoveOrderSource::TT;
    if (move.promotion_piece != PieceType::None) return MoveOrderSource::Promotion;
    if (move.piece_captured != PieceType::None) {
        return see_move(pos, move) >= 0
            ? MoveOrderSource::WinningCapture
            : MoveOrderSource::LosingCapture;
    }
    if (move == tls->killer_moves[ply][0] || move == tls->killer_moves[ply][1]) {
        return MoveOrderSource::Killer;
    }
    if (previous_move.from_square != NO_SQUARE &&
        move == tls->counter_moves[color_index(previous_move.move_color)]
            [piece_index(previous_move.piece_moved)][previous_move.to_square]) {
        return MoveOrderSource::Countermove;
    }
    return MoveOrderSource::History;
}
#endif
