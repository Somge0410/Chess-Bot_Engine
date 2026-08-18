#include "benchmark.h"

#include <chrono>
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>

#include "board.h"
#include "engine.h"
#include "MoveGenerator.h"
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

template <typename... Values>
std::string diagnostic_text(const Values&... values) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(2);
    (output << ... << values);
    return output.str();
}

void print_diagnostic_section(const std::string& prefix, const std::string& title) {
    std::cout << prefix << "---- " << title << " ----\n";
}

void print_diagnostic_row(const std::string& prefix, const std::string& label,
    const std::string& value) {
    std::cout << prefix << "  " << std::left << std::setw(38) << label
              << ": " << std::right << value << '\n';
}

void print_tt_diagnostics(const std::string& prefix, const char* mode_name,
    const TTDiagnostics& diagnostics) {
    print_diagnostic_section(prefix, diagnostic_text("TRANSPOSITION TABLE / ", mode_name));
    if (diagnostics.probes == 0 && diagnostics.stores == 0) {
        print_diagnostic_row(prefix, "Status", "inactive (no probes or stores)");
        return;
    }

    const uint64_t usable_hits = diagnostics.exact_hits + diagnostics.bound_cutoffs;
    print_diagnostic_row(prefix, "Probes", diagnostic_text(diagnostics.probes));
    print_diagnostic_row(prefix, "Raw key hits",
        diagnostic_text(diagnostics.key_hits, "  (", percentage(diagnostics.key_hits, diagnostics.probes), "% of probes)"));
    print_diagnostic_row(prefix, "Usable hits",
        diagnostic_text(usable_hits, "  (", percentage(usable_hits, diagnostics.probes), "% of probes)"));
    print_diagnostic_row(prefix, "Exact hits",
        diagnostic_text(diagnostics.exact_hits, "  (", percentage(diagnostics.exact_hits, diagnostics.probes), "% of probes)"));
    print_diagnostic_row(prefix, "Bound hits",
        diagnostic_text(diagnostics.bound_hits, "  (", percentage(diagnostics.bound_hits, diagnostics.probes), "% of probes)"));
    print_diagnostic_row(prefix, "Bound cutoffs",
        diagnostic_text(diagnostics.bound_cutoffs, "  (", percentage(diagnostics.bound_cutoffs, diagnostics.probes), "% of probes)"));
    print_diagnostic_row(prefix, "Shallow score rejections",
        diagnostic_text(diagnostics.shallow_hits, "  (", percentage(diagnostics.shallow_hits, diagnostics.key_hits), "% of key hits)"));
    print_diagnostic_row(prefix, "Tempered rejections",
        diagnostic_text(diagnostics.tempered_rejections, "  (", percentage(diagnostics.tempered_rejections, diagnostics.key_hits), "% of key hits)"));
    print_diagnostic_row(prefix, "Invalid-move rejections",
        diagnostic_text(diagnostics.invalid_move_rejections, "  (", percentage(diagnostics.invalid_move_rejections, diagnostics.key_hits), "% of key hits)"));
    print_diagnostic_row(prefix, "Empty probe terminations",
        diagnostic_text(diagnostics.empty_terminations, "  (", percentage(diagnostics.empty_terminations, diagnostics.probes), "% of probes)"));
    const double average_slots = diagnostics.probes > 0
        ? static_cast<double>(diagnostics.slots_examined) / static_cast<double>(diagnostics.probes)
        : 0.0;
    print_diagnostic_row(prefix, "Average slots examined", diagnostic_text(average_slots));
    print_diagnostic_row(prefix, "Stores", diagnostic_text(diagnostics.stores));
    print_diagnostic_row(prefix, "Store flag / exact", diagnostic_text(diagnostics.exact_stores));
    print_diagnostic_row(prefix, "Store flag / lower bound", diagnostic_text(diagnostics.lowerbound_stores));
    print_diagnostic_row(prefix, "Store flag / upper bound", diagnostic_text(diagnostics.upperbound_stores));
    print_diagnostic_row(prefix, "Store flag / tempered", diagnostic_text(diagnostics.tempered_stores));
    print_diagnostic_row(prefix, "Same-key updates",
        diagnostic_text(diagnostics.same_key_updates, "  (", percentage(diagnostics.same_key_updates, diagnostics.stores), "% of stores)"));
    print_diagnostic_row(prefix, "Deeper entries kept",
        diagnostic_text(diagnostics.deeper_entries_kept, "  (", percentage(diagnostics.deeper_entries_kept, diagnostics.stores), "% of stores)"));
    print_diagnostic_row(prefix, "Empty inserts",
        diagnostic_text(diagnostics.empty_inserts, "  (", percentage(diagnostics.empty_inserts, diagnostics.stores), "% of stores)"));
    print_diagnostic_row(prefix, "Replacements",
        diagnostic_text(diagnostics.replacements, "  (", percentage(diagnostics.replacements, diagnostics.stores), "% of stores)"));
    print_diagnostic_row(prefix, "Dropped stores",
        diagnostic_text(diagnostics.dropped_stores, "  (", percentage(diagnostics.dropped_stores, diagnostics.stores), "% of stores)"));

    if (diagnostics.replacements > 0) {
        const double average_replaced_depth = static_cast<double>(diagnostics.replaced_depth_sum)
            / static_cast<double>(diagnostics.replacements);
        const double average_replacement_depth = static_cast<double>(diagnostics.replacement_depth_sum)
            / static_cast<double>(diagnostics.replacements);
        const double average_replaced_age = static_cast<double>(diagnostics.replaced_age_sum)
            / static_cast<double>(diagnostics.replacements);
        print_diagnostic_row(prefix, "Replacement avg old -> new depth",
            diagnostic_text(average_replaced_depth, " -> ", average_replacement_depth));
        print_diagnostic_row(prefix, "Replacement average age", diagnostic_text(average_replaced_age));
    }
}

