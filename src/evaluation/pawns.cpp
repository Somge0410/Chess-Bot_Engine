#include "eval_context.h"
#include "types.h"
#include "trace.h"
#include "attacks.h"
#include "pawn_table.h"
#include "search_parameters.h"





namespace {
	PawnTable& default_pawn_table() {
		static thread_local PawnTable table;
		table.resize(PAWN_TABLE_SIZE_LOG2);
		return table;
	}

	uint64_t expand_file_mask(uint8_t file_mask) {
		return static_cast<uint64_t>(file_mask) * 0x0101010101010101ULL;
	}

	void classify_pawns(EvalContext& ctx) {
		ctx.init_file_info();
		const uint64_t white_pawns = ctx.get_pieces(Color::White, PieceType::Pawn);
		const uint64_t black_pawns = ctx.get_pieces(Color::Black, PieceType::Pawn);

		uint64_t black_front_span = black_pawns >> 8;
		black_front_span |= black_front_span >> 8;
		black_front_span |= black_front_span >> 16;
		black_front_span |= black_front_span >> 32;
		const uint64_t white_passer_blockers = black_front_span
			| ((black_front_span & NOT_FILE_H) << 1)
			| ((black_front_span & NOT_FILE_A) >> 1);
		ctx.pawn_data->passed_pawns[to_int(Color::White)] = white_pawns & ~white_passer_blockers;

		uint64_t white_front_span = white_pawns << 8;
		white_front_span |= white_front_span << 8;
		white_front_span |= white_front_span << 16;
		white_front_span |= white_front_span << 32;
		const uint64_t black_passer_blockers = white_front_span
			| ((white_front_span & NOT_FILE_H) << 1)
			| ((white_front_span & NOT_FILE_A) >> 1);
		ctx.pawn_data->passed_pawns[to_int(Color::Black)] = black_pawns & ~black_passer_blockers;

		for (int color = 0; color < 2; ++color) {
			const uint64_t own_pawns = color == to_int(Color::White)
				? white_pawns : black_pawns;
			uint64_t folded_files = own_pawns;
			folded_files |= folded_files >> 32;
			folded_files |= folded_files >> 16;
			folded_files |= folded_files >> 8;
			const uint8_t occupied_files = static_cast<uint8_t>(folded_files);
			const uint8_t adjacent_occupied_files = static_cast<uint8_t>(
				(occupied_files << 1) | (occupied_files >> 1));
			const uint8_t isolated_files = static_cast<uint8_t>(
				occupied_files & ~adjacent_occupied_files);
			ctx.pawn_data->isolated[color] = own_pawns
				& expand_file_mask(isolated_files)
				& ~ctx.pawn_data->passed_pawns[color];
		}
	}
}

template <bool isTracing>
void eval_pawns(EvaluationResult& score, EvalContext& ctx, PawnTable* pawn_table,Trace* trace) {

	if constexpr (isTracing) {
		eval_cacheable_pawns<isTracing>(ctx.pawn_data->structure_score, ctx, trace);
	}
	else {
		PawnTable& table = pawn_table ? *pawn_table : default_pawn_table();
		const uint64_t key = ctx.pos.get_pawn_key();
		if (const PawnEntry* cached = table.probe(key)) {
			*ctx.pawn_data = cached->data;
		}
		else {
			eval_cacheable_pawns<isTracing>(ctx.pawn_data->structure_score, ctx, trace);
			table.store(key, *ctx.pawn_data);
		}
	}
	score += ctx.pawn_data->structure_score;
	eval_dynamic_pawns<isTracing>(score, ctx, trace);
	eval_backward<isTracing>(score, ctx, trace);
}

