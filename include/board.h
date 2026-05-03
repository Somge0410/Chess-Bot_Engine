#pragma once

#include <string>
#include "constants.h"
#include <cstdint>
#include "Move.h"
#include <array>
#include "utils.h"

//helper Structs
struct PositionalScore {
    int mg;
    int eg;
};
struct MaterialScore{
    int score;
    int piece_count[2][6];
};
struct CheckInfo{
    int count;
    int attacker_square;
};

#include <unordered_map>
#include <cstdint>
#include <vector>

struct RepetitionTracker {
    static constexpr int CAP = 600;
    std::array<uint64_t, CAP> keys{};
	std::array<uint8_t, CAP> vals{};
	uint16_t current_start = 0;
    uint16_t current_end = 0;
    uint8_t current_twofold_positions = 0;

    void clear(){
        current_start = 0;
        current_end = 0;
		current_twofold_positions = 0;
	}
    void reset_boundaries(int new_start,int new_current_twofold) {
        current_end = current_start;
        current_start = new_start;
        current_twofold_positions = new_current_twofold;
	}
    uint8_t count(uint64_t h) const {
        for (int i = current_start; i != current_end; i++) {
            if (keys[i] == h) {
                return vals[i];
            }
        }
        return 0;
	}
    bool has_any_twofold() const {
        return current_twofold_positions > 0;
	}
    bool has_any_threefold() const {
        for (int i = current_start; i != current_end; i++) {
            if (vals[i] >= 3) return true;
        }
        return false;
    }
    void push(uint64_t h) {
        // find existing
        for(int i=current_start;i!=current_end;++i){
            if (keys[i]==h){
                if (vals[i]==1) current_twofold_positions++;
                vals[i]++;
                return;
            }
        }
        keys[current_end] = h;
        vals[current_end] = 1;
        current_end++;
    }
    void pop(uint64_t h) {
        for (int i = current_start; i != current_end; i++) {
            if (keys[i] == h) {
                if (vals[i] == 2) current_twofold_positions--;
                if (--vals[i] == 0) {
                    // Remove entry by swapping with last
                    keys[i] = keys[current_end - 1];
                    vals[i] = vals[current_end - 1];
                    current_end--;
                }
                return;
            }
        }
		throw std::runtime_error("Hash not found in repetition tracker during pop");
	}
    void recover_from_old(uint64_t h,int old_start, int old_twofold) {
        pop(h);
        current_start = old_start;
        current_twofold_positions = old_twofold;

    }
    uint16_t get_start() const {
        return current_start;
    }
    uint16_t get_end() const {
        return current_end;
    }
    uint8_t get_twofold() const {
        return current_twofold_positions;
    }
    void reset(uint64_t h) {
		current_start = current_end;
		current_twofold_positions = 0;
        push(h);
	}
};
struct BoardState {
    // 1. Largest Types First (8-byte aligned)
    std::array<std::array<uint64_t, 6>, 2> pieces; // 96 bytes
    std::array<uint64_t, 2> color_pieces;          // 16 bytes
    uint64_t all_pieces;                           // 8 bytes
    uint64_t zobrist_hash;                         // 8 bytes
    uint64_t pawn_key;                             // 8 bytes

    // 2. Medium Types (Size depends on your implementation, usually 2 to 4 bytes)
	EvaluationResult positional_score;             // 4 bytes
    EvaluationResult material_score;               // 4 bytes

    // 3. Bit-Fields (All 10 variables packed into a single 8-byte memory block)
    uint64_t turn : 1;
    uint64_t game_phase : 5;
    uint64_t white_king_square : 6;
    uint64_t black_king_square : 6;
    uint64_t en_passant_square : 7;
    uint64_t castling_rights : 4;
    uint64_t current_twofold_count : 6;
    uint64_t half_moves : 10;
    uint64_t move_count : 9;
    uint64_t current_repetition_tracker_start : 10;
};
// Board class
class Board{
    public:
		// Constructor and Setup 
        Board(const std::string& fen=STARTING_FEN);
        void display() const;
		void reserve_history(size_t size);
        Board(const Board& other);

		// Core State Manipulation
        void make_move(const Move& move);
        void undo_move(const Move& move);
        int make_null_move();
        void undo_null_move(int en_passant_square);

