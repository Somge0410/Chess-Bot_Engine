




#include "eval_context.h"
#include "eval_internal.h"
#include "see.h"
#include "trace.h"

template<bool isTracing>
void eval_mobility(EvaluationResult& score, const EvalContext& ctx, Trace* trace) {
	for (PieceType pt : {PieceType::Knight, PieceType::Bishop, PieceType::Rook, PieceType::Queen}) {
		int mob_count = 0;
		for (int color = 0; color < 2; color++) {
			int ecolor = color == 0 ? 1 : 0;
			uint64_t enemy_attacks = ctx.get_attacks(static_cast<Color>(ecolor));
			uint64_t pieces = ctx.get_pieces(color, pt);
			while (pieces) {
				int sq = lsb(pieces);
				if (color == 0)
					mob_count += popcount(piece_attacks(to_square(sq), Color::Black, ctx.get_all_pieces(), pt) & ~ctx.get_color_pieces(color) & ~enemy_attacks);
				else
					mob_count -= popcount(piece_attacks(to_square(sq), Color::White, ctx.get_all_pieces(), pt) & ~ctx.get_color_pieces(color) & ~enemy_attacks);
				pieces &= pieces - 1;
			}
		}
		addTerm<isTracing>(score, static_cast<EvalParam>(EvalParam::MOBILITY_START + to_int(pt) - 1), mob_count, trace);
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
		if (!ctx.does_color_have_pawns_on_file(file, Color::White)) no_white_pawns_files |= FILE_MASK[file];
		if (!ctx.does_color_have_pawns_on_file(file, Color::Black)) no_black_pawns_files |= FILE_MASK[file];

	}
	semi_open_count += popcount(ctx.get_pieces(0, PieceType::Rook) & ~no_black_pawns_files & no_white_pawns_files) - popcount(ctx.get_pieces(1, PieceType::Rook) & ~no_white_pawns_files & no_black_pawns_files);
	open_count += popcount(ctx.get_pieces(0, PieceType::Rook) & no_black_pawns_files & no_white_pawns_files) - popcount(ctx.get_pieces(1, PieceType::Rook) & no_white_pawns_files & no_black_pawns_files);

	int white_rook_square = lsb(ctx.get_pieces(0, PieceType::Rook));
	int black_rook_square = lsb(ctx.get_pieces(1, PieceType::Rook));
	uint64_t white_connected = rook_attacks(to_square(white_rook_square), ctx.get_all_pieces()) & ctx.get_pieces(0, PieceType::Rook);
	uint64_t black_connected = rook_attacks(to_square(black_rook_square), ctx.get_all_pieces()) & ctx.get_pieces(1, PieceType::Rook);
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
	if (popcount(ctx.pos.get_pieces(Color::White, PieceType::Bishop)) >= 2) bishop_pair_count++;
	if (popcount(ctx.pos.get_pieces(Color::Black, PieceType::Bishop)) >= 2) bishop_pair_count--;
	addTerm<isTracing>(score, EvalParam::BISHOP_PAIR, bishop_pair_count, trace);
}
template<bool isTracing>
void eval_bad_bishop(EvaluationResult& score, const EvalContext& ctx, Trace* trace) {
	int blocked_penalty_count = 0;
	int unblocked_penalty_count = 0;
	for (int color = 0; color < 2; color++) {
		uint64_t bishops = ctx.get_pieces(color, PieceType::Bishop);

		while (bishops) {
			int sq = lsb(bishops);
			uint64_t bishop_color_mask = (bit64(sq) & LIGHT_SQUARES) != 0 ? LIGHT_SQUARES : DARK_SQUARES;


			// Zähle blockierte eigene Bauern auf der gleichen Farbe
			int blocked_pawns = 0;
			uint64_t pawns_to_check = ctx.get_pieces(color, PieceType::Pawn) & bishop_color_mask;

			while (pawns_to_check) {
				int pawn_sq = lsb(pawns_to_check);
				int forward_sq = (color == 0) ? pawn_sq + 8 : pawn_sq - 8;

				// Prüfe ob Bauer blockiert ist
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
	// - Läufer auf a7/h7 (Weiß) oder a2/h2 (Schwarz) eingekesselt von Bauern
	// - Springer in der Ecke ohne Fluchtfelder

	for (int color = 0; color < 2; color++) {
		int ecolor = 1 - color;

		// Läufer-Fallen
		uint64_t bishops = ctx.get_pieces(color, PieceType::Bishop);
		while (bishops) {
			int sq = lsb(bishops);

			// Läufer gefangen am Brettrand
			bool is_trapped = false;

			if (color == 0) {
				// a7-Falle: Läufer auf a7, Bauer auf b6
				if (sq == 48 && (ctx.get_pieces(ecolor, PieceType::Pawn) & (1ULL << 41))) is_trapped = true;
				// h7-Falle: Läufer auf h7, Bauer auf g6
				if (sq == 55 && (ctx.get_pieces(ecolor, PieceType::Pawn) & (1ULL << 46))) is_trapped = true;
			}
			else {
				// a2-Falle: Läufer auf a2, Bauer auf b3
				if (sq == 8 && (ctx.get_pieces(ecolor, PieceType::Pawn) & (1ULL << 17))) is_trapped = true;
				// h2-Falle: Läufer auf h2, Bauer auf g3
				if (sq == 15 && (ctx.get_pieces(ecolor, PieceType::Pawn) & (1ULL << 22))) is_trapped = true;
			}

			if (is_trapped) {
				trapped_bishop_count += color == 0 ? 1 : -1;
			}

			bishops &= bishops - 1;
		}

		// Springer-Fallen (Ecken mit blockierten Fluchtfeldern)
		uint64_t knights = ctx.get_pieces(color, PieceType::Knight);
		while (knights) {
			int sq = lsb(knights);
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
						int escape_sq = lsb(unoccupied_escapes);
						PieceType pt = ctx.get_piece_on_square(escape_sq);
						if (see_capture_ge(ctx.pos, sq, escape_sq, static_cast<Color>(color), PieceType::Knight, pt, 0)) {
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

	// Fianchetto-Positionen: b2, g2 (Weiß), b7, g7 (Schwarz)
	const uint64_t WHITE_FIANCHETTO = (1ULL << 9) | (1ULL << 14);  // b2, g2
	const uint64_t BLACK_FIANCHETTO = (1ULL << 49) | (1ULL << 54); // b7, g7

	for (int color = 0; color < 2; color++) {
		uint64_t fianchetto_mask = (color == 0) ? WHITE_FIANCHETTO : BLACK_FIANCHETTO;
		uint64_t bishops_fianchetto = ctx.get_pieces(color, PieceType::Bishop) & fianchetto_mask;

		while (bishops_fianchetto) {
			int sq = lsb(bishops_fianchetto);

			// Prüfe ob Bauernstruktur intakt ist (Bauer auf b3/g3 oder b6/g6)
			int pawn_sq = (color == 0) ? sq + 8 : sq - 8;
			bool pawn_structure_intact = (ctx.get_pieces(color, PieceType::Pawn) & (1ULL << pawn_sq)) != 0;

			if (pawn_structure_intact) {
				intact_count += (color == 0) ? 1 : -1;
			}
			else {
				// Strafe wenn Fianchetto-Bauer fehlt (schwacher König)
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
		uint64_t defended_by_own_pawn = pawn_attacks(ctx.get_pieces(color, PieceType::Pawn), static_cast<Color>(color));
		uint64_t out_post_mask = color == 0 ? WHITE_OUTPOST_MASK : BLACK_OUTPOST_MASK;
		uint64_t possible_bishop_outposts[2];
		uint64_t possible_knight_outposts[2];
		possible_bishop_outposts[0] = defended_by_own_pawn & out_post_mask & LIGHT_SQUARES & (ctx.get_pieces(color, PieceType::Bishop));
		possible_bishop_outposts[1] = defended_by_own_pawn & out_post_mask & DARK_SQUARES & (ctx.get_pieces(color, PieceType::Bishop));
		possible_knight_outposts[0] = defended_by_own_pawn & out_post_mask & LIGHT_SQUARES & (ctx.get_pieces(color, PieceType::Knight));
		possible_knight_outposts[1] = defended_by_own_pawn & out_post_mask & DARK_SQUARES & (ctx.get_pieces(color, PieceType::Knight));
		bool light_square_bishop_exists = (ctx.get_pieces(ecolor, PieceType::Bishop) & LIGHT_SQUARES) != 0;
		bool dark_square_bishop_exists = (ctx.get_pieces(ecolor, PieceType::Bishop) & DARK_SQUARES) != 0;
		auto count_outposts = [&](uint64_t outposts, bool enemy_bishop_exists, int& counter_no_op_bishop, int& counter_with_op_bishop) {
			while (outposts) {
				int square = lsb(outposts);
				if ((ctx.get_pieces(ecolor, PieceType::Pawn) & OUTPOST_MASK[color][square]) == 0) {
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

// Only these entry points are called from other translation units.
// Their specializations also instantiate the private piece helpers they call.
template void eval_mobility<false>(EvaluationResult&, const EvalContext&, Trace*);
template void eval_mobility<true>(EvaluationResult&, const EvalContext&, Trace*);
template void eval_rook_activity<false>(EvaluationResult&, const EvalContext&, Trace*);
template void eval_rook_activity<true>(EvaluationResult&, const EvalContext&, Trace*);
template void eval_minor_pieces<false>(EvaluationResult&, const EvalContext&, Trace*);
template void eval_minor_pieces<true>(EvaluationResult&, const EvalContext&, Trace*);
