#pragma once
#include <cstddef>
#include <cmath>
#include <string>
#include "types.h"
enum MoveFlag : uint8_t {
    Castle = 1 << 0,
    EnPassant = 1 << 1,
    GivesDirectCheck = 1 << 2,
    GivesDiscoveredCheck = 1 << 3,
    CheckKnown = 1 << 4,
};
struct Move{
    Square from_square;
    Square to_square;

    PieceType piece_moved;
    PieceType piece_captured;
    PieceType promotion_piece;
    Color move_color;
    uint8_t flags;
    Move()
        : from_square(NO_SQUARE),
          to_square(NO_SQUARE),
          piece_moved(PieceType::None),
          piece_captured(PieceType::None),
          promotion_piece(PieceType::None),
          move_color(Color::White),
          flags(0)
    {
    }
    Move(int from, int to, PieceType moved, Color color,
        PieceType captured = PieceType::None,
        PieceType promo = PieceType::None,
        bool castle = false,
        bool en_passant = false,
        bool direct_check = false,
        bool discovered_check = false,
        bool check_known = true) :
         from_square(static_cast<Square>(from)),
         to_square(static_cast<Square>(to)),
	         piece_moved(moved),
	         piece_captured(captured),
	         promotion_piece(promo),
	         move_color(color),
	         flags(static_cast<uint8_t>(
	             (castle ? MoveFlag::Castle : 0)
	             | (en_passant ? MoveFlag::EnPassant : 0)
	             | (direct_check ? MoveFlag::GivesDirectCheck : 0)
	             | (discovered_check ? MoveFlag::GivesDiscoveredCheck : 0)
	             | (check_known ? MoveFlag::CheckKnown : 0)))
    {
    }
    void set_flag(MoveFlag move_flag, bool enabled) {
        if (enabled) {
            flags |= move_flag;
        }
        else {
            flags &= static_cast<uint8_t>(~move_flag);
        }
    }
    bool has_flag(MoveFlag move_flag) const {
        return (flags & move_flag) != 0;
    }
    bool is_castle() const {
        return has_flag(MoveFlag::Castle);
    }
    bool is_en_passant() const {
        return has_flag(MoveFlag::EnPassant);
    }
    void set_castle(bool value) {
        set_flag(MoveFlag::Castle, value);
    }
    void set_en_passant(bool value) {
        set_flag(MoveFlag::EnPassant, value);
    }
    bool is_double_pawn_move() const {
        return piece_moved==PieceType::Pawn && std::abs(to_square-from_square)==16;
    }
    int get_capture_square() const {
        if (is_en_passant()) {
            // For en passant, the captured pawn is on the same file as the 'to_square',
            // but on a different rank.
            return (this->move_color == Color::White)
                ? this->to_square - 8  // White's pawn captures on rank 5
                : this->to_square + 8; // Black's pawn captures on rank 4
        } else {
            return this->to_square;
        }
    }
    Color get_capture_color() const {
        return (move_color==Color::White) ? Color::Black:Color::White;
    }
    bool operator==(const Move& other) const {
        return from_square==other.from_square &&
        to_square == other.to_square &&
        promotion_piece == other.promotion_piece;
    }
    uint16_t get_int() const {
        uint16_t move = 0;
        if (from_square == NO_SQUARE || to_square == NO_SQUARE) return 1u <<15;
        move |= from_square;
        move |= to_square << 6;
        move |= uint16_t((static_cast<uint16_t>(promotion_piece) & 0x0F) << 12);
        return move;
    }
    bool direct_check() const {
        return has_flag(MoveFlag::GivesDirectCheck);
    }
    bool discovered_check() const {
        return has_flag(MoveFlag::GivesDiscoveredCheck);
    }
    bool check_is_known() const {
        return has_flag(MoveFlag::CheckKnown);
    }
    bool gives_check() const {
        return direct_check() || discovered_check();
    }
    bool is_quiet() const {
        return piece_captured == PieceType::None &&
            promotion_piece == PieceType::None;
    }
    bool gives_double_check() const {
        return direct_check() && discovered_check();
    }
    PieceType get_piece_reached() const {
		return promotion_piece == PieceType::None ? piece_moved : promotion_piece;
    }

};
inline Move recover_move_from_int(uint16_t m_int) {
    if (m_int == 1u << 15) return Move();
    int from_square = m_int & 0x3F;
    int to_square = (m_int >> 6) & 0x3F;
    int promo_int = (m_int >> 12) & 0x0F;
    return Move(from_square, to_square, PieceType::None, Color::White,
        PieceType::None, static_cast<PieceType>(promo_int),
        false, false, false, false, false);
}

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
