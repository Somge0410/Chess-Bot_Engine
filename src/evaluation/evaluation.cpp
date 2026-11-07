#include "evaluation.h"
#include "bitboard.h"
#include "eval_context.h"
#include "eval_internal.h"


template<bool isTracing>
int evaluate(const Position& pos, Trace* trace, uint8_t terms_mask, PawnTable* pawn_table) {
	EvaluationResult score = { 0,0 };
	PawnData pawn_data{};
	EvalContext ctx(pos, pawn_data);

	eval_material<isTracing>(score, pos, trace);
	eval_positional<isTracing>(score, pos, trace);
	eval_pawns<isTracing>(score, ctx, pawn_table, trace);
	if (terms_mask != EvalAll) return tapered(score, pos.get_game_phase());
	eval_king_safety<isTracing>(score, ctx, trace);

	eval_mobility<isTracing>(score, ctx, trace);
	eval_rook_activity<isTracing>(score, ctx, trace);
	eval_minor_pieces<isTracing>(score, ctx, trace);
	return tapered(score, pos.get_game_phase());

}
template<bool isTracing>
void eval_material(EvaluationResult& score, const Position& pos, Trace* trace) {
	score += pos.get_material_score();
	if (isTracing && trace) {
		trace->add(EvalParam::Pawn, popcount(pos.get_pieces(Color::White, PieceType::Pawn)) - popcount(pos.get_pieces(Color::Black, PieceType::Pawn)));
		trace->add(EvalParam::Knight, popcount(pos.get_pieces(Color::White, PieceType::Knight)) - popcount(pos.get_pieces(Color::Black, PieceType::Knight)));
		trace->add(EvalParam::Bishop, popcount(pos.get_pieces(Color::White, PieceType::Bishop)) - popcount(pos.get_pieces(Color::Black, PieceType::Bishop)));
		trace->add(EvalParam::Rook, popcount(pos.get_pieces(Color::White, PieceType::Rook)) - popcount(pos.get_pieces(Color::Black, PieceType::Rook)));
		trace->add(EvalParam::Queen, popcount(pos.get_pieces(Color::White, PieceType::Queen)) - popcount(pos.get_pieces(Color::Black, PieceType::Queen)));
	}
}
template <bool isTracing>
void eval_positional(EvaluationResult& score, const Position& pos, Trace* trace) {
	score += pos.get_positional_score();
	// No tracing for positional score for now
	if (isTracing && trace) {
		for (int pieceType = 0; pieceType < 6; pieceType++) {
			for (int color = 0; color < 2; color++) {
				Bitboard pieces = pos.get_pieces(static_cast<Color>(color), static_cast<PieceType>(pieceType));
				while (pieces) {
					int square = pop_lsb(pieces);
					if (color == 1) square = flip_square(square);
					EvalParam param = static_cast<EvalParam>(pieceType * 64 + square + EvalParam::PAWN_PST_START);
					trace->add(param, color == 0 ? 1 : -1);
				}
			}
		}
	}
}


bool trace_eval_agree(const Position& pos, const EvaluationResult weights[PARAM_COUNT]) {
	Trace trace;
	int eval = evaluate<true>(pos, &trace);
	int trace_eval = get_trace_eval(&trace, weights, pos.get_game_phase());
	if (trace_eval != eval) {
		std::cerr << "Trace evaluation" << trace_eval << " does not match actual evaluation " << eval << std::endl;
	}
	return trace_eval == eval;
}

// Emit both tracing variants in the translation unit containing the definitions.
template int evaluate<false>(const Position&, Trace*, uint8_t, PawnTable*);
template int evaluate<true>(const Position&, Trace*, uint8_t, PawnTable*);
template void eval_material<false>(EvaluationResult&, const Position&, Trace*);
template void eval_material<true>(EvaluationResult&, const Position&, Trace*);
template void eval_positional<false>(EvaluationResult&, const Position&, Trace*);
template void eval_positional<true>(EvaluationResult&, const Position&, Trace*);
