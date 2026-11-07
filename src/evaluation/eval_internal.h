#pragma once 
#include "types.h"
#include "phase.h"
#include <algorithm>

struct EvalContext;
struct Trace;
class PawnTable;

inline int tapered(EvaluationResult score, int game_phase) {
	game_phase = std::clamp(game_phase, 0, MAX_GAME_PHASE);
	return (score.mg_score * game_phase + score.eg_score * (24 - game_phase)) / 24;
}
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

template <bool isTracing>
void eval_mobility(EvaluationResult& score, const EvalContext& ctx, Trace* trace);

template <bool isTracing>
void eval_rook_activity(EvaluationResult& score, const EvalContext& ctx, Trace* trace);

template <bool isTracing>
void eval_minor_pieces(EvaluationResult& score, const EvalContext& ctx, Trace* trace);

template <bool isTracing>
void eval_bishop_pair(EvaluationResult& score, const EvalContext& ctx, Trace* trace);

template <bool isTracing>
void eval_bad_bishop(EvaluationResult& score, const EvalContext& ctx, Trace* trace);

template <bool isTracing>
void eval_trapped_minor(EvaluationResult& score, const EvalContext& ctx, Trace* trace);

template <bool isTracing>
void eval_fianchetto_bishop(EvaluationResult& score, const EvalContext& ctx, Trace* trace);

template <bool isTracing>
void eval_outpost(EvaluationResult& score, const EvalContext& ctx, Trace* trace);

template <bool isTracing>
void eval_pawns(EvaluationResult& score, EvalContext& ctx, PawnTable* pawn_table, Trace* trace);

template <bool isTracing>
void eval_cacheable_pawns(EvaluationResult& score, EvalContext& ctx, Trace* trace);

template <bool isTracing>
void eval_iso_passed(EvaluationResult& score, EvalContext& ctx, Trace* trace);

template <bool isTracing>
void eval_dynamic_pawns(EvaluationResult& score, EvalContext& ctx, Trace* trace);

template <bool isTracing>
void eval_backward(EvaluationResult& score, EvalContext& ctx, Trace* trace);

template <bool isTracing>
void eval_double_pawns(EvaluationResult& score, EvalContext& ctx, Trace* trace);

template <bool isTracing>
void eval_king_safety(EvaluationResult& score, const EvalContext& ctx, Trace* trace);


