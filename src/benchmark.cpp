#include "benchmark.h"

#include <chrono>
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>

#include "board.h"
#include "engine.h"
#include "uci_helpers.h"

#if ENABLE_QSEARCH_DIAGNOSTICS
namespace {
double percentage(uint64_t value, uint64_t total) {
    return total > 0
        ? 100.0 * static_cast<double>(value) / static_cast<double>(total)
        : 0.0;
}

uint64_t detail_value(const SearchDiagnostics& diagnostics, SearchDiagCounter counter) {
    return diagnostics.detail[static_cast<std::size_t>(counter)];
}

void print_tt_diagnostics(const std::string& prefix, const char* mode_name,
    const TTDiagnostics& diagnostics) {
    if (diagnostics.probes == 0 && diagnostics.stores == 0) {
        std::cout << prefix << "TT " << mode_name << ": inactive (no probes or stores)\n";
        return;
    }

    const uint64_t usable_hits = diagnostics.exact_hits + diagnostics.bound_cutoffs;
    std::cout << prefix << "TT " << mode_name << " probes: " << diagnostics.probes << '\n';
    std::cout << prefix << "TT " << mode_name << " raw key hits: " << diagnostics.key_hits
              << " (" << percentage(diagnostics.key_hits, diagnostics.probes) << "%)\n";
    std::cout << prefix << "TT " << mode_name << " usable hits: " << usable_hits
              << " (" << percentage(usable_hits, diagnostics.probes) << "%)\n";
    std::cout << prefix << "TT " << mode_name << " exact hits: " << diagnostics.exact_hits
              << " (" << percentage(diagnostics.exact_hits, diagnostics.probes) << "%)\n";
    std::cout << prefix << "TT " << mode_name << " bound hits: " << diagnostics.bound_hits
              << " (" << percentage(diagnostics.bound_hits, diagnostics.probes) << "%)\n";
    std::cout << prefix << "TT " << mode_name << " bound cutoffs: " << diagnostics.bound_cutoffs
              << " (" << percentage(diagnostics.bound_cutoffs, diagnostics.probes) << "%)\n";
    std::cout << prefix << "TT " << mode_name << " shallow score rejections: " << diagnostics.shallow_hits
              << " (" << percentage(diagnostics.shallow_hits, diagnostics.key_hits) << "% of key hits)\n";
    std::cout << prefix << "TT " << mode_name << " tempered rejections: " << diagnostics.tempered_rejections
              << " (" << percentage(diagnostics.tempered_rejections, diagnostics.key_hits) << "% of key hits)\n";
    std::cout << prefix << "TT " << mode_name << " invalid-move rejections: " << diagnostics.invalid_move_rejections
              << " (" << percentage(diagnostics.invalid_move_rejections, diagnostics.key_hits) << "% of key hits)\n";
    std::cout << prefix << "TT " << mode_name << " empty terminations: " << diagnostics.empty_terminations
              << " (" << percentage(diagnostics.empty_terminations, diagnostics.probes) << "% of probes)\n";
    const double average_slots = diagnostics.probes > 0
        ? static_cast<double>(diagnostics.slots_examined) / static_cast<double>(diagnostics.probes)
        : 0.0;
    std::cout << prefix << "TT " << mode_name << " average slots examined: " << average_slots << '\n';

    std::cout << prefix << "TT " << mode_name << " stores: " << diagnostics.stores << '\n';
    std::cout << prefix << "TT " << mode_name << " store flags exact/lower/upper/tempered: "
              << diagnostics.exact_stores << '/' << diagnostics.lowerbound_stores << '/'
              << diagnostics.upperbound_stores << '/' << diagnostics.tempered_stores << '\n';
    std::cout << prefix << "TT " << mode_name << " same-key updates: " << diagnostics.same_key_updates
              << " (" << percentage(diagnostics.same_key_updates, diagnostics.stores) << "%)\n";
    std::cout << prefix << "TT " << mode_name << " deeper entries kept: " << diagnostics.deeper_entries_kept
              << " (" << percentage(diagnostics.deeper_entries_kept, diagnostics.stores) << "%)\n";
    std::cout << prefix << "TT " << mode_name << " empty inserts: " << diagnostics.empty_inserts
              << " (" << percentage(diagnostics.empty_inserts, diagnostics.stores) << "%)\n";
    std::cout << prefix << "TT " << mode_name << " replacements: " << diagnostics.replacements
              << " (" << percentage(diagnostics.replacements, diagnostics.stores) << "%)\n";
    std::cout << prefix << "TT " << mode_name << " dropped stores: " << diagnostics.dropped_stores
              << " (" << percentage(diagnostics.dropped_stores, diagnostics.stores) << "%)\n";

    if (diagnostics.replacements > 0) {
        const double average_replaced_depth = static_cast<double>(diagnostics.replaced_depth_sum)
            / static_cast<double>(diagnostics.replacements);
        const double average_replacement_depth = static_cast<double>(diagnostics.replacement_depth_sum)
            / static_cast<double>(diagnostics.replacements);
        const double average_replaced_age = static_cast<double>(diagnostics.replaced_age_sum)
            / static_cast<double>(diagnostics.replacements);
        std::cout << prefix << "TT " << mode_name << " replacement avg old/new depth: "
                  << average_replaced_depth << '/' << average_replacement_depth << '\n';
        std::cout << prefix << "TT " << mode_name << " replacement average age: "
                  << average_replaced_age << '\n';
    }
}
}

