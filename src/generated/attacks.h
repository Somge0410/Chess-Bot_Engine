#pragma once

#include "types.h"
#include "Square.h"
#include "attack_rays.h"
#include "bishop_tables.h"
#include "rook_tables.h"

inline Bitboard knight_attacks(Square from) noexcept {
	return KNIGHT_ATTACKS[square_index(from)];
}
inline Bitboard bishop_attacks(Square from, Bitboard occupied) noexcept {
	if (from == NO_SQUARE) return 0;
	const int sq = square_index(from);
	const Bitboard blockers = BISHOP_BLOCKER_MASK[sq] & occupied;
	const auto index = (blockers * MAGIC_BISHOP_NUMBER[sq]) >> BISHOP_SHIFT_NUMBERS[sq];
	return BISHOP_ATTACK_TABLE[BISHOP_ATTACK_OFFSET[sq] + index];
}
inline Bitboard rook_attacks(Square from, Bitboard occupied) 
{
	if (from == NO_SQUARE) return 0;
	const int sq = square_index(from);
	uint64_t rook_blockers = ROOK_BLOCKER_MASK[sq] & occupied;
	uint64_t index = (rook_blockers * MAGIC_ROOK_NUMBER[sq]) >> ROOK_SHIFT_NUMBERS[sq];
	return ROOK_ATTACK_TABLE[ROOK_ATTACK_OFFSET[sq] + index];
}
inline Bitboard queen_attacks(Square from, Bitboard occupied) {
	return bishop_attacks(from, occupied) | rook_attacks(from, occupied);
}
inline Bitboard king_attacks(Square from) {
	return KING_ATTACKS[square_index(from)];
}
inline Bitboard pawn_attacks(Square from, Color color) {
	return PAWN_ATTACKS[color_index(color)][square_index(from)];
}
inline Bitboard piece_attacks(Square from, Color attacker_color, Bitboard occupied, PieceType piece_type) {
	switch (piece_type) {
		case PieceType::Pawn:
			return pawn_attacks(from, attacker_color);
		case PieceType::Knight:
			return knight_attacks(from);
		case PieceType::Bishop:
			return bishop_attacks(from, occupied);
		case PieceType::Rook:
			return rook_attacks(from, occupied);
		case PieceType::Queen:
			return queen_attacks(from, occupied);
		case PieceType::King:
			return king_attacks(from);
		default:
			return 0ULL;
	}
}

inline Bitboard pawn_attacks(Bitboard pawns,Color color) {
	Bitboard attacks = 0ULL;
	while (pawns) {
		Square from = pop_lsb(pawns);
		attacks |= pawn_attacks(from, color);
	}
	return attacks;
}