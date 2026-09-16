#pragma once

#include <cstdint>

enum EvalParam {
	Pawn,
	Knight,
	Bishop,
	Rook,
	Queen,
	PAWN_PST_START,
	PAWN_PST_END = PAWN_PST_START + 63,
	KNIGHT_PST_START,
	KNIGHT_PST_END = KNIGHT_PST_START + 63,
	BISHOP_PST_START,
	BISHOP_PST_END = BISHOP_PST_START + 63,
	ROOK_PST_START,
	ROOK_PST_END = ROOK_PST_START + 63,
	QUEEN_PST_START,
	QUEEN_PST_END = QUEEN_PST_START + 63,
	KING_PST_START,
	KING_PST_END = KING_PST_START + 63,
	PASSED_PAWNS_START,
	PASSED_PAWNS_END = PASSED_PAWNS_START + 15,
	PROTECTED_PASSED_PAWNS_START,
	PROTECTED_PASSED_PAWNS_END = PROTECTED_PASSED_PAWNS_START + 15,
	BLOCKED_FREE_PAWN_START,
	BLOCKED_FREE_PAWN_END = BLOCKED_FREE_PAWN_START + 15,
	CANT_REACHED_BY_ENEMY_KING_START,
	CANT_REACHED_BY_ENEMY_KING_END = CANT_REACHED_BY_ENEMY_KING_START + 15,
	OWN_KING_IS_CLOSE_START,
	OWN_KING_IS_CLOSE_END = OWN_KING_IS_CLOSE_START + 15,
	OWN_KING_IS_FAR_START,
	OWN_KING_IS_FAR_END = OWN_KING_IS_FAR_START + 15,
	ROOK_BEHIND_FREE_PAWN_START,
	ROOK_BEHIND_FREE_PAWN_END = ROOK_BEHIND_FREE_PAWN_START + 15,
	OP_ROOK_BEHIND_FREE_PAWN_START,
	OP_ROOK_BEHIND_FREE_PAWN_END = OP_ROOK_BEHIND_FREE_PAWN_START + 15,
	ISOLANI_START,
	ISOLANI_END = ISOLANI_START + 15,
	BLOCKED_ISOLANI_START,
	BLOCKED_ISOLANI_END = BLOCKED_ISOLANI_START + 15,
	PROTECTED_ISOLANI_START,
	PROTECTED_ISOLANI_END = PROTECTED_ISOLANI_START + 15,
	FORWARD_BLOCKED_BACKWARD,
	FORWARD_CONTROLLED_BACKWARD,
	FREE_TO_ADV_BACKWARD,
	DOUBLE_PAWN_FILE_START,
	DOUBLE_PAWN_FILE_END = DOUBLE_PAWN_FILE_START + 7,
	PAWN_SHIELD_BONUS,
	DIRECTLY_ON_OPEN_FILE_NEXT_TO_OPEN_PENALTY,
	DIRECTLY_ON_OPEN_FILE_NOT_NEXT_TO_OPEN_PENALTY,
	NEXT_TO_OPEN_FILE_PENALTY,
	DIRECTLY_ON_SEMI_OPEN_FILE_NEXT_TO_OPEN_PENALTY,
	DIRECTLY_ON_SEMI_OPEN_FILE_NOT_NEXT_TO_OPEN_PENALTY,
	NEXT_TO_SEMI_OPEN_FILE_PENALTY,
	NEXT_TO_OPEN_DIAGONAL_PENALTY_START,
	NEXT_TO_OPEN_DIAGONAL_PENALTY_END = NEXT_TO_OPEN_DIAGONAL_PENALTY_START + 6,
	MOBILITY_START,
	MOBILITY_END = MOBILITY_START + 3,
	ROOK_ON_OPEN_FILE,
	ROOK_ON_SEMI_OPEN_FILE,
	CONNECTED_ROOKS,
	BISHOP_PAIR,
	BAD_BISHOP_BLOCKED,
	BAD_BISHOP_UNBLOCKED,
	TRAPPED_BISHOP,
	TRAPPED_KNIGHT,
	FIANCHETTO_BISHOP,
	BROKEN_FIANCHETTO,
	BISHOP_OUTPOST_NO_OPPOSITE_BISHOP,
	BISHOP_OUTPOST_WITH_OPPOSITE_BISHOP,
	KNIGHT_OUTPOST_NO_OPPOSITE_BISHOP,
	KNIGHT_OUTPOST_WITH_OPPOSITE_BISHOP,
	PARAM_COUNT
};

