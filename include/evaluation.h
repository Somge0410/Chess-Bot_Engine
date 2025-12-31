#pragma once
#include "board.h"
struct KingSafetyScore {
    int mg_score;
    int eg_score;
};
struct PawnStructureScore {
	int mg_score;
	int eg_score;
};
namespace {
	struct PawnEvalEntry {
		uint64_t key;
		int pawn_structure_score;
		int mg_king_safety;
		int eg_king_safety;
		bool valid;
	};
	constexpr int PAWN_HASH_SIZE = 1 << 16;
	PawnEvalEntry pawn_evaluation_table[PAWN_HASH_SIZE] = {};
}
enum EvalTerms : uint8_t {
	EVAL_MATERIAL = 1<<0,
	EVAL_POSITIONAL = 1 << 1,
	EVAL_PAWN_STRUCTURE = 1 << 2,
	EVAL_KING_SAFETY = 1 << 3,
	EVAL_MOBILITY = 1 << 4,
	EVAL_ROOK_ACTIVITY = 1 << 5,
	EVAL_BISHOP_PAIR = 1 << 6,

	EvalAll = EVAL_MATERIAL | EVAL_POSITIONAL | EVAL_PAWN_STRUCTURE | EVAL_KING_SAFETY | EVAL_MOBILITY | EVAL_ROOK_ACTIVITY | EVAL_BISHOP_PAIR
};
int evaluate(const Board& board,uint8_t terms_mask=EvalAll);