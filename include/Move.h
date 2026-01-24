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
    Color move_color;

    bool is_castle;
    bool is_en_passant;
    Move()
        : from_square(-1),
          to_square(-1),
          piece_moved(PieceType::NONE),
          move_color(Color::WHITE),
          piece_captured(PieceType::NONE),
          promotion_piece(PieceType::NONE),
          is_castle(false),
          is_en_passant(false)
    {
    }
    Move(int from, int to, PieceType moved,Color color, PieceType captured=PieceType::NONE,PieceType promo=PieceType::NONE,bool castle=false, bool ente_passente=false):
         from_square(from),
         to_square(to),
         piece_moved(moved),
         move_color(color),
         piece_captured(captured),
         promotion_piece(promo),
         is_castle(castle),
         is_en_passant(ente_passente)
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
    uint16_t get_int() const {
        uint16_t move = 0;
        if (from_square == -1 || to_square == -1) return 1u <<15;
        move |= from_square;
        move |= to_square << 6;
        move |= uint16_t((static_cast<uint16_t>(promotion_piece) & 0x0F) << 12);
        return move;
    }
};
struct MoveList {
    Move moves[256]; int count = 0;
    void push_back(const Move& m) { moves[count++] = m; }
    Move* begin() { return moves; } Move* end() { return moves + count; } const Move* begin() const { return moves; } const Move* end() const { return moves + count; }
    Move& operator[](int index) { return moves[index]; }
    const Move& operator[](int index) const { return moves[index]; }
    size_t size() const { return count; }
    bool empty() const { return count == 0; }
    void clear() { count = 0; }
    void swap_items(int i, int j) {
		std::swap(moves[i], moves[j]);
    }
   
};