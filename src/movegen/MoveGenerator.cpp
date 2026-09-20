
#include "MoveGenerator.h"
#include <cstdint>
#include "bitboard.h"
#include "bitboard_masks.h"
#include <array>
#include "attack_rays.h"
#include "bitboard_masks.h"
#include "rook_tables.h"
#include "bishop_tables.h"
#include "attacks.h"

const int DIRECTIONS[] = { 7,8,9,1,-7,-8,-9,-1 };
const std::vector<int> QUEEN_DIR_IND = { 0,1,2,3,4,5,6,7 };
const std::vector<int> ROOK_DIR_IND = { 1,3,5,7 };
const std::vector<int> BISHOP_DIR_IND = { 0,2,4,6 };

template <bool captures_only, bool with_checks>
inline void append_piece_moves(MoveList& moves, const Position& pos,
    Square from, PieceType piece, Color own_color, Color other_color,
    Bitboard possible_moves, Bitboard enemy_pieces,
    Bitboard king_checking_squares,bool is_discovered) {
    Bitboard non_direct_checking_captures = possible_moves & enemy_pieces & ~king_checking_squares;
	Bitboard direct_checckig_captures = possible_moves & enemy_pieces & king_checking_squares;
    while (non_direct_checking_captures) {
        const Square to = pop_lsb(non_direct_checking_captures);
        moves.push_back(Move(from, to, piece, own_color,
            pos.get_piece_type_on_square(other_color, to)));
    }
    while(direct_checckig_captures){
        const Square to = pop_lsb(direct_checckig_captures);
        moves.push_back(Move(from, to, piece, own_color,
            pos.get_piece_type_on_square(other_color, to)));
	}

    if constexpr (!captures_only) {
        Bitboard quiets = possible_moves & ~enemy_pieces & ~king_checking_squares;
        while (quiets) {
            moves.push_back(Move(from, pop_lsb(quiets), piece, own_color,
                PieceType::None));
        }
        Bitboard non_capture_check = possible_moves & ~enemy_pieces & king_checking_squares;
        while (non_capture_check) {
            moves.push_back(Move(from, pop_lsb(non_capture_check), piece, own_color,
                PieceType::None));
        }
    }
    else if constexpr (with_checks) {
        Bitboard quiet_checks = 0;
        if (is_discovered) {
            quiet_checks = possible_moves & ~enemy_pieces;
        }else{
			quiet_checks = possible_moves & ~enemy_pieces & king_checking_squares;
		}
        while (quiet_checks) {
            moves.push_back(Move(from, pop_lsb(quiet_checks), piece,
                own_color, PieceType::None));
        }
		Bitboard direct_checks_no_captures = possible_moves & ~enemy_pieces & king_checking_squares;
        while (direct_checks_no_captures) {
            moves.push_back(Move(from, pop_lsb(direct_checks_no_captures), piece,
                own_color, PieceType::None));
		}
    }
}

MoveGenerator::MoveGenerator()
{

}
template <bool captures_only, bool with_checks>
void MoveGenerator::generate_moves(const Position& pos, MoveList& move_list) {
    generate_moves<captures_only, with_checks>(pos, move_list, pos.get_checkers());
}

