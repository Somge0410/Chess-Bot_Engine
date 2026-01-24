#pragma once
#include "board.h"
#include <vector>
#include <cstdint>
#include "constants.h"

class MoveGenerator
{
private:
    static std::array<uint64_t,64> calculate_pinned_pieces(const Board& board,const Color friendly_color, int king_square);
    static void generate_king_moves(MoveList& moves,const Board& board, Color own_color,const uint64_t own_pieces, const int king_square, bool captures_only=false);
    static void generate_queen_moves(MoveList& moves, const Board& board, Color own_color,const std::array<uint64_t,64>& pinned_info,uint64_t remedy_mask=BOARD_ALL_SET, bool captures_only=false,bool with_checks=false);
    static void generate_rook_moves(MoveList& moves, const Board& board, Color own_color,const std::array<uint64_t,64>& pinned_info,uint64_t remedy_mask=BOARD_ALL_SET, bool captures_only=false,bool with_checks=false);
    static void generate_bishop_moves(MoveList& moves, const Board& board, Color own_color,const std::array<uint64_t,64>& pinned_info,uint64_t remedy_mask=BOARD_ALL_SET, bool captures_only=false,bool with_checks=false);
    static void generate_knight_moves(MoveList& moves, const Board& board, Color own_color,const std::array<uint64_t,64>& pinned_info,uint64_t remedy_mask=BOARD_ALL_SET, bool captures_only=false,bool with_checks=false);
    static void generate_pawn_moves(MoveList& moves, const Board& board, Color own_color,const int king_square,const std::array<uint64_t,64>& pinned_info,const uint64_t& remedy_mask=BOARD_ALL_SET, bool captures_only=false,bool with_checks=false);
    static void generate_sliding_moves(MoveList& moves, PieceType piece,const Board& board, Color own_color, const std::array<uint64_t,64>& pinned_info, const uint64_t& remedy_mask=BOARD_ALL_SET, bool captures_only=false);
    static void generate_pawn_pushes(MoveList& moves,const Board& board,Color own_color,const std::array<uint64_t,64>&pinned_info,uint64_t remedy_mask=BOARD_ALL_SET,bool with_checks=false);
    static void generate_pawn_captures(MoveList& moves, const Board& board, Color own_color,const int king_square,const std::array<uint64_t,64>& pinned_info,const uint64_t& remedy_mask);

public:
    MoveGenerator();
    
    static void generate_moves(const Board& board,MoveList& moves, bool captures_only=false,bool with_checks=false);
    static void generate_captures(const Board& board,MoveList& moves);
	static void generate_captures_with_checks(const Board& board,MoveList& moves);
    };


