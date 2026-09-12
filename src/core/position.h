#pragma once
#include "types.h"
class Position {
public:
	Position();
	explicit Position(std::string_view fen_text);

private:
	void rebuild_derived_state();

	PieceBoards pieces{};
	Bitboard color_pieces[2]{};
	Bitboard all_pieces{};

	Color side_to_move{ Color::White };
	CastlingRights castling_rights{};
	EnPassantRights en_passant_square{ NO_SQUARE };
	int halfmove_clock{ 0 };
	int full_move_number{ 1 };
	int game_phase{ 24 };


	std::uint64_t zobrist_hash{ 0 };
	std::uint64_t pawn_hash{ 0 };

	int calculate_game_phase() const noexcept;
	std::uint64_t calculate_zobrist_hash() const noexcept;
	std::uint64_t calculate_pawn_hash() const noexcept;

};