#include "notation_utils.h"
#include "constants.h"
#include <vector>
#include "utils.h"
// Helper function

std::string square_to_algebraic(int square){
    char file=(square % 8)+'a';
    char rank=(square /8)+'1';
    return {file,rank};
}

std::string to_san(const Move& move, const std:: vector<Move>& all_legal_moves){
    if (move.is_castle){
        return (move.to_square%8 ==6) ? "O-O" : "O-O-O";
    }

    std::string piece_symbol={PIECE_CHAR_LIST[to_int(move.piece_moved)]};
    std::string dest_square = square_to_algebraic(move.to_square);
    std::string capture_symbol=(move.piece_captured!= PieceType::NONE) ? "x" : "";

    if (move.piece_moved == PieceType::PAWN){
        std::string notation;
        if (!capture_symbol.empty()){
            notation=std::string(1,square_to_algebraic(move.from_square)[0])+capture_symbol+dest_square;
        } else {
            notation=dest_square;
        }
        if (move.promotion_piece !=PieceType::NONE){
            notation+="="+std::string(1,PIECE_CHAR_LIST[to_int(move.promotion_piece)]);
        }
        return notation;

    } else {
        std::string disambiguation_str="";
        std::vector<Move> competitors;
        for (const auto& other_move: all_legal_moves){
            if (other_move.piece_moved == move.piece_moved &&
                other_move.to_square == move.to_square &&
                other_move.from_square != move.from_square) {
                competitors.push_back(other_move);
            }
        }

        if (!competitors.empty()){
            int from_file= move.from_square % 8;
            bool same_file=false;
            for (const auto& other_move : competitors){
                if (other_move.from_square %8==from_file){
                    same_file=true;
                    break;
                }
            }
        if (!same_file){
            disambiguation_str=square_to_algebraic(move.from_square)[0];
        }else{
            int from_rank=move.from_square /8;
            bool same_rank=false;
            for (const auto & other_move : competitors){
                if (other_move.from_square/8==from_rank){
                    same_rank=true;
                    break;
                }
            }
            if (!same_rank){
                disambiguation_str=square_to_algebraic(move.from_square)[1];
            }else {
                disambiguation_str=square_to_algebraic(move.from_square);
            }
        }
     }   
    return piece_symbol+disambiguation_str+capture_symbol+dest_square;

    }
}