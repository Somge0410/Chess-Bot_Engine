#include "evaluation.h"

template<bool isTracing>
int evaluate(const Board& board, Trace* trace, uint8_t terms_mask) {
	EvaluationResult score = { 0,0 };
}
template<bool isTracing>
void eval_material(EvaluationResult& score, const Board& board, Trace* trace){
	score += board.get_material_score();
	if (isTracing && trace) {
		trace->add(EvalParam::PAWN, popcount(board.get_pieces(Color::WHITE, PieceType::PAWN)) - popcount(board.get_pieces(Color::BLACK, PieceType::PAWN)));
		trace->add(EvalParam::KNIGHT, popcount(board.get_pieces(Color::WHITE, PieceType::KNIGHT)) - popcount(board.get_pieces(Color::BLACK, PieceType::KNIGHT)));
		trace->add(EvalParam::BISHOP, popcount(board.get_pieces(Color::WHITE, PieceType::BISHOP)) - popcount(board.get_pieces(Color::BLACK, PieceType::BISHOP)));
		trace->add(EvalParam::ROOK, popcount(board.get_pieces(Color::WHITE, PieceType::ROOK)) - popcount(board.get_pieces(Color::BLACK, PieceType::ROOK)));
		trace->add(EvalParam::QUEEN, popcount(board.get_pieces(Color::WHITE, PieceType::QUEEN)) - popcount(board.get_pieces(Color::BLACK, PieceType::QUEEN)));
	}
}
void evale_positional(EvaluationResult& score, const Board& board, Trace* trace) {
	score += board.get_positional_score();
	// No tracing for positional score for now
}