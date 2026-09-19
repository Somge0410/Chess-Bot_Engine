#include "engine.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>

#include "MoveGenerator.h"
#include "evaluation.h"
#include "exceptions.h"
#include "move_ordering.h"
#include "search_parameters.h"
#include "uci_helpers.h"

thread_local ThreadLocalData tls_data;

void ThreadLocalData::flush_counters(Engine* engine,bool force) {
    if (force || nodes > 10000) {
        engine->nodes.fetch_add(nodes, std::memory_order_relaxed);
        nodes = 0;
    }
    if (force || qnodes > 10000) {
        engine->qnodes.fetch_add(qnodes, std::memory_order_relaxed);
        qnodes = 0;

#if ENABLE_QSEARCH_DIAGNOSTICS
        engine->qply_sum.fetch_add(qply_sum, std::memory_order_relaxed);
        engine->quiet_checks_searched.fetch_add(quiet_checks_searched, std::memory_order_relaxed);
        engine->qnodes_in_check.fetch_add(qnodes_in_check, std::memory_order_relaxed);
        engine->cycle_cutoffs.fetch_add(cycle_cutoffs, std::memory_order_relaxed);
        engine->hard_cap_hits.fetch_add(hard_cap_hits, std::memory_order_relaxed);
        engine->move_order_nodes.fetch_add(move_order_nodes, std::memory_order_relaxed);
        engine->moves_searched_sum.fetch_add(moves_searched_sum, std::memory_order_relaxed);
        engine->best_move_index_sum.fetch_add(best_move_index_sum, std::memory_order_relaxed);
        engine->best_move_first.fetch_add(best_move_first, std::memory_order_relaxed);
        engine->beta_cutoffs.fetch_add(beta_cutoffs, std::memory_order_relaxed);
        engine->beta_cutoff_index_sum.fetch_add(beta_cutoff_index_sum, std::memory_order_relaxed);
        engine->first_move_beta_cutoffs.fetch_add(first_move_beta_cutoffs, std::memory_order_relaxed);

        uint32_t observed_max = engine->max_qply.load(std::memory_order_relaxed);
        while (observed_max < max_qply &&
            !engine->max_qply.compare_exchange_weak(
                observed_max, max_qply,
                std::memory_order_relaxed,
                std::memory_order_relaxed)) {
        }

        observed_max = engine->max_best_move_index.load(std::memory_order_relaxed);
        while (observed_max < max_best_move_index &&
            !engine->max_best_move_index.compare_exchange_weak(
                observed_max, max_best_move_index,
                std::memory_order_relaxed,
                std::memory_order_relaxed)) {
        }

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

        for (std::size_t i = 0; i < TT_DIAGNOSTIC_MODE_COUNT; ++i) {
            engine->tt_diagnostics[i].add(tt_diagnostics[i]);
            tt_diagnostics[i] = {};
        }
        for (std::size_t i = 0; i < SEARCH_DIAG_COUNTER_COUNT; ++i) {
            engine->detail_diagnostics[i].fetch_add(detail_diagnostics[i], std::memory_order_relaxed);
            detail_diagnostics[i] = 0;
        }
#endif
    }
}

