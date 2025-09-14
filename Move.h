#pragma once
#include "constants.h"
#include <cmath>
#include <string>
struct Move{
    int from_square;
    int to_square;
    PieceType piece_moved;
    PieceType piece_captured;
    PieceType promotion_piece;
    bool is_castle;
    bool is_en_passant;
    std::string old_castling_rights;
    int old_en_passant_square;
    Color move_color;
    Move()
        : from_square(-1),
          to_square(-1),
          piece_moved(PieceType::NONE),
          move_color(Color::WHITE),
          piece_captured(PieceType::NONE),
          promotion_piece(PieceType::NONE),
          is_castle(false),
          is_en_passant(false),
          old_castling_rights(""),
          old_en_passant_square(-1)
    {
    }
    Move(int from, int to, PieceType moved,Color color, PieceType captured=PieceType::NONE, std::string old_castling="", int old_ente_passente=-1,PieceType promo=PieceType::NONE,bool castle=false, bool ente_passente=false):
         from_square(from),
         to_square(to),
         piece_moved(moved),
         move_color(color),
         piece_captured(captured),
         promotion_piece(promo),
         is_castle(castle),
         is_en_passant(ente_passente),
         old_castling_rights(old_castling),
         old_en_passant_square(old_ente_passente)
    {
         }
    bool is_double_pawn_move() const {
        return piece_moved==PieceType::PAWN && std::abs(to_square-from_square)==16;
    }
    int get_capture_square() const {
        if (this->is_en_passant) {
            // For en passant, the captured pawn is on the same file as the 'to_square',
            // but on a different rank.
            return (this->move_color == Color::WHITE)
                ? this->to_square - 8  // White's pawn captures on rank 5
                : this->to_square + 8; // Black's pawn captures on rank 4
        } else {
            return this->to_square;
        }
    }
    Color get_capture_color() const {
        return (move_color==Color::WHITE) ? Color::BLACK:Color::WHITE;
    }
    bool operator==(const Move& other) const {
        return from_square==other.from_square &&
        to_square == other.to_square &&
        promotion_piece == other.promotion_piece;
    }
};