template <bool captures_only, bool with_checks>
void MoveGenerator::generate_moves(const Position& pos, MoveList& move_list, Bitboard checkers) {
    //if (pos.is_fifty_move_rule_draw() || pos.is_repetition_draw()) return move_list;
    CheckInfo check_info=CheckInfo(pos.get_turn(),flip_color(pos.get_turn()), pos.get_king_square(pos.get_turn()),
        pos.get_king_square(flip_color(pos.get_turn())),
        pos.get_color_pieces(pos.get_turn()), 
        pos.get_color_pieces(flip_color(pos.get_turn())));
    const int check_count = popcount(checkers);
    if (check_count > 1)
    {
        generate_king_moves<captures_only>(move_list, pos, check_info);
    }
    else if (check_count == 1)
    {   
		PieceType checker=calculate_CheckInfo(pos, check_info,checkers,true);
        generate_king_moves<captures_only>(move_list, pos, check_info);
        generate_queen_moves<captures_only, with_checks>(move_list, pos, check_info);

        generate_rook_moves<captures_only, with_checks>(move_list, pos, check_info);

        generate_bishop_moves<captures_only, with_checks>(move_list, pos, check_info);
        generate_knight_moves<captures_only, with_checks>(move_list, pos, check_info);

        if (pos.get_en_passant_rights() != NO_SQUARE && checker == PieceType::Pawn) check_info.remedy_mask |= bit64(pos.get_en_passant_rights());
        generate_pawn_moves<captures_only, with_checks>(move_list, pos, check_info);
    }
    else
    {
		calculate_CheckInfo(pos, check_info, checkers,false);
        generate_queen_moves<captures_only, with_checks>(move_list, pos, check_info);

        generate_rook_moves<captures_only, with_checks>(move_list, pos, check_info);

        generate_bishop_moves<captures_only, with_checks>(move_list, pos, check_info);
        generate_knight_moves<captures_only, with_checks>(move_list, pos, check_info);

        generate_pawn_moves<captures_only, with_checks>(move_list, pos, check_info);

        generate_king_moves<captures_only>(move_list, pos, check_info);
    }
}
PieceType MoveGenerator::calculate_CheckInfo(const Position& pos, CheckInfo& check_info, const Bitboard& checkers, bool with_remedy) {
    calculate_pinned_pieces(pos, check_info);
    calculate_discovered_check(pos, check_info);
    rook_checking_squares(pos, check_info);
    bishop_checking_squares(pos, check_info);
    knight_checking_squares(pos, check_info);
    pawn_checking_squares(pos, check_info);
    PieceType checker = PieceType::None;
    if(with_remedy) checker=remedy_mask_for_check(pos, check_info, checkers);
    return checker;
}
void MoveGenerator::calculate_pinned_pieces(const Position& pos, CheckInfo& check_info) {
    Bitboard all_rook_bockers = rook_attacks(to_square(check_info.own_king_sq), pos.get_all_pieces());
    Bitboard all_bishop_blockers = bishop_attacks(to_square(check_info.own_king_sq), pos.get_all_pieces());
    Bitboard possible_rook_pinned = all_rook_bockers & pos.get_color_pieces(check_info.own_color) & ~FOUR_CORNER_MASK;
    Bitboard possible_bishop_pinned = all_bishop_blockers & pos.get_color_pieces(check_info.own_color) & ~FOUR_CORNER_MASK;
    Bitboard opponent_rooks_queens = pos.get_pieces(check_info.op_color, PieceType::Rook) | pos.get_pieces(check_info.op_color, PieceType::Queen);
    Bitboard opponent_bishops_queens = pos.get_pieces(check_info.op_color, PieceType::Bishop) | pos.get_pieces(check_info.op_color, PieceType::Queen);
    if (opponent_rooks_queens != 0) {
        Bitboard second_blockers = rook_attacks(to_square(check_info.own_king_sq), pos.get_all_pieces() ^ possible_rook_pinned) & opponent_rooks_queens & ~all_rook_bockers;
        while (second_blockers) {
            Square second_blocker_sq = lsb(second_blockers);
            Bitboard between_mask = LINE_BETWEEN(check_info.own_king_sq, second_blocker_sq);
            Square first_blocker_sq = lsb(between_mask & possible_rook_pinned);
            check_info.pinned_info |= bit64(first_blocker_sq);
            second_blockers &= second_blockers - 1;
        }
    }
    if (opponent_bishops_queens != 0) {
        Bitboard second_blockers = bishop_attacks(to_square(check_info.own_king_sq), pos.get_all_pieces() ^ possible_bishop_pinned) & opponent_bishops_queens & ~all_bishop_blockers;
        while (second_blockers) {
            Square second_blocker_sq = lsb(second_blockers);
            Bitboard between_mask = LINE_BETWEEN(check_info.own_king_sq, second_blocker_sq);
            Square first_blocker_sq = lsb(between_mask & possible_bishop_pinned);
            check_info.pinned_info |= bit64(first_blocker_sq);
            second_blockers &= second_blockers - 1;
        }
    }
}
void MoveGenerator::calculate_discovered_check(const Position& pos, CheckInfo& check_info) {
    Bitboard all_rook_bockers = rook_attacks(check_info.op_king_sq, pos.get_all_pieces());
    Bitboard all_bishop_blockers = bishop_attacks(check_info.op_king_sq, pos.get_all_pieces());
    Bitboard possible_rook_pinned = all_rook_bockers & pos.get_color_pieces(check_info.own_color) & ~FOUR_CORNER_MASK;
    Bitboard possible_bishop_pinned = all_bishop_blockers & pos.get_color_pieces(check_info.own_color) & ~FOUR_CORNER_MASK;
    Bitboard opponent_rooks_queens = pos.get_pieces(check_info.own_color, PieceType::Rook) | pos.get_pieces(check_info.own_color, PieceType::Queen);
    Bitboard opponent_bishops_queens = pos.get_pieces(check_info.own_color, PieceType::Bishop) | pos.get_pieces(check_info.own_color, PieceType::Queen);
    if (opponent_rooks_queens != 0) {
        Bitboard second_blockers = rook_attacks(check_info.op_king_sq, pos.get_all_pieces() ^ possible_rook_pinned) & opponent_rooks_queens & ~all_rook_bockers;
        while (second_blockers) {
            Square second_blocker_sq = lsb(second_blockers);
            Bitboard between_mask = LINE_BETWEEN(check_info.op_king_sq, second_blocker_sq);
            Square first_blocker_sq = lsb(between_mask & possible_rook_pinned);
            check_info.disc_check_info |= bit64(first_blocker_sq);
            second_blockers &= second_blockers - 1;
        }
    }
    if (opponent_bishops_queens != 0) {
        Bitboard second_blockers = bishop_attacks(check_info.op_king_sq, pos.get_all_pieces() ^ possible_bishop_pinned) & opponent_bishops_queens & ~all_bishop_blockers;
        while (second_blockers) {
            Square second_blocker_sq = lsb(second_blockers);
            Bitboard between_mask = LINE_BETWEEN(check_info.op_king_sq, second_blocker_sq);
            Square first_blocker_sq = lsb(between_mask & possible_bishop_pinned);
            check_info.disc_check_info |= bit64(first_blocker_sq);
            second_blockers &= second_blockers - 1;
        }
    }

}
void MoveGenerator::rook_checking_squares(const Position& pos, CheckInfo& check_info) {
    Bitboard stopping_squares = rook_attacks(check_info.op_king_sq, check_info.occupied) & ~check_info.own_pieces;
    while (stopping_squares) {
        Square sq = pop_lsb(stopping_squares);
        check_info.rook_checking_squares |= LINE_BETWEEN(check_info.op_king_sq, sq) & ~bit64(check_info.op_king_sq);
    }
}
void MoveGenerator::bishop_checking_squares(const Position& pos, CheckInfo& check_info) {
	Bitboard stopping_squares = bishop_attacks(check_info.op_king_sq, check_info.occupied) & ~check_info.own_pieces;
    while (stopping_squares) {
		Square sq = pop_lsb(stopping_squares);
		check_info.bishop_checking_squares |= LINE_BETWEEN(check_info.op_king_sq, sq) & ~bit64(check_info.op_king_sq);
    }

}
void MoveGenerator::knight_checking_squares(const Position& pos, CheckInfo& check_info) {
    Bitboard stopping_squares = knight_attacks(check_info.op_king_sq) & ~check_info.own_pieces;
}
void MoveGenerator::pawn_checking_squares(const Position& pos, CheckInfo& check_info) {
    check_info.pawn_checking_squares = pawn_attacks(check_info.op_king_sq, flip_color(check_info.own_color)) & ~check_info.own_pieces;
}
PieceType MoveGenerator::remedy_mask_for_check(const Position& pos, CheckInfo& check_info, const Bitboard& checkers) {
    const Square checker_square = lsb(checkers);
    PieceType checker = pos.get_piece_type_on_square(check_info.op_color, checker_square);
    check_info.remedy_mask = bit64(checker_square);
    if (checker == PieceType::Queen || checker == PieceType::Rook || checker == PieceType::Bishop) check_info.remedy_mask |= LINE_BETWEEN(check_info.own_king_sq, checker_square);
    return checker;
}
template <bool captures_only>
void MoveGenerator::generate_king_moves(MoveList& moves, const Position& pos, const CheckInfo& check_info) {
    Bitboard possible_moves = KING_ATTACKS[check_info.own_king_sq] & ~check_info.own_pieces;
    Bitboard quiet = possible_moves & ~check_info.op_pieces;
    Bitboard captures = possible_moves & check_info.op_pieces;

    while (captures)
    {
        Square destination_square = pop_lsb(captures);
        if (pos.attackers_more_than<false>(destination_square, check_info.op_color, 1).count > 0)
        {
            continue;
        }
        moves.push_back(Move(check_info.own_king_sq, destination_square, PieceType::King, check_info.own_color,
            pos.get_piece_type_on_square(check_info.op_color, destination_square)));
    }
    if constexpr (captures_only) return;

    while (quiet) {

        Square destination_square = pop_lsb(quiet);
        if (pos.attackers_more_than<false>(destination_square, check_info.op_color, 1).count > 0)
        {
            continue;
        }
        moves.push_back(Move(check_info.own_king_sq, destination_square, PieceType::King, check_info.own_color, PieceType::None));
    }

    if (pos.attackers_more_than<false>(to_square(check_info.own_king_sq), check_info.op_color, 1).count > 0) return;

    CastlingRights king_castle_mask = check_info.own_color == Color::White ? CastlingRights::WhiteKingside : CastlingRights::BlackKingside;
    CastlingRights queen_castle_mask = check_info.own_color == Color::White ? CastlingRights::WhiteQueenside : CastlingRights::BlackQueenside;
    if ((pos.get_castle_rights() & king_castle_mask) != 0)
    {
        Square rook_square = check_info.own_king_sq + 3;
        Bitboard line_between = LINE_BETWEEN(check_info.own_king_sq + 1, check_info.own_king_sq + 2);

        if ((line_between & pos.get_all_pieces()) == 0)
        {

            if (pos.attackers_more_than<false>(to_square(check_info.own_king_sq + 1), check_info.op_color, 1).count == 0 && pos.attackers_more_than<false>(to_square(check_info.own_king_sq + 2), check_info.op_color, 1).count == 0)
            {
                moves.push_back(Move(check_info.own_king_sq, check_info.own_king_sq + 2, PieceType::King, check_info.own_color,
                    PieceType::None, PieceType::None, true));
            }

        }

    }
    if ((pos.get_castle_rights() & queen_castle_mask) != 0)
    {
        Bitboard line_between = LINE_BETWEEN(to_square(check_info.own_king_sq - 1), to_square(check_info.own_king_sq - 3));
        if ((line_between & pos.get_all_pieces()) == 0)
        {
            if (pos.attackers_more_than<false>(to_square(check_info.own_king_sq - 1), check_info.op_color, 1).count == 0 && pos.attackers_more_than<false>(to_square(check_info.own_king_sq - 2), check_info.op_color, 1).count == 0)
            {
                moves.push_back(Move(check_info.own_king_sq, check_info.own_king_sq - 2, PieceType::King, check_info.own_color,
                    PieceType::None, PieceType::None, true));
            }

        }

    }
    return;
}
template <bool captures_only, bool with_checks>
void MoveGenerator::generate_queen_moves(MoveList& moves, const Position& pos, const CheckInfo& check_info) {
    Bitboard queens = pos.get_pieces(check_info.own_color, PieceType::Queen);
    while (queens) {
        Square queen_sq = pop_lsb(queens);
        Bitboard possible_queen_moves = queen_attacks(queen_sq, check_info.occupied) & check_info.remedy_mask & ~check_info.own_pieces;
        if (bit64(queen_sq) & check_info.pinned_info) {
            possible_queen_moves &= Complete_Line(to_square(queen_sq), pos.get_king_square(check_info.own_color));
        }
        append_piece_moves<captures_only, with_checks>(moves, pos, queen_sq,
            PieceType::Queen, check_info.own_color, check_info.op_color, possible_queen_moves,
            check_info.op_pieces, check_info.rook_checking_squares|check_info.bishop_checking_squares, false);
    }

}