SearchResult Engine::negamax(Position& pos, int depth, int alpha, int beta, int ply, ThreadLocalData* tls,
    const Move& previous_move, uint64_t checkers, bool null_move_allowed) {
    if (stop_search.load(std::memory_order_relaxed)) {
        return { .score = 0,.best_move = Move(),.is_tempered = true };
    }
    if (tls && depth > 0) {
        tls->nodes++;
        tls->flush_counters(this);
        if (tls->should_check_time() && time_manager.is_time_up()) {
            stop_search.store(true, std::memory_order_relaxed);
            return { .score = 0,.best_move = Move(),.is_tempered = true };
        }
    }
#if ENABLE_QSEARCH_DIAGNOSTICS
    if (pos.is_fifty_move_rule_draw()) {
        increment_diagnostic(tls, SearchDiagCounter::SearchFiftyMoveDraws);
        return { .score = 0,.best_move = Move(),.is_tempered = true };
    }
    if (pos.is_repetition_draw(3)) {
        increment_diagnostic(tls, SearchDiagCounter::SearchRepetitionDraws);
        return { .score = 0,.best_move = Move(),.is_tempered = true };
    }
#else
    if (pos.is_fifty_move_rule_draw() || pos.is_repetition_draw(3)) {
        return { .score = 0,.best_move = Move(),.is_tempered = true };
    }
#endif
    uint64_t hash=pos.get_hash();
    int original_alpha=alpha;
    int tt_score;
    Move tt_move;

    bool is_from_depth_0 = false;
#if ENABLE_QSEARCH_DIAGNOSTICS
    if (checkers == CHECKERS_UNKNOWN) {
        checkers = pos.get_checkers();
    }
    tls->current_tt_probe_in_check = checkers != 0;
    tls->last_tt_probe_was_shallow = false;
#endif
    if (tt.probe(hash, depth, alpha, beta, tt_score, tt_move, ply, is_from_depth_0)) {
        bool is_draw = move_could_result_in_repetition(pos, tt_move);
        //is_draw = false;
        if (!is_draw) {
            recover_move_fully(tt_move, pos);
            return { tt_score,tt_move};
        }
#if ENABLE_QSEARCH_DIAGNOSTICS
        increment_diagnostic(tls, SearchDiagCounter::TTRepetitionRejectedReturns);
#endif
    }
    
    if (depth==0)
    {
        int q_score=quiescence_search(pos, alpha, beta, ply, 0, tls, checkers);
        
        return {q_score,Move()};
    }

    if (checkers == CHECKERS_UNKNOWN) {
        checkers = pos.get_checkers();
    }
    bool king_is_in_check = checkers != 0;
	int static_eval = -MATE_SCORE;
    //REVERSE FUTILITY PRUNING
    // 

    bool is_pv_node = (beta - alpha) > 1;
#if ENABLE_QSEARCH_DIAGNOSTICS
    increment_diagnostic(tls, is_pv_node ? SearchDiagCounter::PvNodes : SearchDiagCounter::NonPvNodes);
    if (king_is_in_check) increment_diagnostic(tls, SearchDiagCounter::InCheckNodes);
#endif
    const bool nmp_candidate = null_move_allowed && !is_pv_node && depth >= NMP_MIN_DEPTH && !king_is_in_check &&
        std::abs(beta) < MATE_THRESHOLD && pos.has_enough_material_for_nmp();
#if ENABLE_QSEARCH_DIAGNOSTICS
    if (nmp_candidate) increment_diagnostic(tls, SearchDiagCounter::NmpCandidates);
#endif
    if (!king_is_in_check && ((depth <= REVERSE_FUTILITY_MAX_DEPTH && !is_pv_node) || nmp_candidate)) {
        static_eval = pos.is_white_to_move() ? evaluate(pos, nullptr, EVAL_MATERIAL | EVAL_POSITIONAL | EVAL_PAWN_STRUCTURE) : -evaluate(pos, nullptr, EVAL_MATERIAL | EVAL_POSITIONAL | EVAL_PAWN_STRUCTURE);
    }
    if (!king_is_in_check && depth <= REVERSE_FUTILITY_MAX_DEPTH && std::abs(beta) < MATE_THRESHOLD && !is_pv_node) {
#if ENABLE_QSEARCH_DIAGNOSTICS
        increment_diagnostic(tls, SearchDiagCounter::RfpAttempts);
#endif
        int rfp_margin = REVERSE_FUTILITY_MARGIN * depth; // This margin can be tuned
        if (static_eval - rfp_margin >= beta) {
#if ENABLE_QSEARCH_DIAGNOSTICS
            increment_diagnostic(tls, SearchDiagCounter::RfpCutoffs);
#endif
            return { static_eval,Move() };
        }
    }
   
    
    // NULL Move Pruning Here
    int nmp_score;
    if(nmp_candidate && static_eval >= beta
#if ENABLE_QSEARCH_DIAGNOSTICS
        && (increment_diagnostic(tls, SearchDiagCounter::NmpSearches), true)
#endif
        && try_null_move_pruning(pos,king_is_in_check,depth,alpha,beta,ply,static_eval,nmp_score,tls))
    {
#if ENABLE_QSEARCH_DIAGNOSTICS
        increment_diagnostic(tls, SearchDiagCounter::NmpCutoffs);
#endif
        return {nmp_score,Move()};
	}
    // End of Null-move pruning
	//Generate moves
	MoveList& moves = tls->move_lists[ply];
	MoveList& searched_quiets = tls->searched_quiets[ply];
    moves.clear();
	searched_quiets.clear();
    MoveGenerator::generate_moves(pos,moves,checkers);
#if ENABLE_QSEARCH_DIAGNOSTICS
    increment_diagnostic(tls, SearchDiagCounter::MainMovesGenerated, moves.size());
    increment_diagnostic(tls, is_pv_node ? SearchDiagCounter::PvMovesGenerated : SearchDiagCounter::NonPvMovesGenerated,
        moves.size());
    if (king_is_in_check) increment_diagnostic(tls, SearchDiagCounter::InCheckMovesGenerated, moves.size());
#endif
	//If only one move available, no need to search further
    if ((ply == 0) && (moves.size() == 1)) {
        return { 0,moves[0] };
    }
    //Sort moves
    //sort_moves(moves,pos,ply,tt_move);
    int best_score=-MATE_SCORE;
    Move best_move;
	//If no moves available, check for checkmate or stalemate
    if (moves.empty())
    {
		return terminal_eval(pos, king_is_in_check,ply);
    }
    

    //Futility Purning prerequisites here Here
    int current_eval=-MATE_SCORE;
    if (depth<=2)
	{
        if (static_eval != -MATE_SCORE)
            current_eval = static_eval;
        else
            current_eval = pos.is_white_to_move() ? evaluate(pos, nullptr, EVAL_MATERIAL | EVAL_POSITIONAL | EVAL_PAWN_STRUCTURE) : -evaluate(pos,nullptr,EVAL_MATERIAL | EVAL_POSITIONAL | EVAL_PAWN_STRUCTURE);
    }
    
    // Late Move Reduction prerequisites here
    int moves_searched=0;
#if ENABLE_QSEARCH_DIAGNOSTICS
    uint32_t best_move_discovery_index = 0;
    uint32_t beta_cutoff_index = 0;
    MoveOrderSource best_move_source = MoveOrderSource::Unknown;
    MoveOrderSource beta_cutoff_source = MoveOrderSource::Unknown;
    const bool shallow_tt_move = tls->last_tt_probe_was_shallow &&
        tt_move.from_square != NO_SQUARE;
#endif
    // PVS 
    bool first = true;
    bool is_any_tempered = false;
    bool is_best_move_tempered = false;
    bool current_move_tempered = false;
    int* scores = tls->move_scores[ply];
	score_moves(moves, scores, ply, tt_move, is_from_depth_0,pos,tls,previous_move,true);
    for (int i=0;i<(int)moves.size();++i)
    {   
		pick_next_staged(moves, scores, i, pos);
		const Move move = moves[i];
#if ENABLE_QSEARCH_DIAGNOSTICS
        const MoveOrderSource current_move_source = classify_move_order_source(
            move, tt_move, is_from_depth_0, pos, tls, previous_move, ply);
        if (shallow_tt_move && i == 0 && move == tt_move) {
            increment_diagnostic(tls, SearchDiagCounter::ShallowTTMoveFirst);
        }
#endif
        // Now do futility pruning. If positions evaluation is already way worse than alpha, cut it off since it is
        //unlikely to get that much better in just 1 or two moves
        if(!first
#if ENABLE_QSEARCH_DIAGNOSTICS
            && (increment_diagnostic(tls, SearchDiagCounter::FutilityChecks), true)
#endif
            && should_futility_prune(depth,current_eval,alpha,king_is_in_check,move))
        {
#if ENABLE_QSEARCH_DIAGNOSTICS
            increment_diagnostic(tls, SearchDiagCounter::FutilityPrunes);
#endif
            continue;
		}
        // Late Move Reduction
        int reduction = 0;
        if (!is_dangerous_passer_push(pos, move)) {
            reduction = late_move_reduction(depth, moves_searched, move, ply, tls, previous_move);
        }
#if ENABLE_QSEARCH_DIAGNOSTICS
        if (reduction > 0) increment_diagnostic(tls, SearchDiagCounter::LmrReductions);
#endif
        moves_searched++;
        //Now make the move
        pos.make_move(move);

		//If in check, we should increase depth by 1
        int extension = 0;
        const uint64_t child_checkers = pos.get_checkers();
        if (child_checkers != 0 && ply<64 && depth<=3)
        {
            extension=1;
#if ENABLE_QSEARCH_DIAGNOSTICS
            increment_diagnostic(tls, SearchDiagCounter::CheckExtensions);
#endif
		}
        int evaluation;
        if (first) { 
			SearchResult first_result = negamax(pos, depth - 1 + extension, -beta, -alpha, ply + 1,tls,move,child_checkers);
            evaluation = -first_result.score;
            current_move_tempered = first_result.is_tempered;
			first = false;
        }
        else {
#if ENABLE_QSEARCH_DIAGNOSTICS
            increment_diagnostic(tls, SearchDiagCounter::PvsZeroWindowSearches);
#endif
            int new_depth = depth - 1 + extension;
			int reduced_depth = new_depth - reduction;

            SearchResult other_result = negamax(pos, reduced_depth, -alpha - 1, -alpha, ply + 1,tls,move,child_checkers);
            evaluation = -other_result.score;
            current_move_tempered = other_result.is_tempered;

            if (reduction > 0 && evaluation > alpha) {
#if ENABLE_QSEARCH_DIAGNOSTICS
                increment_diagnostic(tls, SearchDiagCounter::LmrResearches);
#endif
                other_result = negamax(pos, new_depth, -alpha - 1, -alpha, ply + 1,tls,move,child_checkers);
                evaluation = -other_result.score;
				current_move_tempered = other_result.is_tempered;
#if ENABLE_QSEARCH_DIAGNOSTICS
                if (evaluation > alpha) increment_diagnostic(tls, SearchDiagCounter::LmrResearchImproved);
#endif
            }

            if(evaluation > alpha && evaluation < beta) {
#if ENABLE_QSEARCH_DIAGNOSTICS
                increment_diagnostic(tls, SearchDiagCounter::PvsFullWindowResearches);
#endif
                 other_result = negamax(pos, new_depth, -beta, -alpha, ply + 1,tls,move,child_checkers);
				 evaluation = -other_result.score;
				current_move_tempered = other_result.is_tempered;
            }
        }
        is_any_tempered |= current_move_tempered;
        pos.undo_move();
		bool quiet = move.piece_captured == PieceType::None && move.promotion_piece == PieceType::None;

        if (quiet) {
			searched_quiets.push_back(move);
        }
        
        if (stop_search.load(std::memory_order_relaxed))
        {   // Better: Best Move so far??
            return{0,Move(),true};
        }
        if (evaluation > best_score)
        {
            best_score = evaluation;
            best_move = move;
            is_best_move_tempered = current_move_tempered;
#if ENABLE_QSEARCH_DIAGNOSTICS
            best_move_discovery_index = static_cast<uint32_t>(moves_searched);
            best_move_source = current_move_source;
#endif
        }
        alpha = std::max(alpha, best_score);
       
            
        

        if (beta<=alpha)
        {   
#if ENABLE_QSEARCH_DIAGNOSTICS
            beta_cutoff_index = static_cast<uint32_t>(moves_searched);
            beta_cutoff_source = current_move_source;
#endif
			update_history_killer(move, depth, ply,tls,previous_move,searched_quiets);
            break;
        }
        
    }

#if ENABLE_QSEARCH_DIAGNOSTICS
    increment_diagnostic(tls, SearchDiagCounter::MainMovesSearched, moves_searched);
    increment_diagnostic(tls, is_pv_node ? SearchDiagCounter::PvMovesSearched : SearchDiagCounter::NonPvMovesSearched,
        moves_searched);
    if (king_is_in_check) increment_diagnostic(tls, SearchDiagCounter::InCheckMovesSearched, moves_searched);
    if (shallow_tt_move && best_move == tt_move) {
        increment_diagnostic(tls, SearchDiagCounter::ShallowTTMoveBest);
        if (beta_cutoff_index > 0) increment_diagnostic(tls, SearchDiagCounter::ShallowTTMoveCutoff);
    }
    record_move_order_diagnostics(tls, static_cast<uint32_t>(moves_searched),
        best_move_discovery_index, beta_cutoff_index, best_move_source, beta_cutoff_source);
#endif
    bool is_result_tempered = tt.store(hash, depth, original_alpha, beta, best_score,
        best_move, ply, is_best_move_tempered, is_any_tempered);
    return {best_score,best_move,is_result_tempered};
}

