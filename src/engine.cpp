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

Engine::Engine(size_t tt_size_mb){
    init_tt(tt_size_mb);
    std::cerr << tt.size() << std::endl;
    stop_search=false;
    checks_count=0;
    ep_count=0;
    capture_count=0;
    checkmate_count=0;
	int overwrite_tt_counter = 0;
}
Move Engine::search(const Board& position, const SearchLimits& limits) {
    // Decide thinking time and max depth
    //position.display();
    auto tc = decide_time_control(position, limits);

    // Set engine timers
    this->start_time = std::chrono::steady_clock::now();
    this->time_limit = std::chrono::milliseconds(tc.time_ms);
    this->stop_search = false;

    nodes = 0;
	tt_probes_nm = 0;
    tt_hits_nm = 0;
    tt_misses_nm = 0;
    tt_updates_nm = 0;
	tt_overwrites_nm = 0;
	tt_stores_nm = 0;

    Move   best_move_so_far;
    int best_score_so_far = 0;

    // Mutable copy of the board
    Board board = position;
    int window = DEFAULT_SEARCH_WINDOW;
    int alpha;
    int beta;
    generation++;
    for (int current_depth = 1; current_depth <= tc.max_depth; ++current_depth) {
        
        if (this->stop_search)
            break; // in case some external flag already asked us to stop
        if (current_depth > 1) {
            alpha = best_score_so_far - window;
            beta = best_score_so_far + window;
        }
        else {
            alpha = -MATE_SCORE;
            beta = MATE_SCORE;
		}
        SearchResult result = negamax(board, current_depth, alpha, beta, 0);
        int eval_score = result.score;
        Move move_this_iteration = result.best_move;
        if (eval_score <= alpha or eval_score>= beta) {
			alpha = -MATE_SCORE;
			beta = MATE_SCORE;
            redo_window_search++;
			result = negamax(board, current_depth, alpha, beta, 0);
			eval_score = result.score;
            move_this_iteration = result.best_move;
        }

        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - this->start_time;

        // Check time inside the loop too (negamax should also check a stop flag)
        if (now-start_time>= time_limit || stop_search) {
            std::cerr << "\nTime limit of " << (tc.time_ms / 1000.0)
                << "s reached! Search is canceled.\n";
            break;
        }
        double nps = (nodes+qnodes) / elapsed.count();


        if (move_this_iteration.from_square != -1) {
            best_move_so_far = move_this_iteration;
            best_score_so_far = eval_score;

            // For logging: legal moves from root position
            MoveList legal_moves;
            MoveGenerator::generate_moves(position,legal_moves);
            std::cerr << "Depth " << current_depth << " ended. Best move so far: "
                << to_san(best_move_so_far, legal_moves)
                << ", Score: " << eval_score
                << ", Time: " << elapsed.count() << "s\n"
				<< " Nodes: " << this->nodes << ", NPS: " << nps << "\n"
				<< "Nodes per second" << nps << "\n";
        }
        if (std::abs(best_score_so_far) >= MATE_SCORE) {
            std::cerr << "Mate found, end search.\n";
            break;
        }
    }
    compute_tt_fill_rate();

    std::cerr << "Nodes (negamax): " << nodes << "\n";
    std::cerr << "QNodes: " << qnodes << "\n";

    std::cerr << "TT Negamax: probes " << tt_probes_nm
        << " hits " << tt_hits_nm
        << " misses " << tt_misses_nm
        << " stores " << tt_stores_nm
        << " updates " << tt_updates_nm
        << " overwrites " << tt_overwrites_nm
        << "Not stored because no space and depth too small " << tt_skip_same_gen_nm << "\n";

    std::cerr << "TT Quiescence: probes " << tt_probes_qs
        << " hits " << tt_hits_qs
        << " misses " << tt_misses_qs
        << " stores " << tt_stores_qs
        << " updates " << tt_updates_qs
        << " overwrites " << tt_overwrites_qs
        << "Not stored because no space and depth too small " << tt_skip_same_gen_qs <<"\n";
    std::cerr << "TT_Store-changes:"
        << "Tempered flag stored: " << tt_temp_flag_stored
        << "Flag not stored as exact because of tempered: " << tt_temp_not_exact
        << "Tempered flag found: " << tt_tempered_flag_found<< "\n";
    std::cerr << "How many times did we redo the search because window was too small:" << redo_window_search;
	std::cerr << "\nTransposition Table fill rate: " << (tt_filled_slots * 100.0) / (tt.size() * 4) << "%\n";
	std::cerr << "Possible repetitions detected: " << could_result_in_rep << "\n";
	std::cerr << "Of which we needed to make the move to recognize" << have_to_make_move_count << "\n";
	std::cerr << "How many cutoffs in Negamax? " << cutoff_count << "\n";
	std::cerr << "How many moves have been searched in Negamax before cutoff in total:" << moves_before_cutoff << "\n";
	std::cerr << "ratio:" << (moves_before_cutoff == 0 ? 0 : (double)moves_before_cutoff / cutoff_count) << "\n";
    // Optional: clear heuristics between moves
    /*for (auto& c : tt) {
        for (auto& e : c.entries) e.key = 0;
    }*/

    std::memset(this->killer_moves, 0, sizeof(this->killer_moves));
    std::memset(this->history_scores, 0, sizeof(this->history_scores));

    return best_move_so_far;
}
SearchResult Engine::negamax(Board& board, int depth, int alpha, int beta, int ply){
    uint64_t compare = board.get_all_pieces();
    if (std::chrono::steady_clock::now()-start_time>=time_limit)
    {
        stop_search=true;
        return {.score=0,.best_move=Move(),.is_tempered=true};
    }
    if (board.is_fifty_move_rule_draw() || board.is_repetition_draw(3)) {
        return { .score = 0,.best_move = Move(),.is_tempered = true };
    }
    uint64_t hash=board.get_hash();
    int original_alpha=alpha;
    int tt_score;
    Move tt_move;

    bool is_from_depth_0 = false;
    if (probe_tt(hash, depth, alpha, beta, tt_score, tt_move,is_from_depth_0)) {
        bool is_draw = move_could_result_in_repetition(board, tt_move);
        if(is_draw) could_result_in_rep++;
        //is_draw = false;
        if (!is_draw) {
            recover_move_fully(tt_move, board);
            return { tt_score,tt_move};
        }
    }
    
    if (depth==0)
    {
        int q_score=quiescence_search(board,alpha,beta,0);
        
        return {q_score,Move()};
    }
    nodes++;
    // NULL Move Pruning Here
    int nmp_score;
	bool king_is_in_check = board.in_check();
    if(try_null_move_pruning(board,king_is_in_check,depth,alpha,beta,ply,nmp_score))
    {
        return {nmp_score,Move()};
	}
    // End of Null-move pruning
	//Generate moves
    MoveList moves;
    MoveGenerator::generate_moves(board,moves);
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
		return terminal_eval(board, king_is_in_check);
    }
    

    //Futility Purning prerequisites here Here
    int current_eval=-MATE_SCORE;
    if (depth<=2)
    {
        current_eval = board.is_white_to_move() ? evaluate(board, EVAL_MATERIAL | EVAL_POSITIONAL | EVAL_PAWN_STRUCTURE) : -evaluate(board, EVAL_MATERIAL | EVAL_POSITIONAL | EVAL_PAWN_STRUCTURE);
    }
    
	// Late Move Reduction prerequisites here
    int moves_searched=0;
    // PVS 
    bool first = true;
    bool is_any_tempered = false;
    bool is_best_move_tempered = false;
    bool current_move_tempered = false;
    int scores[256];
	score_moves_NEW(moves, scores, ply, tt_move, is_from_depth_0,board);
    for (int i=0;i<(int)moves.size();++i)
    {   
		pick_best(moves, scores, i);
		const Move move = moves[i];
        // Now do futility pruning. If positions evaluation is already way worse than alpha, cut it off since it is
        //unlikely to get that much better in just 1 or two moves
        if(!first && should_futility_prune(depth,current_eval,alpha,king_is_in_check,move))
        {
            continue;
		}
        // Late Move Reduction
		int reduction = late_move_reduction(depth, moves_searched, move, ply);
        moves_searched++;
        uint64_t compare2 = board.get_all_pieces();
        //Now make the move
        board.make_move(move);

		//If in check, we should increase depth by 1
        int extension = 0;
        if (board.in_check() && ply<64 && depth<=3)
        {
            extension=1;
		}
        int evaluation;
        if (first) { 
			SearchResult first_result = negamax(board, depth - 1 + extension, -beta, -alpha, ply + 1);
            evaluation = -first_result.score;
            current_move_tempered = first_result.is_tempered;
			first = false;
        }
        else {
            SearchResult other_result = negamax(board, depth - 1 - reduction + extension, -alpha - 1, -alpha, ply + 1);
            evaluation = -other_result.score;
            current_move_tempered = other_result.is_tempered;
            if (evaluation > alpha && evaluation  < beta) {
                other_result = negamax(board, depth - 1 + extension, -beta, -alpha, ply + 1);
                evaluation = -other_result.score;
                current_move_tempered = other_result.is_tempered;
            }
        }
        is_any_tempered |= current_move_tempered;
        board.undo_move(move);
        
        if (stop_search)
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
			update_history_killer(move, depth, ply);
            cutoff_count++;
            moves_before_cutoff += moves_searched;
            break;
        }
        
    }

    bool is_result_tempered=store_tt(hash, depth, original_alpha, beta, best_score, best_move,is_best_move_tempered,is_any_tempered);
    return {best_score,best_move,is_result_tempered};
}
int Engine::score_move(const Move& move, int ply,const Move& tt_move,bool depth_0,const Board& board) {
    /*int history_score= history_scores[to_int(move.move_color)][to_int(move.piece_moved)][move.to_square];
    if (move == tt_move && !depth_0) return 20000;
    if (move.promotion_piece!=PieceType::NONE)
    {
        return 12000 +PIECE_VALUES[to_int(Color::WHITE)][to_int(move.promotion_piece)];
    }

    if (move.piece_captured != PieceType::NONE)
    {   
		int see = see_move(board,move);
        int attacker_val =PIECE_VALUES[to_int(Color::WHITE)][to_int(move.piece_moved)];
        int victim_val=PIECE_VALUES[to_int(Color::WHITE)][to_int(move.piece_captured)];
        int tiebreak = (victim_val - attacker_val) / 16;
        if (see >= 0) return 10000 + see + tiebreak;
        else return -10000 + see+tiebreak;
    }
    int score = history_score;
    if (move == killer_moves[ply][0] || move== killer_moves[ply][1])
    {
        return std::max(score,9000);
    }
    if(move.piece_moved==PieceType::PAWN)
    {
        score+= relevant_pawn_push(board, move);
	}
    
    return score;*/

    int stage = 0;
    int sub = 0;

    if (move == tt_move && !depth_0) { stage = 6; sub = 0; }
    else if (move.promotion_piece != PieceType::NONE) {
        stage = 5; sub = PIECE_VALUES[to_int(Color::WHITE)][to_int(move.promotion_piece)] - PIECE_VALUES[to_int(Color::WHITE)][to_int(move.piece_captured)];
    }
    else if (move.piece_captured != PieceType::NONE) {
        int see = see_move(board, move);

        int attacker_val = PIECE_VALUES[to_int(Color::WHITE)][to_int(move.piece_moved)];
        int victim_val = PIECE_VALUES[to_int(Color::WHITE)][to_int(move.piece_captured)];
        int tiebreak = (victim_val - attacker_val) / 16;
        if(see>=0) { stage = 4; sub = see+tiebreak; }
		else { stage = 1; sub = see; }
    }
    else if(move == killer_moves[ply][0] || move == killer_moves[ply][1]) { stage = 3; sub = 0; }
    else {
        stage = 2; sub = history_scores[to_int(move.move_color)][to_int(move.piece_moved)][move.to_square] +relevant_pawn_push(board,move);
	}
	return stage * 100000 + sub;
}      
void Engine::sort_moves(MoveList& moves,const Board& board, int ply,const Move& tt_move,bool tt_depth_0){
    std::vector<std::pair<int, Move>> scored;
    scored.reserve(moves.size());
    for (const auto& m : moves) scored.emplace_back(score_move(m, ply, tt_move,tt_depth_0,board), m);

    std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) { return a.first > b.first; });

    for (size_t i = 0; i < moves.size(); ++i) moves[i]=scored[i].second;
}
int Engine::quiescence_search(Board& board,int alpha, int beta,int ply){
    if (ply>=MAX_QUIET_PLY) return board.is_white_to_move() ? evaluate(board): -evaluate(board);

    uint64_t hash=board.get_hash();

    int tt_score;
    Move tt_move;
    bool depth_0=0;
    if (probe_tt(hash, 0 , alpha, beta, tt_score, tt_move,depth_0,TTMode::Quiescence)) {
        return tt_score;
    }
    qnodes++;
	int stand_pat_score = board.is_white_to_move() ? evaluate(board) : -evaluate(board);
    if (stand_pat_score >= beta) {
        Move move;
        store_tt(hash, 0, alpha, beta, stand_pat_score, move,false,false, TTMode::Quiescence);
        return stand_pat_score;
    } 

    int original_alpha=alpha;
    alpha=std::max(alpha,stand_pat_score);
    MoveList moves_to_search;
	bool evade_check = board.in_check();
    if (evade_check)
        MoveGenerator::generate_moves(board,moves_to_search); // evasions
    else
        MoveGenerator::generate_captures(board,moves_to_search); // captures only

    //auto cap_score = [&](const Move& m) {

    //    int victim = PIECE_VALUES[0][to_int(m.piece_captured)];
    //    int attacker = PIECE_VALUES[0][to_int(m.piece_moved)];
    //    return victim * 16 - attacker; // larger victim first, cheaper attacker first
    //    };
    //std::sort(moves_to_search.begin(), moves_to_search.end(),
    //    [&](const Move& a, const Move& b) {
    //        return cap_score(a) > cap_score(b);
    //    });
    int best_score=stand_pat_score;

    Move best_move;
    bool searched = !moves_to_search.empty();
    int scores[256];
    score_quiet_moves(moves_to_search,scores,board,evade_check);
    for (int i = 0; i < (int)moves_to_search.size(); ++i)
    {
        pick_best(moves_to_search, scores, i);
        //if (scores[i] == NEG_SEE_SCORE) break;
        Move move = moves_to_search[i];
        if (!evade_check) {
        int gain = 0;
        gain += PIECE_VALUES[0][to_int(move.piece_captured)];
        if (move.promotion_piece != PieceType::NONE)
            gain += PIECE_VALUES[0][to_int(move.promotion_piece)] - PIECE_VALUES[0][to_int(PieceType::PAWN)];
        if (stand_pat_score + gain + DELTA_MARGIN < alpha) continue;
        }
        board.make_move(move);

        int score=quiescence_search(board,-beta,-alpha,ply+1);
        score=-score;

        board.undo_move(move);

        if (score>best_score) best_move=move;
        best_score=std::max(best_score,score);

        alpha=std::max(alpha,best_score);
        if (alpha>=beta) break;
    }
    if (alpha >= beta || ply < 2 && searched) {
        //store_tt(hash, 0, original_alpha, beta, best_score, best_move,false,false, TTMode::Quiescence);
    }
    return best_score;
}
uint64_t Engine::perft_driver(Board& board, int depth, int original_depth){
    if (depth==0){
        // if (board.in_check() && MoveGenerator::generate_moves(board).empty())
        // {
        //     checkmate_count+=1;
        // }
        
        
        return 1;
        
    }

    
    
    uint64_t nodes=0;
	MoveList legal_moves;
    MoveGenerator::generate_moves(board,legal_moves);
    int new_nodes=0;
    for (const Move& move : legal_moves)
    {   
        
        // if(move.is_en_passant) ep_count+=1;
        // if(move.piece_captured!=PieceType::NONE) capture_count+=1;
        board.make_move(move);
        // if (depth==1 and move.is_en_passant)
        // {
            
        //     special_boards.push_back(board);
        // }
        
        // if(board.in_check()){
        //     checks_count+=1;
        // }
        
		evaluate(board);
     
        new_nodes=perft_driver(board,depth-1,original_depth);
        nodes+=new_nodes;
        board.undo_move(move);
        // if (depth==original_depth)
        // {
        //     std::cout << to_san(move,legal_moves) << "    " << new_nodes << std::endl;
        // }
        
    }

    return nodes;  
}