void accumulate_tt_snapshot(SearchDiagnostics& total, const SearchDiagnostics& snapshot) {
    total.tt_capacity_entries += snapshot.tt_capacity_entries;
    total.tt_occupied_entries += snapshot.tt_occupied_entries;
    total.tt_current_generation_entries += snapshot.tt_current_generation_entries;
    for (std::size_t i = 0; i < TT_CLUSTER_OCCUPANCY_BUCKET_COUNT; ++i) {
        total.tt_cluster_occupancy[i] += snapshot.tt_cluster_occupancy[i];
    }
}

void accumulate_search_diagnostics(SearchDiagnostics& total, const SearchDiagnostics& diagnostics,
    bool include_tt_snapshot) {
    total.main_nodes += diagnostics.main_nodes;
    total.qnodes += diagnostics.qnodes;
    total.qply_sum += diagnostics.qply_sum;
    total.quiet_checks_searched += diagnostics.quiet_checks_searched;
    total.qnodes_in_check += diagnostics.qnodes_in_check;
    total.cycle_cutoffs += diagnostics.cycle_cutoffs;
    total.hard_cap_hits += diagnostics.hard_cap_hits;
    total.max_qply = std::max(total.max_qply, diagnostics.max_qply);
    total.move_order_nodes += diagnostics.move_order_nodes;
    total.moves_searched_sum += diagnostics.moves_searched_sum;
    total.best_move_index_sum += diagnostics.best_move_index_sum;
    total.best_move_first += diagnostics.best_move_first;
    total.beta_cutoffs += diagnostics.beta_cutoffs;
    total.beta_cutoff_index_sum += diagnostics.beta_cutoff_index_sum;
    total.first_move_beta_cutoffs += diagnostics.first_move_beta_cutoffs;
    total.max_best_move_index = std::max(
        total.max_best_move_index, diagnostics.max_best_move_index);

    for (std::size_t i = 0; i < TT_DIAGNOSTIC_MODE_COUNT; ++i) {
        total.tt[i].add(diagnostics.tt[i]);
    }
    if (include_tt_snapshot) {
        accumulate_tt_snapshot(total, diagnostics);
    }
    for (std::size_t i = 0; i < SEARCH_DIAG_COUNTER_COUNT; ++i) {
        total.detail[i] += diagnostics.detail[i];
    }
    for (std::size_t i = 0; i < DIAGNOSTIC_ITERATION_DEPTH_COUNT; ++i) {
        total.iteration_nodes[i] += diagnostics.iteration_nodes[i];
        total.iteration_time_ms[i] += diagnostics.iteration_time_ms[i];
    }
}
}

