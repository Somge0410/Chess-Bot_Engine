#pragma once
#include "position.h"
#include <vector>
#include <cstdint>
#include "constants.h"

class MoveGenerator
{
private:
    static uint64_t calculate_pinned_pieces(const Position& pos,const Color friendly_color,const Color opponent_color, int king_square);
	template <bool captures_only = false>
    static void generate_king_moves(MoveList& moves,const Position& pos, Color own_color,const uint64_t& own_pieces, const int king_square);

    template <bool captures_only = false, bool with_checks = false>
    static void generate_queen_moves(MoveList& moves, const Position& pos, Color own_color,const uint64_t& pinned_info,uint64_t remedy_mask=BOARD_ALL_SET);

    template <bool captures_only = false, bool with_checks = false>
    static void generate_rook_moves(MoveList& moves, const Position& pos, Color own_color,const uint64_t& pinned_info,uint64_t remedy_mask=BOARD_ALL_SET);

    template <bool captures_only = false, bool with_checks = false>
    static void generate_bishop_moves(MoveList& moves, const Position& pos, Color own_color,const uint64_t& pinned_info,uint64_t remedy_mask=BOARD_ALL_SET);

    template <bool captures_only = false, bool with_checks = false>
    static void generate_knight_moves(MoveList& moves, const Position& pos, Color own_color,const uint64_t& pinned_info,uint64_t remedy_mask=BOARD_ALL_SET);

    template <bool captures_only = false, bool with_checks = false>
    static void generate_pawn_moves(MoveList& moves, const Position& pos, Color own_color,const int king_square,const uint64_t& pinned_info,const uint64_t& remedy_mask=BOARD_ALL_SET);

    template <bool captures_only = false, bool with_checks = false>
    static void generate_sliding_moves(MoveList& moves, PieceType piece,const Position& pos, Color own_color, const uint64_t& pinned_info, const uint64_t& remedy_mask=BOARD_ALL_SET);

    template <bool with_checks = false>
    static void generate_pawn_pushes(MoveList& moves,const Position& pos,Color own_color,const uint64_t& pinned_info,uint64_t remedy_mask=BOARD_ALL_SET);

    template <bool captures_only = false, bool with_checks = false>
    static void generate_pawn_captures(MoveList& moves, const Position& pos, Color own_color,const int king_square,const uint64_t& pinned_info,const uint64_t& remedy_mask);
public:
    MoveGenerator();

    template <bool captures_only = false, bool with_checks = false>
    static void generate_moves(const Position& pos,MoveList& moves);

    template <bool captures_only = false, bool with_checks = false>
    static void generate_moves(const Position& pos, MoveList& moves, uint64_t checkers);

    static void generate_captures(const Position& pos,MoveList& moves);
	static void generate_captures(const Position& pos, MoveList& moves, uint64_t checkers);
	static void generate_captures_with_checks(const Position& pos,MoveList& moves, uint64_t checkers);
    };


