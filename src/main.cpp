#include <iostream>
#include "Board.h"
#include "utils.h"    
#include "constants.h" 
#include "zobrist.h"
#include "attack_rays.h"
#include "bitboard_masks.h"
#include "MoveGenerator.h"
#include "notation_utils.h"
#include "evaluation.h"
#include "engine.h"
#include "rook_tables.h"
#include "bishop_tables.h"
#include <chrono>
#include "prepare_data.h"
#include <vector>
#include <iostream>
#include "uci.h"
int main(){
    Zobrist::initialize_keys();
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    uci_loop();
    return 0;
    Engine engine;
    // }
    Board board;
	
    double duration = 0;
    uint64_t nodes = 0;
     for (size_t i = 4; i < 5 ; ++i) 
      {     
         PerftRes x=engine.perft_test(board,5);
         duration += x.duration;
         nodes += x.nodes;
     }
	 std::cout << "Average duration for depth 5 over 100 runs: " << duration / 100 << " ms" << std::endl;
	 std::cout << "Nodes per second:" << nodes / duration << std::endl;
     //return 0;
    std::cout << "Choose a color, w for white, b for black" << std::endl;
    char color_choice;
    std::cin >> color_choice;
    Color player_color =(color_choice=='w') ? Color::WHITE : Color::BLACK;  
    
    bool game_running =true;
	std::vector<Move> move_history;
    move_history.reserve(256);
    while (game_running)
    {
        board.display();
        std::vector<Move> legal_moves = MoveGenerator::generate_moves(board);

        if (legal_moves.empty())
        {
            if (board.in_check())
            {
                std::string winner = board.get_turn() == Color::WHITE ? "Black" : "Whte";
                std::cout << "Checkmate! " << winner << " wins." << std::endl;
            }else
            {
                std::cout << "Stalement! It's a draw." << std::endl;
            }
            game_running=false;
            continue;
        }
        if (board.get_turn() == player_color)
        {
            while (true)
            {
                std::cout << "Your Move: ";
                std::string move_str;
                std::cin >> move_str;

                if (move_str == "exit")
                {
                    game_running =false;
                    break;
                }
                if(move_str=="undo")
                {
                    if (!move_history.empty()) {
                        Move last_move = move_history.back();
                        board.undo_move(last_move);
                        move_history.pop_back();
                        if(!move_history.empty()) {
                            last_move = move_history.back();
                            board.undo_move(last_move);
                            move_history.pop_back();
                            std::cout << "Undo move" << std::endl;
                            board.display();
                            continue;
                        }
                        else {

                            std::cout << "Undo move" << std::endl;
                            board.display();
                            break;
                        }
                    } else {
						std::cout << "No moves to undo." << std::endl;
                        board.display();
                        continue;
                    }
                    board.display();
					continue;
				}
                
                Move move_object = parse_move(move_str,legal_moves);
                if (move_object.from_square != -1)
                {
                    board.make_move(move_object);
					move_history.push_back(move_object);
                    break;
                }else
                {
                    std::cout << "\n!!! Invalid Move. Please try again. !!!\n" << std::endl;
                }
            }
            
        }else
        {
            std::cout << "\nComputer is thinking..." << std::endl;
            SearchLimits limits;
			limits.movetime = 15000;
            Move best_move = engine.search(board,limits);
            if (best_move.from_square !=-1)
            {
                std::cout << "Computer plays:" << to_san(best_move, legal_moves) << std::endl;
                board.make_move(best_move);
				move_history.push_back(best_move);
            }else
            {
                std::cout << "Engine cannot find a move. Game over." << std::endl;
                game_running =false;
            }   
        }
    }

    return 0;


}