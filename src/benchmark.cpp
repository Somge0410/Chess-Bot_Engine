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

void print_tt_diagnostics(const char* mode_name, const TTDiagnostics& diagnostics) {
    if (diagnostics.probes == 0 && diagnostics.stores == 0) {
        std::cout << "TT " << mode_name << ": inactive (no probes or stores)\n";
        return;
    }

    const uint64_t usable_hits = diagnostics.exact_hits + diagnostics.bound_cutoffs;
    std::cout << "TT " << mode_name << " probes: " << diagnostics.probes << '\n';
    std::cout << "TT " << mode_name << " raw key hits: " << diagnostics.key_hits
              << " (" << percentage(diagnostics.key_hits, diagnostics.probes) << "%)\n";
    std::cout << "TT " << mode_name << " usable hits: " << usable_hits
              << " (" << percentage(usable_hits, diagnostics.probes) << "%)\n";
    std::cout << "TT " << mode_name << " exact hits: " << diagnostics.exact_hits
              << " (" << percentage(diagnostics.exact_hits, diagnostics.probes) << "%)\n";
    std::cout << "TT " << mode_name << " bound hits: " << diagnostics.bound_hits
              << " (" << percentage(diagnostics.bound_hits, diagnostics.probes) << "%)\n";
    std::cout << "TT " << mode_name << " bound cutoffs: " << diagnostics.bound_cutoffs
              << " (" << percentage(diagnostics.bound_cutoffs, diagnostics.probes) << "%)\n";
    std::cout << "TT " << mode_name << " shallow score rejections: " << diagnostics.shallow_hits
              << " (" << percentage(diagnostics.shallow_hits, diagnostics.key_hits) << "% of key hits)\n";
    std::cout << "TT " << mode_name << " tempered rejections: " << diagnostics.tempered_rejections
              << " (" << percentage(diagnostics.tempered_rejections, diagnostics.key_hits) << "% of key hits)\n";
    std::cout << "TT " << mode_name << " invalid-move rejections: " << diagnostics.invalid_move_rejections
              << " (" << percentage(diagnostics.invalid_move_rejections, diagnostics.key_hits) << "% of key hits)\n";
    std::cout << "TT " << mode_name << " empty terminations: " << diagnostics.empty_terminations
              << " (" << percentage(diagnostics.empty_terminations, diagnostics.probes) << "% of probes)\n";
    const double average_slots = diagnostics.probes > 0
        ? static_cast<double>(diagnostics.slots_examined) / static_cast<double>(diagnostics.probes)
        : 0.0;
    std::cout << "TT " << mode_name << " average slots examined: " << average_slots << '\n';

    std::cout << "TT " << mode_name << " stores: " << diagnostics.stores << '\n';
    std::cout << "TT " << mode_name << " store flags exact/lower/upper/tempered: "
              << diagnostics.exact_stores << '/' << diagnostics.lowerbound_stores << '/'
              << diagnostics.upperbound_stores << '/' << diagnostics.tempered_stores << '\n';
    std::cout << "TT " << mode_name << " same-key updates: " << diagnostics.same_key_updates
              << " (" << percentage(diagnostics.same_key_updates, diagnostics.stores) << "%)\n";
    std::cout << "TT " << mode_name << " deeper entries kept: " << diagnostics.deeper_entries_kept
              << " (" << percentage(diagnostics.deeper_entries_kept, diagnostics.stores) << "%)\n";
    std::cout << "TT " << mode_name << " empty inserts: " << diagnostics.empty_inserts
              << " (" << percentage(diagnostics.empty_inserts, diagnostics.stores) << "%)\n";
    std::cout << "TT " << mode_name << " replacements: " << diagnostics.replacements
              << " (" << percentage(diagnostics.replacements, diagnostics.stores) << "%)\n";
    std::cout << "TT " << mode_name << " dropped stores: " << diagnostics.dropped_stores
              << " (" << percentage(diagnostics.dropped_stores, diagnostics.stores) << "%)\n";

    if (diagnostics.replacements > 0) {
        const double average_replaced_depth = static_cast<double>(diagnostics.replaced_depth_sum)
            / static_cast<double>(diagnostics.replacements);
        const double average_replacement_depth = static_cast<double>(diagnostics.replacement_depth_sum)
            / static_cast<double>(diagnostics.replacements);
        const double average_replaced_age = static_cast<double>(diagnostics.replaced_age_sum)
            / static_cast<double>(diagnostics.replacements);
        std::cout << "TT " << mode_name << " replacement avg old/new depth: "
                  << average_replaced_depth << '/' << average_replacement_depth << '\n';
        std::cout << "TT " << mode_name << " replacement average age: "
                  << average_replaced_age << '\n';
    }
}
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
#if ENABLE_QSEARCH_DIAGNOSTICS
    const double qnode_percentage = total_nodes > 0
        ? 100.0 * static_cast<double>(total_diagnostics.qnodes) / static_cast<double>(total_nodes)
        : 0.0;
    const double average_qply = total_diagnostics.qnodes > 0
        ? static_cast<double>(total_diagnostics.qply_sum) / static_cast<double>(total_diagnostics.qnodes)
        : 0.0;
    const double average_moves_searched = total_diagnostics.move_order_nodes > 0
        ? static_cast<double>(total_diagnostics.moves_searched_sum)
            / static_cast<double>(total_diagnostics.move_order_nodes)
        : 0.0;
    const double average_best_move_index = total_diagnostics.move_order_nodes > 0
        ? static_cast<double>(total_diagnostics.best_move_index_sum)
            / static_cast<double>(total_diagnostics.move_order_nodes)
        : 0.0;
    const double average_beta_cutoff_index = total_diagnostics.beta_cutoffs > 0
        ? static_cast<double>(total_diagnostics.beta_cutoff_index_sum)
            / static_cast<double>(total_diagnostics.beta_cutoffs)
        : 0.0;
