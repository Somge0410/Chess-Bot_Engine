#pragma once
#include "position.h"
#include <vector>
#include <cstdint>
#include "bitboard.h"


struct CheckInfo {
    Bitboard pinned_info = 0;
    Bitboard remedy_mask = BOARD_ALL_SET;
	Bitboard rook_checking_squares = 0;
	Bitboard bishop_checking_squares = 0;
    Bitboard knight_checking_squares = 0;
    Bitboard pawn_checking_squares = 0;
	Bitboard disc_check_info = 0;
	const Color own_color;
	const Color op_color;
	const Square own_king_sq;
	const Square op_king_sq;
	const Bitboard own_pieces;
    const Bitboard op_pieces;
	const Bitboard occupied = own_pieces | op_pieces;

    CheckInfo(Color friendly, Color opponent, Square own_king, Square opponent_king, Bitboard own_pieces, Bitboard op_pieces)
		: own_color(friendly), op_color(opponent), own_king_sq(own_king), op_king_sq(opponent_king), own_pieces(own_pieces), op_pieces(op_pieces) {
	    }
    Bitboard queen_checking_squares() {
        return rook_checking_squares | bishop_checking_squares;
    };
};

class MoveGenerator
{
private:
    static PieceType calculate_CheckInfo(const Position& pos, CheckInfo& check_info, const Bitboard& checkers,bool with_remedy);
    static void calculate_pinned_pieces(const Position& pos, CheckInfo& check_info);
	static void calculate_discovered_check(const Position& pos, CheckInfo& check_info);
	static void rook_checking_squares(const Position& pos, CheckInfo& check_info);
	static void bishop_checking_squares(const Position& pos, CheckInfo& check_info);
	static void knight_checking_squares(const Position& pos, CheckInfo& check_info);
	static void pawn_checking_squares(const Position& pos, CheckInfo& check_info);
	static PieceType remedy_mask_for_check(const Position& pos, CheckInfo& check_info, const Bitboard& checkers);

	template <bool captures_only = false, bool with_checks = false>
    static void generate_king_moves(MoveList& moves,const Position& pos, const CheckInfo& check_info);

    template <bool captures_only = false, bool with_checks = false>
    static void generate_queen_moves(MoveList& moves, const Position& pos, const CheckInfo& check_info);

    template <bool captures_only = false, bool with_checks = false>
    static void generate_rook_moves(MoveList& moves, const Position& pos, const CheckInfo& check_info);

    template <bool captures_only = false, bool with_checks = false>
    static void generate_bishop_moves(MoveList& moves, const Position& pos, const CheckInfo& check_info);

    template <bool captures_only = false, bool with_checks = false>
    static void generate_knight_moves(MoveList& moves, const Position& pos, const CheckInfo& check_info);

    template <bool captures_only = false, bool with_checks = false>
    static void generate_pawn_moves(MoveList& moves, const Position& pos,CheckInfo& check_info);

    template <bool captures_only = false, bool with_checks = false>
    static void generate_sliding_moves(MoveList& moves, PieceType piece,const Position& pos, const CheckInfo& check_info);

    template <bool with_checks = false>
    static void generate_pawn_pushes(MoveList& moves,const Position& pos,CheckInfo& check_info);

    template <bool captures_only = false, bool with_checks = false>
    static void generate_pawn_captures(MoveList& moves, const Position& pos, const CheckInfo& check_info);
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


