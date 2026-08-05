#include "engine.h"
#include "evaluation.h"
#include <chrono>
#include <thread>
#include <exception>
#include <iostream>
#include "notation_utils.h"
#include "MoveGenerator.h"
#include "exceptions.h"
#include "utils.h"
#include <fstream>
#include "see.h"
#include "bitboard_masks.h"
#include <atomic>
#include <bit>
#include <mutex>
#include <algorithm>
#include <cmath>
#include "uci_helpers.h"
#include "SPSA_parameters.h"
void ThreadLocalData::flush_counters(Engine* engine,bool force) {
    if (force || nodes > 10000) {
        engine->nodes.fetch_add(nodes, std::memory_order_relaxed);
        nodes = 0;
    }
    if (force || qnodes > 10000) {
        engine->qnodes.fetch_add(qnodes, std::memory_order_relaxed);
        qnodes = 0;
    }
}
static thread_local ThreadLocalData tls_data;
constexpr int PIECE_VALUES_MG[7] = {100,320,320,500,900,10000,0};

Engine::Engine(size_t tt_size_mb){
    init_tt(tt_size_mb);
    tls_data.clear_heuristics();
    //int thread_count = std::thread::hardware_concurrency();
	int thread_count = 1;
    start_thread_pool(thread_count);
    //std::cerr << "Engine initialized with threads=" << thread_count
	//	<< " TT size=" << tt_size_mb << " MB, entries=" <<  4*tt.size() << std::endl << "\n";
	stop_search.store(false, std::memory_order_relaxed);
    checks_count=0;
    ep_count=0;
    capture_count=0;
    checkmate_count=0;
	int overwrite_tt_counter = 0;
}
SearchResult Engine::negamax(Board& board, int depth, int alpha, int beta, int ply, ThreadLocalData* tls,
    const Move& previous_move, uint64_t checkers, bool null_move_allowed) {
    if (stop_search.load(std::memory_order_relaxed)) {
        return { .score = 0,.best_move = Move(),.is_tempered = true };
    }
    if (tls && depth > 0) {
        tls->nodes++;
        tls->flush_counters(this);
        if (tls->should_check_time() && is_time_up()) {
            stop_search.store(true, std::memory_order_relaxed);
            return { .score = 0,.best_move = Move(),.is_tempered = true };
        }
    }
    if (board.is_fifty_move_rule_draw() || board.is_repetition_draw(3)) {
        return { .score = 0,.best_move = Move(),.is_tempered = true };
    }
    uint64_t hash=board.get_hash();
    int original_alpha=alpha;
    int tt_score;
    Move tt_move;

    bool is_from_depth_0 = false;
    if (probe_tt(hash, depth, alpha, beta, tt_score, tt_move, ply, is_from_depth_0)) {
        bool is_draw = move_could_result_in_repetition(board, tt_move);
        //is_draw = false;
        if (!is_draw) {
            recover_move_fully(tt_move, board);
            return { tt_score,tt_move};
        }
    }
    
    if (depth==0)
    {
        int q_score=quiescence_search(board, alpha, beta, ply, 0, tls, checkers);
        
        return {q_score,Move()};
    }

    if (checkers == CHECKERS_UNKNOWN) {
        checkers = board.get_checkers();
    }
    bool king_is_in_check = checkers != 0;
	int static_eval = -MATE_SCORE;
    //REVERSE FUTILITY PRUNING
    // 

    bool is_pv_node = (beta - alpha) > 1;
    const bool nmp_candidate = null_move_allowed && !is_pv_node && depth >= NMP_MIN_DEPTH && !king_is_in_check &&
        std::abs(beta) < MATE_THRESHOLD && board.has_enough_material_for_nmp();
    if (!king_is_in_check && ((depth <= REVERSE_FUTILITY_MAX_DEPTH && !is_pv_node) || nmp_candidate)) {
        static_eval = board.is_white_to_move() ? evaluate(board, nullptr, EVAL_MATERIAL | EVAL_POSITIONAL | EVAL_PAWN_STRUCTURE) : -evaluate(board, nullptr, EVAL_MATERIAL | EVAL_POSITIONAL | EVAL_PAWN_STRUCTURE);
    }
    if (!king_is_in_check && depth <= REVERSE_FUTILITY_MAX_DEPTH && std::abs(beta) < MATE_THRESHOLD && !is_pv_node) {
        int rfp_margin = REVERSE_FUTILITY_MARGIN * depth; // This margin can be tuned
        if (static_eval - rfp_margin >= beta) {
            return { static_eval,Move() };
        }
    }
   
    
    // NULL Move Pruning Here
    int nmp_score;
    if(nmp_candidate && static_eval >= beta &&
        try_null_move_pruning(board,king_is_in_check,depth,alpha,beta,ply,static_eval,nmp_score,tls))
    {
        return {nmp_score,Move()};
	}
    // End of Null-move pruning
	//Generate moves
	MoveList& moves = tls->move_lists[ply];
	MoveList& searched_quiets = tls->searched_quiets[ply];
    moves.clear();
	searched_quiets.clear();
    MoveGenerator::generate_moves(board,moves,checkers);
	//If only one move available, no need to search further
    if ((ply == 0) && (moves.size() == 1)) {
        return { 0,moves[0] };
    }
    //Sort moves
    //sort_moves(moves,board,ply,tt_move);
    int best_score=-MATE_SCORE;
    Move best_move;
	//If no moves available, check for checkmate or stalemate
    if (moves.empty())
    {
		return terminal_eval(board, king_is_in_check,ply);
    }
    

    //Futility Purning prerequisites here Here
    int current_eval=-MATE_SCORE;
    if (depth<=2)
	{
        if (static_eval != -MATE_SCORE)
            current_eval = static_eval;
        else
            current_eval = board.is_white_to_move() ? evaluate(board, nullptr, EVAL_MATERIAL | EVAL_POSITIONAL | EVAL_PAWN_STRUCTURE) : -evaluate(board,nullptr,EVAL_MATERIAL | EVAL_POSITIONAL | EVAL_PAWN_STRUCTURE);
    }
    
	// Late Move Reduction prerequisites here
    int moves_searched=0;
    // PVS 
    bool first = true;
    bool is_any_tempered = false;
    bool is_best_move_tempered = false;
    bool current_move_tempered = false;
    int* scores = tls->move_scores[ply];
	score_moves(moves, scores, ply, tt_move, is_from_depth_0,board,tls,previous_move,true);
    for (int i=0;i<(int)moves.size();++i)
    {   
		pick_next_staged(moves, scores, i, board);
		const Move move = moves[i];
        // Now do futility pruning. If positions evaluation is already way worse than alpha, cut it off since it is
        //unlikely to get that much better in just 1 or two moves
        if(!first && should_futility_prune(depth,current_eval,alpha,king_is_in_check,move))
        {
            continue;
		}
        // Late Move Reduction
        int reduction = 0;
        if (!board.is_dangerous_passer_push(move)) {
            reduction = late_move_reduction(depth, moves_searched, move, ply, tls, previous_move);
        }
        moves_searched++;
        //Now make the move
        board.make_move(move);

		//If in check, we should increase depth by 1
        int extension = 0;
        const uint64_t child_checkers = board.get_checkers();
        if (child_checkers != 0 && ply<64 && depth<=3)
        {
            extension=1;
		}
        int evaluation;
        if (first) { 
			SearchResult first_result = negamax(board, depth - 1 + extension, -beta, -alpha, ply + 1,tls,move,child_checkers);
            evaluation = -first_result.score;
            current_move_tempered = first_result.is_tempered;
			first = false;
        }
        else {
            int new_depth = depth - 1 + extension;
			int reduced_depth = new_depth - reduction;

            SearchResult other_result = negamax(board, reduced_depth, -alpha - 1, -alpha, ply + 1,tls,move,child_checkers);
            evaluation = -other_result.score;
            current_move_tempered = other_result.is_tempered;

            if (reduction > 0 && evaluation > alpha) {
                other_result = negamax(board, new_depth, -alpha - 1, -alpha, ply + 1,tls,move,child_checkers);
                evaluation = -other_result.score;
				current_move_tempered = other_result.is_tempered;
            }

            if(evaluation > alpha && evaluation < beta) {
                 other_result = negamax(board, new_depth, -beta, -alpha, ply + 1,tls,move,child_checkers);
				 evaluation = -other_result.score;
				current_move_tempered = other_result.is_tempered;
            }
        }
        is_any_tempered |= current_move_tempered;
        board.undo_move(move);
		bool quiet = move.piece_captured == PieceType::NONE && move.promotion_piece == PieceType::NONE;

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
        }
        alpha = std::max(alpha, best_score);
       
            
        

        if (beta<=alpha)
        {   
			update_history_killer(move, depth, ply,tls,previous_move,searched_quiets);
            break;
        }
        
    }

    bool is_result_tempered = store_tt(hash, depth, original_alpha, beta, best_score,
        best_move, ply, is_best_move_tempered, is_any_tempered);
    return {best_score,best_move,is_result_tempered};
}
int Engine::score_move(const Move& move, int ply,const Move& tt_move,bool depth_0,const Board& board, ThreadLocalData* tls, const Move& previous_move) {
   

    int stage = 0;
    int sub = 0;

    if (move == tt_move && !depth_0) { stage = TT_STAGE; sub = 0; }
    else if (move.promotion_piece != PieceType::NONE) {
        stage = PROMO_STAGE; sub = PIECE_VALUES_MG[to_int(move.promotion_piece)] - PIECE_VALUES_MG[to_int(move.piece_captured)];
    }
    else if (move.piece_captured != PieceType::NONE) {
        int see = see_move(board, move);

        int attacker_val = PIECE_VALUES_MG[to_int(move.piece_moved)];
        int victim_val = PIECE_VALUES_MG[to_int(move.piece_captured)];
        int tiebreak = (victim_val - attacker_val) / CAPTURE_SCORE_TIEBREAK_DIVISOR;
        if(see>=0) { stage = MVV_LVA_STAGE; sub = see+tiebreak; }
		else { stage = LOSING_CAPTURE_STAGE; sub = see; }
    }
    else if(move == tls->killer_moves[ply][0] || move == tls->killer_moves[ply][1]) { stage = KILLER_STAGE; sub = 0; }
    else if (previous_move.from_square != NO_SQUARE &&
        move == tls->counter_moves[to_int(previous_move.move_color)]
        [to_int(previous_move.piece_moved)]
        [previous_move.to_square]
        ) {
        stage = COUNTERMOVE_STAGE;
        sub = 0;
    }
    else {
        stage = QUIET_STAGE; sub = tls->history_scores[to_int(move.move_color)][to_int(move.piece_moved)][move.to_square] +relevant_pawn_push(board,move);
	}
	return stage * 100000 + sub;
}      
void Engine::sort_moves(MoveList& moves,const Board& board, int ply,const Move& tt_move,bool tt_depth_0, ThreadLocalData* tls, const Move& previous_move){
    std::vector<std::pair<int, Move>> scored;
    scored.reserve(moves.size());
    for (const auto& m : moves) scored.emplace_back(score_move(m, ply, tt_move,tt_depth_0,board,tls,previous_move), m);

    std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) {
        if(a.first != b.first)
            return a.first > b.first;
		return a.second.get_int() < b.second.get_int();
        });

    for (size_t i = 0; i < moves.size(); ++i) moves[i]=scored[i].second;
}
int Engine::quiescence_search(Board& board, int alpha, int beta, int search_ply, int qply,
    ThreadLocalData* tls, uint64_t checkers) {
    if (stop_search.load(std::memory_order_relaxed)) {
        return 0;
    }
    if (tls) {
        tls->qnodes++;
        tls->flush_counters(this);
        if (tls->should_check_time() && is_time_up()) {
            stop_search.store(true, std::memory_order_relaxed);
            return 0;
        }
    }
    if (board.is_fifty_move_rule_draw() || board.is_repetition_draw(3)) {
        return 0;
    }
    if (checkers == CHECKERS_UNKNOWN) {
        checkers = board.get_checkers();
    }
    bool in_check = checkers != 0;
    constexpr int max_qply_index = ThreadLocalData::QSEARCH_PLY_CAPACITY - 1;
    const int qmove_list_index = std::min(qply, max_qply_index);
    MoveList& moves = tls->qmove_lists[qmove_list_index];
    moves.clear();

    if (qply >= MAX_QUIET_PLY || qply >= max_qply_index) {
        if (in_check) {
            MoveGenerator::generate_moves(board, moves, checkers);
            if (moves.empty()) return -MATE_SCORE + search_ply;
        }
        return board.is_white_to_move() ? evaluate(board) : -evaluate(board);
    }

    if (in_check) {
        MoveGenerator::generate_moves(board, moves, checkers);
        if (moves.empty()) return -MATE_SCORE + search_ply;
    }
    else {
        int stand_pat = board.is_white_to_move() ? evaluate(board) : -evaluate(board);
        if (stand_pat >= beta) return stand_pat;
        if (stand_pat > alpha) alpha = stand_pat;

        MoveGenerator::generate_captures(board, moves, checkers);
    }

    int best_score = in_check ? -MATE_SCORE : alpha;
    int scores[256];
    score_qsearch_moves(moves, scores);

    for (int i = 0; i < (int)moves.size();) {
        if (stop_search.load(std::memory_order_relaxed)) {
            break;
        }
        pick_best(moves, scores, i);
        Move move = moves[i];

        if (!in_check) {
            if (scores[i] == std::numeric_limits<int>::min()) break;
            if (see_move(board, move) < 0) {
                scores[i] = std::numeric_limits<int>::min();
                continue;
            }
        }

        board.make_move(move);
        const uint64_t child_checkers = board.get_checkers();
        int score = -quiescence_search(board, -beta, -alpha, search_ply + 1, qply + 1, tls, child_checkers);
        board.undo_move(move);
		++i;

        if (score > best_score) best_score = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) break;
    }

    return best_score;
}
double interpolate_phase(double phase, double endgame_value, double midgame_value, double opening_value) {
    phase = std::clamp(phase, 0.0, 24.0);
    if (phase <= 12) {
        const double t = phase / 12.0;
		return endgame_value + t * (midgame_value - endgame_value);
    }

	const double t = (phase - 12.0) / 12.0;
	return midgame_value + t * (opening_value - midgame_value);
}
TimeControlDecision Engine::decide_time_control(const Board& position, const SearchLimits& limits) {
    TimeControlDecision tc{};
    tc.max_depth = limits.depth > 0 ? limits.depth : INFINITE_DEPTH;
    if (limits.movetime > 0) {
        tc.time_ms = limits.movetime;
		tc.max_time_ms = limits.movetime;
    }
    else if (limits.wtime > 0 || limits.btime > 0) {
		const bool white_to_move = position.get_turn() == Color::WHITE;

        const int time_left = (position.get_turn() == Color::WHITE) ? limits.wtime : limits.btime;
        const int inc = (position.get_turn() == Color::WHITE) ? limits.winc : limits.binc;

        const int usable_time = std::max(1, time_left - MOVE_OVERHEAD_MS);

        const double phase = position.get_game_phase();

		double moves_to_go = interpolate_phase(phase, MOVES_TO_GO_EG, MOVES_TO_GO_MG, MOVES_TO_GO);

		const int moves_after_threshold = std::max(0, position.get_move_count() - MOVE_COUNT_THRESHOLD);

        moves_to_go -= std::min(MAX_MOVE_COUNT_REDUCTION,MOVE_COUNT_WEIGHT * moves_after_threshold);
        moves_to_go = std::clamp(moves_to_go, MIN_MOVES_TO_GO, MAX_MOVES_TO_GO);

        const double increment_contribution = inc * INC_USAGE_FACTOR;
        
        const double base_time = usable_time / moves_to_go + increment_contribution;

        //Estimate the clock resources avalable over the expected remainng number of moves

        const double effective_time = usable_time + inc * moves_to_go;

        const double time_scale = std::clamp(effective_time / REFERENCE_TIME, 0.0, 1.0);

        double max_multiplier = MAX_MULTIPLIER_FAST + time_scale * (MAX_MULTIPLIER_SLOW - MAX_MULTIPLIER_FAST);
        max_multiplier=std::max(1.0,max_multiplier);

        tc.time_ms = std::clamp(static_cast<int>(base_time), 1, usable_time);
		tc.max_time_ms = std::clamp(static_cast<int>(base_time * max_multiplier), tc.time_ms, usable_time);

    }
    else if (limits.depth > 0) {
        tc.time_ms = INFINITE_TIME_MS;
        tc.max_depth = limits.depth;
    }
    else if (limits.infinite) {
        tc.time_ms = INFINITE_TIME_MS;
    }
    else {
        tc.time_ms = DEFAULT_TIME_MS;
    }
    return tc;
}
bool Engine::probe_tt(uint64_t hash, int depth, int alpha, int beta, int& out_score,
    Move& out_move, int ply, bool depth_0, TTMode mode) {
    TTCluster& cluster = tt[hash & (tt.size() - 1)];
    const uint16_t key16 = static_cast<uint16_t>(hash >> 48);
    bool hits = false;
    for (int i = 0; i < 4; i++) {
        TTEntry& slot = cluster.entries[i];
        uint64_t w = tt_load(slot);
        TTEntry entry;
        entry.entry = w;
		if (entry.empty()) return false;
		if (entry.key() != key16) continue;

        out_move = entry.move();
        const int score = score_from_tt(entry.score(), ply);
        out_score = score;
        if (entry.depth() < depth) {
            return false;
        }

        if (entry.flag() == TEMPERED) {
            return false;
        }
        int a = alpha, b = beta;
        if (entry.flag() == EXACT) {
            if (out_move.from_square == NO_SQUARE) return false;
            return true;
		}
        if (entry.flag() == LOWERBOUND) a = std::max(a, score);
        if (entry.flag() == UPPERBOUND) b = std::min(b, score);
        
        if (beta - alpha > 1) {
            alpha = a;
            beta = b;
            hits = true;
        }
        if (a >= b) {
            if (out_move.from_square == NO_SQUARE) return false;
            return true;
        }
    }
    return false;

}
bool Engine::store_tt(uint64_t hash, int depth, int original_alpha, int beta, int best_score,
    Move& best_move, int ply, bool is_best_tempered, bool is_any_tempered, TTMode mode) {
    bool score_tempered=false;
    TTFlag flag_to_store;
    // Do some position from repeat logic here

    if (is_best_tempered) {
        flag_to_store = TEMPERED;
        score_tempered = true;
    }
    else if (is_any_tempered) {
        if (best_score >= beta) {
            flag_to_store = LOWERBOUND;
        }
        else if (best_score >= original_alpha) {
            flag_to_store = LOWERBOUND;
        }
        else {
            flag_to_store = TEMPERED;
            // Dont set score_tempered to true because the best move is clean, it might not actually be that best move but we can guarantee at least the result.
        }
    }
    else {
        if (best_score >= beta)
        {
            flag_to_store = LOWERBOUND;
        }
        else if (best_score <= original_alpha)
        {
            flag_to_store = UPPERBOUND;
        }
        else
        {
            flag_to_store = EXACT;
        }
    }

    TTEntry new_entry = TTEntry(score_to_tt(best_score, ply), depth, flag_to_store,
        generation, best_move, static_cast<uint16_t>(hash >> 48));
	TTCluster& cluster = tt[hash & (tt.size() - 1)];

    uint16_t key16 = static_cast<uint16_t>(hash >> 48);
    for(int i=0;i<4;i++){
        // If key already exists in cluster, update that slot.
		uint64_t oldw = tt_load(cluster.entries[i]);
        TTEntry old; old.entry = oldw;
        if (!old.empty() && old.key() == key16) {
            if (old.depth() <= depth) {
                tt_store(cluster.entries[i],new_entry.entry);
            }

            
            return score_tempered;
        }
	}
    for (int i = 0; i < 4; i++) {
        // Find an empty slot to store the new entry.

        uint64_t oldw = tt_load(cluster.entries[i]);
        TTEntry old; old.entry = oldw;
        if (old.empty()) {
            tt_store(cluster.entries[i],new_entry.entry);
			return score_tempered;
        }
    }

	// If no empty slot, replace the least recently used (last) entry

    int pos_index = -1;
    uint16_t max_generation_diff = 0;
    int pos_depth = depth;
    for (size_t i = 0; i < 4; ++i) {
		TTEntry e; e.entry = tt_load(cluster.entries[i]);
		int8_t curr_gen_diff = dist_mod64_fast(e.generation(),generation);
        if (curr_gen_diff>max_generation_diff) {
            pos_index = i;
            max_generation_diff = curr_gen_diff;
            pos_depth = e.depth();
        }
        else if (max_generation_diff==curr_gen_diff && e.depth() <= pos_depth) {
            pos_index = i;
            pos_depth = e.depth();
        }
    }
    if (pos_index != -1) {
        tt_store(cluster.entries[pos_index],new_entry.entry);
    }
    return score_tempered;
   

}
bool Engine::should_futility_prune(int depth, int eval, int alpha, bool in_check,const Move& move) {
	if (depth > 3) return false;
    bool is_quiet = move.piece_captured == PieceType::NONE && move.promotion_piece == PieceType::NONE;
    if (in_check || !is_quiet) return false;
    if (depth == 1 && eval + FUTILITY_MARGIN_D1 <= alpha) return true;
    if (depth == 2 && eval + FUTILITY_MARGIN_D2 <= alpha) return true;
    return false;
}
int Engine::late_move_reduction(int depth, int moves_searched, const Move& move, int ply, ThreadLocalData* tls, const Move& previous_move) {
    if (depth >= 64 || moves_searched >= 218) return 7;
    if (depth <= 1 || moves_searched <= 1) return 0; // No reduction for the first move
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
bool Engine::try_null_move_pruning(Board& board, bool king_is_in_check, int depth, int alpha, int beta, int ply,
    int static_eval, int& out_score,ThreadLocalData* tls) {
	bool is_mate_score_possible = (alpha >= MATE_THRESHOLD || beta <= -MATE_THRESHOLD);

    if(is_mate_score_possible|| depth < NMP_MIN_DEPTH || king_is_in_check || !board.has_enough_material_for_nmp()) {
        return false;
	}
	const int eval_reduction = std::clamp((static_eval - beta) / std::max(1, NMP_EVAL_DIVISOR),
        0, NMP_MAX_EVAL_REDUCTION);
	const int reduction = NMP_REDUCTION + depth / std::max(1, NMP_DEPTH_DIVISOR) + eval_reduction;
	const int null_depth = std::max(0, depth - 1 - reduction);
	int original_ep_square = board.make_null_move();
	int null_move_score = negamax(board, null_depth, -beta, -beta + 1, ply + 1,tls,
        Move(), CHECKERS_UNKNOWN, false).score;
	null_move_score = -null_move_score;
	board.undo_null_move(original_ep_square);
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
SearchResult Engine::terminal_eval(const Board& board, bool king_is_in_check,int ply) {
    if (king_is_in_check) {
		return { -MATE_SCORE+ply, Move() };
    }
    else return { 0,Move() };
}
void Engine::update_history_killer(const Move& move, int depth, int ply,ThreadLocalData* tls,const Move& previous_move,
    const MoveList& serached_quiets) {
    if (!tls) return;
	bool quiet = move.piece_captured == PieceType::NONE && move.promotion_piece == PieceType::NONE;
    if (quiet)
    {
        tls->killer_moves[ply][1] = tls->killer_moves[ply][0];
        tls->killer_moves[ply][0] = move;

        if(previous_move.from_square != NO_SQUARE) {
            tls->counter_moves[to_int(previous_move.move_color)]
                [to_int(previous_move.piece_moved)]
                [previous_move.to_square] = move;
		}
    }
    int bonus = depth * depth*HISTORY_BONUS_MULTIPLIER;
    add_history(tls, move, bonus);
    for(int i=0;i<(int)serached_quiets.size();++i){
        const Move& m = serached_quiets[i];
		if (m == move) continue;
        add_history(tls, m, -bonus);
	}
}
void Engine::init_tt(size_t tt_size_mb) {
    size_t bytes = tt_size_mb * 1024ull * 1024ull;
    size_t clusters = bytes / sizeof(TTCluster);
    if (clusters == 0) clusters = 1;
    clusters = std::bit_floor(clusters);
    tt.resize(clusters);
}
bool Engine::move_could_result_in_repetition(Board& board, Move& move, int count) {
    if (move.piece_captured != PieceType::NONE || move.piece_moved == PieceType::PAWN || move.is_castle) return false;
    return board.has_twofold();
}
void Engine::recover_move_fully(Move& move,const Board& board) {
    move.move_color = board.get_turn();
    move.piece_moved = board.get_piece_on_square(move.from_square);
    move.piece_captured = board.get_piece_on_square(move.to_square);
	int abs = std::abs(move.to_square - move.from_square);
	move.is_castle = move.piece_moved == PieceType::KING && abs == 2;
    move.is_en_passant = move.piece_moved == PieceType::PAWN && move.to_square==board.get_en_passant_rights();
}
void Engine::score_moves(const MoveList& moves, int* scores,
    int ply, const Move& tt_move, bool tt_depth_0,const Board& board,ThreadLocalData* tls,
    const Move& previous_move, bool lazy_see) {
    for (int i = 0; i < (int)moves.size(); ++i) {
        const Move& move = moves[i];
        const bool is_tt_move = move == tt_move && !tt_depth_0;
        if (lazy_see && !is_tt_move && move.promotion_piece == PieceType::NONE &&
            move.piece_captured != PieceType::NONE) {
            const int attacker = PIECE_VALUES_MG[to_int(move.piece_moved)];
            const int victim = PIECE_VALUES_MG[to_int(move.piece_captured)];
            // The offset marks captures whose SEE has not been evaluated yet while
            // keeping them between promotions and killers in the staged ordering.
            scores[i] = MVV_LVA_STAGE * 100000 + 50000 +
                (victim - attacker) / CAPTURE_SCORE_TIEBREAK_DIVISOR;
        }
        else {
            scores[i] = score_move(move, ply, tt_move, tt_depth_0,board, tls,previous_move);
        }
    }
}
void Engine::pick_next_staged(MoveList& moves, int* scores, int start, const Board& board) {
    while (true) {
        int best = start;
        for (int i = start + 1; i < static_cast<int>(moves.size()); ++i) {
            if (scores[i] > scores[best]) best = i;
        }

        const Move& candidate = moves[best];
        const bool untested_capture = candidate.piece_captured != PieceType::NONE &&
            candidate.promotion_piece == PieceType::NONE &&
            scores[best] >= MVV_LVA_STAGE * 100000 + 40000 &&
            scores[best] < MVV_LVA_STAGE * 100000 + 60000;
        if (untested_capture) {
            const int see = see_move(board, candidate);
            if (see < 0) {
                scores[best] = LOSING_CAPTURE_STAGE * 100000 + see;
                continue;
            }
            const int attacker = PIECE_VALUES_MG[to_int(candidate.piece_moved)];
            const int victim = PIECE_VALUES_MG[to_int(candidate.piece_captured)];
            scores[best] = MVV_LVA_STAGE * 100000 + see +
                (victim - attacker) / CAPTURE_SCORE_TIEBREAK_DIVISOR;
        }

        if (best != start) {
            std::swap(moves[best], moves[start]);
            std::swap(scores[best], scores[start]);
        }
        return;
    }
}
void Engine::score_qsearch_moves(const MoveList& moves, int* scores) {
    for (int i = 0; i < (int)moves.size(); ++i) {
        scores[i] = 0;
		const Move& m = moves[i];
        int victim = PIECE_VALUES_MG[to_int(m.piece_captured)]/100;
        int attacker = PIECE_VALUES_MG[to_int(m.piece_moved)]/100;
        scores[i] += victim - attacker;
    }
}
int Engine::relevant_pawn_push(const Board& board, const Move& move) {
    if (move.piece_moved != PieceType::PAWN) return 0;
    int score = 0;
    Color us = board.get_turn();

    int to = move.to_square;
	int rank = to / 8;
	int relative_rank = (us == Color::WHITE) ? rank : 7 - rank;

    bool passed = board.is_passed_after(move);

    if (passed) {
        score += PAWN_PUSH_SCORE1;
        if (relative_rank >= 4) score += PAWN_PUSH_SCORE2;
        if (relative_rank >= 5) score += PAWN_PUSH_SCORE3;
        if (relative_rank >= 6) score += PAWN_PUSH_SCORE4;

        if (board.count_attacker_on_square(to, flip_color(us), 1, false).count == 0) {
            score += PAWN_PUSH_SCORE5;
        }
    }
    int king_square = board.get_king_square(flip_color(us));
    if (KING_ZONE[king_square] & bit64(to)) {
        score += PAWN_PUSH_SCORE6;
    }
    return score;

}
void Engine::set_threads(int n) {
    n = std::max(1, n);
	if (n == thread_count) return;
    stop_thread_pool();
    start_thread_pool(n);
}
void Engine::start_thread_pool(int n) {
	thread_count = n;
    terminate_pool = false;
	tls_data.clear_counters();
    workers.clear();
	workers.reserve((size_t)thread_count - 1);

    for (int t = 1; t < thread_count; ++t) {
		workers.emplace_back([this, t]() {worker_loop(t); });
    }

}
void Engine::stop_thread_pool() {
    {
        std::lock_guard<std::mutex> lk(pool_mtx);
		terminate_pool = true;
        job_id++;
    }
    cv_start.notify_all();
    for (auto& th : workers) {
        if(th.joinable())
			th.join();
    }
    workers.clear();
    terminate_pool = false;
    thread_count = 1;
    tls_data.clear_counters();
}

void Engine::worker_loop(int thread_id) {
    uint64_t seen_job = 0;
    Move local_best;
    int local_score = 0;

    while (true) {
        Board pos;
        SearchLimits limits;
        {
			std::unique_lock<std::mutex> lk(pool_mtx);
			cv_start.wait(lk, [&] {return terminate_pool || job_id != seen_job; });
			if (terminate_pool) return;
			seen_job = job_id;
            pos = job_position;
            limits = job_limits;
        }

        tls_data.clear_counters();
        Move tmp_best = local_best;
		int tmp_score = local_score;    
		TimeControlDecision tc = decide_time_control(pos, limits);
        iterative_deepening_new(thread_id, false, tmp_best, tmp_score, pos, tc, &tls_data);
        tls_data.flush_counters(this, true);
		local_best = tmp_best;
		local_score = tmp_score;
        {
            std::lock_guard<std::mutex> lk(pool_mtx);
            active_workers--;
            if (active_workers == 0) cv_done.notify_one();
        }
    }
}

void Engine::iterative_deepening_new(int thread_id, bool is_master, Move& io_best_move, int& io_best_score, const Board& position, TimeControlDecision& tc , ThreadLocalData* tls) {
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
    Move recent_best_moves[MAX_RECENT_BEST_COUNT] = { };
    int recent_best_move_count = 0;
    const int initial_budget_ms = tc.time_ms;
    const int maximum_budget = tc.max_time_ms;
    int current_budget_ms = initial_budget_ms;

    int prev_root_score = 0;
    bool has_prev_root_score = false;
    int dominant_gap_streak = 0;

    int second_best_score = -MATE_SCORE;
    for (int current_depth = start_depth; current_depth <= tc.max_depth; ++current_depth) {
        Board board = position;
        MoveList root_moves;

        MoveGenerator::generate_moves(board, root_moves);
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

        sort_moves(root_moves, board, 0, io_best_move, false, tls);
        perturb_root_order(root_moves, thread_id, current_depth,board.get_zobrist_hash());

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
            root_pvs(position, root_moves, current_depth, alpha, beta, best_score, best_move,second_best_score,second_best_move, tls);
            if (stop_search.load(std::memory_order_relaxed)) break;

            if (current_depth == 1) break; // no aspiration on depth 1

            if (best_score <= alpha || best_score >= beta) {

                //WIden around the reported score and try again.
                window = std::min(static_cast<int>(window * ASPIRATION_WINDOW_MULTIPLIER), MATE_SCORE);
                alpha = std::max(-MATE_SCORE, best_score - window);
                beta = std::min(MATE_SCORE, best_score + window);

                //Put the current best move first to speed up re-search.
                sort_moves(root_moves, board, 0, best_move, false, tls);
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
            const auto now_tp = std::chrono::steady_clock::now();
            const int64_t now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                now_tp.time_since_epoch()
            ).count();

            auto elapsed = now_tp - start_time;

            recent_best_moves[recent_best_move_count % MAX_RECENT_BEST_COUNT] = best_move;
            recent_best_move_count++;

            if (recent_best_move_count >= 6 && current_budget_ms < maximum_budget) {
                int changes = 0;
                int consecutive_changes = 0;
                bool has_two_consecutive_changes = false;
                int start = recent_best_move_count - MAX_RECENT_BEST_COUNT;
                for (int i = 0; i < MAX_RECENT_BEST_COUNT; ++i) {
                    const Move& a = recent_best_moves[(start + i) % MAX_RECENT_BEST_COUNT];
                    const Move& b = recent_best_moves[(start + i + 1) % MAX_RECENT_BEST_COUNT];
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
                        current_budget_ms = std::clamp(current_budget_ms + extra_time_ms, initial_budget_ms, maximum_budget);
                        set_time_budget_ms(current_budget_ms);
                    }
            }
            if (maximum_budget > current_budget_ms && has_prev_root_score) {
                const bool curr_is_mate = std::abs(best_score) >= MATE_THRESHOLD;
                const bool prev_is_mate = std::abs(prev_root_score) >= MATE_THRESHOLD;

                if (!curr_is_mate && !prev_is_mate) {
                    const int delta_cp = std::abs(best_score - prev_root_score);
                    const bool sign_flip = (best_score > 0) != (prev_root_score > 0);

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

                        current_budget_ms = std::clamp(current_budget_ms + extra_time_ms, initial_budget_ms, maximum_budget);
                        set_time_budget_ms(current_budget_ms);
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
                std::cout << " score cp " << (board.get_turn() == Color::WHITE ? best_score : -best_score);
            }
            std::cout << " time " << elapsed_ms
                      << " nodes " << total_nodes
                      << " nps " << nps
                      << " pv " << create_pv_string(board, best_move, current_depth)
                      << "\n";
            std::cout.flush();
			iteration_nodes = total_nodes - prev_total_nodes;
			iteration_ms = elapsed_ms - prev_elapsed_ms;
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
            const int64_t deadline_ns = search_deadline_ns.load(std::memory_order_acquire);
            const int64_t remaining_ms = std::max<int64_t>(0, (deadline_ns - now_ns) / 1000000LL);
			prev_root_score = best_score;
			has_prev_root_score = true;
            const int64_t HARD_SAFETY_MS = 10 + tc.time_ms / 50;

            if (remaining_ms <= HARD_SAFETY_MS) {
                break;
            }
            if (static_cast<double>(predicted_next_iteration_ms) * TIME_MARGIN > static_cast<double>(remaining_ms)) {
                break;
            }
        }
    }

}
void Engine::perturb_root_order(MoveList& moves, int thread_id, int depth, uint64_t hash) {
    if (thread_id == 0) return;
    if (moves.size() <= ROOT_PERTURBATION_MIN_HELPERS) return;

    int helpers = std::max(0, thread_count - 1);
    int K = std::clamp(2 * helpers, ROOT_PERTURBATION_MIN_BAND_SIZE, ROOT_PERTURBATION_MAX_BAND_SIZE);
    int bandSize = std::min<int>(K, (int)moves.size() - 1);
    if (bandSize <= 1) return;

    //Mix position+thread+depth int a pseudo-random deterministic value
    uint64_t seed = hash
        ^ (uint64_t(thread_id) * 0xD1B54A32D192ED03ULL)
        ^ (uint64_t(depth) * 0x9E3779B97F4A7C15ULL);

    size_t shift = (size_t)(splitmix64(seed) % (uint64_t)bandSize);
    if (shift == 0) return;
    auto first = moves.begin() + 1;
    auto mid = first + shift;
    auto last = first + bandSize;

    std::rotate(first, mid, last);

    //// Keep the first move as-is (usually TT/PV), rotate the rest.
    //size_t shift = (size_t)((thread_id * 7 + depth * 3) % (moves.size() - 1));
    //if (shift == 0) return;
    //std::rotate(moves.begin() + 1, moves.begin() + 1 + shift, moves.end());

    }
void Engine::root_pvs(const Board& pos, MoveList& root_moves,
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


    Board b = pos;
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
            r = negamax(b, current_depth - 1, -(local_alpha + 1), -local_alpha, 1, tls, m, child_checkers);
            int score = -r.score;
            if (!stop_search.load(std::memory_order_relaxed) && score > local_alpha && score < beta) {
                r = negamax(b, current_depth - 1, -beta, -local_alpha, 1, tls, m, child_checkers);
            }
        }

        int score = -r.score;
        b.undo_move(m);

        if (stop_search.load(std::memory_order_relaxed)) break;

        if (i == 0 || score > best_score) {
            second_best_score = best_score;
            local_second_best_move = best_move;
            best_score = score;
            best_move = m;
        } 
		else if (score > second_best_score) {
            second_best_score = score;
            local_second_best_move = m;
        }

        if (score > local_alpha) local_alpha = score;
        if (local_alpha >= beta) break;
    }

    out_best_score = best_score;
    out_best_move = best_move;
    out_second_best_score = second_best_score;
    out_second_best_move = local_second_best_move;
}
Move Engine::search(const Board& position, const SearchLimits& limits) {
    //decide time control
	auto tc = decide_time_control(position, limits);
    initialize_lmr_tables();
    generation = static_cast<uint8_t>((generation + 1) & 0x3F);
    int use_threads = thread_count;
    if (tc.time_ms < 20) use_threads = 1;

    nodes.store(0, std::memory_order_relaxed);
    qnodes.store(0, std::memory_order_relaxed);
    tls_data.clear_counters();

    //reset timer +stop flag AFTER you publish job if you want workers to see consisten values
	stop_search.store(false, std::memory_order_relaxed);

    //Seed fallback move (ideally after sort_moves so its not "first generated"
    Board board = position;
	MoveList root_moves;
	MoveGenerator::generate_moves(board, root_moves);
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
	sort_moves(root_moves, board, 0, Move(), false, &tls_data);
    Move best_move_so_far = root_moves[0];
	int best_score_so_far = -MATE_SCORE;

    //publish job to helpers
    {
		std::lock_guard<std::mutex> lk(pool_mtx);
		stop_search.store(false, std::memory_order_relaxed);

    const auto now = std::chrono::steady_clock::now();
    start_time = now;

    const int64_t start_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()
    ).count();

    search_start_ns.store(start_ns, std::memory_order_release);
    search_deadline_ns.store(
        start_ns + static_cast<int64_t>(tc.time_ms) * 1000000LL,
        std::memory_order_release
    );

		job_position = position;
        job_limits = limits;
		active_workers = std::max(0, use_threads - 1);
        job_id++;
    }
    if (use_threads > 1) {
        cv_start.notify_all();
    }

    //master search in this thread (thread_id=0)
	iterative_deepening_new(0, true, best_move_so_far, best_score_so_far, position, tc, &tls_data);

    //stop helpers and waith them to finish
    stop_search.store(true, std::memory_order_relaxed);
	
    if(use_threads>1) {
		std::unique_lock<std::mutex> lk(pool_mtx);
        cv_done.wait(lk, [&] {return active_workers == 0; });
	}
    tls_data.flush_counters(this, true);
    //std::cout << rev_fut_count;
	return best_move_so_far;
}
Engine::~Engine() {
    shutdown();
}

