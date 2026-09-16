#pragma once
#include <cctype>
#include <string>
#include <vector>
#include "Move.h"

inline const std::string PIECE_CHAR_LIST = "PNBRQK.";

std::string to_san(const Move & move,const MoveList& all_legal_moves);

inline char get_piece_char(const PieceType& piece, const Color& color) {
    return color == Color::White ? PIECE_CHAR_LIST[to_int(piece)] : tolower(PIECE_CHAR_LIST[to_int(piece)]);
}

inline Move parse_move(const std::string& move_str, MoveList& move_list) {
    for (const Move& move : move_list)
    {
        if (move_str == to_san(move, move_list)) return move;

    }
    return Move();

}
