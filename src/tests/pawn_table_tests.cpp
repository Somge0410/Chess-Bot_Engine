#include "pawn_table_tests.h"

#include <iostream>
#include <string_view>

#include "engine.h"
#include "evaluation.h"
#include "pawn_table.h"
#include "search_parameters.h"

int run_pawn_table_tests() {
    int failures = 0;
    const auto check = [&](bool condition, std::string_view message) {
        if (!condition) {
            ++failures;
            std::cerr << "[FAIL] " << message << '\n';
        }
    };

    PawnTable table;
    const uint64_t collision_key = table.capacity();
    PawnData data{};
    data.structure_score = {31, -9};
    data.passed_pawns[0] = uint64_t{1} << 44;
    check(table.probe(0) == nullptr, "an empty slot must not match a zero key");
    table.store(0, data);
    const PawnEntry* cached = table.probe(0);
    check(cached && cached->data.structure_score == data.structure_score
        && cached->data.passed_pawns == data.passed_pawns,
        "a valid zero-key entry must preserve the pawn payload");
    table.store(0, data);
    table.store(collision_key, data);
    check(table.probe(0) == nullptr, "a collision must reject the old full key");
    check(table.probe(collision_key) != nullptr, "the replacement key must hit");

#if ENABLE_QSEARCH_DIAGNOSTICS
    const PawnTableDiagnostics events = table.diagnostics();
    check(events.probes == 4 && events.hits == 2 && events.misses == 2,
        "probe counters must distinguish hits and misses");
    check(events.stores == 3 && events.empty_inserts == 1
        && events.same_key_updates == 1 && events.replacements == 1,
        "store counters must distinguish inserts, updates, and replacements");
    check(events.occupied_entries == 1 && events.capacity_entries == table.capacity(),
        "updates and replacements must not increase occupancy");
    table.reset_diagnostics();
    const PawnTableDiagnostics reset = table.diagnostics();
    check(reset.probes == 0 && reset.stores == 0 && reset.occupied_entries == 1,
        "resetting events must retain occupied cache entries");
    check(table.probe(collision_key) != nullptr, "a counter reset must retain cached data");
#endif

    table.clear();
#if ENABLE_QSEARCH_DIAGNOSTICS
    check(table.diagnostics().occupied_entries == 0 && table.diagnostics().probes == 0,
        "clearing the cache must reset occupancy and events");
#endif
    check(table.probe(collision_key) == nullptr, "clear must invalidate entries");

    {
        PawnTable sized(12);
        check(sized.capacity() == (std::size_t{1} << 12),
            "an explicit exponent must select the table's entry count");
        sized.store(0, data);
        sized.resize(12);
        check(sized.probe(0) != nullptr,
            "requesting the current size must preserve cached entries");
#if ENABLE_QSEARCH_DIAGNOSTICS
        check(sized.diagnostics().stores == 1 && sized.diagnostics().occupied_entries == 1,
            "an unchanged size must preserve diagnostics and occupancy");
#endif
        sized.resize(14);
        check(sized.capacity() == (std::size_t{1} << 14),
            "growing must change the table's entry count");
#if ENABLE_QSEARCH_DIAGNOSTICS
        check(sized.diagnostics().probes == 0 && sized.diagnostics().occupied_entries == 0,
            "a changed size must reset events and occupancy");
#endif
        check(sized.probe(0) == nullptr, "growing must invalidate the previous cache");
        const uint64_t high_key = ~uint64_t{0};
        sized.store(high_key, data);
        check(sized.probe(high_key) != nullptr,
            "a resized table must index keys using its new mask");
        sized.resize(12);
        check(sized.capacity() == (std::size_t{1} << 12)
            && sized.probe(high_key) == nullptr,
            "shrinking must select the smaller table and invalidate old entries");
        sized.resize(PAWN_TABLE_MIN_SIZE_LOG2 - 100);
        check(sized.capacity() == (std::size_t{1} << PAWN_TABLE_MIN_SIZE_LOG2),
            "an out-of-range small exponent must clamp before shifting");
        sized.resize(PAWN_TABLE_MAX_SIZE_LOG2 + 100);
        check(sized.capacity() == (std::size_t{1} << PAWN_TABLE_MAX_SIZE_LOG2),
            "an out-of-range large exponent must clamp before allocating");
    }

    struct RestorePawnTableSize {
        int original = PAWN_TABLE_SIZE_LOG2;
        ~RestorePawnTableSize() { PAWN_TABLE_SIZE_LOG2 = original; }
    } restore_size;
    PAWN_TABLE_SIZE_LOG2 = 12;
    {
        PawnTable tuned;
        check(tuned.capacity() == (std::size_t{1} << 12),
            "new default tables must use the tunable exponent");
    }
    PAWN_TABLE_SIZE_LOG2 = restore_size.original;

    // Reuse the same pawn key while changing non-pawn blockers, kings, rooks, and turn.
    constexpr std::string_view positions[] = {
        "k7/8/4P3/8/8/8/8/K7 w - - 0 1",
        "k7/4b3/4P3/8/8/8/8/K7 w - - 0 1",
        "k7/8/4P3/8/4R3/8/8/K7 w - - 0 1",
        "8/8/4P1k1/8/8/8/8/K7 w - - 0 1",
        "k7/8/4P3/8/8/8/8/K7 b - - 0 1",
        "k7/4p3/8/4P3/8/8/8/K3R3 w - - 0 1"
    };
    for (std::string_view fen : positions) {
        Position pos(fen);
        const int score = evaluate(pos, nullptr, EvalAll, &table);
        check(evaluate(pos, nullptr, EvalAll, &table) == score,
            "a cache hit must preserve full evaluation");
#if ENABLE_QSEARCH_DIAGNOSTICS
        const PawnTableDiagnostics before_trace = table.diagnostics();
#endif
        Trace trace;
        const int fresh = evaluate<true>(pos, &trace, EvalAll, &table);
        check(score == fresh, "cached and fresh traced evaluation must agree");
        check(get_trace_eval(&trace, EvalWeights, pos.get_game_phase()) == fresh,
            "feature counts must reproduce the evaluated score");
#if ENABLE_QSEARCH_DIAGNOSTICS
        const PawnTableDiagnostics after_trace = table.diagnostics();
        check(before_trace.probes == after_trace.probes && before_trace.stores == after_trace.stores,
            "tracing must bypass cache probes and stores");
#endif
    }

#if ENABLE_QSEARCH_DIAGNOSTICS
    Engine engine(1);
    SearchLimits limits;
    limits.depth = 4;
    engine.search(Position{}, limits);
    const SearchDiagnostics search = engine.get_search_diagnostics(false);
    check(search.pawn_table.probes > 0 && search.pawn_table.hits > 0,
        "search must publish pawn-table events");
    check(search.pawn_table.probes == search.pawn_table.hits + search.pawn_table.misses,
        "search probe totals must be internally consistent");
    check(search.pawn_table.capacity_entries == table.capacity(),
        "a one-worker search must report one pawn table");
    const SearchDiagnostics repeated = engine.get_search_diagnostics(false);
    check(repeated.pawn_table.probes == search.pawn_table.probes
        && repeated.pawn_table.occupied_entries == search.pawn_table.occupied_entries,
        "repeated snapshots must not double count events or occupancy");

    Position checkmate("7k/6Q1/5K2/8/8/8/8/8 b - - 0 1");
    engine.search(checkmate, limits);
    const SearchDiagnostics next = engine.get_search_diagnostics(false);
    check(next.pawn_table.probes == 0 && next.pawn_table.stores == 0,
        "a subsequent search must start with fresh event counters");
    check(next.pawn_table.occupied_entries == search.pawn_table.occupied_entries,
        "search resets must retain pawn-table occupancy");

    engine.set_threads(3);
    limits.depth = 6;
    limits.movetime = 1000;
    engine.search(Position{}, limits);
    const PawnTableDiagnostics workers = engine.get_search_diagnostics(false).pawn_table;
    check(workers.capacity_entries >= 2 * table.capacity()
        && workers.capacity_entries <= 3 * table.capacity(),
        "multi-worker searches must aggregate their allocated pawn tables");
    check(workers.probes == workers.hits + workers.misses
        && workers.stores == workers.misses
        && workers.occupied_entries <= workers.capacity_entries,
        "multi-worker counters and occupancy must remain consistent");

    for (int exponent : {12, 14}) {
        PAWN_TABLE_SIZE_LOG2 = exponent;
        engine.search(Position{}, limits);
        const PawnTableDiagnostics resized = engine.get_search_diagnostics(false).pawn_table;
        const std::size_t per_worker = std::size_t{1} << exponent;
        check(resized.capacity_entries >= 2 * per_worker
            && resized.capacity_entries <= 3 * per_worker,
            "an existing engine and its helpers must use the new exponent");
        check(resized.occupied_entries == resized.empty_inserts
            && resized.probes == resized.hits + resized.misses
            && resized.stores == resized.misses,
            "resized worker caches must start empty and publish coherent counters");
    }
    engine.shutdown();
#endif

    if (failures == 0) {
        std::cout << "[PASS] Pawn-table storage, evaluation, and diagnostics\n";
    }
    return failures == 0 ? 0 : 1;
}