bool Engine::should_futility_prune(int depth, int eval, int alpha, bool in_check,const Move& move) {
	if (depth > 3) return false;
    bool is_quiet = move.piece_captured == PieceType::None && move.promotion_piece == PieceType::None;
    if (in_check || !is_quiet) return false;
    if (depth == 1 && eval + FUTILITY_MARGIN_D1 <= alpha) return true;
    if (depth == 2 && eval + FUTILITY_MARGIN_D2 <= alpha) return true;
    return false;
}
int Engine::late_move_reduction(int depth, int moves_searched, const Move& move, int ply, ThreadLocalData* tls, const Move& previous_move) {
    if (depth >= 64 || moves_searched >= 218) return 7;
    if (depth < LMR_MIN_DEPTH || moves_searched +1<LMR_FIRST_REDUCED_MOVE) return 0; // No reduction for the first move
    bool is_killer = (ply > 0 && (move == tls->killer_moves[ply][0] || move == tls->killer_moves[ply][1]));
    if (is_killer) return 0;
    bool is_quiet = move.is_quiet();
    if (is_quiet) {
        return quiet_lmr[depth][moves_searched];
    }
    else {
        return tactical_lmr[depth][moves_searched];
    }
}
void Engine::initialize_lmr_tables() {
    for (int depth = 0; depth < LMR_DEPTH_COUNT; ++depth) {
        for (int move_count = 0; move_count < LMR_MOVE_COUNT; ++move_count) {
            if (depth <= 1 || move_count <= 1) {
                quiet_lmr[depth][move_count] = 0;
                tactical_lmr[depth][move_count] = 0;
                continue;
            }
            quiet_lmr[depth][move_count] = static_cast<uint8_t>(std::clamp(
                static_cast<int>(Q_LOG_BASE + std::log(depth) * std::log(move_count) / Q_LOG_DIV), 0, depth - 1));
            tactical_lmr[depth][move_count] = static_cast<uint8_t>(std::clamp(
                static_cast<int>(LOG_BASE + std::log(depth) * std::log(move_count) / LOG_DIV), 0, depth - 1));
        }
    }
}
bool Engine::try_null_move_pruning(Position& pos, bool king_is_in_check, int depth, int alpha, int beta, int ply,
    int static_eval, int& out_score,ThreadLocalData* tls) {
	bool is_mate_score_possible = (alpha >= MATE_THRESHOLD || beta <= -MATE_THRESHOLD);

    if(is_mate_score_possible|| depth < NMP_MIN_DEPTH || king_is_in_check || !pos.has_enough_material_for_nmp()) {
        return false;
	}
	const int eval_reduction = std::clamp((static_eval - beta) / std::max(1, NMP_EVAL_DIVISOR),
        0, NMP_MAX_EVAL_REDUCTION);
	const int reduction = NMP_REDUCTION + depth / std::max(1, NMP_DEPTH_DIVISOR) + eval_reduction;
	const int null_depth = std::max(0, depth - 1 - reduction);
	Square original_ep_square = pos.make_null_move();
	int null_move_score = negamax(pos, null_depth, -beta, -beta + 1, ply + 1,tls,
        Move(), CHECKERS_UNKNOWN, false).score;
	null_move_score = -null_move_score;
	pos.undo_null_move(original_ep_square);
    if (stop_search.load(std::memory_order_relaxed)) {
        out_score = 0;
        return true;
    }
    if (null_move_score >= beta) {
        out_score = beta;
        return true;
    }
	return false;
}
SearchResult Engine::terminal_eval(const Position& pos, bool king_is_in_check,int ply) {
    if (king_is_in_check) {
#if ENABLE_QSEARCH_DIAGNOSTICS
        increment_diagnostic(&tls_data, SearchDiagCounter::TerminalCheckmates);
#endif
		return { -MATE_SCORE+ply, Move() };
    }
    else {
#if ENABLE_QSEARCH_DIAGNOSTICS
        increment_diagnostic(&tls_data, SearchDiagCounter::TerminalStalemates);
#endif
        return { 0,Move() };
    }
}