template <bool isTracing>
void eval_cacheable_pawns(EvaluationResult& score, EvalContext& ctx, Trace* trace) {
	classify_pawns(ctx);

	eval_iso_passed<isTracing>(score, ctx, trace);
	eval_double_pawns<isTracing>(score, ctx, trace);
}
template <bool isTracing>
void eval_iso_passed(EvaluationResult& score, EvalContext& ctx, Trace* trace) {
	for (int color = 0; color < 2; ++color) {
		const int enemy_color = 1 - color;
		const int sign = color == to_int(Color::White) ? 1 : -1;
		uint64_t passed = ctx.pawn_data->passed_pawns[color];
		while (passed) {
			const int pawn_square = pop_lsb(passed);
			const int bucket = PASSED_PAWN_BUCKET[
				color == to_int(Color::White) ? pawn_square : flip_square(pawn_square)];
			addTerm<isTracing>(score,
				static_cast<EvalParam>(EvalParam::PASSED_PAWNS_START + bucket),
				sign, trace);

			const uint64_t defenders = pawn_attacks(
				bit64(pawn_square), static_cast<Color>(enemy_color));
			const int defender_count = popcount(
				defenders & ctx.get_pieces(color, PieceType::Pawn));
			addTerm<isTracing>(score,
				static_cast<EvalParam>(EvalParam::PROTECTED_PASSED_PAWNS_START + bucket),
				sign * defender_count, trace);
		}
	}
}

