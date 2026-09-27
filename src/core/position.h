#pragma once
#include "types.h"
#include <algorithm>
#include <string_view>
#include <vector>
#include "phase.h"
#include "state.h"
#include "repetition.h"
#include "Move.h"
struct AttackerInfo {
	int count{};
	Square first_attacker_square{ NO_SQUARE };
};
struct RegressionTestState {
	uint64_t zobrist;
	uint64_t pawn_key;
	int game_phase;
	EvaluationResult material_score;
	EvaluationResult positional_score;
	Bitboard checkers;

	bool operator==(const RegressionTestState& other) const {
		return zobrist == other.zobrist &&
			   pawn_key == other.pawn_key &&
			   game_phase == other.game_phase &&
			   material_score == other.material_score &&
			   positional_score == other.positional_score &&
			   checkers == other.checkers;
	}
};
class Position {
public:
	Position();
	explicit Position(std::string_view fen_text);

	void make_move(const Move& move);
	void undo_move();
	Square make_null_move();
	void undo_null_move(Square en_passant_square);
	Color get_color_on_square(Square square) const;
	Bitboard get_attacks_for_color_piece(Color color, PieceType piece_type) const;
	Bitboard get_attacks_for_color(Color color) const;
	const Bitboard get_pieces(Color color, PieceType piece_type) const {
		return pieces(color,piece_type);
	}
	Bitboard get_color_pieces(Color color) const {
        return color_pieces(color);
	}
	Bitboard get_all_pieces() const {
		return all_pieces;
	}
	const std::vector<StateInfo>& get_history() const {
		return history;
	}
	int get_half_moves() const {
		return halfmove_clock;
	}
	int get_move_count() const {
		return full_move_number;
	}
	int get_position_repeat_count() const {
		return repetition_tracker.count(zobrist_hash);
	}
	bool has_twofold() const {
		return repetition_tracker.has_any_twofold();
	}
	PieceBoards get_pieces_table() const {
		return pieces;
	}
	
	Color get_turn() const {
		return side_to_move;
	}
	bool is_white_to_move() const {
		return side_to_move == Color::White;
	}
	Square get_en_passant_rights() const {
		return en_passant_square;
	}
	CastlingRights get_castle_rights() const {
		return castling_rights;
	}
	uint64_t get_hash() const {
		return zobrist_hash;
	}
	EvaluationResult get_material_score() const {
		return material_score;
	}
	EvaluationResult get_positional_score() const {
		return positional_score;
	}
	int get_game_phase() const {
		return std::clamp(game_phase, 0, MAX_GAME_PHASE);
	}
	int get_raw_game_phase() const {
		return game_phase;
	}
	Square get_king_square(Color color) const {
		return king_squares(color);
	}
	bool is_square_attacked(Square square, Color attacker_color) const;
	Bitboard get_square_attackers(Square square, Color attacker_color) const;
	Bitboard get_checkers() const {
		return checkers;
	}
	bool in_check() const {
		return checkers != 0;
	}
	StateInfo get_state_info() const;
	bool is_repetition_draw(int repeat = 3) const {
		return get_position_repeat_count() >= repeat;
	}
	bool is_fifty_move_rule_draw() const {
		return get_half_moves() >= 100;
	}
	bool any_appeared_more_than(int count) const;
	uint64_t get_zobrist_hash() const {
		return zobrist_hash;
	}
	uint64_t get_pawn_key() const {
		return pawn_hash;
	}
	void display() const;
	char get_char_on_square(Square square) const;
	PieceType get_piece_type_on_square(Square square) const noexcept;
	PieceType get_piece_type_on_square(Color color, Square square) const noexcept;
	// Advanced Search Helpers
	template <const bool need_sq=true>
	AttackerInfo attackers_more_than(const Square square, const Color attacker_color, const int bound = 2) const;
	bool has_enough_material_for_nmp() const;
	//other
	RegressionTestState get_regression_test_state() const {
		return RegressionTestState{
			zobrist_hash,
			pawn_hash,
			game_phase,
			material_score,
			positional_score,
			checkers,
		};
	}
	RegressionTestState calculate_regression_test_state() const {
		return RegressionTestState{
			calculate_zobrist_hash(),
			calculate_pawn_hash(),
			calculate_game_phase(),
			calculate_material_score(),
			calculate_positional_score(),
			calculate_checkers(),
		};
	}


private:
	void rebuild_derived_state();
	void rebuild_occupancy();
	void find_king_squares();
	void push_current_state_to_history();
	void update_material_score(const Move& move);
	void update_positional_score(const Move& move);
	void update_game_phase(const Move& move);
	void update_castle_rights(const Move& move);
	void update_en_passsant_rights(const Move& move);
	void update_turn_rights(const Move& move);
	void update_king_square(const Move& move);
	void update_pieces(const Move& move);
	void update_pieces_hash(const Move& move);
	void update_move_count(const Move& move);
	void update_repetition_tracker();
	void update_checkers(const Move& move);
	void recover_position_state(const StateInfo& previous_state);
	bool is_square_attacked_by_pawn(Square square, Color attacker_color) const;

	int calculate_game_phase() const noexcept;
	std::uint64_t calculate_zobrist_hash() const noexcept;
	std::uint64_t calculate_pawn_hash() const noexcept;
	EvaluationResult calculate_material_score() const noexcept;
	EvaluationResult calculate_positional_score() const noexcept;
	Bitboard calculate_checkers() const noexcept;

	PieceBoards pieces{};
	ColorBoards color_pieces{};
	Bitboard all_pieces{};

	Color side_to_move{ Color::White };
	CastlingRights castling_rights{};
	EnPassantRights en_passant_square{ NO_SQUARE };
	int halfmove_clock{ 0 };
	int full_move_number{ 1 };
	int game_phase{ 24 };
	KingSquares king_squares{ Square::NO_SQUARE, Square::NO_SQUARE };
	std::vector<StateInfo> history;
	RepetitionTracker repetition_tracker;


	std::uint64_t zobrist_hash{ 0 };
	std::uint64_t pawn_hash{ 0 };
	EvaluationResult positional_score{ 0, 0 };
	EvaluationResult material_score{ 0,0 };
	Bitboard checkers{};

};