bool Engine::move_could_result_in_repetition(Position& pos, Move& move, int count) {
    if (move.piece_captured != PieceType::None || move.piece_moved == PieceType::Pawn || move.is_castle) return false;
    return pos.has_twofold();
}
void Engine::recover_move_fully(Move& move,const Position& pos) {
    move.move_color = pos.get_turn();
    move.piece_moved = pos.get_piece_type_on_square(move.from_square);
    move.piece_captured = pos.get_piece_type_on_square(move.to_square);
	int abs = std::abs(move.to_square - move.from_square);
	move.is_castle = move.piece_moved == PieceType::King && abs == 2;
    move.is_en_passant = move.piece_moved == PieceType::Pawn && move.to_square==pos.get_en_passant_rights();
}

void Engine::iterative_deepening_new(int thread_id, bool is_master, Move& io_best_move, int& io_best_score, const Position& position, TimeControlDecision& tc , ThreadLocalData* tls) {
    int start_depth = 1 + (thread_id & 1);
    uint64_t prev_total_nodes = 0;
    uint64_t prev_elapsed_ms = 0;
    uint64_t prev_iteration_nodes = 0;
    uint64_t prev_iteration_ms = 0;
    uint64_t iteration_nodes = 0;
	uint64_t iteration_ms = 0;
    double effective_branching_factor = 1;
    double time_growth = 1;
    double predicted_next_iteration_ms = 0;
    Move recent_best_moves[RECENT_BEST_MOVE_WINDOW] = { };
    int recent_best_move_count = 0;
    const int initial_budget_ms = tc.time_ms;
    const int maximum_budget = tc.max_time_ms;
    int current_budget_ms = initial_budget_ms;

    int prev_root_score = 0;
    bool has_prev_root_score = false;
    int dominant_gap_streak = 0;

    int second_best_score = -MATE_SCORE;
    for (int current_depth = start_depth; current_depth <= tc.max_depth; ++current_depth) {
#if ENABLE_QSEARCH_DIAGNOSTICS
        const Move previous_iteration_best = io_best_move;
#endif
        Position pos = position;
        MoveList root_moves;

        MoveGenerator::generate_moves(pos, root_moves);
        if (root_moves.empty()) {
            if (is_master) {
                std::cerr << "No legal moves available, stopping search.\n";
            }
            break;
        }
        if(root_moves.size()==1){
            io_best_move=root_moves[0];
            // Score: matt oder centipawns
            std::cout << "info depth " << 0;
            std::cout << " score cp " << 0;
            std::cout << " time " << 0
                << " nodes " << 0
                << " nps " << 0
                << " pv " << move_to_uci(io_best_move)
                << "\n";
            std::cout.flush();
		}

        //If we dont have a valid previous best yet, seed it so ordering is stable.

        sort_moves(root_moves, pos, 0, io_best_move, false, tls);
        perturb_root_order(root_moves, thread_id, current_depth, pos.get_zobrist_hash(),
            thread_pool.thread_count());

        //Aspiration window (per thread).
        int window = ASPIRATION_WINDOW_INITIAL;
        int alpha = -MATE_SCORE;
        int beta = MATE_SCORE;

        if (current_depth > 1) {
            alpha = io_best_score - window;
            beta = io_best_score + window;
        }
        alpha = std::max(-MATE_SCORE, alpha);
        beta = std::min(MATE_SCORE, beta);
        int best_score = -MATE_SCORE;
        Move second_best_move = Move();
        Move best_move = root_moves[0];
        //Retry loop for aspiration failures: re-search the whole root with a wider window.
        for (int attempt = 0; attempt < 4; ++attempt) {
#if ENABLE_QSEARCH_DIAGNOSTICS
            if (current_depth > 1) increment_diagnostic(tls, SearchDiagCounter::AspirationAttempts);
#endif
            root_pvs(position, root_moves, current_depth, alpha, beta, best_score, best_move,second_best_score,second_best_move, tls);
            if (stop_search.load(std::memory_order_relaxed)) break;

            if (current_depth == 1) break; // no aspiration on depth 1

            if (best_score <= alpha || best_score >= beta) {
#if ENABLE_QSEARCH_DIAGNOSTICS
                increment_diagnostic(tls, best_score <= alpha
                    ? SearchDiagCounter::AspirationFailLows
                    : SearchDiagCounter::AspirationFailHighs);
#endif

                //WIden around the reported score and try again.
                window = std::min(static_cast<int>(window * ASPIRATION_WINDOW_MULTIPLIER), MATE_SCORE);
                alpha = std::max(-MATE_SCORE, best_score - window);
                beta = std::min(MATE_SCORE, best_score + window);

                //Put the current best move first to speed up re-search.
                sort_moves(root_moves, pos, 0, best_move, false, tls);
                continue;
            }
            //Inside the window-> done.
            break;
        }
        if (!stop_search.load(std::memory_order_relaxed)) {
            io_best_move = best_move;
            io_best_score = best_score;
        }
        else {
            break;
        }

        // --- UCI info output (nur Master-Thread, auf stdout) ---
        if (is_master) {
#if ENABLE_QSEARCH_DIAGNOSTICS
            increment_diagnostic(tls, SearchDiagCounter::IterationsCompleted);
            if (current_depth > start_depth && best_move != previous_iteration_best) {
                increment_diagnostic(tls, SearchDiagCounter::BestMoveChanges);
            }
#endif
            const auto now_tp = std::chrono::steady_clock::now();
            const int64_t now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                now_tp.time_since_epoch()
            ).count();

            auto elapsed = now_tp - time_manager.start_time();

            recent_best_moves[recent_best_move_count % RECENT_BEST_MOVE_WINDOW] = best_move;
            recent_best_move_count++;

            if (recent_best_move_count >= RECENT_BEST_MOVE_WINDOW && current_budget_ms < maximum_budget) {
                int changes = 0;
                int consecutive_changes = 0;
                bool has_two_consecutive_changes = false;
                const int start = recent_best_move_count - RECENT_BEST_MOVE_WINDOW;
                for (int i = 0; i + 1 < RECENT_BEST_MOVE_WINDOW; ++i) {
                    const Move& a = recent_best_moves[(start + i) % RECENT_BEST_MOVE_WINDOW];
                    const Move& b = recent_best_moves[(start + i + 1) % RECENT_BEST_MOVE_WINDOW];
                    if (a != b) {
                        changes++;
                        consecutive_changes++;
                        if (consecutive_changes >= 2) {
                            has_two_consecutive_changes = true;
                        }
                    }
                    else {
                        consecutive_changes = 0;
                    }
                }
				const bool unstable = has_two_consecutive_changes || changes >= 3;
                    if (unstable) {
                        int gap = std::max(0, maximum_budget-current_budget_ms);
                        double extra_fraction = 0.0;
                        if (has_two_consecutive_changes) {
							extra_fraction = (changes >= 3) ? TIME_CHANGES_COUNT_BIG : TIME_CHANGES_COUNT_MEDIUM;
                        }
                        else {
                            extra_fraction = TIME_CHANGES_COUNT_SMALL;
                        }
                        int extra_time_ms = static_cast<int>(static_cast<double>(gap) * extra_fraction);
#if ENABLE_QSEARCH_DIAGNOSTICS
                        const int previous_budget_ms = current_budget_ms;
#endif
                        current_budget_ms = std::clamp(current_budget_ms + extra_time_ms, initial_budget_ms, maximum_budget);
#if ENABLE_QSEARCH_DIAGNOSTICS
                        const int added_ms = current_budget_ms - previous_budget_ms;
                        if (added_ms > 0) {
                            increment_diagnostic(tls, SearchDiagCounter::InstabilityTimeExtensions);
                            increment_diagnostic(tls, SearchDiagCounter::InstabilityTimeAddedMs,
                                static_cast<uint64_t>(added_ms));
                        }
#endif
                        time_manager.set_budget_ms(current_budget_ms);
                    }
            }
            if (maximum_budget > current_budget_ms && has_prev_root_score) {
                const bool curr_is_mate = std::abs(best_score) >= MATE_THRESHOLD;
                const bool prev_is_mate = std::abs(prev_root_score) >= MATE_THRESHOLD;

                if (!curr_is_mate && !prev_is_mate) {
                    const int delta_cp = std::abs(best_score - prev_root_score);
                    const bool sign_flip = (best_score > 0) != (prev_root_score > 0);
#if ENABLE_QSEARCH_DIAGNOSTICS
                    if (sign_flip) increment_diagnostic(tls, SearchDiagCounter::ScoreSignFlips);
#endif

                    // Tuning: ab ~50cp Unterschied reagieren
                    if (delta_cp >= DELTA_BEST_SCORE || sign_flip) {
                        const int gap = std::max(0, maximum_budget - current_budget_ms);

                        // sanfte Skalierung
                        const double volatility = std::clamp(static_cast<double>(delta_cp - DELTA_BEST_SCORE) / VOLATILITY_DIV, 0.0, 1.0);
                        double extra_fraction = EXTRA_BEST_BASE + EXTRA_BEST_WEIGHT * volatility;
                        if (sign_flip) {
                            extra_fraction += EXTRA_BEST_FLIP;
                        }

                        int extra_time_ms = static_cast<int>(static_cast<double>(gap) * extra_fraction);

#if ENABLE_QSEARCH_DIAGNOSTICS
                        const int previous_budget_ms = current_budget_ms;
#endif
                        current_budget_ms = std::clamp(current_budget_ms + extra_time_ms, initial_budget_ms, maximum_budget);
#if ENABLE_QSEARCH_DIAGNOSTICS
                        const int added_ms = current_budget_ms - previous_budget_ms;
                        if (added_ms > 0) {
                            increment_diagnostic(tls, SearchDiagCounter::ScoreTimeExtensions);
                            increment_diagnostic(tls, SearchDiagCounter::ScoreTimeAddedMs,
                                static_cast<uint64_t>(added_ms));
                        }
#endif
                        time_manager.set_budget_ms(current_budget_ms);
                    }
                }
            }

            uint64_t elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

            uint64_t total_nodes = get_total_nodes();
            uint64_t nps = (elapsed_ms > 0) ? (total_nodes * 1000 / elapsed_ms) : 0;

            std::string best_uci = move_to_uci(best_move);

            // Score: matt oder centipawns
            bool is_mate = std::abs(best_score) >= MATE_THRESHOLD;
            std::cout << "info depth " << current_depth;
            if (is_mate) {
                int mate_in = (best_score > 0)
                    ? (MATE_SCORE - best_score + 1) / 2
                    : -(MATE_SCORE + best_score + 1) / 2;
                std::cout << " score mate " << mate_in;
            } else {
                std::cout << " score cp " << (pos.get_turn() == Color::White ? best_score : -best_score);
            }
            std::cout << " time " << elapsed_ms
                      << " nodes " << total_nodes
                      << " nps " << nps
                      << " pv " << create_pv_string(pos, best_move, current_depth)
                      << "\n";
            std::cout.flush();
			iteration_nodes = total_nodes - prev_total_nodes;
			iteration_ms = elapsed_ms - prev_elapsed_ms;
#if ENABLE_QSEARCH_DIAGNOSTICS
            if (current_depth >= 0 && current_depth < static_cast<int>(DIAGNOSTIC_ITERATION_DEPTH_COUNT)) {
                diagnostic_iteration_nodes[static_cast<std::size_t>(current_depth)] = iteration_nodes;
                diagnostic_iteration_time_ms[static_cast<std::size_t>(current_depth)] = iteration_ms;
            }
            if (predicted_next_iteration_ms > 0.0) {
                const uint64_t predicted_ms = static_cast<uint64_t>(std::llround(predicted_next_iteration_ms));
                increment_diagnostic(tls, SearchDiagCounter::PredictionSamples);
                increment_diagnostic(tls, SearchDiagCounter::PredictedIterationMs, predicted_ms);
                increment_diagnostic(tls, SearchDiagCounter::ActualIterationMs, iteration_ms);
                increment_diagnostic(tls, SearchDiagCounter::AbsolutePredictionErrorMs,
                    predicted_ms > iteration_ms ? predicted_ms - iteration_ms : iteration_ms - predicted_ms);
            }
#endif
			if (prev_iteration_nodes > 0) {
                effective_branching_factor = static_cast<double>(iteration_nodes) / static_cast<double>(prev_iteration_nodes);
			}
            time_growth = effective_branching_factor;
            if (prev_iteration_ms > 0) {
				time_growth = static_cast<double>(iteration_ms) / static_cast<double>(prev_iteration_ms);
            }
			predicted_next_iteration_ms = static_cast<double>(iteration_ms) * time_growth;
            prev_total_nodes = total_nodes;
            prev_elapsed_ms = elapsed_ms;
            prev_iteration_nodes = iteration_nodes;
			prev_iteration_ms = iteration_ms;
            const int64_t remaining_ms = time_manager.remaining_ms(now_ns);
			prev_root_score = best_score;
			has_prev_root_score = true;
            const int64_t HARD_SAFETY_MS = 10 + tc.time_ms / 50;

            if (remaining_ms <= HARD_SAFETY_MS) {
#if ENABLE_QSEARCH_DIAGNOSTICS
                increment_diagnostic(tls, SearchDiagCounter::HardSafetyStops);
#endif
                break;
            }
            if (static_cast<double>(predicted_next_iteration_ms) * TIME_MARGIN > static_cast<double>(remaining_ms)) {
#if ENABLE_QSEARCH_DIAGNOSTICS
                increment_diagnostic(tls, SearchDiagCounter::PredictedIterationStops);
#endif
                break;
            }
        }
    }

}

