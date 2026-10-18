#include "eval_internal.h"
#include "types.h"	
#include "bitboard.h"
#include "eval_context.h"
#include "trace.h"
#include "attacks.h"

namespace{
	inline bool is_on_center_files(Square king_square) {
		Bitboard center_file_mask = FILE_MASK[3] | FILE_MASK[4] | FILE_MASK[5];
		return (bit64(king_square) & center_file_mask) != 0;
	}
}
template<bool isTracing>
void eval_king_safety(EvaluationResult& score, const EvalContext& ctx, Trace* trace) {
	{
		int king_squares[2] = { ctx.pos.get_king_square(Color::White), ctx.pos.get_king_square(Color::Black) };
		int pawn_shield_count = 0;
		int directly_on_open_not_next_to_open_count = 0;
		int directly_on_open_next_to_open_count = 0;
		int next_to_open_count = 0;
		int directly_on_semi_open_count_not_next_to_open = 0;
		int directly_on_semi_open_count_next_to_open = 0;
		int next_to_semi_open_count = 0;
		int next_to_open_diagonal_count[7] = { 0,0,0,0,0,0,0 };
		for (size_t color = 0; color < 2; color++) {
			int ecolor = color == 0 ? 1 : 0;
			const Color own_color = static_cast<Color>(color);
			const Color enemy_color = static_cast<Color>(ecolor);
			auto is_semi_open_for_king = [&](int file) {
				return !ctx.does_color_have_pawns_on_file(file, own_color)
					&& ctx.does_color_have_pawns_on_file(file, enemy_color);
				};
			uint64_t king_square_colors = ((bit64(king_squares[color]) & LIGHT_SQUARES) != 0) ? LIGHT_SQUARES : DARK_SQUARES;
			uint64_t shield_mask = KING_SHIELD[color][king_squares[color]];
			int king_file_index = king_squares[color] % 8;

			//1. Pawn Shield Bonus
			if (!is_on_center_files(to_square(king_squares[color]))) {
				int shield_pawns_count = popcount(ctx.get_pieces(color, PieceType::Pawn) & shield_mask);
				pawn_shield_count += color == 0 ? shield_pawns_count : -shield_pawns_count;
			}
			// 2 Next to Open File Penalty
			bool is_any_next_open = false;
			if (ctx.is_file_open(king_file_index + 1)) {
				next_to_open_count += color == 0 ? 1 : -1;
				is_any_next_open = true;
			}
			if (ctx.is_file_open(king_file_index - 1)) {
				next_to_open_count += color == 0 ? 1 : -1;
				is_any_next_open = true;
			}
			// 2.5 Open File Penalty
			bool is_king_file_open = ctx.is_file_open(king_file_index);
			if (is_king_file_open) {
				if (is_any_next_open) {
					directly_on_open_next_to_open_count += color == 0 ? 1 : -1;
				}
				else {
					directly_on_open_not_next_to_open_count += color == 0 ? 1 : -1;
				}
			}
			// 3. Semi Open File Penalty
			bool on_semi_open = is_semi_open_for_king(king_file_index);
			if (on_semi_open) {
				if (!is_any_next_open) {
					directly_on_semi_open_count_not_next_to_open += color == 0 ? 1 : -1;
				}
				else {
					directly_on_semi_open_count_next_to_open += color == 0 ? 1 : -1;
				}
			}
			//3.5 Semi Open File Penalty for files next to the king
			if (is_semi_open_for_king(king_file_index + 1)) {
				next_to_semi_open_count += color == 0 ? 1 : -1;
			}
			if (is_semi_open_for_king(king_file_index - 1)) {
				next_to_semi_open_count += color == 0 ? 1 : -1;
			}

			//5. Open Diagonal Penalty
			uint64_t bishop_attack_mask = bishop_attacks(to_square(king_squares[color]), ctx.get_color_pieces(ecolor));
			uint64_t op_bishop_queen_on_mask = bishop_attack_mask & (ctx.get_pieces(ecolor, PieceType::Bishop) | ctx.get_pieces(ecolor, PieceType::Queen));
			while (op_bishop_queen_on_mask) {
				int sq = lsb(op_bishop_queen_on_mask);
				uint64_t line_between = LINE_BETWEEN(to_square(sq), to_square(king_squares[color]));
				int count = popcount(line_between & ctx.get_pieces(color, PieceType::Pawn));
				if (count > 6) count = 6;
				next_to_open_diagonal_count[count] += color == 0 ? 1 : -1;

				op_bishop_queen_on_mask &= op_bishop_queen_on_mask - 1;
			}
		}
		//TODO: Scale safety score with enemy material.
		addTerm<isTracing>(score, EvalParam::PAWN_SHIELD_BONUS, pawn_shield_count, trace);
		addTerm<isTracing>(score, EvalParam::DIRECTLY_ON_OPEN_FILE_NOT_NEXT_TO_OPEN_PENALTY, directly_on_open_not_next_to_open_count, trace);
		addTerm<isTracing>(score, EvalParam::DIRECTLY_ON_OPEN_FILE_NEXT_TO_OPEN_PENALTY, directly_on_open_next_to_open_count, trace);
		addTerm<isTracing>(score, EvalParam::NEXT_TO_OPEN_FILE_PENALTY, next_to_open_count, trace);
		addTerm<isTracing>(score, EvalParam::DIRECTLY_ON_SEMI_OPEN_FILE_NEXT_TO_OPEN_PENALTY, directly_on_semi_open_count_next_to_open, trace);
		addTerm<isTracing>(score, EvalParam::DIRECTLY_ON_SEMI_OPEN_FILE_NOT_NEXT_TO_OPEN_PENALTY, directly_on_semi_open_count_not_next_to_open, trace);
		addTerm<isTracing>(score, EvalParam::NEXT_TO_SEMI_OPEN_FILE_PENALTY, next_to_semi_open_count, trace);
		for (size_t count = 0; count < 7; count++) {
			addTerm<isTracing>(score, static_cast<EvalParam>(EvalParam::NEXT_TO_OPEN_DIAGONAL_PENALTY_START + count), next_to_open_diagonal_count[count], trace);

		}

	}
}

template void eval_king_safety<false>(EvaluationResult&, const EvalContext&, Trace*);
template void eval_king_safety<true>(EvaluationResult&, const EvalContext&, Trace*);
