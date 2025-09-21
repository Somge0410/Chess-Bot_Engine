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
Move Engine::search(const Board& position, int max_depth, int time_limit_seconds) {
   

    // Set the time control member variables for this search
    this->start_time = std::chrono::steady_clock::now();
    this->time_limit = std::chrono::seconds(time_limit_seconds);
    this->stop_search=false;
    Move best_move_so_far;
    double best_score_so_far = 0;

    // --- The Fix for the const Board ---
    // Create a mutable copy of the board for the search to use
    Board board = position;

        // The iterative deepening loop
        for (int current_depth = 1; current_depth <= max_depth; ++current_depth) {
            std::cout << "Searching with depth " << current_depth << std::endl;

            // CORRECTED: Call negamax with 'this->' and the 'ply' argument
            auto [eval_score, move_this_iteration] = this->negamax(board, current_depth, -MATE_SCORE, MATE_SCORE, 0);
            
            auto end_time = std::chrono::steady_clock::now();
            std::chrono::duration<double> duration = end_time - this->start_time;
            if (stop_search)
            {
                std::cout << "\nTime limit of " << time_limit_seconds << "s reached! Search is canceled." << std::endl;
                if (best_move_so_far.from_square!=-1)
                {   
                    std::vector<Move> legal_moves = MoveGenerator::generate_moves(position);
                        std::cout <<"Final Decision:"
                                  << to_san(best_move_so_far, legal_moves) 
                                    << ", Score: " << best_score_so_far << std::endl;
                    break;
                }
            }
            

            if (move_this_iteration.from_square != -1) { // Check if a valid move was found
                best_move_so_far = move_this_iteration;
                best_score_so_far = eval_score;
                
                // For printing, we need the legal moves. It's slow but okay for logging.
                std::vector<Move> legal_moves = MoveGenerator::generate_moves(board);
                std::cout << "Depth " << current_depth << " ended. Best move so far: " 
                          << to_san(best_move_so_far, legal_moves) 
                          << ", Score: " << eval_score
                          << ", Time: " << duration.count() << "s" << std::endl; // CORRECTED: use .count()
            }

            if (abs(best_score_so_far) >= MATE_SCORE) {
                std::cout << "Mate found, end search." << std::endl;
                break;
            }
        }
        this->transposition_table.clear();
        std::memset(this->killer_moves, 0, sizeof(this->killer_moves));
		std::cout << "hi" << std::endl;
        std::memset(this->history_scores, 0, sizeof(this->history_scores));

    // CORRECTED: Add the missing return statement
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
        if (king_is_in_check) return {-MATE_SCORE,Move()};
        else return {0,Move()};
    }
    

    //Futility Purning prerequisites here Here
    double current_eval=-MATE_SCORE;
    if (depth<=2)
    {
        int weight= board.get_turn()==Color::WHITE? 1:-1;
        current_eval=evaluate(board)*weight;
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
        


        board.make_move(move);
        auto [evaluation, returned_move]=negamax(board,depth-1-reduction,-beta,-alpha,ply+1);
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
    std::vector<Move> captures=MoveGenerator::generate_captures(board);
    sort_moves(captures,board,ply);
    double best_score=stand_pat_score;
    Move best_move;
    for (const Move& move : captures)
    {
        int capture_value=PIECE_VALUES[0][to_int(move.piece_captured)];
        if (stand_pat_score+capture_value+DELTA_MARGIN<alpha) continue;
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
		//evaluate(board);
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

void Engine::perft_test(Board& board, int depth) {
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
}