void Engine::root_pvs(const Position& pos, MoveList& root_moves,
    int current_depth,
    int alpha,
    int beta,
    int& out_best_score,
    Move& out_best_move,
    int& out_second_best_score,
	Move& out_second_best_move,
    ThreadLocalData* tls) {
    int best_score = -MATE_SCORE;
	int second_best_score = -MATE_SCORE;
	Move local_second_best_move = out_second_best_move;
    Move best_move = root_moves[0];
    int local_alpha = alpha;
#if ENABLE_QSEARCH_DIAGNOSTICS
    uint32_t root_moves_searched = 0;
    uint32_t root_best_move_discovery_index = 0;
    uint32_t root_beta_cutoff_index = 0;
    increment_diagnostic(tls, SearchDiagCounter::PvNodes);
    increment_diagnostic(tls, SearchDiagCounter::MainMovesGenerated, root_moves.size());
    increment_diagnostic(tls, SearchDiagCounter::PvMovesGenerated, root_moves.size());
#endif


    Position b = pos;
    for (size_t i = 0; i < root_moves.size(); ++i) {
        if (stop_search.load(std::memory_order_relaxed)) break;

        const Move m = root_moves[i];
        b.make_move(m);
        const uint64_t child_checkers = b.get_checkers();

        SearchResult r;
        if (i == 0) {
            r = negamax(b, current_depth - 1, -beta, -local_alpha, 1, tls, m, child_checkers);
        }
		else {
#if ENABLE_QSEARCH_DIAGNOSTICS
            increment_diagnostic(tls, SearchDiagCounter::PvsZeroWindowSearches);
#endif
            r = negamax(b, current_depth - 1, -(local_alpha + 1), -local_alpha, 1, tls, m, child_checkers);
            int score = -r.score;
            if (!stop_search.load(std::memory_order_relaxed) && score > local_alpha && score < beta) {
#if ENABLE_QSEARCH_DIAGNOSTICS
                increment_diagnostic(tls, SearchDiagCounter::PvsFullWindowResearches);
#endif
                r = negamax(b, current_depth - 1, -beta, -local_alpha, 1, tls, m, child_checkers);
            }
        }

        int score = -r.score;
        b.undo_move();

        if (stop_search.load(std::memory_order_relaxed)) break;
#if ENABLE_QSEARCH_DIAGNOSTICS
        root_moves_searched = static_cast<uint32_t>(i + 1);
#endif

        if (i == 0 || score > best_score) {
            second_best_score = best_score;
            local_second_best_move = best_move;
            best_score = score;
            best_move = m;
#if ENABLE_QSEARCH_DIAGNOSTICS
            root_best_move_discovery_index = static_cast<uint32_t>(i + 1);
#endif
        } 
		else if (score > second_best_score) {
            second_best_score = score;
            local_second_best_move = m;
        }

        if (score > local_alpha) local_alpha = score;
        if (local_alpha >= beta) {
#if ENABLE_QSEARCH_DIAGNOSTICS
            root_beta_cutoff_index = static_cast<uint32_t>(i + 1);
#endif
            break;
        }
    }

#if ENABLE_QSEARCH_DIAGNOSTICS
    if (!stop_search.load(std::memory_order_relaxed)) {
        increment_diagnostic(tls, SearchDiagCounter::MainMovesSearched, root_moves_searched);
        increment_diagnostic(tls, SearchDiagCounter::PvMovesSearched, root_moves_searched);
        record_move_order_diagnostics(tls, root_moves_searched,
            root_best_move_discovery_index, root_beta_cutoff_index);
    }
#endif

    out_best_score = best_score;
    out_best_move = best_move;
    out_second_best_score = second_best_score;
    out_second_best_move = local_second_best_move;
}

