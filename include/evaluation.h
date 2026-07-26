#pragma once
#include "board.h"
#include "see.h"
#include "eval_params.h"
struct EvalContext {
	const Board& board;
	uint64_t backward[2];
	uint64_t isolated[2];
	uint64_t passed[2];
	uint8_t files_with_no_color_pawns[2];
	mutable uint64_t attacks[2];
	mutable uint8_t attacks_initialized;

	EvalContext(const Board& b)
		: board(b),
		backward{ 0,0 },
		isolated{ 0,0 },
		passed{ 0,0 },
		files_with_no_color_pawns{ 0,0 },
		attacks{ 0,0 },
		attacks_initialized(0) {
	}
	void init_file_info() {
		for (size_t file = 0; file < 8; ++file) {
			uint64_t file_mask = FILE_MASK[file];
			bool white_has_pawns = (board.get_pieces(Color::WHITE, PieceType::PAWN) & file_mask) != 0;
			bool black_has_pawns = (board.get_pieces(Color::BLACK, PieceType::PAWN) & file_mask) != 0;
			if (!white_has_pawns)
				files_with_no_color_pawns[0] |= bit8(file);
			if (!black_has_pawns)
				files_with_no_color_pawns[1] |= bit8(file);
		}
	}
	uint64_t get_pieces(Color color, PieceType piece_type) const {
		return board.get_pieces(color, piece_type);
	}
	uint64_t get_color_pieces(Color color) const {
		return board.get_color_pieces(color);
	}
	uint64_t get_pieces(int color, int piece_type) const {
		return board.get_pieces(static_cast<Color>(color), static_cast<PieceType>(piece_type));
	}
	uint64_t get_color_pieces(int color) const {
		return board.get_color_pieces(static_cast<Color>(color));
	}
	uint64_t get_pieces(int color, PieceType piece_type) const {
		return board.get_pieces(static_cast<Color>(color), piece_type);
	}
	uint8_t get_open_files() const {
		return files_with_no_color_pawns[0] & files_with_no_color_pawns[1];
	}
	uint64_t get_all_pieces() const {
		return board.get_all_pieces();
	}
	uint64_t get_attacks(Color color) const {
		const int color_index = to_int(color);
		const uint8_t initialized_bit = bit8(color_index);
		if ((attacks_initialized & initialized_bit) == 0) {
			attacks[color_index] = board.get_attacks_for_color(color);
			attacks_initialized |= initialized_bit;
		}
		return attacks[color_index];
	}
	bool is_file_open(int file) const {
		if (file < 0 || file>7) return false;
		return (get_open_files() & bit8(file)) != 0;
	}
	bool does_color_have_pawns_on_file(int file, Color color) const {
		if (file < 0 || file>7) return false;
		return (files_with_no_color_pawns[to_int(color)] & bit8(file)) == 0;
	}
	bool color_has_free_pawn(int file, Color color) const {
		if (file < 0 || file>7) return false;
		return does_color_have_pawns_on_file(file, color) && !does_color_have_pawns_on_file(file - 1, flip_color(color)) && !does_color_have_pawns_on_file(file, flip_color(color)) && !does_color_have_pawns_on_file(file + 1, flip_color(color));
	}
	PieceType get_piece_on_square(int square) const {
		return board.get_piece_on_square(square);
	}
};
struct PawnEvalEntry {
	uint64_t key = 0;
	EvaluationResult score = { 0,0 };
	uint8_t file_info[2] = { 0,0 };
	bool valid = false;
};
static_assert(sizeof(PawnEvalEntry) == 16);

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

inline int tapered(EvaluationResult score, int game_phase) {
	return (score.mg_score * game_phase + score.eg_score * (24 - game_phase)) / 24;
}



struct Trace {
	int counts[PARAM_COUNT] = { 0 };
	void add(EvalParam param, int count = 1) {
		counts[param] += count;
	}
};
static int get_trace_eval(Trace* trace, const EvaluationResult weights[PARAM_COUNT], int game_phase) {
	EvaluationResult eval = { 0,0 };
	for (int i = 0; i < PARAM_COUNT; i++) {
		eval += weights[i] * trace->counts[i];
	}
	return tapered(eval, game_phase);
}

bool trace_eval_agree(const Board& board, const EvaluationResult weights[PARAM_COUNT]);

template <bool isTracing>
void addTerm(EvaluationResult& score, EvalParam param, int count = 1, Trace* trace = nullptr) {
	score += EvalWeights[param] * count;
	if constexpr (isTracing) {
		if (trace) trace->add(param, count);
	}
}

template <bool isTracing = false>
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
void eval_double_pawns(EvaluationResult& score, EvalContext& ctx, Trace* trace);

template <bool isTracing>
void eval_king_safety(EvaluationResult& score, const EvalContext& ctx, Trace* trace);

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
