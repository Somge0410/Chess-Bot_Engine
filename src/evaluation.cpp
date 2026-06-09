#include "evaluation.h"

static thread_local std::unique_ptr<std::array<PawnEvalEntry, PAWN_HASH_SIZE>> pawn_evaluation_table;
PawnEvalEntry& get_pawn_entry(size_t idx) {
	if (!pawn_evaluation_table) {
		pawn_evaluation_table = std::make_unique<std::array<PawnEvalEntry, PAWN_HASH_SIZE>>();
	}
	return (*pawn_evaluation_table)[idx];
}
template<bool isTracing>
int evaluate(const Board& board, Trace* trace, uint8_t terms_mask) {
	EvaluationResult score = { 0,0 };
	EvalContext ctx(board);

	eval_material<isTracing>(score, board, trace);
	eval_positional<isTracing>(score, board, trace);

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
template <bool isTracing>
void eval_positional(EvaluationResult& score, const Board& board, Trace* trace) {
	score += board.get_positional_score();
	// No tracing for positional score for now
	if (isTracing && trace) {
		for (int pieceType = 0; pieceType < 6; pieceType++) {
			for (int color = 0; color < 2; color++) {
				Bitboard pieces = board.get_pieces(static_cast<Color>(color), static_cast<PieceType>(pieceType));
				while (pieces) {
					int square = poplsb(pieces);
					if(color==-1) square = flip_square(square);
					EvalParam param = static_cast<EvalParam>(pieceType * 64 + square + EvalParam::PAWN_PST_START);
					trace->add(param, color == 0 ? 1 : -1);
				}
			}
		}
	}
}
template <bool isTracing>
void eval_pawns(EvaluationResult& score, EvalContext& ctx, Trace* trace) {

	uint64_t pawn_key = board.get_pawn_key();
	int idx = pawn_key & (PAWN_HASH_SIZE - 1);
	PawnEvalEntry& entry = get_pawn_entry(idx);
	if ( !isTracing && entry.valid && entry.key==pawn_key){
		score += entry.score;
		ctx.backward = entry.backward;
		ctx.isolani = entry.isolani;
		ctx.backward = entry.backward;
		return;
	}
	EvaluationResult entry_score = { 0,0 };
	eval_iso_passed<isTracing>(entry_score,ctx,trace);
	eval_backward<isTracing>(entry_score, ctx, trace);



}
template <bool isTracing>
void eval_iso_passed(EvaluationResult& score, EvalContext& ctx, Trace* trace){
	for (size_t color = 0; color < 2; color++) {
		int ecolor = color == 0 ? 1 : 0;
		uint64_t pawns = ctx.pieces[color][to_int(PieceType::PAWN)];
		while (pawns)
		{
			int pawn_square = get_lsb(pawns);
			int file_index = pawn_square % 8;
			int rank_index = color == 0 ? pawn_square / 8 : 7 - pawn_square / 8;
			if ((ctx.pieces[ecolor][to_int(PieceType::PAWN)] & PASSED_PAWN_MASK[color][pawn_square]) == 0)
			{
				ctx.passed_pawns[color] |= (1ULL << pawn_square);
				if (color == 0) {
					addTerm<isTracing>(score, EvalParam::PASSED_PAWNS_START + square,1, trace);
				}
				else {
					addTerm<isTracing>(score,EvalParam::PASSED_PAWNS_START + flip_square(square),-1, trace);

				}
				pawns &= pawns - 1;
				continue;
			}
			if ((ctx.pieces[color][to_int(PieceType::PAWN)] & ADJACENT_FILE_MASK[file_index]) == 0)
			{
				ctx.isolated_pawns[color] |= (1ULL << pawn_square);
				int forward_square = color == 0 ? pawn_square + 8 : pawn_square - 8;
				int fsquare = color == 0 ? square : flip_square(square);
				if ((ctx.board.get_color_pieces(Color(ecolor)) & bit64(forward_square)) != 0) 
					addTerm<isTracing>(score, EvalParam::BLOCKED_ISOLATED_PAWNS_START + fsquare, color == 0 ? 1 : -1, trace);
				else
					addTerm<isTracing>(score, EvalParam::ISOLATED_PAWNS_START + fsquare, color == 0 ? 1 : -1, trace);

			}
			pawns &= pawns - 1;
		}
	}
}
template <bool isTracing>
void eval_backward(EvaluationResult& score, EvalContext& ctx, Trace* trace) {
	int blocked_backward_count = 0;
	int forwad_controlled_backward_count = 0;
	int free_to_advance_backward_count = 0;
	for (size_t color = 0; color < 2; color++) {
		int ecolor = color == 0 ? 1 : 0;
		uint64_t pawns = ctx.pieces[color][to_int(PieceType::PAWN)] & ~ctx.passed_pawns[color] & ~ctx.isolated_pawns[color];

		while (pawns)
		{
			int pawn_square = get_lsb(pawns);
			int file_index = pawn_square % 8;
			int forward_square = color == to_int(Color::WHITE) ? pawn_square + 8 : pawn_square - 8;

			if (forward_square >= 0 && forward_square < 64) {
				uint64_t forward_mask = bit64(forward_square);
				uint64_t adjacent_backwards = PAWN_ATTACKS[ecolor][pawn_square];
				bool has_adjacent_support = (ctx.pieces[color][to_int(PieceType::PAWN)] & adjacent_backwards) != 0;
				bool forward_blocked = (ctx.all & forward_mask) != 0;
				if (!has_adjacent_support) {
					if (forward_blocked) {
						ctx.backward_pawns[color] |= (1ULL << pawn_square);
						blocked_backward_count += color == 0 ? 1 : -1;
					}
					else if ((PAWN_ATTACKS[color][forward_square] & ctx.pieces[ecolor][to_int(PieceType::PAWN)]) != 0)
					{
						ctx.backward_pawns[color] |= (1ULL << pawn_square);
						forwad_controlled_backward_count += color == 0 ? 1 : -1;

					}
					else {
						free_to_advance_backward_count += color == 0 ? 1 : -1;
					}
				}

			}
			pawns &= pawns - 1;
		}

	}
	addTerm<isTracing>(score,EvalParam::FORWARD_BLOCKED_BACKWARD, blocked_backward_count,trace);
	addTerm<isTracing>(score,EvalParam::FORWARD_CONTROLLED_BACKWARD, forwad_controlled_backward_count,trace);
	addTerm<isTracing>(score,EvalParam::FREE_TO_ADVANCE_BACKWARD, free_to_advance_backward_count,trace);
}