std::string Engine::create_pv_string(const Position& pos, const Move& best_move, int depth) {
    std::string pv = move_to_uci(best_move);
    Position b = pos;
    b.make_move(best_move);

    // Sammle bis zu depth-1 weitere Züge aus der TT
    for (int i = 1; i < depth; ++i) {
        if (b.is_fifty_move_rule_draw() || b.is_repetition_draw(2)) {
            break;
        }
        uint64_t hash = b.get_hash();
        Move tt_move;
        int tt_score;
        bool depth_0 = false;

        // depth=0 akzeptiert jeden TT-Eintrag mit depth>=0
        if (!tt.probe(hash, 0, -MATE_SCORE, MATE_SCORE, tt_score, tt_move, i, depth_0,
            TTMode::PrincipalVariation))
            break;
        if (tt_move.from_square == NO_SQUARE || tt_move.to_square == NO_SQUARE)
            break;

        // TT speichert nur from/to/promotion  Rest muss rekonstruiert werden
        recover_move_fully(tt_move, b);

        // Prüfe ob der rekonstruierte Zug gültig ist (piece_moved darf nicht NONE sein)
        if (tt_move.piece_moved == PieceType::None)
            break;

        pv += " " + move_to_uci(tt_move);
        b.make_move(tt_move);
       
    }
    return pv;
}