		// Getters for Board State
        inline uint64_t get_pieces(Color color, PieceType piece_type) const {
			return pieces[to_int(color)][to_int(piece_type)];
        }
        inline const std::array<std::array<uint64_t,6>,2>& get_pieces_table() const {
            return pieces;
		}
        inline uint64_t get_color_pieces(const Color color) const {
			return color_pieces[to_int(color)];
        }
        inline uint64_t get_all_pieces() const {
            return all_pieces;
        }
        PieceType get_piece_on_square(int square)const;
        Color get_color_on_square(int square) const;
        char get_char_on_square(int square) const;
		uint64_t get_pawn_attacks_for_color(Color color) const;
		uint64_t get_knight_attacks_for_color(Color color) const;
		uint64_t get_king_attacks_for_color(Color color) const;
		uint64_t get_bishop_attacks_for_color(Color color) const;
		uint64_t get_rook_attacks_for_color(Color color) const;
		uint64_t get_queen_attacks_for_color(Color color) const;
        uint64_t get_attacks_for_color(Color color) const;
        const std::vector<BoardState>& get_history() const;
		int get_half_moves() const;
		int get_move_count() const;
		int get_position_repeat_count() const;
        const bool has_twofold() const {
			return repetition_tracker.has_any_twofold();
        }

		// Getters for Game State
        Color get_turn() const;
		bool is_white_to_move() const;
        uint8_t get_en_passant_rights() const;
        uint8_t get_castle_rights() const;
        uint64_t get_hash() const;
        EvaluationResult get_material_score() const;
        EvaluationResult get_positional_score() const;
        int get_game_phase() const;
		int get_king_square(Color color) const;
        bool in_check() const;
        BoardState get_board_state() const;
		void push_current_state_to_history();
        bool is_repetition_draw(int repeat=3) const;
		bool is_fifty_move_rule_draw() const;
		bool any_appeared_more_than(int count) const;
		uint64_t get_zobrist_hash() const;
        uint64_t get_pawn_key() const;
		bool is_free_file(const int square, const Color pawn_color) const;
		// Advanced Search Helpers
        CheckInfo count_attacker_on_square(const int square,const Color attacker_color,const int bound=2, const bool need_sq=true) const;
        bool has_enough_material_for_nmp() const;
        //other
    private:
		// Member Variables
        uint64_t zobrist_hash;
        uint64_t pawn_key;
        uint8_t castling_rights;
        int en_passant_square;
        int game_phase;
        int turn;
		int white_king_square;
		int black_king_square; 

        // Bitboards

        std::array<std::array<uint64_t, 6>, 2> pieces;
        std::array<uint64_t, 2> color_pieces;
        uint64_t all_pieces = 0;

        //Scores and move counters
        EvaluationResult positional_score;
        EvaluationResult material_score;
        int half_moves;
        int move_count;
        std::vector<BoardState> history;
		RepetitionTracker repetition_tracker;
		// Private Helper Methods

		// Initialization Helpers
        void initialize_board();
        void initialize_game_phase();
        uint64_t initialize_hash() const;
		uint64_t initialize_pawn_key() const;
        EvaluationResult initialize_material_score() const;
        EvaluationResult initialize_positional_score() const;
        void debug_check_pawn_key() const;

		// FEN Parsing Helpers
        void parse_fen(const std::string& fen);
        void parse_fen_pieces(const std:: string& piece_data);
        void parse_fen_turn(const std:: string& turn_data);
        void parse_fen_castling(const std::string& castling_data);
        void parse_fen_en_passant(const std::string& en_passant_data);
        void parse_fen_half_move(const std::string& half_move_data);
        void parse_fen_move(const std::string& half_move_data);

		//Inceremental Update Helpers
        void update_pieces(const Move& move);
        void update_turn_rights(const Move& move);
        void update_castle_rights(const Move& move);
        void update_en_passsant_rights(const Move& move);
        void update_pieces_hash(const Move& move);
        void update_material_score(const Move& move);
        void update_positional_score(const Move& move);
        void update_game_phase(const Move& move);
		void update_king_square(const Move& move);
        void recover_board_state(const BoardState& previous_state);
		void update_move_count(const Move& move);
        void update_repetition_tracker();
};