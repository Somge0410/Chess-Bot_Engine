#pragma once
#include "board.h"


struct EvalContext {
	const Board& board;
	const std::array<std::array<uint64_t, 6>, 2> pieces;
	const uint64_t color_pieces[2];
	const uint64_t all;
	const uint8_t castle_rights;
	const int king_sq[2];
	const int game_phase;
	uint64_t open_files;
	uint64_t no_color_pawn_files[2];
	uint8_t no_pawn_files[2][8];
	uint8_t no_pawn_file_count[2];
	bool special_pawns_computed;
	uint64_t backward_pawns[2];
	uint64_t isolated_pawns[2];
	uint64_t passed_pawns[2];

	// Konstruktor-Deklaration (Implementation bleibt in .cpp)
	EvalContext(const Board& b);
};
struct PawnEvalEntry {
	uint64_t key;
	EvaluationResult doubled_score;
	EvaluationResult isolated_passed_score;
	EvaluationResult backward_score;
	uint64_t isolated_pawns[2] = { 0,0 };
	uint64_t passed_pawns[2] = { 0,0 };
	uint64_t backward_pawns[2] = { 0,0 };
	bool valid;
};

constexpr int PAWN_HASH_SIZE = 1 << 18;

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


enum EvalParam {
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,

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
void evaluate_material(EvaluationResult& score, const Board& board, Trace* trace);