void print_search_diagnostics_summary(const SearchDiagnostics& d, const std::string& p,
    bool tt_snapshot_is_final) {
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
    print_diagnostic_section(p, "SEARCH DIAGNOSTICS SUMMARY");
    print_diagnostic_section(p, "NODES");
    print_diagnostic_row(p, "Main-search nodes", diagnostic_text(d.main_nodes));
    print_diagnostic_row(p, "Quiescence nodes", diagnostic_text(d.qnodes));
    print_diagnostic_row(p, "Total nodes", diagnostic_text(total_nodes));
    print_diagnostic_row(p, "Quiescence share", diagnostic_text(percentage(d.qnodes, total_nodes), "%"));
    print_diagnostic_row(p, "Average q-ply", diagnostic_text(average_qply));
    print_diagnostic_row(p, "Maximum q-ply", diagnostic_text(d.max_qply));

    print_diagnostic_section(p, "QUIESCENCE SEARCH");
    print_diagnostic_row(p, "Termination / stand-pat cutoff",
        diagnostic_text(value(SearchDiagCounter::QStandPatCutoffs)));
    print_diagnostic_row(p, "Termination / soft-cap static",
        diagnostic_text(value(SearchDiagCounter::QSoftCapStaticReturns)));
    print_diagnostic_row(p, "Termination / hard-cap static",
        diagnostic_text(value(SearchDiagCounter::QHardCapStaticReturns)));
    print_diagnostic_row(p, "Termination / hard-cap draw",
        diagnostic_text(value(SearchDiagCounter::QHardCapDrawReturns)));
    print_diagnostic_row(p, "Termination / cycle", diagnostic_text(d.cycle_cutoffs));
    print_diagnostic_row(p, "Termination / fifty-move draw",
        diagnostic_text(value(SearchDiagCounter::QFiftyMoveDraws)));
    print_diagnostic_row(p, "Termination / repetition draw",
        diagnostic_text(value(SearchDiagCounter::QRepetitionDraws)));
    print_diagnostic_row(p, "Termination / checkmate",
        diagnostic_text(value(SearchDiagCounter::QCheckmates)));
    print_diagnostic_row(p, "Termination / no tactical move",
        diagnostic_text(value(SearchDiagCounter::QNoTacticalMoves)));
    print_diagnostic_row(p, "Moves generated", diagnostic_text(value(SearchDiagCounter::QMovesGenerated)));
    print_diagnostic_row(p, "Moves searched", diagnostic_text(value(SearchDiagCounter::QMovesSearched)));
    print_diagnostic_row(p, "Moves SEE-pruned", diagnostic_text(value(SearchDiagCounter::QSeePrunes)));
    print_diagnostic_row(p, "Beta cutoffs / capture",
        diagnostic_text(value(SearchDiagCounter::QCaptureBetaCutoffs)));
    print_diagnostic_row(p, "Beta cutoffs / quiet check",
        diagnostic_text(value(SearchDiagCounter::QQuietCheckBetaCutoffs)));
    print_diagnostic_row(p, "Beta cutoffs / promotion",
        diagnostic_text(value(SearchDiagCounter::QPromotionBetaCutoffs)));
    print_diagnostic_row(p, "Beta cutoffs / evasion",
        diagnostic_text(value(SearchDiagCounter::QEvasionBetaCutoffs)));
    print_diagnostic_row(p, "Quiet checks searched", diagnostic_text(d.quiet_checks_searched));
    print_diagnostic_row(p, "Nodes while in check", diagnostic_text(d.qnodes_in_check));
    print_diagnostic_row(p, "Cycle cutoffs", diagnostic_text(d.cycle_cutoffs));
    print_diagnostic_row(p, "Hard-cap hits", diagnostic_text(d.hard_cap_hits));
    print_diagnostic_row(p, "Q-ply histogram buckets", "0 | 1 | 2 | 3-4 | 5-8 | 9-12 | 13+");
    print_diagnostic_row(p, "Q-ply histogram counts",
        diagnostic_text(value(SearchDiagCounter::QPly0), " | ",
            value(SearchDiagCounter::QPly1), " | ", value(SearchDiagCounter::QPly2), " | ",
            value(SearchDiagCounter::QPly3To4), " | ", value(SearchDiagCounter::QPly5To8), " | ",
            value(SearchDiagCounter::QPly9To12), " | ", value(SearchDiagCounter::QPly13Plus)));

    print_diagnostic_section(p, "PRUNING AND REDUCTIONS");
    print_diagnostic_row(p, "RFP attempts", diagnostic_text(value(SearchDiagCounter::RfpAttempts)));
    print_diagnostic_row(p, "RFP cutoffs",
        diagnostic_text(value(SearchDiagCounter::RfpCutoffs), "  (",
            rate(SearchDiagCounter::RfpCutoffs, SearchDiagCounter::RfpAttempts), "%)"));
    print_diagnostic_row(p, "NMP candidates", diagnostic_text(value(SearchDiagCounter::NmpCandidates)));
    print_diagnostic_row(p, "NMP searches", diagnostic_text(value(SearchDiagCounter::NmpSearches)));
    print_diagnostic_row(p, "NMP cutoffs",
        diagnostic_text(value(SearchDiagCounter::NmpCutoffs), "  (",
            rate(SearchDiagCounter::NmpCutoffs, SearchDiagCounter::NmpSearches), "% of searches)"));
    print_diagnostic_row(p, "IIR candidates / no TT move",
        diagnostic_text(value(SearchDiagCounter::IirCandidates), "  (",
            percentage(value(SearchDiagCounter::IirCandidates), d.main_nodes), "% of main nodes)"));
    print_diagnostic_row(p, "IIR reductions",
        diagnostic_text(value(SearchDiagCounter::IirReductions), "  (",
            rate(SearchDiagCounter::IirReductions, SearchDiagCounter::IirCandidates), "% of candidates)"));
    print_diagnostic_row(p, "IIR reductions / PV",
        diagnostic_text(value(SearchDiagCounter::IirPvReductions), "  (",
            rate(SearchDiagCounter::IirPvReductions, SearchDiagCounter::IirReductions), "% of reductions)"));
    print_diagnostic_row(p, "IIR reductions / non-PV",
        diagnostic_text(value(SearchDiagCounter::IirNonPvReductions), "  (",
            rate(SearchDiagCounter::IirNonPvReductions, SearchDiagCounter::IirReductions), "% of reductions)"));
    print_diagnostic_row(p, "IIR skipped / in check",
        diagnostic_text(value(SearchDiagCounter::IirInCheckSkips), "  (",
            rate(SearchDiagCounter::IirInCheckSkips, SearchDiagCounter::IirCandidates), "% of candidates)"));
    print_diagnostic_row(p, "Futility checks", diagnostic_text(value(SearchDiagCounter::FutilityChecks)));
    print_diagnostic_row(p, "Futility prunes",
        diagnostic_text(value(SearchDiagCounter::FutilityPrunes), "  (",
            rate(SearchDiagCounter::FutilityPrunes, SearchDiagCounter::FutilityChecks), "%)"));
    print_diagnostic_row(p, "LMR reductions", diagnostic_text(value(SearchDiagCounter::LmrReductions)));
    print_diagnostic_row(p, "LMR re-searches", diagnostic_text(value(SearchDiagCounter::LmrResearches)));
    print_diagnostic_row(p, "LMR still above alpha",
        diagnostic_text(value(SearchDiagCounter::LmrResearchImproved)));
    print_diagnostic_row(p, "PVS zero-window searches",
        diagnostic_text(value(SearchDiagCounter::PvsZeroWindowSearches)));
    print_diagnostic_row(p, "PVS full-window re-searches",
        diagnostic_text(value(SearchDiagCounter::PvsFullWindowResearches), "  (",
            rate(SearchDiagCounter::PvsFullWindowResearches,
                SearchDiagCounter::PvsZeroWindowSearches), "%)"));
    print_diagnostic_row(p, "Check extensions", diagnostic_text(value(SearchDiagCounter::CheckExtensions)));

    constexpr const char* source_names[MOVE_ORDER_SOURCE_COUNT] = {
        "TT", "win-cap", "promotion", "killer", "counter", "history", "lose-cap"
    };
    print_diagnostic_section(p, "MOVE ORDERING");
    const auto print_sources = [&](bool cutoff) {
        const SearchDiagCounter base = cutoff
            ? SearchDiagCounter::CutoffSourceTT
            : SearchDiagCounter::BestSourceTT;
        uint64_t source_total = 0;
        for (std::size_t i = 0; i < MOVE_ORDER_SOURCE_COUNT; ++i) {
            source_total += d.detail[static_cast<std::size_t>(base) + i];
        }
        const uint64_t overall_total = cutoff ? d.beta_cutoffs : d.move_order_nodes;
        print_diagnostic_row(p, cutoff ? "Cutoff sources attributed" : "Best-move sources attributed",
            diagnostic_text(source_total, " of ", overall_total));
        for (std::size_t i = 0; i < MOVE_ORDER_SOURCE_COUNT; ++i) {
            const uint64_t count = d.detail[static_cast<std::size_t>(base) + i];
            print_diagnostic_row(p,
                diagnostic_text(cutoff ? "Cutoff source / " : "Best-move source / ", source_names[i]),
                diagnostic_text(count, "  (", percentage(count, source_total), "%)"));
        }
    };
    print_sources(false);
    print_sources(true);

    const auto index_histogram = [&](SearchDiagCounter base) {
        return diagnostic_text(
            d.detail[static_cast<std::size_t>(base)], " | ",
            d.detail[static_cast<std::size_t>(base) + 1], " | ",
            d.detail[static_cast<std::size_t>(base) + 2], " | ",
            d.detail[static_cast<std::size_t>(base) + 3], " | ",
            d.detail[static_cast<std::size_t>(base) + 4], " | ",
            d.detail[static_cast<std::size_t>(base) + 5], " | ",
            d.detail[static_cast<std::size_t>(base) + 6]);
    };
    print_diagnostic_row(p, "Discovery-index buckets", "1 | 2 | 3-4 | 5-8 | 9-16 | 17-32 | 33+");
    print_diagnostic_row(p, "Best-move index counts",
        index_histogram(SearchDiagCounter::BestIndex1));
    print_diagnostic_row(p, "Beta-cutoff index counts",
        index_histogram(SearchDiagCounter::CutoffIndex1));
    print_diagnostic_row(p, "Move-order nodes", diagnostic_text(d.move_order_nodes));
    print_diagnostic_row(p, "Average moves searched",
        diagnostic_text(d.move_order_nodes > 0
            ? static_cast<double>(d.moves_searched_sum) / d.move_order_nodes : 0.0));
    print_diagnostic_row(p, "Average best-move index",
        diagnostic_text(d.move_order_nodes > 0
            ? static_cast<double>(d.best_move_index_sum) / d.move_order_nodes : 0.0));
    print_diagnostic_row(p, "Best move found first",
        diagnostic_text(d.best_move_first, "  (", percentage(d.best_move_first, d.move_order_nodes), "%)"));
    print_diagnostic_row(p, "Maximum best-move index", diagnostic_text(d.max_best_move_index));
    print_diagnostic_row(p, "Beta cutoffs", diagnostic_text(d.beta_cutoffs));
    print_diagnostic_row(p, "Average beta-cutoff index",
        diagnostic_text(d.beta_cutoffs > 0
            ? static_cast<double>(d.beta_cutoff_index_sum) / d.beta_cutoffs : 0.0));
    print_diagnostic_row(p, "First-move beta cutoffs",
        diagnostic_text(d.first_move_beta_cutoffs, "  (",
            percentage(d.first_move_beta_cutoffs, d.beta_cutoffs), "%)"));
    print_diagnostic_row(p, "Shallow TT move ordered first",
        diagnostic_text(value(SearchDiagCounter::ShallowTTMoveFirst)));
    print_diagnostic_row(p, "Shallow TT move became best",
        diagnostic_text(value(SearchDiagCounter::ShallowTTMoveBest)));
    print_diagnostic_row(p, "Shallow TT move caused cutoff",
        diagnostic_text(value(SearchDiagCounter::ShallowTTMoveCutoff)));
    print_diagnostic_row(p, "TT returns rejected by repetition",
        diagnostic_text(value(SearchDiagCounter::TTRepetitionRejectedReturns)));

    print_diagnostic_section(p, "BRANCHING");
    const auto print_node_class = [&](const char* name, SearchDiagCounter node_counter,
        SearchDiagCounter generated, SearchDiagCounter searched) {
        print_diagnostic_row(p, name,
            diagnostic_text("nodes ", value(node_counter), " | avg generated ",
                average(generated, node_counter), " | avg searched ", average(searched, node_counter)));
    };
    print_node_class("PV", SearchDiagCounter::PvNodes,
        SearchDiagCounter::PvMovesGenerated, SearchDiagCounter::PvMovesSearched);
    print_node_class("nonPV", SearchDiagCounter::NonPvNodes,
        SearchDiagCounter::NonPvMovesGenerated, SearchDiagCounter::NonPvMovesSearched);
    print_node_class("incheck", SearchDiagCounter::InCheckNodes,
        SearchDiagCounter::InCheckMovesGenerated, SearchDiagCounter::InCheckMovesSearched);
    print_diagnostic_row(p, "Main moves generated / searched",
        diagnostic_text(value(SearchDiagCounter::MainMovesGenerated), " / ",
            value(SearchDiagCounter::MainMovesSearched)));

    const TTDiagnostics& negamax_tt = d.tt[static_cast<std::size_t>(TTMode::Negamax)];
    constexpr const char* category_names[TT_PROBE_CATEGORY_COUNT] = {
        "depth0", "depth>0", "PV", "nonPV", "incheck"
    };
    print_diagnostic_section(p, "TT PROBE CATEGORIES");
    for (std::size_t i = 0; i < TT_PROBE_CATEGORY_COUNT; ++i) {
        print_diagnostic_row(p, diagnostic_text("Category / ", category_names[i]),
            diagnostic_text("probes ", negamax_tt.category_probes[i],
                " | raw ", negamax_tt.category_key_hits[i], " (",
                percentage(negamax_tt.category_key_hits[i], negamax_tt.category_probes[i]), "%)",
                " | usable ", negamax_tt.category_usable_hits[i], " (",
                percentage(negamax_tt.category_usable_hits[i], negamax_tt.category_probes[i]), "%)",
                " | cutoffs ", negamax_tt.category_cutoffs[i], " (",
                percentage(negamax_tt.category_cutoffs[i], negamax_tt.category_probes[i]), "%)"));
    }

    print_diagnostic_section(p, "SEARCH CONTROL AND TERMINALS");
    print_diagnostic_row(p, "Main-search fifty-move draws",
        diagnostic_text(value(SearchDiagCounter::SearchFiftyMoveDraws)));
    print_diagnostic_row(p, "Main-search repetition draws",
        diagnostic_text(value(SearchDiagCounter::SearchRepetitionDraws)));
    print_diagnostic_row(p, "Terminal checkmates",
        diagnostic_text(value(SearchDiagCounter::TerminalCheckmates)));
    print_diagnostic_row(p, "Terminal stalemates",
        diagnostic_text(value(SearchDiagCounter::TerminalStalemates)));
    print_diagnostic_row(p, "Aspiration attempts",
        diagnostic_text(value(SearchDiagCounter::AspirationAttempts)));
    print_diagnostic_row(p, "Aspiration fail-low / fail-high",
        diagnostic_text(value(SearchDiagCounter::AspirationFailLows), " / ",
            value(SearchDiagCounter::AspirationFailHighs)));
    print_diagnostic_row(p, "Iterations completed",
        diagnostic_text(value(SearchDiagCounter::IterationsCompleted)));
    print_diagnostic_row(p, "Best-move changes / score sign flips",
        diagnostic_text(value(SearchDiagCounter::BestMoveChanges), " / ",
            value(SearchDiagCounter::ScoreSignFlips)));
    print_diagnostic_row(p, "Instability time extensions",
        diagnostic_text(value(SearchDiagCounter::InstabilityTimeExtensions), " events | ",
            value(SearchDiagCounter::InstabilityTimeAddedMs), " ms"));
    print_diagnostic_row(p, "Score time extensions",
        diagnostic_text(value(SearchDiagCounter::ScoreTimeExtensions), " events | ",
            value(SearchDiagCounter::ScoreTimeAddedMs), " ms"));
    print_diagnostic_row(p, "Hard / predicted stops",
        diagnostic_text(value(SearchDiagCounter::HardSafetyStops), " / ",
            value(SearchDiagCounter::PredictedIterationStops)));
    print_diagnostic_row(p, "Iteration prediction samples",
        diagnostic_text(value(SearchDiagCounter::PredictionSamples)));
    print_diagnostic_row(p, "Average predicted / actual time",
        diagnostic_text(average(SearchDiagCounter::PredictedIterationMs,
            SearchDiagCounter::PredictionSamples), " / ",
            average(SearchDiagCounter::ActualIterationMs, SearchDiagCounter::PredictionSamples), " ms"));
    print_diagnostic_row(p, "Average absolute prediction error",
        diagnostic_text(average(SearchDiagCounter::AbsolutePredictionErrorMs,
            SearchDiagCounter::PredictionSamples), " ms"));

    print_diagnostic_section(p, "ITERATIONS BY DEPTH");
    print_diagnostic_row(p, "Columns", "depth | nodes | time ms | EBF");
    uint64_t previous_nodes = 0;
    for (std::size_t depth = 0; depth < DIAGNOSTIC_ITERATION_DEPTH_COUNT; ++depth) {
        if (d.iteration_nodes[depth] == 0) continue;
        const double ebf = previous_nodes > 0
            ? static_cast<double>(d.iteration_nodes[depth]) / static_cast<double>(previous_nodes)
            : 0.0;
        print_diagnostic_row(p, diagnostic_text("Depth ", depth),
            diagnostic_text(d.iteration_nodes[depth], " | ", d.iteration_time_ms[depth], " | ", ebf));
        previous_nodes = d.iteration_nodes[depth];
    }

    print_diagnostic_section(p, tt_snapshot_is_final ? "TT FINAL FILL" : "TT FILL");
    print_diagnostic_row(p, "Occupied entries", diagnostic_text(d.tt_occupied_entries));
    print_diagnostic_row(p, "Capacity entries", diagnostic_text(d.tt_capacity_entries));
    print_diagnostic_row(p, "Fill rate",
        diagnostic_text(percentage(d.tt_occupied_entries, d.tt_capacity_entries), "%"));
    print_diagnostic_row(p, "Current-generation entries",
        diagnostic_text(d.tt_current_generation_entries, "  (",
            percentage(d.tt_current_generation_entries, d.tt_capacity_entries), "% of capacity)"));
    print_diagnostic_row(p, "Cluster occupancy buckets", "0 | 1 | 2 | 3 | 4");
    print_diagnostic_row(p, "Cluster occupancy counts",
        diagnostic_text(d.tt_cluster_occupancy[0], " | ", d.tt_cluster_occupancy[1], " | ",
            d.tt_cluster_occupancy[2], " | ", d.tt_cluster_occupancy[3], " | ",
            d.tt_cluster_occupancy[4]));
    print_tt_diagnostics(p, "Negamax", negamax_tt);
    print_tt_diagnostics(p, "Quiescence", d.tt[static_cast<std::size_t>(TTMode::Quiescence)]);
    std::cout << p << "========================================\n";
    std::cout.flags(old_flags);
    std::cout.precision(old_precision);
}
#endif

