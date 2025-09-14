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
int main(){
    Zobrist::initialize_keys();
    Engine engine;
    // std::cout << "Loading transposition table..." << std::endl;
    // if (engine.load_tt("engine_cache.tt")) {
    //     std::cout << "Table loaded successfully." << std::endl;
    // } else {
    //     std::cout << "No existing table found. Starting fresh." << std::endl;
    // }
    Board board;
    // std::cout << board.get_material_score() << std::endl;
    
    // // std::cout << board.get_positional_score() << std::endl;
     for (size_t i = 0; i < 7; ++i)
      {
         engine.perft_test(board,i);
     }
     return 0;
    // std::cout << engine.special_boards[0].get_material_score() << std::endl;
    // engine.special_boards[0].display();
    std::cout << "Choose a color, w for white, b for black" << std::endl;
    char color_choice;
    std::cin >> color_choice;
    Color player_color =(color_choice=='w') ? Color::WHITE : Color::BLACK;  
    
    bool game_running =true;

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
                
                Move move_object = parse_move(move_str,legal_moves);
                if (move_object.from_square != -1)
                {
                    board.make_move(move_object);
                    break;
                }else
                {
                    std::cout << "\n!!! Invalid Move. Please try again. !!!\n" << std::endl;
                }
            }
            
        }else
        {
            std::cout << "\nComputer is thinking..." << std::endl;
            Move best_move = engine.search(board,15,30);
            if (best_move.from_square !=-1)
            {
                std::cout << "Computer plays:" << to_san(best_move, legal_moves) << std::endl;
                board.make_move(best_move);
            }else
            {
                std::cout << "Engine cannot find a move. Game over." << std::endl;
                game_running =false;
            }   
        }
    }
    // std::cout << "Saving transposition table..." << std::endl;
    // engine.save_tt("engine_cache.tt");
    // std::cout << "Table saved." << std::endl;

    return 0;


}