extern EvaluationResult EvalWeights[PARAM_COUNT];
inline EvaluationResult get_piece_values(const Color& color, const PieceType& piece) {
	EvaluationResult result = { 0,0 };
	if (piece==PieceType::Pawn) {
		return color == Color::White ? EvalWeights[EvalParam::Pawn] : EvalWeights[EvalParam::Pawn] * -1;
	}
	else if (piece == PieceType::Knight) {
		return color == Color::White ? EvalWeights[EvalParam::Knight] : EvalWeights[EvalParam::Knight] * -1;
	}
	else if (piece == PieceType::Bishop) {
		return color == Color::White ? EvalWeights[EvalParam::Bishop] : EvalWeights[EvalParam::Bishop] * -1;
	}
	else if (piece == PieceType::Rook) {
		return color == Color::White ? EvalWeights[EvalParam::Rook] : EvalWeights[EvalParam::Rook] * -1;
	}
	else if (piece == PieceType::Queen) {
		return color == Color::White ? EvalWeights[EvalParam::Queen] : EvalWeights[EvalParam::Queen] * -1;
	}
	else if (piece == PieceType::King) {
		return { 0,0 };
	}
	else return { 0,0 };
}

inline int get_mg_pos_score(const Color& color, const PieceType& piece, const int& square) {
	if (color == Color::White) {
		if (piece == PieceType::Pawn) return EvalWeights[PAWN_PST_START + square].mg_score;
		else if (piece == PieceType::Knight) return EvalWeights[KNIGHT_PST_START + square].mg_score;
		else if (piece == PieceType::Bishop) return EvalWeights[BISHOP_PST_START + square].mg_score;
		else if (piece == PieceType::Rook) return EvalWeights[ROOK_PST_START + square].mg_score;
		else if (piece == PieceType::Queen) return EvalWeights[QUEEN_PST_START + square].mg_score;
		else if (piece == PieceType::King) return EvalWeights[KING_PST_START + square].mg_score;
		else return 0;
	}
	else {
		if (piece == PieceType::Pawn) return -EvalWeights[PAWN_PST_START + flip_square(square)].mg_score;
		else if (piece == PieceType::Knight) return -EvalWeights[KNIGHT_PST_START + flip_square(square)].mg_score;
		else if (piece == PieceType::Bishop) return -EvalWeights[BISHOP_PST_START + flip_square(square)].mg_score;
		else if (piece == PieceType::Rook) return -EvalWeights[ROOK_PST_START + flip_square(square)].mg_score;
		else if (piece == PieceType::Queen) return -EvalWeights[QUEEN_PST_START + flip_square(square)].mg_score;
		else if (piece == PieceType::King) return -EvalWeights[KING_PST_START + flip_square(square)].mg_score;
		else return 0;
	}
}
inline int get_eg_pos_score(const Color& color, const PieceType& piece, const int& square) {
	if (color == Color::White) {
		if (piece == PieceType::Pawn) return EvalWeights[PAWN_PST_START + square].eg_score;
		else if (piece == PieceType::Knight) return EvalWeights[KNIGHT_PST_START + square].eg_score;
		else if (piece == PieceType::Bishop) return EvalWeights[BISHOP_PST_START + square].eg_score;
		else if (piece == PieceType::Rook) return EvalWeights[ROOK_PST_START + square].eg_score;
		else if (piece == PieceType::Queen) return EvalWeights[QUEEN_PST_START + square].eg_score;
		else if (piece == PieceType::King) return EvalWeights[KING_PST_START + square].eg_score;
		else return 0;
	}
	else {
		if (piece == PieceType::Pawn) return -EvalWeights[PAWN_PST_START + flip_square(square)].eg_score;
		else if (piece == PieceType::Knight) return -EvalWeights[KNIGHT_PST_START + flip_square(square)].eg_score;
		else if (piece == PieceType::Bishop) return -EvalWeights[BISHOP_PST_START + flip_square(square)].eg_score;
		else if (piece == PieceType::Rook) return -EvalWeights[ROOK_PST_START + flip_square(square)].eg_score;
		else if (piece == PieceType::Queen) return -EvalWeights[QUEEN_PST_START + flip_square(square)].eg_score;
		else if (piece == PieceType::King) return -EvalWeights[KING_PST_START + flip_square(square)].eg_score;
		else return 0;
	}
}