#pragma once

#include <string>
#include "constants.h"
#include <cstdint>
#include "Move.h"
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
class Board{
    public:
        Board(const std::string& fen=STARTING_FEN);
        void display() const;
        uint64_t get_pieces(Color color, PieceType piece_type) const;
        
        void make_move(const Move& move);
        void undo_move(const Move& move);
        int make_null_move();
        void undo_null_move(int en_passant_square);
        Color get_turn() const;
        uint64_t get_color_pieces(const Color color) const;
        uint64_t get_all_pieces() const;
        PieceType get_piece_on_square(int square)const;
		Color get_color_on_square(int square) const;
        char get_char_on_square(int square) const;
        CheckInfo count_attacker_on_square(const int square,const Color attacker_color,const int bound=2, const bool need_sq=true) const;
        int get_en_passant_rights() const;
        uint8_t get_castle_rights() const;
        uint64_t get_hash() const;
        int get_material_score() const;
        double get_positional_score() const;
        double get_game_phase() const;
        bool has_enough_material_for_nmp() const;
        bool in_check() const;
    private:
        uint64_t zobrist_hash;
        int game_phase;
        int turn;
        uint8_t castling_rights;
        int en_passant_square;
        MaterialScore material_score;
        PositionalScore positional_score;
        int half_moves;
        int move_count;
        uint64_t pieces[2][6]={};
        uint64_t color_pieces[2]={};
        uint64_t all_pieces=0;
        char square_to_piece_map[64];
        uint64_t initialize_hash() const;
        MaterialScore initialize_material_score() const;
        PositionalScore initialize_positional_score() const;


        void parse_fen(const std::string& fen);
        void parse_fen_pieces(const std:: string& piece_data);
        void parse_fen_turn(const std:: string& turn_data);
        void parse_fen_castling(const std::string& castling_data);
        void parse_fen_en_passant(const std::string& en_passant_data);
        void parse_fen_half_move(const std::string& half_move_data);
        void parse_fen_move(const std::string& half_move_data);

        void initialize_board();
        void initialize_game_phase();
        void update_material_score(const Move& move,const bool undo=false);
        void update_positional_score(const Move& move,const bool undo=false);
        void update_piece_map(const Move& move, const bool undo=false);
        void update_pieces(const Move& move);
        void update_game_phase(const Move& move, const bool undo=false);
        void update_turn_rights();
        void update_castle_rights(const Move& move,const bool undo=false);
        void update_en_passsant_rights(const Move& move,const bool undo=false);
        void update_pieces_hash(const Move& move);
};