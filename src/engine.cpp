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
Move Engine::search(const Board& position, const SearchLimits& limits) {
    // Decide thinking time and max depth
    //position.display();
    int time_ms = -1;
    int max_depth = (limits.depth > 0) ? limits.depth : 64; // some upper bound

    // Case A: movetime
    if (limits.movetime > 0) {
        time_ms = limits.movetime;
    }
    // Case B: wtime/btime (tournament time control)
    else if (limits.wtime > 0 || limits.btime > 0) {
        int time_left = (position.get_turn() == Color::WHITE)
            ? limits.wtime
            : limits.btime;
        int inc = (position.get_turn() == Color::WHITE)
            ? limits.winc
            : limits.binc;

        // simple rule-of-thumb
        time_ms = time_left / 40 + inc;

        // safety caps
        if (time_ms > time_left / 2)
            time_ms = time_left / 2;

    }
    // Case C: fixed depth only
    else if (limits.depth > 0) {
        time_ms = 100000000; // effectively "infinite" for practical purposes
        max_depth = limits.depth;
    }
    // Case D: infinite analysis
    else if (limits.infinite) {
        time_ms = 100000000; // very large
        // max_depth stays large, you'll rely on stop_search
    }
    // Fallback default if nothing else was set
    if (time_ms < 0) {
        time_ms = 10000; // 10 seconds default
    }

    // Set engine timers
    this->start_time = std::chrono::steady_clock::now();
    this->time_limit = std::chrono::milliseconds(time_ms);
    this->stop_search = false;

    Move   best_move_so_far;
    double best_score_so_far = 0;

    // Mutable copy of the board
    Board board = position;

    for (int current_depth = 1; current_depth <= max_depth; ++current_depth) {
        if (this->stop_search)
            break; // in case some external flag already asked us to stop

        auto [eval_score, move_this_iteration] =
            this->negamax(board, current_depth, -MATE_SCORE, MATE_SCORE, 0);

        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - this->start_time;

        // Check time inside the loop too (negamax should also check a stop flag)
        if (elapsed >= std::chrono::duration<double, std::milli>(time_ms) || stop_search) {
            std::cerr << "\nTime limit of " << (time_ms / 1000.0)
                << "s reached! Search is canceled.\n";
            break;
        }

        if (move_this_iteration.from_square != -1) {
            best_move_so_far = move_this_iteration;
            best_score_so_far = eval_score;

            // For logging: legal moves from root position
            std::vector<Move> legal_moves = MoveGenerator::generate_moves(position);
            std::cerr << "Depth " << current_depth << " ended. Best move so far: "
                << to_san(best_move_so_far, legal_moves)
                << ", Score: " << eval_score
                << ", Time: " << elapsed.count() << "s\n";
        }

        if (std::abs(best_score_so_far) >= MATE_SCORE) {
            std::cerr << "Mate found, end search.\n";
            break;
        }
    }

    // Optional: clear heuristics between moves
    this->transposition_table.clear();
    std::memset(this->killer_moves, 0, sizeof(this->killer_moves));
    std::memset(this->history_scores, 0, sizeof(this->history_scores));

    return best_move_so_far;
}
std::pair<double,Move> Engine::negamax(Board& board, int depth, double alpha, double beta, int ply){
    if (std::chrono::steady_clock::now()-start_time>=time_limit)
    {
        stop_search=true;
        return {0,Move()};
    }
    
    uint64_t hash=board.get_hash();
    double original_alpha=alpha;
    auto it =transposition_table.find(hash);

    if (it!= transposition_table.end())
    {
        TTEntry& entry=it->second;

        if (entry.depth>=depth)
        {
            if (entry.flag == EXACT)
            {
                return {entry.score,entry.best_move};
            }
            if (entry.flag ==LOWERBOUND)
            {
                alpha = std::max(alpha,entry.score);
            }
            if (entry.flag == UPPERBOUND)
            {
                beta = std::min(beta,entry.score);
            }
            
            if (alpha>=beta)
            {
                return {entry.score,entry.best_move};
            }
            
            
            
        }
        
    }
    
    if (depth==0)
    {
        double q_score=quiescence_search(board,alpha,beta,0);
        
        return {q_score,Move()};
    }
    
    // NULL Move Pruning Here
    bool is_mate_score_possible = (alpha>=MATE_THRESHOLD || beta<=-MATE_THRESHOLD);
    bool king_is_in_check=board.in_check();
    if (!is_mate_score_possible && depth>=3 && !king_is_in_check && board.has_enough_material_for_nmp())
    {
        int original_ep_square=board.make_null_move();
        auto [null_move_score,null_move_move]=negamax(board,depth-3,-beta,-beta+1,ply+1);
        null_move_score=-null_move_score;
        board.undo_null_move(original_ep_square);
        if (null_move_score>=beta)
        {
            return {beta,Move()};
        }
        
        if (stop_search)
        {
            return {0,Move()};
        }
        
    }
    // End of Null-move pruning
    std::vector<Move> moves=MoveGenerator::generate_moves(board);
    sort_moves(moves,board,ply);
    double best_score=-MATE_SCORE;
    Move best_move;
    if (moves.empty())
    {
        if (king_is_in_check) {
            if(board.is_fifty_move_rule_draw() || board.is_repetition_draw()) {
                return {0, Move()}; // Stalemate due to 50-move rule or repetition
            }
            else {

                return { -MATE_SCORE,Move() };
            }
        }
        else return {0,Move()};
    }
    

    //Futility Purning prerequisites here Here
    double current_eval=-MATE_SCORE;
    if (depth<=2)
    {
        int weight= board.get_turn()==Color::WHITE? 1:-1;
        current_eval=evaluate(board,EVAL_MATERIAL|EVAL_POSITIONAL|EVAL_PAWN_STRUCTURE)*weight;
    }
    

    int moves_searched=0;
    for (const Move& move : moves)
    {   
        bool is_quiet_move= move.piece_captured==PieceType::NONE && move.promotion_piece==PieceType::NONE;
        // Now do futility pruning. If positions evaluation is already way worse than alpha, cut it off since it is
        //unlikely to get that much better in just 1 or two moves
        if (current_eval!=-MATE_SCORE && is_quiet_move && !king_is_in_check)
        {
            if (depth==1 && current_eval+FUTILITY_MARGIN_D1<=alpha) continue;
            if (depth==2 && current_eval+FUTILITY_MARGIN_D2<=alpha) continue;
        }
        moves_searched+=1;
        int reduction=0;
        bool is_special_move=(move.piece_captured!=PieceType::NONE 
                            || move.promotion_piece!=PieceType::NONE
                            || (ply > 0 && (move== killer_moves[ply][0] || move==killer_moves[ply][1])));
        if (!is_special_move && depth>=3 && moves_searched>3) reduction=2;
        

        int extension = 0;
        board.make_move(move);
        if (board.in_check() && ply<64)
        {
            extension=1;
		}
        auto [evaluation, returned_move]=negamax(board,depth-1-reduction+extension,-beta,-alpha,ply+1);
        evaluation=-evaluation;
        
        if (reduction>0 && evaluation>alpha)
        {
            evaluation=negamax(board,depth-1,-beta,-alpha,ply+1).first;
            evaluation=-evaluation;
        }
        
        board.undo_move(move);
        
        if (stop_search)
        {
            return{0,Move()};
        }
        
        if (evaluation>best_score)
        {
            best_score=evaluation;
            best_move=move;
        }

        alpha=std::max(alpha,best_score);
        if (beta<=alpha)
        {   
            if (move.piece_captured == PieceType::NONE)
            {
                killer_moves[ply][1]=killer_moves[ply][0];
                killer_moves[ply][0]=move;
            }
            int bonus=depth*depth;
            history_scores[to_int(move.move_color)][to_int(move.piece_moved)][move.to_square]+=bonus;
            
            break;
        }
        
    }
    TTFlag flag_to_store;
    // Do some position from repeat logic here
    if (best_score>=beta)
    {
        flag_to_store=LOWERBOUND;
    }else if (best_score<=original_alpha)
    {
        flag_to_store=UPPERBOUND;
    }else
    {
        flag_to_store=EXACT;
    }
    TTEntry entry={
        .score=best_score,
        .depth=depth,
        .flag=flag_to_store,
        .best_move=best_move
    };
    transposition_table[hash]=entry;
    return {best_score,best_move};
}
int Engine::score_move(const Move& move, const Board& board, int ply) {
    auto tt_entry_it = transposition_table.find(board.get_hash());
    if (tt_entry_it!=transposition_table.end())
    {
        if (move == tt_entry_it-> second.best_move)
        {
            return 20000;
        }
        
    }
    
    if (move.promotion_piece!=PieceType::NONE)
    {
        return 12000 +PIECE_VALUES[to_int(Color::WHITE)][to_int(move.promotion_piece)];
    }

    if (move.piece_captured != PieceType::NONE)
    {
        int attacker_val =PIECE_VALUES[to_int(Color::WHITE)][to_int(move.piece_moved)];
        int victim_val=PIECE_VALUES[to_int(Color::WHITE)][to_int(move.piece_captured)];
        return 10000+victim_val-attacker_val;
    }
    if (move == killer_moves[ply][0] || move== killer_moves[ply][1])
    {
        return 9000;
    }
    return history_scores[to_int(move.move_color)][to_int(move.piece_moved)][move.to_square];
    
}      
void Engine::sort_moves(std::vector<Move>& moves,const Board& board, int ply){
    std::sort(moves.begin(),moves.end(),
        [&](const Move&a, const Move&b){
            return score_move(a,board,ply)> score_move(b,board,ply);
        }
    );
}
double Engine::quiescence_search(Board& board,double alpha, double beta,int ply){
    if (ply>=7) return evaluate(board);
    uint64_t hash=board.get_hash();
    auto it =transposition_table.find(hash);

    if (it!= transposition_table.end())
    {
        TTEntry& entry=it->second;

        
        if (entry.flag == EXACT)
        {
            return entry.score;
        }
        if (entry.flag ==LOWERBOUND)
        {
            alpha = std::max(alpha,entry.score);
        }
        if (entry.flag == UPPERBOUND)
        {
            beta = std::min(beta,entry.score);
        }
            
        if (alpha>=beta)
        {
            return entry.score;
        }
    }
    int weight=board.get_turn()==Color::WHITE ? 1:-1;
    double stand_pat_score= evaluate(board) * weight;
    if (stand_pat_score>=beta) return stand_pat_score;
    double original_alpha=alpha;
    alpha=std::max(alpha,stand_pat_score);
    std::vector<Move> moves_to_search;
    if(ply==0) moves_to_search=MoveGenerator::generate_captures_with_checks(board);
	else moves_to_search = MoveGenerator::generate_captures(board);
    sort_moves(moves_to_search,board,ply);
    double best_score=stand_pat_score;
    Move best_move;
    for (const Move& move : moves_to_search)
    {   
        if (move.piece_captured != PieceType::NONE) {

            int capture_value = PIECE_VALUES[0][to_int(move.piece_captured)];
            if (stand_pat_score + capture_value + DELTA_MARGIN < alpha) continue;
        }
        board.make_move(move);
        double score=quiescence_search(board,-beta,-alpha,ply+1);
        score=-score;
        board.undo_move(move);
        if (score>best_score) best_move=move;
        best_score=std::max(best_score,score);
        alpha=std::max(alpha,best_score);
        if (alpha>=beta) break;
    }
    if (it == transposition_table.end() || (it!=transposition_table.end() && it->second.depth == 0)){
            TTFlag flag_to_store;
            if (best_score>=beta)
            {
                flag_to_store=LOWERBOUND;
            }else if (best_score<=original_alpha)
            {
                flag_to_store=UPPERBOUND;
            }else
            {
                flag_to_store=EXACT;
            }
                TTEntry entry={
                .score=best_score,
                .depth=0,
                .flag=flag_to_store,
                .best_move=best_move
            };
            transposition_table[hash]=entry;
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
    std::vector<Move> legal_moves=MoveGenerator::generate_moves(board);
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