template <bool isTracing>
void eval_dynamic_pawns(EvaluationResult& score, EvalContext& ctx, Trace* trace) {
	for (int color = 0; color < 2; ++color) {
		const int enemy_color = 1 - color;
		const int sign = color == to_int(Color::White) ? 1 : -1;
		const Color pawn_color = static_cast<Color>(color);
		uint64_t passed = ctx.pawn_data->passed_pawns[color];
		while (passed) {
			const int pawn_square = pop_lsb(passed);
			const int rank_index = rank(pawn_square);
			const int bucket = PASSED_PAWN_BUCKET[
				color == to_int(Color::White) ? pawn_square : flip_square(pawn_square)];
			const int block_count = is_occupied(
				to_square(get_forward_square(pawn_square, pawn_color)), ctx.get_all_pieces()) ? 1 : 0;
			addTerm<isTracing>(score,
				static_cast<EvalParam>(EvalParam::BLOCKED_FREE_PAWN_START + bucket),
				sign * block_count, trace);

			if (block_count == 0) {
				const int promo_square = get_promotion_square(pawn_square, pawn_color);
				int enemy_king_distance = king_distance(
					ctx.pos.get_king_square(static_cast<Color>(enemy_color)), promo_square);
				const int pawn_distance = color == to_int(Color::White)
					? 7 - rank_index : rank_index;
				if (ctx.pos.get_turn() == static_cast<Color>(enemy_color)) {
					enemy_king_distance--;
				}
				if (enemy_king_distance > pawn_distance) {
					addTerm<isTracing>(score,
						static_cast<EvalParam>(EvalParam::CANT_REACHED_BY_ENEMY_KING_START + bucket),
						sign, trace);
				}
			}

			const int own_king_distance = king_distance(
				ctx.pos.get_king_square(pawn_color), pawn_square);
			if (own_king_distance <= 2) {
				addTerm<isTracing>(score,
					static_cast<EvalParam>(EvalParam::OWN_KING_IS_CLOSE_START + bucket),
					sign, trace);
			}
			if (own_king_distance >= 5) {
				addTerm<isTracing>(score,
					static_cast<EvalParam>(EvalParam::OWN_KING_IS_FAR_START + bucket),
					sign, trace);
			}

			uint64_t rooks = ctx.get_pieces(color, PieceType::Rook)
				& FORWARD_WAY_MASK[enemy_color][pawn_square];
			while (rooks) {
				const Square rook_square = to_square(pop_lsb(rooks));
				if (bit64(pawn_square) & rook_attacks(rook_square, ctx.get_all_pieces())) {
					addTerm<isTracing>(score,
						static_cast<EvalParam>(EvalParam::ROOK_BEHIND_FREE_PAWN_START + bucket),
						sign, trace);
					break;
				}
			}

			uint64_t enemy_rooks = ctx.get_pieces(enemy_color, PieceType::Rook)
				& FORWARD_WAY_MASK[enemy_color][pawn_square];
			while (enemy_rooks) {
				const Square rook_square = to_square(pop_lsb(enemy_rooks));
				if (bit64(pawn_square) & rook_attacks(rook_square, ctx.get_all_pieces())) {
					addTerm<isTracing>(score,
						static_cast<EvalParam>(EvalParam::OP_ROOK_BEHIND_FREE_PAWN_START + bucket),
						sign, trace);
					break;
				}
			}
		}

		uint64_t isolated = ctx.pawn_data->isolated[color];
		while (isolated) {
			const int pawn_square = pop_lsb(isolated);
			const int bucket = ISOLATED_PAWN_BUCKET[
				color == to_int(Color::White) ? pawn_square : flip_square(pawn_square)];
			if (is_occupied(to_square(get_forward_square(pawn_square, pawn_color)), ctx.get_all_pieces())) {
				addTerm<isTracing>(score,
					static_cast<EvalParam>(EvalParam::BLOCKED_ISOLANI_START + bucket),
					sign, trace);
			}
			else {
				addTerm<isTracing>(score,
					static_cast<EvalParam>(EvalParam::ISOLANI_START + bucket),
					sign, trace);
			}

			if (ctx.get_attacks(pawn_color) & bit64(pawn_square)) {
				addTerm<isTracing>(score,
					static_cast<EvalParam>(EvalParam::PROTECTED_PASSED_PAWNS_START + bucket),
					sign, trace);
			}
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
		uint64_t pawns = ctx.pos.get_pieces(static_cast<Color>(color), PieceType::Pawn)
			& ~ctx.pawn_data->passed_pawns[color] & ~ctx.pawn_data->isolated[color];

		while (pawns)
		{
			int pawn_square = lsb(pawns);
			int file_index = pawn_square % 8;
			int forward_square = color == to_int(Color::White) ? pawn_square + 8 : pawn_square - 8;

			if (forward_square >= 0 && forward_square < 64) {
				uint64_t forward_mask = bit64(forward_square);
				uint64_t adjacent_backwards = PAWN_ATTACKS[ecolor][pawn_square];
				bool has_adjacent_support = (ctx.pos.get_pieces(static_cast<Color>(color), PieceType::Pawn) & adjacent_backwards) != 0;
				bool forward_blocked = (ctx.pos.get_all_pieces() & forward_mask) != 0;
				if (!has_adjacent_support) {
					if (forward_blocked) {
						ctx.backward[color] |= (1ULL << pawn_square);
						blocked_backward_count += color == 0 ? 1 : -1;
					}
					else if ((PAWN_ATTACKS[color][forward_square] & ctx.pos.get_pieces(static_cast<Color>(ecolor), PieceType::Pawn)) != 0)
					{
						ctx.backward[color] |= (1ULL << pawn_square);
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
	addTerm<isTracing>(score, EvalParam::FORWARD_BLOCKED_BACKWARD, blocked_backward_count, trace);
	addTerm<isTracing>(score, EvalParam::FORWARD_CONTROLLED_BACKWARD, forwad_controlled_backward_count, trace);
	addTerm<isTracing>(score, EvalParam::FREE_TO_ADV_BACKWARD, free_to_advance_backward_count, trace);
}
template <bool isTracing>
void eval_double_pawns(EvaluationResult& score, EvalContext& ctx, Trace* trace) {
	int doubled_count = 0;
	for (size_t file = 0; file < 8; ++file)
	{
		doubled_count = 0;
		uint64_t file_mask = FILE_MASK[file];
		int white_doubled = popcount(ctx.pos.get_pieces(static_cast<Color>(Color::White), PieceType::Pawn) & file_mask);
		if (white_doubled > 1)
		{
			doubled_count += white_doubled - 1;


		}
		int black_doubled = popcount(ctx.pos.get_pieces(static_cast<Color>(Color::Black), PieceType::Pawn) & file_mask);
		if (black_doubled > 1)
		{
			doubled_count -= black_doubled - 1;
		}

		addTerm<isTracing>(score, static_cast<EvalParam>(EvalParam::DOUBLE_PAWN_FILE_START + file), doubled_count, trace);
	}
}

// The remaining pawn helpers are instantiated through these entry points.
template void eval_pawns<false>(EvaluationResult&, EvalContext&, PawnTable*, Trace*);
template void eval_pawns<true>(EvaluationResult&, EvalContext&, PawnTable*, Trace*);