void print_search_diagnostics_summary(const SearchDiagnostics& d, const std::string& p) {
    const std::ios::fmtflags old_flags = std::cout.flags();
    const std::streamsize old_precision = std::cout.precision();
    std::cout << std::fixed << std::setprecision(2);
    const auto value = [&](SearchDiagCounter counter) { return detail_value(d, counter); };
    const auto rate = [&](SearchDiagCounter numerator, SearchDiagCounter denominator) {
        return percentage(value(numerator), value(denominator));
    };
    const auto average = [&](SearchDiagCounter numerator, SearchDiagCounter denominator) {
        return value(denominator) > 0
            ? static_cast<double>(value(numerator)) / static_cast<double>(value(denominator))
            : 0.0;
    };

    const uint64_t total_nodes = d.main_nodes + d.qnodes;
    const double average_qply = d.qnodes > 0
        ? static_cast<double>(d.qply_sum) / static_cast<double>(d.qnodes)
        : 0.0;
    std::cout << p << "nodes main/q/total " << d.main_nodes << '/' << d.qnodes << '/' << total_nodes
              << " qpercent " << percentage(d.qnodes, total_nodes)
              << " qply avg/max " << average_qply << '/' << d.max_qply << '\n';
    std::cout << p << "qsearch termination standpat/softcap/hardstatic/harddraw/cycle/fifty/repetition/mate/notactical "
              << value(SearchDiagCounter::QStandPatCutoffs) << '/'
              << value(SearchDiagCounter::QSoftCapStaticReturns) << '/'
              << value(SearchDiagCounter::QHardCapStaticReturns) << '/'
              << value(SearchDiagCounter::QHardCapDrawReturns) << '/'
              << d.cycle_cutoffs << '/'
              << value(SearchDiagCounter::QFiftyMoveDraws) << '/'
              << value(SearchDiagCounter::QRepetitionDraws) << '/'
              << value(SearchDiagCounter::QCheckmates) << '/'
              << value(SearchDiagCounter::QNoTacticalMoves) << '\n';
    std::cout << p << "qsearch moves generated/searched/SEE-pruned "
              << value(SearchDiagCounter::QMovesGenerated) << '/'
              << value(SearchDiagCounter::QMovesSearched) << '/'
              << value(SearchDiagCounter::QSeePrunes)
              << " beta cutoffs capture/quiet-check/promotion/evasion "
              << value(SearchDiagCounter::QCaptureBetaCutoffs) << '/'
              << value(SearchDiagCounter::QQuietCheckBetaCutoffs) << '/'
              << value(SearchDiagCounter::QPromotionBetaCutoffs) << '/'
              << value(SearchDiagCounter::QEvasionBetaCutoffs) << '\n';
    std::cout << p << "qsearch activity quiet-checks/in-check-nodes/cycle-cutoffs/hard-cap-hits "
              << d.quiet_checks_searched << '/' << d.qnodes_in_check << '/'
              << d.cycle_cutoffs << '/' << d.hard_cap_hits << '\n';
    std::cout << p << "qply histogram 0/1/2/3-4/5-8/9-12/13+ "
              << value(SearchDiagCounter::QPly0) << '/'
              << value(SearchDiagCounter::QPly1) << '/'
              << value(SearchDiagCounter::QPly2) << '/'
              << value(SearchDiagCounter::QPly3To4) << '/'
              << value(SearchDiagCounter::QPly5To8) << '/'
              << value(SearchDiagCounter::QPly9To12) << '/'
              << value(SearchDiagCounter::QPly13Plus) << '\n';

    std::cout << p << "pruning RFP attempts/cutoffs/rate "
              << value(SearchDiagCounter::RfpAttempts) << '/'
              << value(SearchDiagCounter::RfpCutoffs) << '/'
              << rate(SearchDiagCounter::RfpCutoffs, SearchDiagCounter::RfpAttempts)
              << "% NMP candidates/searches/cutoffs/search-cutoff-rate "
              << value(SearchDiagCounter::NmpCandidates) << '/'
              << value(SearchDiagCounter::NmpSearches) << '/'
              << value(SearchDiagCounter::NmpCutoffs) << '/'
              << rate(SearchDiagCounter::NmpCutoffs, SearchDiagCounter::NmpSearches) << "%\n";
    std::cout << p << "pruning futility checks/prunes/rate "
              << value(SearchDiagCounter::FutilityChecks) << '/'
              << value(SearchDiagCounter::FutilityPrunes) << '/'
              << rate(SearchDiagCounter::FutilityPrunes, SearchDiagCounter::FutilityChecks)
              << "% LMR reductions/researches/still-above-alpha "
              << value(SearchDiagCounter::LmrReductions) << '/'
              << value(SearchDiagCounter::LmrResearches) << '/'
              << value(SearchDiagCounter::LmrResearchImproved) << '\n';
    std::cout << p << "PVS zero-window/full-window-research/rate "
              << value(SearchDiagCounter::PvsZeroWindowSearches) << '/'
              << value(SearchDiagCounter::PvsFullWindowResearches) << '/'
              << rate(SearchDiagCounter::PvsFullWindowResearches,
                  SearchDiagCounter::PvsZeroWindowSearches)
              << "% check extensions " << value(SearchDiagCounter::CheckExtensions) << '\n';

    constexpr const char* source_names[MOVE_ORDER_SOURCE_COUNT] = {
        "TT", "win-cap", "promotion", "killer", "counter", "history", "lose-cap"
    };
    const auto print_sources = [&](bool cutoff) {
        const SearchDiagCounter base = cutoff
            ? SearchDiagCounter::CutoffSourceTT
            : SearchDiagCounter::BestSourceTT;
        uint64_t source_total = 0;
        for (std::size_t i = 0; i < MOVE_ORDER_SOURCE_COUNT; ++i) {
            source_total += d.detail[static_cast<std::size_t>(base) + i];
        }
        const uint64_t overall_total = cutoff ? d.beta_cutoffs : d.move_order_nodes;
        std::cout << p << (cutoff ? "cutoff move sources attributed " : "best move sources attributed ")
                  << source_total << '/' << overall_total << ' ';
        for (std::size_t i = 0; i < MOVE_ORDER_SOURCE_COUNT; ++i) {
            const uint64_t count = d.detail[static_cast<std::size_t>(base) + i];
            if (i > 0) std::cout << ' ';
            std::cout << source_names[i] << ':' << count << '(' << percentage(count, source_total) << "%)";
        }
        std::cout << '\n';
    };
    print_sources(false);
    print_sources(true);

    const auto print_index_histogram = [&](bool cutoff) {
        const SearchDiagCounter base = cutoff
            ? SearchDiagCounter::CutoffIndex1
            : SearchDiagCounter::BestIndex1;
        constexpr const char* buckets[MOVE_INDEX_BUCKET_COUNT] = {
            "1", "2", "3-4", "5-8", "9-16", "17-32", "33+"
        };
        std::cout << p << (cutoff ? "cutoff index histogram " : "best index histogram ");
        for (std::size_t i = 0; i < MOVE_INDEX_BUCKET_COUNT; ++i) {
            if (i > 0) std::cout << ' ';
            std::cout << buckets[i] << ':' << d.detail[static_cast<std::size_t>(base) + i];
        }
        std::cout << '\n';
    };
    print_index_histogram(false);
    print_index_histogram(true);
    std::cout << p << "move order nodes/avg-searched/avg-best-index/best-first/max-best-index "
              << d.move_order_nodes << '/'
              << (d.move_order_nodes > 0
                    ? static_cast<double>(d.moves_searched_sum) / d.move_order_nodes : 0.0) << '/'
              << (d.move_order_nodes > 0
                    ? static_cast<double>(d.best_move_index_sum) / d.move_order_nodes : 0.0) << '/'
              << percentage(d.best_move_first, d.move_order_nodes) << "%/"
              << d.max_best_move_index << '\n';
    std::cout << p << "move order beta-cutoffs/avg-cutoff-index/first-move-cutoff-rate "
              << d.beta_cutoffs << '/'
              << (d.beta_cutoffs > 0
                    ? static_cast<double>(d.beta_cutoff_index_sum) / d.beta_cutoffs : 0.0) << '/'
              << percentage(d.first_move_beta_cutoffs, d.beta_cutoffs) << "%\n";

    std::cout << p << "shallow TT moves first/best/cutoff "
              << value(SearchDiagCounter::ShallowTTMoveFirst) << '/'
              << value(SearchDiagCounter::ShallowTTMoveBest) << '/'
              << value(SearchDiagCounter::ShallowTTMoveCutoff)
              << " TT returns rejected by repetition "
              << value(SearchDiagCounter::TTRepetitionRejectedReturns) << '\n';

    const auto print_node_class = [&](const char* name, SearchDiagCounter nodes,
        SearchDiagCounter generated, SearchDiagCounter searched) {
        std::cout << ' ' << name << ':' << value(nodes)
                  << " avggen " << average(generated, nodes)
                  << " avgsearched " << average(searched, nodes);
    };
    std::cout << p << "branching";
    print_node_class("PV", SearchDiagCounter::PvNodes,
        SearchDiagCounter::PvMovesGenerated, SearchDiagCounter::PvMovesSearched);
    print_node_class("nonPV", SearchDiagCounter::NonPvNodes,
        SearchDiagCounter::NonPvMovesGenerated, SearchDiagCounter::NonPvMovesSearched);
    print_node_class("incheck", SearchDiagCounter::InCheckNodes,
        SearchDiagCounter::InCheckMovesGenerated, SearchDiagCounter::InCheckMovesSearched);
    std::cout << " main generated/searched " << value(SearchDiagCounter::MainMovesGenerated)
              << '/' << value(SearchDiagCounter::MainMovesSearched) << '\n';

    const TTDiagnostics& negamax_tt = d.tt[static_cast<std::size_t>(TTMode::Negamax)];
    constexpr const char* category_names[TT_PROBE_CATEGORY_COUNT] = {
        "depth0", "depth>0", "PV", "nonPV", "incheck"
    };
    for (std::size_t i = 0; i < TT_PROBE_CATEGORY_COUNT; ++i) {
        std::cout << p << "TT category " << category_names[i]
                  << " probes/raw/usable/cutoffs "
                  << negamax_tt.category_probes[i] << '/'
                  << negamax_tt.category_key_hits[i] << '/'
                  << negamax_tt.category_usable_hits[i] << '/'
                  << negamax_tt.category_cutoffs[i]
                  << " rates " << percentage(negamax_tt.category_key_hits[i], negamax_tt.category_probes[i])
                  << '/' << percentage(negamax_tt.category_usable_hits[i], negamax_tt.category_probes[i])
                  << '/' << percentage(negamax_tt.category_cutoffs[i], negamax_tt.category_probes[i])
                  << "%\n";
    }

    std::cout << p << "terminal main fifty/repetition/checkmate/stalemate "
              << value(SearchDiagCounter::SearchFiftyMoveDraws) << '/'
              << value(SearchDiagCounter::SearchRepetitionDraws) << '/'
              << value(SearchDiagCounter::TerminalCheckmates) << '/'
              << value(SearchDiagCounter::TerminalStalemates) << '\n';
    std::cout << p << "aspiration attempts/fail-low/fail-high "
              << value(SearchDiagCounter::AspirationAttempts) << '/'
              << value(SearchDiagCounter::AspirationFailLows) << '/'
              << value(SearchDiagCounter::AspirationFailHighs)
              << " iterations/best-changes/sign-flips "
              << value(SearchDiagCounter::IterationsCompleted) << '/'
              << value(SearchDiagCounter::BestMoveChanges) << '/'
              << value(SearchDiagCounter::ScoreSignFlips) << '\n';
    std::cout << p << "time extensions instability/events-ms score/events-ms "
              << value(SearchDiagCounter::InstabilityTimeExtensions) << '-'
              << value(SearchDiagCounter::InstabilityTimeAddedMs) << ' '
              << value(SearchDiagCounter::ScoreTimeExtensions) << '-'
              << value(SearchDiagCounter::ScoreTimeAddedMs)
              << " stop hard/predicted " << value(SearchDiagCounter::HardSafetyStops) << '/'
              << value(SearchDiagCounter::PredictedIterationStops) << '\n';
    std::cout << p << "iteration prediction samples avg-predicted/avg-actual/avg-abs-error "
              << value(SearchDiagCounter::PredictionSamples) << ' '
              << average(SearchDiagCounter::PredictedIterationMs, SearchDiagCounter::PredictionSamples) << '/'
              << average(SearchDiagCounter::ActualIterationMs, SearchDiagCounter::PredictionSamples) << '/'
              << average(SearchDiagCounter::AbsolutePredictionErrorMs, SearchDiagCounter::PredictionSamples)
              << " ms\n";
    std::cout << p << "iteration depth nodes/time/EBF";
    uint64_t previous_nodes = 0;
    for (std::size_t depth = 0; depth < DIAGNOSTIC_ITERATION_DEPTH_COUNT; ++depth) {
        if (d.iteration_nodes[depth] == 0) continue;
        const double ebf = previous_nodes > 0
            ? static_cast<double>(d.iteration_nodes[depth]) / static_cast<double>(previous_nodes)
            : 0.0;
        std::cout << ' ' << depth << ':' << d.iteration_nodes[depth] << '/'
                  << d.iteration_time_ms[depth] << '/' << ebf;
        previous_nodes = d.iteration_nodes[depth];
    }
    std::cout << '\n';

    std::cout << p << "TT fill occupied/capacity/rate/current-generation-rate "
              << d.tt_occupied_entries << '/' << d.tt_capacity_entries << '/'
              << percentage(d.tt_occupied_entries, d.tt_capacity_entries) << "%/"
              << percentage(d.tt_current_generation_entries, d.tt_capacity_entries) << "%\n";
    std::cout << p << "TT cluster occupancy 0/1/2/3/4 ";
    for (std::size_t i = 0; i < TT_CLUSTER_OCCUPANCY_BUCKET_COUNT; ++i) {
        if (i > 0) std::cout << '/';
        std::cout << d.tt_cluster_occupancy[i];
    }
    std::cout << '\n';
    print_tt_diagnostics(p, "Negamax", negamax_tt);
    print_tt_diagnostics(p, "Quiescence", d.tt[static_cast<std::size_t>(TTMode::Quiescence)]);
    std::cout.flags(old_flags);
    std::cout.precision(old_precision);
}
#endif