#endif

    std::cout << "info string bench total positions " << positions.size()
              << " depth " << depth
              << " time " << total_time_ms
              << " nodes " << total_nodes
              << " nps " << nps << "\n";
    std::cout << "Nodes searched: " << total_nodes << "\n";
    std::cout << "Nodes/second: " << nps << '\n';
#if ENABLE_QSEARCH_DIAGNOSTICS
    std::cout << "Main nodes: " << total_diagnostics.main_nodes << '\n';
    std::cout << "QNodes: " << total_diagnostics.qnodes << '\n';
    std::cout << "QNode percentage: " << std::fixed << std::setprecision(2)
              << qnode_percentage << "%\n";
    std::cout << "Maximum qply: " << total_diagnostics.max_qply << '\n';
    std::cout << "Average qply: " << average_qply << '\n';
    std::cout << "Quiet checks searched: " << total_diagnostics.quiet_checks_searched << '\n';
    std::cout << "QNodes while in check: " << total_diagnostics.qnodes_in_check << '\n';
    std::cout << "Cycle cutoffs: " << total_diagnostics.cycle_cutoffs << '\n';
    std::cout << "Hard-cap hits: " << total_diagnostics.hard_cap_hits << '\n';
    std::cout << "Move-order nodes: " << total_diagnostics.move_order_nodes << '\n';
    std::cout << "Average moves searched per node: " << average_moves_searched << '\n';
    std::cout << "Average best-move discovery index: " << average_best_move_index << '\n';
    std::cout << "Best move found first: "
              << percentage(total_diagnostics.best_move_first,
                  total_diagnostics.move_order_nodes) << "%\n";
    std::cout << "Maximum best-move discovery index: "
              << total_diagnostics.max_best_move_index << '\n';
    std::cout << "Average beta-cutoff index: " << average_beta_cutoff_index << '\n';
    std::cout << "Beta cutoffs on first move: "
              << percentage(total_diagnostics.first_move_beta_cutoffs,
                  total_diagnostics.beta_cutoffs) << "%\n";
    std::cout << "TT occupied entries: " << total_diagnostics.tt_occupied_entries
              << '/' << total_diagnostics.tt_capacity_entries << '\n';
    std::cout << "TT fill rate: "
              << percentage(total_diagnostics.tt_occupied_entries,
                  total_diagnostics.tt_capacity_entries) << "%\n";
    std::cout << "TT current-generation fill rate: "
              << percentage(total_diagnostics.tt_current_generation_entries,
                  total_diagnostics.tt_capacity_entries) << "%\n";
    print_tt_diagnostics("Negamax", total_diagnostics.tt[static_cast<std::size_t>(TTMode::Negamax)]);
    print_tt_diagnostics("Quiescence", total_diagnostics.tt[static_cast<std::size_t>(TTMode::Quiescence)]);
#endif
    std::cout.flush();
    return 0;
}
