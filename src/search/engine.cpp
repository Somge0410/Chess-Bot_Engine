#include "engine.h"

#include <algorithm>
#include <iostream>

#include "MoveGenerator.h"
#include "move_ordering.h"
#include "uci_helpers.h"

Engine::Engine(size_t tt_size_mb)
    : thread_pool(*this), tt(tt_size_mb) {
    tls_data.clear_heuristics();
    thread_pool.start(1);
    stop_search.store(false, std::memory_order_relaxed);
}

Move Engine::search(const Position& position, const SearchLimits& limits) {
    auto tc = decide_time_control(position, limits);
    initialize_lmr_tables();
    tt.new_search();
    int use_threads = thread_pool.thread_count();
    if (tc.time_ms < 20) use_threads = 1;

    nodes.store(0, std::memory_order_relaxed);
    qnodes.store(0, std::memory_order_relaxed);
#if ENABLE_QSEARCH_DIAGNOSTICS
    qply_sum.store(0, std::memory_order_relaxed);
    quiet_checks_searched.store(0, std::memory_order_relaxed);
    qnodes_in_check.store(0, std::memory_order_relaxed);
    cycle_cutoffs.store(0, std::memory_order_relaxed);
    hard_cap_hits.store(0, std::memory_order_relaxed);
    max_qply.store(0, std::memory_order_relaxed);
    move_order_nodes.store(0, std::memory_order_relaxed);
    moves_searched_sum.store(0, std::memory_order_relaxed);
    best_move_index_sum.store(0, std::memory_order_relaxed);
    best_move_first.store(0, std::memory_order_relaxed);
    beta_cutoffs.store(0, std::memory_order_relaxed);
    beta_cutoff_index_sum.store(0, std::memory_order_relaxed);
    first_move_beta_cutoffs.store(0, std::memory_order_relaxed);
    max_best_move_index.store(0, std::memory_order_relaxed);
    for (std::atomic<uint64_t>& counter : detail_diagnostics) {
        counter.store(0, std::memory_order_relaxed);
    }
    diagnostic_iteration_nodes.fill(0);
    diagnostic_iteration_time_ms.fill(0);
    for (AtomicTTDiagnostics& diagnostics : tt_diagnostics) {
        diagnostics.reset();
    }
#endif
    tls_data.clear_counters();

	stop_search.store(false, std::memory_order_relaxed);

    Position pos = position;
	MoveList root_moves;
	MoveGenerator::generate_moves(pos, root_moves);
    if (root_moves.empty()) return Move();
    if (root_moves.size() == 1) {
        std::cout << "info depth " << 0;

        std::cout << " score cp " << 0;
        std::cout << " time " << 0
            << " nodes " << 0
            << " nps " << 0
			<< " pv " << move_to_uci(root_moves[0])
            << "\n";
        std::cout.flush();
		return root_moves[0];
    }
	sort_moves(root_moves, pos, 0, Move(), false, &tls_data);
    Move best_move_so_far = root_moves[0];
	int best_score_so_far = -MATE_SCORE;

    time_manager.start(tc.time_ms);
    thread_pool.execute(position, limits, use_threads, tc,
        best_move_so_far, best_score_so_far, &tls_data);
    tls_data.flush_counters(this, true);
	return best_move_so_far;
}

Engine::~Engine() {
    shutdown();
}

void Engine::set_threads(int count) {
    thread_pool.set_thread_count(count);
}

void Engine::shutdown() {
    stop_search.store(true, std::memory_order_relaxed);
    thread_pool.stop();
}

void Engine::resize_tt(size_t tt_size_mb) {
    stop_search.store(true, std::memory_order_relaxed);
    const int saved_threads = thread_pool.thread_count();
    thread_pool.stop();
    tt.resize(tt_size_mb);
    thread_pool.start(saved_threads);
    stop_search.store(false, std::memory_order_relaxed);
}

void Engine::flush_node_counters() {
    tls_data.flush_counters(this, true);
}
uint64_t Engine::get_total_nodes() {
    flush_node_counters();
    return nodes.load(std::memory_order_relaxed) + qnodes.load(std::memory_order_relaxed);
}
uint64_t Engine::get_qnodes() {
	flush_node_counters();
	return qnodes.load(std::memory_order_relaxed);
}
#if ENABLE_QSEARCH_DIAGNOSTICS
SearchDiagnostics Engine::get_search_diagnostics(bool include_tt_occupancy) {
    flush_node_counters();
    SearchDiagnostics diagnostics{};
    diagnostics.main_nodes = nodes.load(std::memory_order_relaxed);
    diagnostics.qnodes = qnodes.load(std::memory_order_relaxed);
    diagnostics.qply_sum = qply_sum.load(std::memory_order_relaxed);
    diagnostics.quiet_checks_searched = quiet_checks_searched.load(std::memory_order_relaxed);
    diagnostics.qnodes_in_check = qnodes_in_check.load(std::memory_order_relaxed);
    diagnostics.cycle_cutoffs = cycle_cutoffs.load(std::memory_order_relaxed);
    diagnostics.hard_cap_hits = hard_cap_hits.load(std::memory_order_relaxed);
    diagnostics.max_qply = max_qply.load(std::memory_order_relaxed);
    diagnostics.move_order_nodes = move_order_nodes.load(std::memory_order_relaxed);
    diagnostics.moves_searched_sum = moves_searched_sum.load(std::memory_order_relaxed);
    diagnostics.best_move_index_sum = best_move_index_sum.load(std::memory_order_relaxed);
    diagnostics.best_move_first = best_move_first.load(std::memory_order_relaxed);
    diagnostics.beta_cutoffs = beta_cutoffs.load(std::memory_order_relaxed);
    diagnostics.beta_cutoff_index_sum = beta_cutoff_index_sum.load(std::memory_order_relaxed);
    diagnostics.first_move_beta_cutoffs = first_move_beta_cutoffs.load(std::memory_order_relaxed);
    diagnostics.max_best_move_index = max_best_move_index.load(std::memory_order_relaxed);
    for (std::size_t i = 0; i < SEARCH_DIAG_COUNTER_COUNT; ++i) {
        diagnostics.detail[i] = detail_diagnostics[i].load(std::memory_order_relaxed);
    }
    diagnostics.iteration_nodes = diagnostic_iteration_nodes;
    diagnostics.iteration_time_ms = diagnostic_iteration_time_ms;
    for (std::size_t i = 0; i < TT_DIAGNOSTIC_MODE_COUNT; ++i) {
        diagnostics.tt[i] = tt_diagnostics[i].snapshot();
    }
    if (include_tt_occupancy) {
        const TTOccupancy occupancy = tt.occupancy();
        diagnostics.tt_capacity_entries = occupancy.capacity_entries;
        diagnostics.tt_occupied_entries = occupancy.occupied_entries;
        diagnostics.tt_current_generation_entries = occupancy.current_generation_entries;
        diagnostics.tt_cluster_occupancy = occupancy.cluster_occupancy;
    }
    return diagnostics;
}
#endif