template <bool captures_only, bool with_checks>
void MoveGenerator::generate_rook_moves(MoveList& moves, const Position& pos,const CheckInfo& check_info) {
    Bitboard rooks = pos.get_pieces(check_info.own_color, PieceType::Rook);
    while (rooks) {
        Square rook_sq = pop_lsb(rooks);
        Bitboard possible_rook_moves = rook_attacks(rook_sq, check_info.occupied) & check_info.remedy_mask & ~check_info.own_pieces;
        if (bit64(rook_sq) & check_info.pinned_info) {
            possible_rook_moves &= Complete_Line(to_square(rook_sq), pos.get_king_square(check_info.own_color));
        }
		bool is_discovered = bit64(rook_sq) & check_info.disc_check_info;
        append_piece_moves<captures_only, with_checks>(moves, pos, rook_sq,
            PieceType::Rook, check_info.own_color, check_info.op_color, possible_rook_moves,
            check_info.op_pieces, check_info.rook_checking_squares,is_discovered);
    }


}
template <bool captures_only, bool with_checks>
void MoveGenerator::generate_bishop_moves(MoveList& moves, const Position& pos, const CheckInfo& check_info) {
    Bitboard bishops = pos.get_pieces(check_info.own_color, PieceType::Bishop);
    while (bishops) {
        Square bishop_sq = pop_lsb(bishops);
        Bitboard possible_bishop_moves = bishop_attacks(bishop_sq, check_info.occupied) & check_info.remedy_mask & ~check_info.own_pieces;
        if (bit64(bishop_sq) & check_info.pinned_info) {
            possible_bishop_moves &= Complete_Line(to_square(bishop_sq), pos.get_king_square(check_info.own_color));
        }
		bool is_discovered = bit64(bishop_sq) & check_info.disc_check_info;
        append_piece_moves<captures_only, with_checks>(moves, pos, bishop_sq,
            PieceType::Bishop, check_info.own_color, check_info.op_color, possible_bishop_moves,
            check_info.op_pieces, check_info.bishop_checking_squares,is_discovered);
    }


}
template <bool captures_only, bool with_checks>
void MoveGenerator::generate_knight_moves(MoveList& moves, const Position& pos, const CheckInfo& check_info) {
    Bitboard knights = pos.get_pieces(check_info.own_color, PieceType::Knight);
    while (knights) {
        Square knight_sq = pop_lsb(knights);
        Bitboard possible_knight_moves = knight_attacks(knight_sq) & check_info.remedy_mask & ~check_info.own_pieces;
        if (bit64(knight_sq) & check_info.pinned_info) {
            continue;
        }
		bool is_discovered = bit64(knight_sq) & check_info.disc_check_info;
        append_piece_moves<captures_only, with_checks>(moves, pos, knight_sq,
            PieceType::Knight, check_info.own_color, check_info.op_color, possible_knight_moves,
            check_info.op_pieces, check_info.knight_checking_squares,is_discovered);
    }

}
template <bool captures_only, bool with_checks>
void MoveGenerator::generate_pawn_moves(MoveList& moves, const Position& pos, CheckInfo& check_info) {

    if constexpr (!captures_only || (captures_only && with_checks))
    {
        generate_pawn_pushes<with_checks>(moves, pos, check_info);
    }

    generate_pawn_captures<captures_only, with_checks>(moves, pos, check_info);
    return;
}
template <bool with_checks>
void MoveGenerator::generate_pawn_pushes(MoveList& moves, const Position& pos, CheckInfo& check_info) {

    Bitboard own_pawns = pos.get_pieces(check_info.own_color, PieceType::Pawn);
    int push_step = (check_info.own_color == Color::White) ? 8 : -8;
    int start_rank = (check_info.own_color == Color::White) ? 1 : 6;
    int promotion_rank = (check_info.own_color == Color::White) ? 6 : 1;
    CastlingRights castle_rights = pos.get_castle_rights();
    Square en_passant_square = pos.get_en_passant_rights();
    if constexpr (with_checks) {
        int op_king_square = square_index(check_info.op_king_sq);
        check_info.remedy_mask &= PAWN_ATTACKS[to_int(check_info.op_color)][op_king_square];
    }
    while (own_pawns)
    {
        Square from_square = lsb(own_pawns);
        int rank = ::rank(from_square);
        Square go_to_square = from_square + push_step;
        if (go_to_square != NO_SQUARE)
        {   
            if (!(bit64(go_to_square) & check_info.occupied))
            {
                Bitboard pinned_mask = BOARD_ALL_SET;
                if (bit64(from_square) & check_info.pinned_info) {
                    pinned_mask = Complete_Line(from_square, check_info.own_king_sq);
                }
                if (pinned_mask & bit64(go_to_square) & check_info.remedy_mask)
                {
                    bool is_promotion = (rank == promotion_rank);
                    if (is_promotion)
                    {
                        bool is_discovered = (bit64(from_square) & check_info.disc_check_info);
                        bool is_direct_rook_check = false; 
                        //to do: determine if the promotion piece is checking piece. means pawn pushes needs all those checking infos seperately
                        moves.push_back(Move(from_square, go_to_square, PieceType::Pawn, check_info.own_color, PieceType::None, PieceType::Queen));
                        moves.push_back(Move(from_square, go_to_square, PieceType::Pawn, check_info.own_color, PieceType::None, PieceType::Rook));
                        moves.push_back(Move(from_square, go_to_square, PieceType::Pawn, check_info.own_color, PieceType::None, PieceType::Bishop));
                        moves.push_back(Move(from_square, go_to_square, PieceType::Pawn, check_info.own_color, PieceType::None, PieceType::Knight));
                    }
                    else
                    {   
                     bool is_discovered = (bit64(from_square) & check_info.disc_check_info) && !(get_file(from_square) == get_file(check_info.op_king_sq));
					 bool is_direct_check = (bit64(go_to_square) & check_info.pawn_checking_squares);            
                        moves.push_back(Move(from_square, go_to_square, PieceType::Pawn, check_info.own_color, PieceType::None));
                    }
                }

                if (rank == start_rank)
                {
                    Square to_square2 = from_square + 2 * push_step;
                    if (!(bit64(to_square2) & check_info.occupied))
                    {
                        if (pinned_mask & bit64(to_square2) & check_info.remedy_mask)
                        {   

                            bool is_discovered = (bit64(from_square) & check_info.disc_check_info) && !(get_file(from_square) == get_file(check_info.op_king_sq));
                            bool is_direct_check = (bit64(go_to_square) & check_info.pawn_checking_squares);
                            moves.push_back(Move(from_square, to_square2, PieceType::Pawn, check_info.own_color, PieceType::None));
                        }

                    }

                }


            }

        }
        own_pawns &= own_pawns - 1;
    }
    return;
}
template <bool captures_only, bool with_checks>
void MoveGenerator::generate_pawn_captures(MoveList& moves, const Position& pos, const CheckInfo& check_info) {

    Bitboard own_pawns = pos.get_pieces(check_info.own_color, PieceType::Pawn);
    Bitboard new_remedy = check_info.remedy_mask;
    if (pos.get_en_passant_rights() != NO_SQUARE) new_remedy |= bit64(pos.get_en_passant_rights());
    while (own_pawns)
    {
        int from_square = square_index(lsb(own_pawns));

        Bitboard attack_bb = PAWN_ATTACKS[to_int(check_info.own_color)][from_square];

        Bitboard capture_bb = attack_bb & check_info.op_pieces;

        if (bit64(from_square) & check_info.pinned_info) {
            capture_bb &= Complete_Line(to_square(from_square), check_info.own_king_sq);
        }
        capture_bb &= check_info.remedy_mask;

        while (capture_bb)
        {
            Square to_square = lsb(capture_bb);
            PieceType captured_piece = pos.get_piece_type_on_square(check_info.op_color, to_square);

            bool is_promotion = (check_info.own_color == Color::White && square_index(to_square) >= 56) || (check_info.own_color == Color::Black && square_index(to_square) <= 7);

            if (is_promotion
                )
            {
                bool is_discovered = (bit64(from_square) & check_info.disc_check_info);
                bool is_direct_rook_check = false;
                //to do: determine if the promotion piece is checking piece. means pawn pushes needs all those checking infos seperately
                moves.push_back(Move(from_square, to_square, PieceType::Pawn, check_info.own_color, captured_piece, PieceType::Queen));
                moves.push_back(Move(from_square, to_square, PieceType::Pawn, check_info.own_color, captured_piece, PieceType::Rook));
                moves.push_back(Move(from_square, to_square, PieceType::Pawn, check_info.own_color, captured_piece, PieceType::Bishop));
                moves.push_back(Move(from_square, to_square, PieceType::Pawn, check_info.own_color, captured_piece, PieceType::Knight));
            }
            else {

                bool is_discovered = (bit64(from_square) & check_info.disc_check_info);
                bool is_direct_check = (bit64(to_square) & check_info.pawn_checking_squares);
                moves.push_back(Move(from_square, to_square, PieceType::Pawn, check_info.own_color, captured_piece));
            }
            capture_bb &= capture_bb - 1;
        }

        Square ep_square = pos.get_en_passant_rights();
        if (ep_square != NO_SQUARE)
        {
            Bitboard pinned_mask = BOARD_ALL_SET;
            if (bit64(from_square) & check_info.pinned_info) {
                pinned_mask = Complete_Line(to_square(from_square), check_info.own_king_sq);
            }
            if (attack_bb & bit64(ep_square) & pinned_mask & check_info.remedy_mask)
            {
                if (rank(check_info.own_king_sq) != rank(from_square))
                {   

                    bool is_discovered = (bit64(from_square) & check_info.disc_check_info);
                    bool is_direct_check = (bit64(ep_square) & check_info.pawn_checking_squares);
                    moves.push_back(Move(from_square, ep_square, PieceType::Pawn, check_info.own_color, PieceType::Pawn, PieceType::None, false, true));
                }
                else
                {
                    int dir_index = (check_info.own_king_sq > from_square) ? 7 : 3;
                    Square capture_square = (check_info.own_color == Color::White) ? ep_square - 8 : ep_square + 8;
                    Bitboard two_pawns_mask = bit64(from_square) | bit64(capture_square);
                    Bitboard opponent_rook_queen = pos.get_pieces(check_info.op_color, PieceType::Rook) | pos.get_pieces(check_info.op_color, PieceType::Queen);
					Bitboard own_rook_queen = pos.get_pieces(check_info.own_color, PieceType::Rook) | pos.get_pieces(check_info.own_color, PieceType::Queen);
                    Bitboard ray = RAY_MASK[dir_index][square_index(check_info.own_king_sq)] ^ two_pawns_mask;
					Bitboard discovered_check_ray = RAY_MASK[dir_index][square_index(check_info.own_king_sq)] ^ two_pawns_mask;
                    ray &= pos.get_all_pieces();
                    discovered_check_ray &= pos.get_all_pieces();
                    Square next_piece_square = (dir_index == 7) ? msb(ray) : lsb(ray);
					Square next_discovered_check_piece_square = (dir_index == 7) ? msb(discovered_check_ray) : lsb(discovered_check_ray);
                    if ((next_piece_square == NO_SQUARE) || (opponent_rook_queen & bit64(next_piece_square)) == 0)
                    {   
						bool is_discovered = (bit64(from_square) & check_info.disc_check_info) || (bit64(next_discovered_check_piece_square) & own_rook_queen);
						bool is_direct_check = (bit64(ep_square) & check_info.pawn_checking_squares);
                        moves.push_back(Move(from_square, ep_square, PieceType::Pawn, check_info.own_color, PieceType::Pawn, PieceType::None, false, true));
                    }
                    else {
                        own_pawns &= own_pawns - 1;
                        continue;
                    }

                }


            }

        }

        own_pawns &= own_pawns - 1;

    }
    return;

}

void MoveGenerator::generate_captures(const Position& pos, MoveList& moves) {
    return generate_moves<true, false>(pos, moves);
}
void MoveGenerator::generate_captures(const Position& pos, MoveList& moves, Bitboard checkers) {
    return generate_moves<true, false>(pos, moves, checkers);
}
void MoveGenerator::generate_captures_with_checks(const Position& pos, MoveList& moves, Bitboard checkers) {
    return generate_moves<true, true>(pos, moves, checkers);
}



// Force the compiler to generate these specific versions of the template
template void MoveGenerator::generate_moves<false, false>(const Position&, MoveList&);
template void MoveGenerator::generate_moves<true, false>(const Position&, MoveList&);
template void MoveGenerator::generate_moves<false, true>(const Position&, MoveList&);
template void MoveGenerator::generate_moves<true, true>(const Position&, MoveList&);
template void MoveGenerator::generate_moves<false, false>(const Position&, MoveList&, Bitboard);
template void MoveGenerator::generate_moves<true, false>(const Position&, MoveList&, Bitboard);
template void MoveGenerator::generate_moves<false, true>(const Position&, MoveList&, Bitboard);
template void MoveGenerator::generate_moves<true, true>(const Position&, MoveList&, Bitboard);