void Engine::shutdown() {
    // Stop any ongoing search loops
    stop_search.store(true, std::memory_order_relaxed);

    // Terminate idle workers waiting on cv_start and join them
    stop_thread_pool();
}

void Engine::resize_tt(size_t tt_size_mb) {
    // Stoppe alle Worker, damit kein Thread auf tt zugreift
    stop_search.store(true, std::memory_order_relaxed);
    int saved_threads = thread_count;
    stop_thread_pool();

    // TT neu allokieren (löscht alle alten Einträge)
    tt.clear();
    init_tt(tt_size_mb);

    //std::cerr << "info string TT resized to " << tt_size_mb
    //          << " MB, entries=" << 4 * tt.size() << "\n";

    // Thread-Pool mit gleicher Thread-Anzahl wieder hochfahren
    start_thread_pool(saved_threads);
    stop_search.store(false, std::memory_order_relaxed);
}
std::string Engine::create_pv_string(const Board& board, const Move& best_move, int depth) {
    std::string pv = move_to_uci(best_move);
    Board b = board;
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
        if (!probe_tt(hash, 0, -MATE_SCORE, MATE_SCORE, tt_score, tt_move, i, depth_0))
            break;
        if (tt_move.from_square == NO_SQUARE || tt_move.to_square == NO_SQUARE)
            break;

        // TT speichert nur from/to/promotion  Rest muss rekonstruiert werden
        recover_move_fully(tt_move, b);

        // Prüfe ob der rekonstruierte Zug gültig ist (piece_moved darf nicht NONE sein)
        if (tt_move.piece_moved == PieceType::NONE)
            break;

        pv += " " + move_to_uci(tt_move);
        b.make_move(tt_move);
       
    }
    return pv;
}
void Engine::add_history(ThreadLocalData* tls, const Move& move, int bonus) {
	int& h = tls->history_scores[to_int(move.move_color)][to_int(move.piece_moved)][move.to_square];
    gravity_update(h, bonus);
}
int64_t Engine::now_ns() {
	return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
void Engine::set_time_budget_ms(int total_time_ms) {
	const int64_t start_ns = search_start_ns.load(std::memory_order_acquire);
    const int64_t deadline_ns = start_ns + static_cast<int64_t>(total_time_ms) * 1000000LL;
	search_deadline_ns.store(deadline_ns, std::memory_order_release);
}
bool Engine::is_time_up() const {
    const int64_t deadline_ns = search_deadline_ns.load(std::memory_order_acquire);
    const int64_t now = now_ns();
    return now >= deadline_ns;
}
void Engine::flush_node_counters() {
    tls_data.flush_counters(this, true);
}
uint64_t Engine::get_total_nodes() {
    flush_node_counters();
    return nodes.load(std::memory_order_relaxed) + qnodes.load(std::memory_order_relaxed);
}
