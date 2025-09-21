#pragma once

#include <string>
#include "constants.h"
#include <cstdint>
#include "Move.h"
#include <array>

//helper Structs
struct PositionalScore {
    int mg;
    int eg;
};
struct MaterialScore{
    int score;
};
struct CheckInfo{
    int count;
    int attacker_square;
};
struct BoardState {
    uint64_t zobrist_hash;
    uint8_t castling_rights;
    int en_passant_square;
    int game_phase;
    int turn;
    int white_king_square;
    int black_king_square;
    std::array<std::array<uint64_t, 6>, 2> pieces;
    std::array<uint64_t, 2> color_pieces;
    uint64_t all_pieces;
    PositionalScore positional_score;
    MaterialScore material_score;
    int half_moves;
    int move_count;
};
// Board class
class Board{
    public:
		// Constructor and Setup 
        Board(const std::string& fen=STARTING_FEN);
        void display() const;

		// Core State Manipulation
        void make_move(const Move& move);
        void undo_move(const Move& move);
        int make_null_move();
        void undo_null_move(int en_passant_square);

		// Getters for Board State
        uint64_t get_pieces(Color color, PieceType piece_type) const;
        uint64_t get_color_pieces(const Color color) const;
        uint64_t get_all_pieces() const;
        PieceType get_piece_on_square(int square)const;
        Color get_color_on_square(int square) const;
        char get_char_on_square(int square) const;

		// Getters for Game State
        Color get_turn() const;
        int get_en_passant_rights() const;
        uint8_t get_castle_rights() const;
        uint64_t get_hash() const;
        int get_material_score() const;
        double get_positional_score() const;
        double get_game_phase() const;
		int get_king_square(Color color) const;
        bool in_check() const;
        BoardState get_board_state() const;
        bool is_repetition_draw() const;
		bool is_fifty_move_rule_draw() const;
		// Advanced Search Helpers
        CheckInfo count_attacker_on_square(const int square,const Color attacker_color,const int bound=2, const bool need_sq=true) const;
        bool has_enough_material_for_nmp() const;

    private:
		// Member Variables
        uint64_t zobrist_hash;
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
        PositionalScore positional_score;
        MaterialScore material_score;
        int half_moves;
        int move_count;
        std::vector<BoardState> history;
		// Private Helper Methods

		// Initialization Helpers
        void initialize_board();
        void initialize_game_phase();
        uint64_t initialize_hash() const;
        MaterialScore initialize_material_score() const;
        PositionalScore initialize_positional_score() const;

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
};