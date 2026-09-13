#pragma once
#include <array>
#include "types.h"

inline constexpr int MAX_GAME_PHASE = 24;

inline constexpr std::array<int, 7> PHASE_WEIGHTS = { 0, 1, 1, 2, 4, 0, 0 };

constexpr int phase_weight(PieceType piece_type) noexcept {
	switch (piece_type)
	{
	case PieceType::Pawn:
			return PHASE_WEIGHTS[0];
	case PieceType::Knight:
			return PHASE_WEIGHTS[1];
	case PieceType::Bishop:
		return PHASE_WEIGHTS[2];
	case PieceType::Rook:
		return PHASE_WEIGHTS[3];
	case PieceType::Queen:
		return PHASE_WEIGHTS[4];
	case PieceType::King:
		return PHASE_WEIGHTS[5];
	case PieceType::None:
		return PHASE_WEIGHTS[6];
	default:
		break;
	}
}