PerftRes Engine::perft_test(Board& board, int depth) {
    std::cout << "Starting perft test to depth " << depth << std::endl;
    
    // Record the starting hash and time
    uint64_t start_hash = board.get_hash();
    auto start_time = std::chrono::steady_clock::now();

    // Call the recursive helper
    uint64_t nodes_found = perft_driver(board, depth,depth);

    // Record the end time and calculate duration
    auto end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;
    uint64_t end_hash = board.get_hash();

    // Print the results
    std::cout << "--------------------" << std::endl;
    std::cout << "Perft test complete." << std::endl;
    std::cout << "Depth: " << depth << std::endl;
    std::cout << "Nodes found: " << nodes_found << std::endl;
    std::cout << "Time elapsed: " << duration.count() << "s" << std::endl; 
    if (duration.count() > 0) {
        std::cout << "Nodes per second: " << static_cast<uint64_t>(nodes_found / duration.count()) << std::endl;
    }

    // Crucial check: verify that the hash is the same after all moves
    if (start_hash == end_hash) {
        std::cout << "Zobrist hash is correct!" << std::endl;
    } else {
        std::cout << "!!! ZOBRIST HASH FAILED !!!" << std::endl;
        std::cout << "Start Hash: " << start_hash << ", End Hash: " << end_hash << std::endl;
    }
    std::cout << "--------------------" << std::endl;
    return { duration.count(), nodes_found };
}
TimeControlDecision Engine::decide_time_control(const Board& position, const SearchLimits& limits) {
    TimeControlDecision tc{};
    tc.max_depth = limits.depth > 0 ? limits.depth : INFINITE_DEPTH;
    if (limits.movetime > 0) {
        tc.time_ms = limits.movetime;
    }
    else if (limits.wtime > 0 || limits.btime > 0) {
        int time_left = (position.get_turn() == Color::WHITE) ? limits.wtime : limits.btime;
        int inc = (position.get_turn() == Color::WHITE) ? limits.winc : limits.binc;

        tc.time_ms = time_left / 40 + inc;
        if (tc.time_ms > time_left / 2) tc.time_ms = time_left / 2;
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
bool Engine::probe_tt(uint64_t hash, int depth, int alpha, int beta, int& out_score, Move& out_move,bool depth_0,TTMode mode) {
    if (mode == TTMode::Negamax) tt_probes_nm++;
    else tt_probes_qs++;
    TTCluster& cluster = tt[hash % tt.size()];
    bool hits = false;
    for (int i = 0; i < 4; i++) {
		TTEntry& entry = cluster.entries[i];
		if (entry.key() != (static_cast<uint16_t>(hash >> 48))) continue;

        out_move = entry.move();
        out_score = entry.score();
        if (entry.depth() < depth) {
            if (mode == TTMode::Negamax) tt_misses_nm++;
            else tt_misses_qs++;
            return false;
        }

        if (entry.flag() == TEMPERED) {
            tt_tempered_flag_found++; return false;
        }
        int score = entry.score();
        int a = alpha, b = beta;
        if (entry.flag() == EXACT) {

            if (mode == TTMode::Negamax) tt_hits_nm++;
            else tt_hits_qs++;
            if (out_move.from_square == -1) return false;
            return true;
		}
        if (entry.flag() == LOWERBOUND) a = std::max(a, score);
        if (entry.flag() == UPPERBOUND) b = std::min(b, score);
        
        if (beta - alpha > 1) {
            alpha = a;
            beta = b;
            if (mode == TTMode::Negamax) tt_hits_nm++;
            else tt_hits_qs++;
            hits = true;
        }
        if (a >= b) {
            if (!hits) {
                if (mode == TTMode::Negamax) tt_hits_nm++;
                else tt_hits_qs++;
            }
            if (out_move.from_square == -1) return false;
            return true;
        }
    }
    if (!hits) {

        if (mode == TTMode::Negamax) tt_misses_nm++;
        else tt_misses_qs++;
    }
    return false;

}
bool Engine::store_tt(uint64_t hash, int depth, int original_alpha, int beta, int best_score, Move& best_move,bool is_best_tempered, bool is_any_tempered, TTMode mode) {
    bool score_tempered=false;
    TTFlag flag_to_store;
    // Do some position from repeat logic here

    if (is_best_tempered) {
        flag_to_store = TEMPERED;
        tt_temp_flag_stored++;
        score_tempered = true;
    }
    else if (is_any_tempered) {
        if (best_score >= beta) {
            flag_to_store = LOWERBOUND;
        }
        else if (best_score >= original_alpha) {
            flag_to_store = LOWERBOUND;
            tt_temp_not_exact++;
        }
        else {
            tt_temp_flag_stored++;
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

    TTEntry entry = TTEntry(best_score, depth, flag_to_store, generation, best_move, static_cast<uint16_t>(hash >> 48));
	TTCluster& cluster = tt[hash % tt.size()];
    for(int i=0;i<4;i++){
        // If key already exists in cluster, update that slot.
		uint16_t key16 = uint16_t(hash);
        if (cluster.entries[i].key() == key16) {
            if (cluster.entries[i].depth() <= depth) {
                cluster.entries[i] = entry;
                if (mode == TTMode::Negamax) tt_updates_nm++;
                else tt_updates_qs++;
            }

            
            return score_tempered;
        }
	}
    for (int i = 0; i < 4; i++) {
        // Find an empty slot to store the new entry.
        if (cluster.entries[i].empty()) {
            cluster.entries[i] = entry;

            if (mode == TTMode::Negamax) tt_stores_nm++;
            else tt_stores_qs++;
			return score_tempered;
        }
    }

	// If no empty slot, replace the least recently used (last) entry

    int pos_index = -1;
    uint16_t max_generation_diff = 0;
    int pos_depth = depth;
    for (size_t i = 0; i < 4; ++i) {
        const auto& e = cluster.entries[i];
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
        cluster.entries[pos_index] = entry;
        if (mode == TTMode::Negamax)
            tt_overwrites_nm++;
        else tt_overwrites_qs++;
        if (mode == TTMode::Negamax) tt_stores_nm++;
        else tt_stores_qs++;
    }
    else {
        if (mode == TTMode::Negamax)
            tt_skip_same_gen_nm++;
        else tt_skip_same_gen_qs++;
    }
    return score_tempered;
   

}
bool Engine::should_futility_prune(int depth, int eval, int alpha, bool in_check,const Move& move) {
	if (depth > 2) return false;
    bool is_quiet = move.piece_captured == PieceType::NONE && move.promotion_piece == PieceType::NONE;
    if (in_check || !is_quiet) return false;
    if (depth == 1 && eval + FUTILITY_MARGIN_D1 <= alpha) return true;
    if (depth == 2 && eval + FUTILITY_MARGIN_D2 <= alpha) return true;
    return false;
}
int Engine::late_move_reduction(int depth, int moves_searched, const Move& move, int ply) {
	bool is_capture = (move.piece_captured != PieceType::NONE);
	bool is_promotion = (move.promotion_piece != PieceType::NONE);
	bool is_killer = (ply > 0 && (move == killer_moves[ply][0] || move == killer_moves[ply][1]));
	bool is_special_move = is_capture || is_promotion || is_killer;
    if (!is_special_move && depth >= 3 && moves_searched > 3) return 2;
	return 0;
}
bool Engine::try_null_move_pruning(Board& board, bool king_is_in_check, int depth, int alpha, int beta, int ply, int& out_score) {
	bool is_mate_score_possible = (alpha >= MATE_THRESHOLD || beta <= -MATE_THRESHOLD);

    if(is_mate_score_possible|| depth < 3 || king_is_in_check || !board.has_enough_material_for_nmp()) {
        return false;
	}
	int original_ep_square = board.make_null_move();
	int null_move_score = negamax(board, depth - 3, -beta, -beta + 1, ply + 1).score;
	null_move_score = -null_move_score;
	board.undo_null_move(original_ep_square);
    if (stop_search) {
        out_score = 0;
        return true;
    }
    if (null_move_score >= beta) {
        out_score = beta;
        return true;
    }
	return false;
}
SearchResult Engine::terminal_eval(const Board& board, bool king_is_in_check) {
    if (king_is_in_check) {
		return { -MATE_SCORE, Move() };
    }
    else return { 0,Move() };
}
void Engine::update_history_killer(const Move& move, int depth, int ply) {

    if (move.piece_captured == PieceType::NONE)
    {
        killer_moves[ply][1] = killer_moves[ply][0];
        killer_moves[ply][0] = move;
    }
    int bonus = depth * depth;
    history_scores[to_int(move.move_color)][to_int(move.piece_moved)][move.to_square] += bonus;
}
void Engine::init_tt(size_t tt_size_mb) {
    size_t bytes = tt_size_mb * 1024ull * 1024ull;
    size_t clusters = bytes / sizeof(TTCluster);
    if (clusters == 0) clusters = 1;
    tt.resize(clusters);
}
bool Engine::move_could_result_in_repetition(Board& board, Move& move, int count) {
    if (move.piece_captured != PieceType::NONE || move.piece_moved == PieceType::PAWN || move.is_castle) return false;
    if (board.any_appeared_more_than(2)) return true;
	/*recover_move_fully(move, board);
    board.make_move(move);
    have_to_make_move_count++;
	bool result = board.any_appeared_more_than(2);
    board.undo_move(move);
    return result;*/
    return false;
}
void Engine::recover_move_fully(Move& move,const Board& board) {
    move.move_color = board.get_turn();
    move.piece_moved = board.get_piece_on_square(move.from_square);
    move.piece_captured = board.get_piece_on_square(move.to_square);
	move.is_castle = move.piece_moved == PieceType::KING && std::abs(move.to_square - move.from_square) == 2;
    move.is_en_passant = move.piece_moved == PieceType::PAWN && std::abs(move.to_square - move.from_square) == 16;
}
void Engine::score_moves_NEW(const MoveList& moves, int* scores,
    int ply, const Move& tt_move, bool tt_depth_0,const Board& board) {
    for (int i = 0; i < (int)moves.size(); ++i)
        scores[i] = score_move(moves[i], ply, tt_move, tt_depth_0,board);
}
void Engine::score_quiet_moves(const MoveList& moves, int* scores,const Board& board,bool evade_check) {
    for (int i = 0; i < (int)moves.size(); ++i) {
        scores[i] = 0;
		const Move& m = moves[i];
        if (!evade_check) {
            int see = see_move(board, m);
            scores[i] += see;
        }
        
        int victim = PIECE_VALUES[0][to_int(m.piece_captured)]/100;
        int attacker = PIECE_VALUES[0][to_int(m.piece_moved)]/100;
        scores[i]+= victim - attacker;
    }
}
int Engine::relevant_pawn_push(const Board& board, const Move& move) {
    if (move.piece_moved != PieceType::PAWN) return 0;
    int score = 0;
    Color color = board.get_turn();
    int king_square = board.get_king_square(flip_color(color));
    uint64_t king_zone = KING_ZONE[king_square];
    if(king_zone & bit64(move.to_square))
    {
        score += 100; // pawn push into opponent king zone
	}
    if (color == Color::WHITE) {
        if (move.to_square>=32) score+=20; // pushed to 5th rank or beyond
        if (move.to_square>=40) score+=20; // pushed to 4th rank
		if (move.to_square >= 48) score+= 20; // pushed to 3rd rank
    }
    else {
		if (move.to_square < 32) score += 20; // pushed to 5th rank or beyond
		if (move.to_square < 24) score += 20; // pushed to 4th rank
		if (move.to_square < 16) score += 20; // pushed to 3rd rank
    }
    if (board.is_free_file(move.to_square, color))
    {
        score += 15; // pawn push to free file
    }
	return score;
}