int run_benchmark(
    const std::vector<std::pair<std::string, std::string>>& positions,
    bool tt_bench,
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
              << " depth " << depth
              << " mode " << (tt_bench ? "tt-stateful" : "isolated") << "\n";
    std::cout.flush();

    std::unique_ptr<Engine> persistent_engine;
    if (tt_bench) {
        persistent_engine = std::make_unique<Engine>(tt_size_mb);
    }

    for (const auto& [name, fen] : positions) {
        Board board(fen);
        std::unique_ptr<Engine> isolated_engine;
        Engine* engine = persistent_engine.get();
        if (!engine) {
            isolated_engine = std::make_unique<Engine>(tt_size_mb);
            engine = isolated_engine.get();
        }

        const auto start = std::chrono::steady_clock::now();
        const Move best = engine->search(board, limits);
        const auto end = std::chrono::steady_clock::now();

        const uint64_t elapsed_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
        const uint64_t nodes = engine->get_total_nodes();
        const uint64_t qnodes = engine->get_qnodes();
        total_nodes += nodes;
        total_qnodes += qnodes;
#if ENABLE_QSEARCH_DIAGNOSTICS
        const SearchDiagnostics diagnostics = engine->get_search_diagnostics(!tt_bench);
        accumulate_search_diagnostics(total_diagnostics, diagnostics, !tt_bench);
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
                      diagnostics.beta_cutoffs);
        if (tt_bench) {
            std::cout << " ttfill deferred";
        }
        else {
            std::cout << " ttfill " << percentage(diagnostics.tt_occupied_entries,
                            diagnostics.tt_capacity_entries)
                      << " ttcurrentfill " << percentage(diagnostics.tt_current_generation_entries,
                            diagnostics.tt_capacity_entries);
        }
#endif
        std::cout << "\n";
        std::cout.flush();
    }