int run_benchmark(
    const std::vector<std::pair<std::string, std::string>>& positions,
    int depth,
    std::size_t tt_size_mb) {
    SearchLimits limits;
    limits.depth = depth;

    uint64_t total_nodes = 0;
#if ENABLE_QSEARCH_DIAGNOSTICS
    SearchDiagnostics total_diagnostics{};
#endif
    uint64_t total_qnodes = 0;
    uint64_t total_time_ms = 0;

    std::cout << "info string bench start positions " << positions.size()
              << " depth " << depth << "\n";
    std::cout.flush();

    for (const auto& [name, fen] : positions) {
        Board board(fen);
        Engine engine(tt_size_mb);

        const auto start = std::chrono::steady_clock::now();
        const Move best = engine.search(board, limits);
        const auto end = std::chrono::steady_clock::now();

        const uint64_t elapsed_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
        const uint64_t nodes = engine.get_total_nodes();
        const uint64_t qnodes = engine.get_qnodes();
        total_nodes += nodes;
        total_qnodes += qnodes;
#if ENABLE_QSEARCH_DIAGNOSTICS
        const SearchDiagnostics diagnostics = engine.get_search_diagnostics();
        total_diagnostics.main_nodes += diagnostics.main_nodes;
        total_diagnostics.qnodes += diagnostics.qnodes;
        total_diagnostics.qply_sum += diagnostics.qply_sum;
        total_diagnostics.quiet_checks_searched += diagnostics.quiet_checks_searched;
        total_diagnostics.qnodes_in_check += diagnostics.qnodes_in_check;
        total_diagnostics.cycle_cutoffs += diagnostics.cycle_cutoffs;
        total_diagnostics.hard_cap_hits += diagnostics.hard_cap_hits;
        total_diagnostics.max_qply = std::max(total_diagnostics.max_qply, diagnostics.max_qply);
        total_diagnostics.move_order_nodes += diagnostics.move_order_nodes;
        total_diagnostics.moves_searched_sum += diagnostics.moves_searched_sum;
        total_diagnostics.best_move_index_sum += diagnostics.best_move_index_sum;
        total_diagnostics.best_move_first += diagnostics.best_move_first;
        total_diagnostics.beta_cutoffs += diagnostics.beta_cutoffs;
        total_diagnostics.beta_cutoff_index_sum += diagnostics.beta_cutoff_index_sum;
        total_diagnostics.first_move_beta_cutoffs += diagnostics.first_move_beta_cutoffs;
        total_diagnostics.max_best_move_index = std::max(
            total_diagnostics.max_best_move_index, diagnostics.max_best_move_index);
        for (std::size_t i = 0; i < TT_DIAGNOSTIC_MODE_COUNT; ++i) {
            total_diagnostics.tt[i].add(diagnostics.tt[i]);
        }
        total_diagnostics.tt_capacity_entries += diagnostics.tt_capacity_entries;
        total_diagnostics.tt_occupied_entries += diagnostics.tt_occupied_entries;
        total_diagnostics.tt_current_generation_entries += diagnostics.tt_current_generation_entries;
        for (std::size_t i = 0; i < TT_CLUSTER_OCCUPANCY_BUCKET_COUNT; ++i) {
            total_diagnostics.tt_cluster_occupancy[i] += diagnostics.tt_cluster_occupancy[i];
        }
        for (std::size_t i = 0; i < SEARCH_DIAG_COUNTER_COUNT; ++i) {
            total_diagnostics.detail[i] += diagnostics.detail[i];
        }
        for (std::size_t i = 0; i < DIAGNOSTIC_ITERATION_DEPTH_COUNT; ++i) {
            total_diagnostics.iteration_nodes[i] += diagnostics.iteration_nodes[i];
            total_diagnostics.iteration_time_ms[i] += diagnostics.iteration_time_ms[i];
        }
#endif
        total_time_ms += elapsed_ms;

#if ENABLE_QSEARCH_DIAGNOSTICS
        const double qnode_percentage = nodes > 0
            ? 100.0 * static_cast<double>(diagnostics.qnodes) / static_cast<double>(nodes)
            : 0.0;
        const double average_qply = diagnostics.qnodes > 0
            ? static_cast<double>(diagnostics.qply_sum) / static_cast<double>(diagnostics.qnodes)
            : 0.0;
        const double average_moves_searched = diagnostics.move_order_nodes > 0
            ? static_cast<double>(diagnostics.moves_searched_sum)
                / static_cast<double>(diagnostics.move_order_nodes)
            : 0.0;
        const double average_best_move_index = diagnostics.move_order_nodes > 0
            ? static_cast<double>(diagnostics.best_move_index_sum)
                / static_cast<double>(diagnostics.move_order_nodes)
            : 0.0;
#endif

        std::cout << "info string bench pos " << name
                  << " bestmove " << move_to_uci(best)
                  << " time " << elapsed_ms
                  << " nodes " << nodes;
#if ENABLE_QSEARCH_DIAGNOSTICS
        std::cout
                  << " mainnodes " << diagnostics.main_nodes
                  << " qnodes " << diagnostics.qnodes
                  << " qpercent " << std::fixed << std::setprecision(2) << qnode_percentage
                  << " maxqply " << diagnostics.max_qply
                  << " avgqply " << average_qply
                  << " quietchecks " << diagnostics.quiet_checks_searched
                  << " inchecknodes " << diagnostics.qnodes_in_check
                  << " cyclecutoffs " << diagnostics.cycle_cutoffs
                  << " hardcaphits " << diagnostics.hard_cap_hits
                  << " ttprobes " << diagnostics.tt[static_cast<std::size_t>(TTMode::Negamax)].probes
                  << " ttrawhits " << diagnostics.tt[static_cast<std::size_t>(TTMode::Negamax)].key_hits
                  << " ttusablehits "
                  << diagnostics.tt[static_cast<std::size_t>(TTMode::Negamax)].exact_hits
                      + diagnostics.tt[static_cast<std::size_t>(TTMode::Negamax)].bound_cutoffs
                  << " ttcutoffs " << diagnostics.tt[static_cast<std::size_t>(TTMode::Negamax)].bound_cutoffs
                  << " ttstores " << diagnostics.tt[static_cast<std::size_t>(TTMode::Negamax)].stores
                  << " ttreplacements " << diagnostics.tt[static_cast<std::size_t>(TTMode::Negamax)].replacements
                  << " ttdrops " << diagnostics.tt[static_cast<std::size_t>(TTMode::Negamax)].dropped_stores
                  << " avgmovessearched " << average_moves_searched
                  << " avgbestmoveindex " << average_best_move_index
                  << " bestmovefirst " << percentage(diagnostics.best_move_first,
                      diagnostics.move_order_nodes)
                  << " maxbestmoveindex " << diagnostics.max_best_move_index
                  << " firstmovecutoffs " << percentage(diagnostics.first_move_beta_cutoffs,
                      diagnostics.beta_cutoffs)
                  << " ttfill " << percentage(diagnostics.tt_occupied_entries,
                      diagnostics.tt_capacity_entries)
                  << " ttcurrentfill " << percentage(diagnostics.tt_current_generation_entries,
                      diagnostics.tt_capacity_entries);
#endif
        std::cout << "\n";
        std::cout.flush();

        engine.shutdown();
    }

    const uint64_t nps = total_time_ms > 0
        ? (total_nodes * 1000ULL) / total_time_ms
        : 0;
    std::cout << "info string bench total positions " << positions.size()
              << " depth " << depth
              << " time " << total_time_ms
              << " nodes " << total_nodes
              << " nps " << nps << "\n";
    std::cout << "Nodes searched: " << total_nodes << "\n";
    std::cout << "Nodes/second: " << nps << '\n';
#if ENABLE_QSEARCH_DIAGNOSTICS
    print_search_diagnostics_summary(total_diagnostics);
#endif
    std::cout.flush();
    return 0;
}
