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
int main(){
	
    Zobrist::initialize_keys();
    Engine engine;
    // }
    Board board("8/8/8/3k4/8/8/2P5/K7 w - - 0 1");
    /*int limit = 100000000;
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
    for(size_t i=0; i<limit; ++i){
        board.in_check();
	}
	std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration1 = end_time - start_time;
	start_time = std::chrono::steady_clock::now();
    for (size_t i = 0; i < limit; ++i) {
        board.in_check2();
    }
	end_time = std::chrono::steady_clock::now();    
    std::chrono::duration<double> duration2 = end_time - start_time;
    std::cout << "in_check() time: " << duration1.count() << " seconds" << std::endl;
    std::cout << "in_check2() time: " << duration2.count() << " seconds" << std::endl;
	std::cout << "in_check2() is " << duration1.count() / duration2.count() << " times faster than in_check()" << std::endl;
	std::cout << board.in_check() << " " << board.in_check2() << std::endl;
    return 0;*/
    board.display();
    std::vector<Move> captures = MoveGenerator::generate_captures(board);
    for(Move move : captures) {
		board.make_move(move);
		board.display();
		board.undo_move(move);
        board.display();
	}
   std::cout << captures.size() << std::endl;
   std::cout << MoveGenerator::generate_captures_with_checks(board).size() << std::endl;
   return 0;
     for (size_t i = 0; i < 7 ; ++i) 
      {
         engine.perft_test(board,i);
     }
     return 0;
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
            Move best_move = engine.search(board,25,15);
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