#if ENABLE_QSEARCH_DIAGNOSTICS
    if (tt_bench && persistent_engine) {
        const SearchDiagnostics final_snapshot = persistent_engine->get_search_diagnostics(true);
        total_diagnostics.tt_capacity_entries = final_snapshot.tt_capacity_entries;
        total_diagnostics.tt_occupied_entries = final_snapshot.tt_occupied_entries;
        total_diagnostics.tt_current_generation_entries = final_snapshot.tt_current_generation_entries;
        total_diagnostics.tt_cluster_occupancy = final_snapshot.tt_cluster_occupancy;
    }
#endif

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
    print_search_diagnostics_summary(total_diagnostics, "", tt_bench);
#endif
    std::cout.flush();
    return 0;
}
int run_benchmark_game(int movetime_ms, std::size_t tt_size_mb, int max_moves) {
    SearchLimits limits;
    limits.movetime = std::max(1, movetime_ms);

    Board board;
    Engine white(tt_size_mb);
    Engine black(tt_size_mb);
    uint64_t total_nodes = 0;
    uint64_t total_time_ms = 0;
    int plies_played = 0;
    std::string termination = "move-limit";
#if ENABLE_QSEARCH_DIAGNOSTICS
    SearchDiagnostics total_diagnostics{};
#endif

    std::cout << "info string bench game start movetime " << limits.movetime
              << " maxmoves " << max_moves
              << " hash-per-side " << tt_size_mb << "MB\n";
    std::cout.flush();

    for (int ply = 1; ply <= 2 * max_moves; ++ply) {
        if (board.is_fifty_move_rule_draw()) {
            termination = "fifty-move";
            break;
        }
        if (board.is_repetition_draw(3)) {
            termination = "repetition";
            break;
        }

        MoveList legal_moves;
        MoveGenerator::generate_moves(board, legal_moves);
        if (legal_moves.empty()) {
            termination = board.get_checkers() != 0 ? "checkmate" : "stalemate";
            break;
        }

        const bool white_to_move = board.is_white_to_move();
        Engine& engine = white_to_move ? white : black;
        const auto start = std::chrono::steady_clock::now();
        const Move best = engine.search(board, limits);
        const auto end = std::chrono::steady_clock::now();
        const uint64_t elapsed_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
        const uint64_t nodes = engine.get_total_nodes();

        if (best.from_square == NO_SQUARE || best.to_square == NO_SQUARE) {
            termination = "no-bestmove";
            break;
        }

        total_nodes += nodes;
        total_time_ms += elapsed_ms;
        plies_played = ply;

#if ENABLE_QSEARCH_DIAGNOSTICS
        // Scanning the complete TT after every ply would dominate short searches.
        // Event counters are collected here; both TT fill snapshots are taken once at the end.
        const SearchDiagnostics diagnostics = engine.get_search_diagnostics(false);
        accumulate_search_diagnostics(total_diagnostics, diagnostics, false);

        const TTDiagnostics& tt = diagnostics.tt[static_cast<std::size_t>(TTMode::Negamax)];
        const uint64_t usable_hits = tt.exact_hits + tt.bound_cutoffs;
        const double average_qply = diagnostics.qnodes > 0
            ? static_cast<double>(diagnostics.qply_sum) / diagnostics.qnodes
            : 0.0;
        const double average_moves_searched = diagnostics.move_order_nodes > 0
            ? static_cast<double>(diagnostics.moves_searched_sum) / diagnostics.move_order_nodes
            : 0.0;
        const double average_best_move_index = diagnostics.move_order_nodes > 0
            ? static_cast<double>(diagnostics.best_move_index_sum) / diagnostics.move_order_nodes
            : 0.0;
        const double average_slots_examined = tt.probes > 0
            ? static_cast<double>(tt.slots_examined) / tt.probes
            : 0.0;
#endif

        std::cout << "info string bench game ply " << ply
                  << " side " << (white_to_move ? "white" : "black")
                  << " bestmove " << move_to_uci(best)
                  << " time " << elapsed_ms
                  << " nodes " << nodes;
#if ENABLE_QSEARCH_DIAGNOSTICS
        std::cout << " mainnodes " << diagnostics.main_nodes
                  << " qnodes " << diagnostics.qnodes
                  << " qpercent " << std::fixed << std::setprecision(2)
                  << percentage(diagnostics.qnodes, diagnostics.main_nodes + diagnostics.qnodes)
                  << " avgqply " << average_qply
                  << " maxqply " << diagnostics.max_qply
                  << " quietchecks " << diagnostics.quiet_checks_searched
                  << " inchecknodes " << diagnostics.qnodes_in_check
                  << " cyclecutoffs " << diagnostics.cycle_cutoffs
                  << " hardcaphits " << diagnostics.hard_cap_hits
                  << " ttprobes " << tt.probes
                  << " ttrawhits " << tt.key_hits
                  << " ttusablehits " << usable_hits
                  << " ttcutoffs " << tt.bound_cutoffs
                  << " ttstores " << tt.stores
                  << " ttsamekey " << tt.same_key_updates
                  << " ttreplacements " << tt.replacements
                  << " ttdrops " << tt.dropped_stores
                  << " avgttslots " << average_slots_examined
                  << " avgmovessearched " << average_moves_searched
                  << " avgbestmoveindex " << average_best_move_index
                  << " bestmovefirst " << percentage(
                      diagnostics.best_move_first, diagnostics.move_order_nodes)
                  << " maxbestmoveindex " << diagnostics.max_best_move_index
                  << " firstmovecutoffs " << percentage(
                      diagnostics.first_move_beta_cutoffs, diagnostics.beta_cutoffs)
                  << " ttfill deferred";
#endif
        std::cout << '\n';
        std::cout.flush();

        board.make_move(best);
    }

#if ENABLE_QSEARCH_DIAGNOSTICS
    accumulate_tt_snapshot(total_diagnostics, white.get_search_diagnostics(true));
    accumulate_tt_snapshot(total_diagnostics, black.get_search_diagnostics(true));
#endif

    const uint64_t nps = total_time_ms > 0
        ? total_nodes * 1000ULL / total_time_ms
        : 0;
    std::cout << "info string bench game total plies " << plies_played
              << " termination " << termination
              << " time " << total_time_ms
              << " nodes " << total_nodes
              << " nps " << nps << '\n';
    std::cout << "Nodes searched: " << total_nodes << '\n';
    std::cout << "Nodes/second: " << nps << '\n';
#if ENABLE_QSEARCH_DIAGNOSTICS
    print_search_diagnostics_summary(total_diagnostics, "", true);
#endif
    std::cout.flush();
    return 0;
}
