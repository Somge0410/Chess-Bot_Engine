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
    /*board.display();
    int square = 3;
    uint64_t piece_blocker = BISHOP_BLOCKER_MASK[square] & board.get_all_pieces();
    std::cout << piece_blocker << std::endl;
    display_bitboard(piece_blocker);
    uint64_t index = (piece_blocker * MAGIC_BISHOP_NUMBER[square]) >> BISHOP_SHIFT_NUMBERS[square];
    display_bitboard(BISHOP_ATTACK_TABLE[BISHOP_ATTACK_OFFSET[square] + index]);
   */
    //return 0;

    // std::cout << board.get_material_score() << std::endl;
    
    // // std::cout << board.get_positional_score() << std::endl;
    
     for (size_t i = 5; i < 6 ; ++i) 
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
            Move best_move = engine.search(board,20,15);
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