#pragma once
#include "board.h"
#include "see.h"

struct EvalContext {
	const Board& board;
	uint64_t backward[2];
	uint64_t isolated[2];
	uint64_t passed[2];
	uint8_t files_with_no_color_pawns[2];

	EvalContext(const Board& b)
		: board(b),
		backward{ 0,0 },
		isolated{ 0,0 },
		passed{ 0,0 },
		files_with_no_color_pawns{ 0,0 } {
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
	bool is_file_open(int file) const {
		if (file < 0 || file>7) return false;
		return (get_open_files() & bit8(file)) != 0;
	}
	bool does_color_have_pawns_on_file(int file, Color color) const {
		if (file < 0 || file>7) return false;
		return (files_with_no_color_pawns[to_int(color)] & bit8(file)) == 0;
	}
	bool color_has_free_pawn(int file,Color color) const {
		if (file < 0 || file>7) return false;
		return (files_with_no_color_pawns[to_int(color)] & bit8(file)) == 0 && (files_with_no_color_pawns[1 - to_int(color)] & bit8(file)) != 0;
	}
	PieceType get_piece_on_square(int square) const {
		return board.get_piece_on_square(square);
	}
};
struct PawnEvalEntry {
	uint64_t key;
	EvaluationResult score;
	uint64_t isolated[2] = { 0,0 };
	uint64_t passed[2] = { 0,0 };
	uint64_t backward[2] = { 0,0 };
	uint8_t file_info[2] = { 0,0 };
	bool valid;
};

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


enum EvalParam {
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	PAWN_PST_START,
	PAWN_PST_END = PAWN_PST_START + 63,
	KNIGHT_PST_START,
	KNIGHT_PST_END = KNIGHT_PST_START + 63,
	BISHOP_PST_START,
	BISHOP_PST_END = BISHOP_PST_START + 63,
	ROOK_PST_START,
	ROOK_PST_END = ROOK_PST_START + 63,
	QUEEN_PST_START,
	QUEEN_PST_END = QUEEN_PST_START + 63,
	KING_PST_START,
	KING_PST_END = KING_PST_START + 63,
	PASSED_PAWNS_START,
	PASSED_PAWNS_END = PASSED_PAWNS_START + 63,
	ISOLANI_START,
	ISOLANI_END = ISOLANI_START + 63,
	BLOCKED_ISOLANI_START,
	BLOCKED_ISOLANI_END = BLOCKED_ISOLANI_START + 63,
	FORWARD_BLOCKED_BACKWARD,
	FORWARD_CONTROLLED_BACKWARD,
	FREE_TO_ADV_BACKWARD,
	DOUBLE_PAWN_FILE_START,
	DOUBLE_PAWN_FILE_END = DOUBLE_PAWN_FILE_START + 7,
	PAWN_SHIELD_BONUS,
	DIRECTLY_ON_OPEN_FILE_NEXT_TO_OPEN_PENALTY,
	DIRECTLY_ON_OPEN_FILE_NOT_NEXT_TO_OPEN_PENALTY,
	NEXT_TO_OPEN_FILE_PENALTY,
	DIRECTLY_ON_SEMI_OPEN_FILE_NEXT_TO_OPEN_PENALTY,
	DIRECTLY_ON_SEMI_OPEN_FILE_NOT_NEXT_TO_OPEN_PENALTY,
	NEXT_TO_SEMI_OPEN_FILE_PENALTY,
	NEXT_TO_OPEN_DIAGONAL_PENALTY_START,
	NEXT_TO_OPEN_DIAGONAL_PENALTY_END = NEXT_TO_OPEN_DIAGONAL_PENALTY_START + 6,
	MOBILITY_START,
	MOBILITY_END=MOBILITY_START+3,
	ROOK_BEHIND_FREE_PAWN_START,
	ROOK_BEHIND_FREE_PAWN_END = ROOK_BEHIND_FREE_PAWN_START + 7,
	ROOK_ON_OPEN_FILE,
	ROOK_ON_SEMI_OPEN_FILE,
	CONNECTED_ROOKS,
	BISHOP_PAIR,
	BAD_BISHOP_BLOCKED,
	BAD_BISHOP_UNBLOCKED,
	TRAPPED_BISHOP,
	TRAPPED_KNIGHT,
	FIANCHETTO_BISHOP,
	BROKEN_FIANCHETTO,
	BISHOP_OUTPOST_NO_OPPOSITE_BISHOP,
	BISHOP_OUTPOST_WITH_OPPOSITE_BISHOP,
	KNIGHT_OUTPOST_NO_OPPOSITE_BISHOP,
	KNIGHT_OUTPOST_WITH_OPPOSITE_BISHOP,
	PARAM_COUNT
};
extern EvaluationResult EvalWeights[PARAM_COUNT];

struct Trace { 
	int counts[PARAM_COUNT]= {0}; 
	void add(EvalParam param, int count = 1) {
		counts[param] += count;
	}
};	
static int get_trace_eval(Trace* trace, const EvaluationResult weights[PARAM_COUNT],int game_phase) {
	EvaluationResult eval = {0,0};
	for (int i = 0; i < PARAM_COUNT; i++) {
		eval += weights[i] *trace->counts[i] ;
	}
	return tapered(eval,game_phase);
}

bool trace_eval_agree(const Board& board, const EvaluationResult weights[PARAM_COUNT]);

template <bool isTracing>
void addTerm(EvaluationResult& score,EvalParam param, int count=1, Trace* trace=nullptr) {
	score += EvalWeights[param] * count;
	if constexpr (isTracing) {
		if (trace) trace->add(param, count);
	}
}

template <bool isTracing=false>
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