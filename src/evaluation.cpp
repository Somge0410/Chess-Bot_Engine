#include "evaluation.h"

static thread_local std::unique_ptr<std::array<PawnEvalEntry, PAWN_HASH_SIZE>> pawn_evaluation_table;
PawnEvalEntry& get_pawn_entry(size_t idx) {
	if (!pawn_evaluation_table) {
		pawn_evaluation_table = std::make_unique<std::array<PawnEvalEntry, PAWN_HASH_SIZE>>();
	}
	return (*pawn_evaluation_table)[idx];
}
bool trace_eval_agree(const Board& board, const EvaluationResult weights[PARAM_COUNT]) {
	Trace trace;
	int eval = evaluate<true>(board, &trace);
	int trace_eval = get_trace_eval(&trace, weights, board.get_game_phase());
	if (trace_eval != eval) {
		std::cerr << "Trace evaluation" << trace_eval << " does not match actual evaluation " << eval << std::endl;
	}
	return trace_eval == eval;
}
template<bool isTracing>
int evaluate(const Board& board, Trace* trace, uint8_t terms_mask) {
	EvaluationResult score = { 0,0 };
	EvalContext ctx(board);

	eval_material<isTracing>(score, board, trace);
	//eval_positional<isTracing>(score, board, trace);
	eval_pawns<isTracing>(score, ctx, trace);
	eval_king_safety<isTracing>(score, ctx, trace);
	eval_mobility<isTracing>(score, ctx, trace);
	eval_rook_activity<isTracing>(score, ctx, trace);
	eval_minor_pieces<isTracing>(score, ctx, trace);
	return tapered(score, board.get_game_phase());

}
template int evaluate<false>(const Board& board, Trace* trace, uint8_t terms_mask);
template int evaluate<true>(const Board& board, Trace* trace, uint8_t terms_mask);

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
				uint64_t pieces = board.get_pieces(static_cast<Color>(color), static_cast<PieceType>(pieceType));
				while (pieces) {
					int square = poplsb(pieces);
					if(color==1) square = flip_square(square);
					EvalParam param = static_cast<EvalParam>(pieceType * 64 + square + EvalParam::PAWN_PST_START);
					trace->add(param, color == 0 ? 1 : -1);
				}
			}
		}
	}
}
template <bool isTracing>
void eval_pawns(EvaluationResult& score, EvalContext& ctx, Trace* trace) {

	uint64_t pawn_key = ctx.board.get_pawn_key();
	int idx = pawn_key & (PAWN_HASH_SIZE - 1);
	PawnEvalEntry& entry = get_pawn_entry(idx);
	if ( !isTracing && entry.valid && entry.key==pawn_key){
		score += entry.score;
		ctx.backward[0] = entry.backward[0];
		ctx.backward[1] = entry.backward[1];
		ctx.isolated[0] = entry.isolated[0];
		ctx.isolated[1] = entry.isolated[1];
		ctx.passed[0] = entry.passed[0];
		ctx.passed[1] = entry.passed[1];
		ctx.files_with_no_color_pawns[0] = entry.file_info[0];
		ctx.files_with_no_color_pawns[1] = entry.file_info[1];
		return;
	}
	EvaluationResult entry_score = { 0,0 };
	ctx.init_file_info();
	eval_iso_passed<isTracing>(entry_score,ctx,trace);
	eval_backward<isTracing>(entry_score, ctx, trace);
	eval_double_pawns<isTracing>(entry_score, ctx, trace);

	score += entry_score;
	if (!isTracing) {
		entry.key = pawn_key;
		entry.score = entry_score;
		entry.isolated[0] = ctx.isolated[0];
		entry.isolated[1] = ctx.isolated[1];
		entry.passed[0] = ctx.passed[0];
		entry.passed[1] = ctx.passed[1];
		entry.backward[0] = ctx.backward[0];
		entry.backward[1] = ctx.backward[1];
		entry.file_info[0] = ctx.files_with_no_color_pawns[0];
		entry.file_info[1] = ctx.files_with_no_color_pawns[1];
		entry.valid = true;
	}
}
template <bool isTracing>
void eval_iso_passed(EvaluationResult& score, EvalContext& ctx, Trace* trace){
	for (size_t color = 0; color < 2; color++) {
		int ecolor = color == 0 ? 1 : 0;
		uint64_t pawns = ctx.board.get_pieces(static_cast<Color>(color), PieceType::PAWN);
		while (pawns)
		{
			int pawn_square = get_lsb(pawns);
			int file_index = pawn_square % 8;
			int rank_index = color == 0 ? pawn_square / 8 : 7 - pawn_square / 8;
			if ((ctx.board.get_pieces(static_cast<Color>(ecolor), PieceType::PAWN) & PASSED_PAWN_MASK[color][pawn_square]) == 0)
			{
				ctx.passed[color] |= (1ULL << pawn_square);
				if (color == 0) {
					addTerm<isTracing>(score, static_cast<EvalParam>(EvalParam::PASSED_PAWNS_START + pawn_square),1, trace);
				}
				else {
					addTerm<isTracing>(score,static_cast<EvalParam>(EvalParam::PASSED_PAWNS_START + flip_square(pawn_square)),-1, trace);

				}
				pawns &= pawns - 1;
				continue;
			}
			if ((ctx.board.get_pieces(static_cast<Color>(color), PieceType::PAWN) & ADJACENT_FILE_MASK[file_index]) == 0)
			{
				ctx.isolated[color] |= (1ULL << pawn_square);
				int forward_square = color == 0 ? pawn_square + 8 : pawn_square - 8;
				int flipped_square = color == 0 ? pawn_square : flip_square(pawn_square);
				if ((ctx.board.get_color_pieces(Color(ecolor)) & bit64(forward_square)) != 0) 
					addTerm<isTracing>(score, static_cast<EvalParam>(EvalParam::BLOCKED_ISOLANI_START + flipped_square), color == 0 ? 1 : -1, trace);
				else
					addTerm<isTracing>(score, static_cast<EvalParam>(EvalParam::ISOLANI_START + flipped_square), color == 0 ? 1 : -1, trace);

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
		uint64_t pawns = ctx.board.get_pieces(static_cast<Color>(color), PieceType::PAWN) & ~ctx.passed[color] & ~ctx.isolated[color];

		while (pawns)
		{
			int pawn_square = get_lsb(pawns);
			int file_index = pawn_square % 8;
			int forward_square = color == to_int(Color::WHITE) ? pawn_square + 8 : pawn_square - 8;

			if (forward_square >= 0 && forward_square < 64) {
				uint64_t forward_mask = bit64(forward_square);
				uint64_t adjacent_backwards = PAWN_ATTACKS[ecolor][pawn_square];
				bool has_adjacent_support = (ctx.board.get_pieces(static_cast<Color>(color), PieceType::PAWN) & adjacent_backwards) != 0;
				bool forward_blocked = (ctx.board.get_all_pieces() & forward_mask) != 0;
				if (!has_adjacent_support) {
					if (forward_blocked) {
						ctx.backward[color] |= (1ULL << pawn_square);
						blocked_backward_count += color == 0 ? 1 : -1;
					}
					else if ((PAWN_ATTACKS[color][forward_square] & ctx.board.get_pieces(static_cast<Color>(ecolor), PieceType::PAWN)) != 0)
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
	addTerm<isTracing>(score,EvalParam::FORWARD_BLOCKED_BACKWARD, blocked_backward_count,trace);
	addTerm<isTracing>(score,EvalParam::FORWARD_CONTROLLED_BACKWARD, forwad_controlled_backward_count,trace);
	addTerm<isTracing>(score,EvalParam::FREE_TO_ADV_BACKWARD, free_to_advance_backward_count,trace);
}
template <bool isTracing>
void eval_double_pawns(EvaluationResult& score, EvalContext& ctx, Trace* trace) {
	int doubled_count = 0;
	for (size_t file = 0; file < 8; ++file)
	{ 
		doubled_count = 0;
		uint64_t file_mask = FILE_MASK[file];
		int white_doubled = popcount(ctx.board.get_pieces(static_cast<Color>(Color::WHITE), PieceType::PAWN) & file_mask);
		if (white_doubled > 1)
		{
			doubled_count += white_doubled - 1;


		}
		int black_doubled = popcount(ctx.board.get_pieces(static_cast<Color>(Color::BLACK), PieceType::PAWN) & file_mask);
		if (black_doubled > 1)
		{
			doubled_count -= black_doubled - 1;
		}

		addTerm<isTracing>(score, static_cast<EvalParam>(EvalParam::DOUBLE_PAWN_FILE_START + file), doubled_count, trace);
	}
}
template<bool isTracing>
void eval_king_safety(EvaluationResult& score, const EvalContext& ctx, Trace* trace) {
	{	
		int king_squares[2] = { ctx.board.get_king_square(Color::WHITE), ctx.board.get_king_square(Color::BLACK) };
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
			uint64_t king_square_colors = ((bit64(king_squares[color]) & LIGHT_SQUARES) != 0) ? LIGHT_SQUARES : DARK_SQUARES;
			uint64_t shield_mask = KING_SHIELD[color][king_squares[color]];
			int king_file_index = king_squares[color] % 8;

			//1. Pawn Shield Bonus
			if (!is_on_center_files(king_squares[color])) {
				int shield_pawns_count = popcount(ctx.get_pieces(color, PieceType::PAWN) & shield_mask);
				pawn_shield_count += color == 0 ? shield_pawns_count : -shield_pawns_count;
			}
			// 2 Next to Open File Penalty
			bool is_any_next_open = false;
			if(ctx.is_file_open(king_file_index+1)){
				next_to_open_count += color == 0 ? 1 : -1;
				is_any_next_open = true;
			}
			if(ctx.is_file_open(king_file_index-1)){
				next_to_open_count += color == 0 ? 1 : -1;
				is_any_next_open = true;
			}
			// 2.5 Open File Penalty
			bool is_king_file_open = ctx.is_file_open(king_file_index);
			if (is_king_file_open) {
				if(is_any_next_open){
					directly_on_open_next_to_open_count += color == 0 ? 1 : -1;
				}
				else{
					directly_on_open_not_next_to_open_count += color == 0 ? 1 : -1;
				}
			}
			// 3. Semi Open File Penalty
			bool on_semi_open = ctx.does_color_have_pawns_on_file(king_file_index, static_cast<Color>(ecolor));
			if(on_semi_open){
				if(!is_any_next_open){
					directly_on_semi_open_count_not_next_to_open += color == 0 ? 1 : -1;
				}
				else{
					directly_on_semi_open_count_next_to_open += color == 0 ? 1 : -1;
				}	
			}
			//3.5 Semi Open File Penalty for files next to the king
			if (ctx.does_color_have_pawns_on_file(king_file_index + 1, static_cast<Color>(ecolor))) {
				next_to_semi_open_count += color == 0 ? 1 : -1;
			}
			if (ctx.does_color_have_pawns_on_file(king_file_index - 1, static_cast<Color>(ecolor))) {
				next_to_semi_open_count += color == 0 ? 1 : -1;
			}

			//5. Open Diagonal Penalty
			uint64_t bishop_attack_mask = get_bishop_attacks(king_squares[color], 0) & ~FILE_MASK[0] & ~FILE_MASK[1];
			uint64_t op_bishop_queen_on_mask = bishop_attack_mask & (ctx.get_pieces(ecolor, PieceType::BISHOP) | ctx.get_pieces(ecolor, PieceType::QUEEN));
			while (op_bishop_queen_on_mask) {
				int sq = get_lsb(op_bishop_queen_on_mask);
				uint64_t line_between = LINE_BETWEEN[sq][king_squares[color]];
				int count = popcount(line_between & ctx.get_pieces(color,PieceType::PAWN));
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
template<bool isTracing>
void eval_mobility(EvaluationResult& score, const EvalContext& ctx, Trace* trace) {
		for (PieceType pt : {PieceType::KNIGHT, PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN}) {
			int mob_count = 0;
			for (int color = 0; color < 2; color++) {
				uint64_t pieces = ctx.get_pieces(color, pt);
				while (pieces) {
					int sq = get_lsb(pieces);
					if (color == 0)
						mob_count += popcount(get_piece_attacks(pt, sq, ctx.get_all_pieces()) & ~ctx.get_color_pieces(color));
					else
						mob_count -= popcount(get_piece_attacks(pt, sq, ctx.get_all_pieces()) & ~ctx.get_color_pieces(color));
					pieces &= pieces - 1;
				}
			}
			addTerm<isTracing>(score,static_cast<EvalParam>(EvalParam::MOBILITY_START + to_int(pt)), mob_count, trace);
		}
	}
template<bool isTracing>
void eval_rook_activity(EvaluationResult& score, const EvalContext& ctx, Trace* trace) {
	int open_count = 0;
	int semi_open_count = 0;
	int connected_rooks = 0;
	uint64_t no_white_pawns_files = 0;
	uint64_t no_black_pawns_files = 0;
	for (size_t file = 0; file < 8; file++) {
		if (!ctx.does_color_have_pawns_on_file(file, Color::WHITE)) no_white_pawns_files |= FILE_MASK[file];
		if (!ctx.does_color_have_pawns_on_file(file, Color::BLACK)) no_black_pawns_files |= FILE_MASK[file];
		for (Color  color :{Color::WHITE,Color::BLACK}) {
			if (!ctx.color_has_free_pawn(file, color)) continue;
			int passed =color==Color::BLACK ? get_lsb(ctx.get_pieces(color,PieceType::PAWN) & FILE_MASK[file]) : get_msb(ctx.get_pieces(color,PieceType::PAWN) & FILE_MASK[file]);
			if (passed == NO_SQUARE) continue;
			if(color == Color::WHITE){
				int count = popcount(ctx.get_pieces(color,PieceType::ROOK) & FORWARD_WAY_MASK[1][passed]);
				addTerm<isTracing>(score, static_cast<EvalParam>(EvalParam::ROOK_BEHIND_FREE_PAWN_START + rank(passed)), count, trace);
			}
			else{
				int count = popcount(ctx.get_pieces(color, PieceType::ROOK) & FORWARD_WAY_MASK[0][passed]);
				addTerm<isTracing>(score, static_cast<EvalParam>(EvalParam::ROOK_BEHIND_FREE_PAWN_START + flip_rank(passed)), -count, trace);
			}
		}
	}
	semi_open_count += popcount(ctx.get_pieces(0, PieceType::ROOK) & no_black_pawns_files & ~no_white_pawns_files) - popcount(ctx.get_pieces(1, PieceType::ROOK) & no_white_pawns_files & ~no_black_pawns_files);
	open_count += popcount(ctx.get_pieces(0, PieceType::ROOK) & no_black_pawns_files & no_white_pawns_files) - popcount(ctx.get_pieces(1, PieceType::ROOK) & no_white_pawns_files & no_black_pawns_files);

	int white_rook_square = get_lsb(ctx.get_pieces(0, PieceType::ROOK));
	int black_rook_square = get_lsb(ctx.get_pieces(1, PieceType::ROOK));
	int white_connected = get_rook_attacks(white_rook_square, ctx.get_all_pieces()) & ctx.get_pieces(0, PieceType::ROOK);
	int black_connected = get_rook_attacks(black_rook_square, ctx.get_all_pieces()) & ctx.get_pieces(1, PieceType::ROOK);
	connected_rooks += popcount(white_connected) - popcount(black_connected);
	addTerm<isTracing>(score, EvalParam::CONNECTED_ROOKS, connected_rooks, trace);
	addTerm<isTracing>(score, EvalParam::ROOK_ON_SEMI_OPEN_FILE, semi_open_count, trace);
	addTerm<isTracing>(score, EvalParam::ROOK_ON_OPEN_FILE, open_count, trace);
}

template<bool isTracing>
void eval_minor_pieces(EvaluationResult& score, const EvalContext& ctx, Trace* trace) {
	eval_bishop_pair<isTracing>(score, ctx, trace);
	eval_bad_bishop<isTracing>(score, ctx, trace);
	eval_trapped_minor<isTracing>(score, ctx, trace);
	eval_fianchetto_bishop<isTracing>(score, ctx, trace);
	eval_outpost<isTracing>(score, ctx, trace);
}

template<bool isTracing>
void eval_bishop_pair(EvaluationResult& score, const EvalContext& ctx, Trace* trace) {
	int bishop_pair_count = 0;
	if (popcount(ctx.board.get_pieces(Color::WHITE, PieceType::BISHOP)) >= 2) bishop_pair_count++;
	if (popcount(ctx.board.get_pieces(Color::BLACK, PieceType::BISHOP)) >= 2) bishop_pair_count--;
	addTerm<isTracing>(score, EvalParam::BISHOP_PAIR, bishop_pair_count, trace);
}
template<bool isTracing>
void eval_bad_bishop(EvaluationResult& score, const EvalContext& ctx, Trace* trace){
	int blocked_penalty_count = 0;
	int unblocked_penalty_count = 0;
	for (int color = 0; color < 2; color++) {
		uint64_t bishops = ctx.get_pieces(color, PieceType::BISHOP);

		while (bishops) {
			int sq = get_lsb(bishops);
			uint64_t bishop_color_mask = (bit64(sq) & LIGHT_SQUARES) != 0 ? LIGHT_SQUARES : DARK_SQUARES;


			// Z‰hle blockierte eigene Bauern auf der gleichen Farbe
			int blocked_pawns = 0;
			uint64_t pawns_to_check = ctx.get_pieces(color, PieceType::PAWN) & bishop_color_mask;

			while (pawns_to_check) {
				int pawn_sq = get_lsb(pawns_to_check);
				int forward_sq = (color == 0) ? pawn_sq + 8 : pawn_sq - 8;

				// Pr¸fe ob Bauer blockiert ist
				if (forward_sq >= 0 && forward_sq < 64 && (ctx.get_all_pieces() & (1ULL << forward_sq))) {
					blocked_penalty_count += color == 0 ? 1 : -1;
				}
				else {
					unblocked_penalty_count += color == 0 ? 1 : -1;
				}

				pawns_to_check &= pawns_to_check - 1;
			}

		bishops &= bishops - 1;
	}
}
	addTerm<isTracing>(score, EvalParam::BAD_BISHOP_BLOCKED, blocked_penalty_count, trace);
	addTerm<isTracing>(score, EvalParam::BAD_BISHOP_UNBLOCKED, unblocked_penalty_count, trace);
}

template<bool isTracing>
void eval_trapped_minor(EvaluationResult& score, const EvalContext& ctx, Trace* trace) {
	int trapped_bishop_count = 0;
	int trapped_knight_count = 0;

	// Typische Fallen:
	// - L‰ufer auf a7/h7 (Weiﬂ) oder a2/h2 (Schwarz) eingekesselt von Bauern
	// - Springer in der Ecke ohne Fluchtfelder

	for (int color = 0; color < 2; color++) {
		int ecolor = 1 - color;

		// L‰ufer-Fallen
		uint64_t bishops = ctx.get_pieces(color, PieceType::BISHOP);
		while (bishops) {
			int sq = get_lsb(bishops);

			// L‰ufer gefangen am Brettrand
			bool is_trapped = false;

			if (color == 0) {
				// a7-Falle: L‰ufer auf a7, Bauer auf b6
				if (sq == 48 && (ctx.get_pieces(ecolor, PieceType::PAWN) & (1ULL << 41))) is_trapped = true;
				// h7-Falle: L‰ufer auf h7, Bauer auf g6
				if (sq == 55 && (ctx.get_pieces(ecolor, PieceType::PAWN) & (1ULL << 46))) is_trapped = true;
			}
			else {
				// a2-Falle: L‰ufer auf a2, Bauer auf b3
				if (sq == 8 && (ctx.get_pieces(ecolor, PieceType::PAWN) & (1ULL << 17))) is_trapped = true;
				// h2-Falle: L‰ufer auf h2, Bauer auf g3
				if (sq == 15 && (ctx.get_pieces(ecolor, PieceType::PAWN) & (1ULL << 22))) is_trapped = true;
			}

			if (is_trapped) {
				trapped_bishop_count += color == 0 ? 1 : -1;
			}

			bishops &= bishops - 1;
		}

		// Springer-Fallen (Ecken mit blockierten Fluchtfeldern)
		uint64_t knights = ctx.get_pieces(color, PieceType::KNIGHT);
		while (knights) {
			int sq = get_lsb(knights);
			int file = sq % 8;
			int rank = sq / 8;

			// In Ecke und alle Fluchtfelder blockiert
			if ((file == 0 || file == 7) && (rank == 0 || rank == 7)) {
				uint64_t escape_squares = KNIGHT_ATTACKS[sq];
				int blocked_escapes = popcount(escape_squares & ctx.get_color_pieces(color));

				if (blocked_escapes >= 2) {  // Meiste Fluchtfelder blockiert
					trapped_knight_count += color == 0 ? 1 : -1;
				}
				else {
					bool is_trapped = true;
					uint64_t unoccupied_escapes = escape_squares & ~ctx.get_color_pieces(color);
					while (unoccupied_escapes) {
						int escape_sq = get_lsb(unoccupied_escapes);
						PieceType pt = ctx.get_piece_on_square(escape_sq);
						if (see_capture(ctx.board, sq, escape_sq, static_cast<Color>(color), PieceType::KNIGHT, pt) >= 0) {
							is_trapped = false;
							break;
						}
						unoccupied_escapes &= unoccupied_escapes - 1;
					}
					if (is_trapped) {
						trapped_knight_count += color == 0 ? 1 : -1;
					}
				}
			}

			knights &= knights - 1;
		}
	}
	addTerm<isTracing>(score, EvalParam::TRAPPED_BISHOP, trapped_bishop_count, trace);
	addTerm<isTracing>(score, EvalParam::TRAPPED_KNIGHT, trapped_knight_count, trace);
}

template<bool isTracing>
void eval_fianchetto_bishop(EvaluationResult& score, const EvalContext& ctx, Trace* trace) {
	int intact_count = 0;
	int broken_count = 0;

	// Fianchetto-Positionen: b2, g2 (Weiﬂ), b7, g7 (Schwarz)
	const uint64_t WHITE_FIANCHETTO = (1ULL << 9) | (1ULL << 14);  // b2, g2
	const uint64_t BLACK_FIANCHETTO = (1ULL << 49) | (1ULL << 54); // b7, g7

	for (int color = 0; color < 2; color++) {
		uint64_t fianchetto_mask = (color == 0) ? WHITE_FIANCHETTO : BLACK_FIANCHETTO;
		uint64_t bishops_fianchetto = ctx.get_pieces(color, PieceType::BISHOP) & fianchetto_mask;

		while (bishops_fianchetto) {
			int sq = get_lsb(bishops_fianchetto);

			// Pr¸fe ob Bauernstruktur intakt ist (Bauer auf b3/g3 oder b6/g6)
			int pawn_sq = (color == 0) ? sq + 8 : sq - 8;
			bool pawn_structure_intact = (ctx.get_pieces(color, PieceType::PAWN) & (1ULL << pawn_sq)) != 0;

			if (pawn_structure_intact) {
				intact_count += (color == 0) ? 1 : -1;
			}
			else {
				// Strafe wenn Fianchetto-Bauer fehlt (schwacher Kˆnig)
				broken_count += (color == 0) ? 1 : -1;
			}

			bishops_fianchetto &= bishops_fianchetto - 1;
		}
	}
	addTerm<isTracing>(score, EvalParam::FIANCHETTO_BISHOP, intact_count, trace);
	addTerm<isTracing>(score, EvalParam::BROKEN_FIANCHETTO, broken_count, trace);
}

template<bool isTracing>
void eval_outpost(EvaluationResult& score, const EvalContext& ctx, Trace* trace) {
	int bishop_outpost_count_no_op_bishop = 0;
	int bishop_outpost_count_with_op_bishop = 0;
	int knight_outpost_count_no_op_bishop = 0;
	int knight_outpost_count_with_op_bishop = 0;
	for (int color = 0; color < 2; color++) {
		int ecolor = color == 0 ? 1 : 0;
		uint64_t defended_by_own_pawn = get_pawn_attacks(ctx.get_pieces(color, PieceType::PAWN), static_cast<Color>(color));
		uint64_t out_post_mask = color == 0 ? WHITE_OUTPOST_MASK : BLACK_OUTPOST_MASK;
		uint64_t possible_bishop_outposts[2];
		uint64_t possible_knight_outposts[2];
		possible_bishop_outposts[0] = defended_by_own_pawn & out_post_mask & LIGHT_SQUARES & (ctx.get_pieces(color, PieceType::BISHOP));
		possible_bishop_outposts[1] = defended_by_own_pawn & out_post_mask & DARK_SQUARES & (ctx.get_pieces(color, PieceType::BISHOP));
		possible_knight_outposts[0] = defended_by_own_pawn & out_post_mask & LIGHT_SQUARES & (ctx.get_pieces(color, PieceType::KNIGHT));
		possible_knight_outposts[1] = defended_by_own_pawn & out_post_mask & DARK_SQUARES & (ctx.get_pieces(color, PieceType::KNIGHT));
		bool light_square_bishop_exists = (ctx.get_pieces(ecolor, PieceType::BISHOP) & LIGHT_SQUARES) != 0;
		bool dark_square_bishop_exists = (ctx.get_pieces(ecolor, PieceType::BISHOP) & DARK_SQUARES) != 0;
		auto count_outposts = [&](uint64_t outposts, bool enemy_bishop_exists, int& counter_no_op_bishop, int& counter_with_op_bishop) {
			while (outposts) {
				int square = get_lsb(outposts);
				if ((ctx.get_pieces(ecolor, PieceType::PAWN) & OUTPOST_MASK[color][square]) == 0) {
					if (enemy_bishop_exists) {
						counter_with_op_bishop += color == 0 ? 1 : -1;
					}
					else {
						counter_no_op_bishop += color == 0 ? 1 : -1;
					}
				}
				outposts &= outposts - 1;
			}
			};
		count_outposts(possible_bishop_outposts[0], light_square_bishop_exists, bishop_outpost_count_no_op_bishop, bishop_outpost_count_with_op_bishop);
		count_outposts(possible_bishop_outposts[1], dark_square_bishop_exists, bishop_outpost_count_no_op_bishop, bishop_outpost_count_with_op_bishop);
		count_outposts(possible_knight_outposts[0], light_square_bishop_exists, knight_outpost_count_no_op_bishop, knight_outpost_count_with_op_bishop);
		count_outposts(possible_knight_outposts[1], dark_square_bishop_exists, knight_outpost_count_no_op_bishop, knight_outpost_count_with_op_bishop);

	}
	addTerm<isTracing>(score, EvalParam::BISHOP_OUTPOST_NO_OPPOSITE_BISHOP, bishop_outpost_count_no_op_bishop, trace);
	addTerm<isTracing>(score, EvalParam::BISHOP_OUTPOST_WITH_OPPOSITE_BISHOP, bishop_outpost_count_with_op_bishop, trace);
	addTerm<isTracing>(score, EvalParam::KNIGHT_OUTPOST_NO_OPPOSITE_BISHOP, knight_outpost_count_no_op_bishop, trace);
	addTerm<isTracing>(score, EvalParam::KNIGHT_OUTPOST_WITH_OPPOSITE_BISHOP, knight_outpost_count_with_op_bishop, trace);
}