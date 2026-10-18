#pragma once
#include "position.h"
#include "bitboard_masks.h"
#include "bitboard.h"
#include "types.h"
#include "pawn_table.h"
struct EvalContext {
	const Position& pos;
	PawnData* pawn_data;
	Bitboard backward[2];
	mutable uint64_t attacks[2];
	mutable uint8_t attacks_initialized;

	EvalContext(const Position& b, PawnData& data)
		: pos(b),
		pawn_data(&data),
		backward{ 0,0 },
		attacks{ 0,0 },
		attacks_initialized(0) {
	}
	void init_file_info() {
		for (size_t file = 0; file < 8; ++file) {
			uint64_t file_mask = FILE_MASK[file];
			bool white_has_pawns = (pos.get_pieces(Color::White, PieceType::Pawn) & file_mask) != 0;
			bool black_has_pawns = (pos.get_pieces(Color::Black, PieceType::Pawn) & file_mask) != 0;
			if (!white_has_pawns)
				pawn_data->pawnless_files[0] |= bit8(file);
			if (!black_has_pawns)
				pawn_data->pawnless_files[1] |= bit8(file);
		}
	}
	uint64_t get_pieces(Color color, PieceType piece_type) const {
		return pos.get_pieces(color, piece_type);
	}
	uint64_t get_color_pieces(Color color) const {
		return pos.get_color_pieces(color);
	}
	uint64_t get_pieces(int color, int piece_type) const {
		return pos.get_pieces(static_cast<Color>(color), static_cast<PieceType>(piece_type));
	}
	uint64_t get_color_pieces(int color) const {
		return pos.get_color_pieces(static_cast<Color>(color));
	}
	uint64_t get_pieces(int color, PieceType piece_type) const {
		return pos.get_pieces(static_cast<Color>(color), piece_type);
	}
	uint8_t get_open_files() const {
		return pawn_data->pawnless_files[0] & pawn_data->pawnless_files[1];
	}
	uint64_t get_all_pieces() const {
		return pos.get_all_pieces();
	}
	uint64_t get_attacks(Color color) const {
		const int color_index = to_int(color);
		const uint8_t initialized_bit = bit8(color_index);
		if ((attacks_initialized & initialized_bit) == 0) {
			attacks[color_index] = pos.get_attacks_for_color(color);
			attacks_initialized |= initialized_bit;
		}
		return attacks[color_index];
	}
	bool is_file_open(int file) const {
		if (file < 0 || file>7) return false;
		return (get_open_files() & bit8(file)) != 0;
	}
	bool does_color_have_pawns_on_file(int file, Color color) const {
		if (file < 0 || file>7) return false;
		return (pawn_data->pawnless_files[to_int(color)] & bit8(file)) == 0;
	}
	bool color_has_free_pawn(int file, Color color) const {
		if (file < 0 || file>7) return false;
		return does_color_have_pawns_on_file(file, color) && !does_color_have_pawns_on_file(file - 1, flip_color(color)) && !does_color_have_pawns_on_file(file, flip_color(color)) && !does_color_have_pawns_on_file(file + 1, flip_color(color));
	}
	PieceType get_piece_on_square(int square) const {
		return pos.get_piece_type_on_square(static_cast<Square>(square));
	}
};
