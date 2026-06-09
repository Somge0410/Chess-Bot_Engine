#pragma once
#include "board.h"


struct EvalContext {
	const Board& board;
	uint64_t backward[2];
	uint64_t isolanis[2];
	uint64_t passed[2];
	EvalContext(const Board& b) : board(b),
		backward{ 0,0 },
		isolanis{ 0,0 },
		passed{ 0,0 } {}
};
struct PawnEvalEntry {
	uint64_t key;
	EvaluationResult score;
	uint64_t isolanis[2] = { 0,0 };
	uint64_t passed[2] = { 0,0 };
	uint64_t backward[2] = { 0,0 };
	bool valid;
};

constexpr int PAWN_HASH_SIZE = 1 << 18;

PawnEvalEntry compute_pawn_eval_entry(EvalContext& ctx);

enum EvalTerms : uint8_t {
	EVAL_MATERIAL = 1 << 0,
	EVAL_POSITIONAL = 1 << 1,
	EVAL_PAWN_STRUCTURE = 1 << 2,
	EVAL_KING_SAFETY = 1 << 3,
	EVAL_MOBILITY = 1 << 4,
	EVAL_ROOK_ACTIVITY = 1 << 5,
	EVAL_MINOR_PIECES = 1 << 6,

	EvalAll = EVAL_MATERIAL | EVAL_POSITIONAL | EVAL_PAWN_STRUCTURE | EVAL_KING_SAFETY | EVAL_MOBILITY | EVAL_ROOK_ACTIVITY | EVAL_MINOR_PIECES
};

int tapered(EvaluationResult score, int game_phase) {
	return (score.mg_score * game_phase + score.eg_score * (24 - game_phase)) / 24;
}

void special_pawn_bb(EvalContext& ctx);
struct EvalContext {
	EvalContext(const Board& b);
};
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
	PASSED_PAWNS_END = PASSED_PAWNS_START + 63,
	ISOLANI_START,
	ISOLANI_END = ISOLANI_START + 63,
	BLOCKED_ISOLANI_START,
	BLOCKED_ISOLANI_END = BLOCKED_ISOLANI_START + 63,
	FORWARD_BLOCKED_BACKWARD,
	FORWARD_CONTROLLED_BACKWARD,
	FREE_TO_ADV_BACKWARD,
	PARAM_COUNT
};
extern EvaluationResult EvalWeights[PARAM_COUNT];

struct Trace { 
	int counts[PARAM_COUNT]= {0}; 
	void add(EvalParam param, int count = 1) {
		counts[param] += count;
	}
};

template <bool isTracing>
void addTerm(EvaluationResult score,EvalParam param, int count=1, Trace* trace=nullptr) {
	score += EvalWeights[param] * count;
	if constexpr (isTracing) {
		if (trace) trace->add(param, count);
	}
}

template <bool isTracing>
int evaluate(const Board& board, Trace* trace = nullptr, uint8_t terms_mask = EvalAll);

template <bool isTracing>
void eval_material(EvaluationResult& score, const Board& board, Trace* trace);

template <bool isTracing>
void eval_positional(EvaluationResult& score, const Board& board, Trace* trace);

template <bool isTracing>
void eval_pawns(EvaluationResult& score, EvalContext& ctx, Trace* trace);

template <bool isTracing>
void eval_iso_passed(EvaluationResult& score, EvalContext& ctx, Trace* trace);

template <bool isTracing>
void eval_backward(EvaluationResult& score, EvalContext& ctx, Trace* trace);
template <bool isTracing>
void eval_double_pawns(EvaluationResult& score, EvalCOntest& ctx, Trace* trace);