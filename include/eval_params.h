#pragma once

#include <cstdint>


struct EvaluationResult {
    int16_t mg_score;
    int16_t eg_score;
    EvaluationResult& operator+=(const EvaluationResult& other) {
        this->mg_score += other.mg_score;
        this->eg_score += other.eg_score;
        return *this;
    }
    EvaluationResult& operator-=(const EvaluationResult& other) {
        this->mg_score -= other.mg_score;
        this->eg_score -= other.eg_score;
        return *this;
	}
    EvaluationResult& operator*=(int multiplier) {
        this->mg_score = static_cast<int16_t>(this->mg_score * multiplier);
        this->eg_score = static_cast<int16_t>(this->eg_score * multiplier);
        return *this;
	}
};
inline EvaluationResult operator+(EvaluationResult lhs, const EvaluationResult& rhs) {
    lhs += rhs;
    return lhs;
}
inline EvaluationResult operator-(EvaluationResult lhs, const EvaluationResult& rhs) {
    lhs.mg_score -= rhs.mg_score;
    lhs.eg_score -= rhs.eg_score;
    return lhs;
}
inline EvaluationResult operator*(EvaluationResult lhs, int multiplier) {
    lhs *= multiplier;
    return lhs;
}

enum EvalParam {
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
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
	KING_DANGER_START,
	KING_DANGER_END = KING_DANGER_START + 32,
	MOBILITY_START,
	MOBILITY_END = MOBILITY_START + 3,
	ROOK_ON_OPEN_FILE,
	ROOK_ON_SEMI_OPEN_FILE,
	CONNECTED_ROOKS,
	BISHOP_PAIR,
	BAD_BISHOP_BLOCKED,
	BAD_BISHOP_UNBLOCKED,
	RESTRICTED_BISHOP,
	RESTRICTED_KNIGHT,
	TRAPPED_BISHOP,
	TRAPPED_KNIGHT,
	FIANCHETTO_BISHOP,
	BROKEN_FIANCHETTO,
	BISHOP_OUTPOST_NO_OPPOSITE_BISHOP,
	BISHOP_OUTPOST_WITH_OPPOSITE_BISHOP,
	KNIGHT_OUTPOST_NO_OPPOSITE_BISHOP,
	KNIGHT_OUTPOST_WITH_OPPOSITE_BISHOP,
	PAWN_THREAT_MINOR,
	PAWN_THREAT_ROOK,
	PAWN_THREAT_QUEEN,
	MINOR_THREAT_ROOK,
	MINOR_THREAT_QUEEN,
	ROOK_THREAT_QUEEN,
	HANGING_PIECE_START,
	HANGING_PIECE_END = HANGING_PIECE_START + 4,
	PARAM_COUNT
};

extern EvaluationResult EvalWeights[PARAM_COUNT];
inline EvaluationResult get_piece_values(const Color& color, const PieceType& piece) {
	EvaluationResult result = { 0,0 };
	if (piece==PieceType::PAWN) {
		return color == Color::WHITE ? EvalWeights[EvalParam::PAWN] : EvalWeights[EvalParam::PAWN] * -1;
	}
	else if (piece == PieceType::KNIGHT) {
		return color == Color::WHITE ? EvalWeights[EvalParam::KNIGHT] : EvalWeights[EvalParam::KNIGHT] * -1;
	}
	else if (piece == PieceType::BISHOP) {
		return color == Color::WHITE ? EvalWeights[EvalParam::BISHOP] : EvalWeights[EvalParam::BISHOP] * -1;
	}
	else if (piece == PieceType::ROOK) {
		return color == Color::WHITE ? EvalWeights[EvalParam::ROOK] : EvalWeights[EvalParam::ROOK] * -1;
	}
	else if (piece == PieceType::QUEEN) {
		return color == Color::WHITE ? EvalWeights[EvalParam::QUEEN] : EvalWeights[EvalParam::QUEEN] * -1;
	}
	else if (piece == PieceType::KING) {
		return { 0,0 };
	}
	else return { 0,0 };
}