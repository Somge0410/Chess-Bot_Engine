#pragma once

#include <algorithm>
#include <array>
#include <map>
#include <string>
#include <string_view>

#include "types.h"

struct FenData {
	PieceBoards pieces{};
	Color side_to_move{Color::White };
	CastlingRights castling_rights{};
	EnPassantRights en_passant_square{ NO_SQUARE };
	int halfmove_clock{ 0 };
	int full_move_number{ 1 };
};



inline void remove_castling_right(std::string& rights, char right_to_remove) {
	rights.erase(
		std::remove(rights.begin(), rights.end(), right_to_remove),
		rights.end()
	);
}

class Position;

namespace fen {
	inline constexpr std::string_view START_POSITION=
		"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	[[nodiscard]] FenData parse(std::string_view text);
	[[nodiscard]] std::string format(const Position& position);
}
