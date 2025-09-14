#pragma once
#include "board.h"
#include <vector>
#include <cstdint>
#include "constants.h"

class MoveGenerator
{
private:
    static std::array<uint64_t,64> calculate_pinned_pieces(const Board& board,const Color friendly_color, int king_square);
    static std::vector<Move> generate_king_moves(const Board& board, Color own_color,const uint64_t own_pieces, const int king_square, bool captures_only);
    static std::vector<Move> generate_queen_moves(const Board& board, Color own_color,const std::array<uint64_t,64>& pinned_info,const uint64_t& remedy_mask=BOARD_ALL_SET, bool captures_only=false);
    static std::vector<Move> generate_rook_moves(const Board& board, Color own_color,const std::array<uint64_t,64>& pinned_info,const uint64_t& remedy_mask=BOARD_ALL_SET, bool captures_only=false);
    static std::vector<Move> generate_bishop_moves(const Board& board, Color own_color,const std::array<uint64_t,64>& pinned_info,const uint64_t& remedy_mask=BOARD_ALL_SET, bool captures_only=false);
    static std::vector<Move> generate_knight_moves(const Board& board, Color own_color,const std::array<uint64_t,64>& pinned_info,const uint64_t& remedy_mask=BOARD_ALL_SET, bool captures_only=false);
    static std::vector<Move> generate_pawn_moves(const Board& board, Color own_color,const int king_square,const std::array<uint64_t,64>& pinned_info,const uint64_t& remedy_mask=BOARD_ALL_SET, bool captures_only=false);
    static std::vector<Move> generate_sliding_moves(PieceType piece,const Board& board, Color own_color, const std::array<uint64_t,64>& pinned_info, const uint64_t& remedy_mask=BOARD_ALL_SET, bool captures_only=false);
    static std::vector<Move> generate_pawn_pushes(const Board& board,Color own_color,const std::array<uint64_t,64>&pinned_info,const uint64_t& remedy_mask=BOARD_ALL_SET);
    static std::vector<Move> generate_pawn_captures(const Board& board, Color own_color,const int king_square,const std::array<uint64_t,64>& pinned_info,const uint64_t& remedy_mask);

public:
    MoveGenerator();
    
    static std::vector<Move> generate_moves(const Board& board, bool captures_only=false);
    static std::vector<Move> generate_captures(const Board& board);
    };


