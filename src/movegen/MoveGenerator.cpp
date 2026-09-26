
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
    Bitboard king_checking_squares, bool is_discovered_candidate,
    Square opponent_king) {
    const Bitboard discovery_line = is_discovered_candidate
        ? LINE_BETWEEN(from, opponent_king)
        : 0;
    const Bitboard direct_moves = possible_moves & king_checking_squares;
    const Bitboard discovered_moves = is_discovered_candidate
        ? possible_moves & ~discovery_line
        : 0;
    const Bitboard checking_moves = direct_moves | discovered_moves;

    Bitboard captures = possible_moves & enemy_pieces & ~checking_moves;
    while (captures) {
        const Square to = pop_lsb(captures);
        moves.push_back(Move(from, to, piece, own_color,
            pos.get_piece_type_on_square(other_color, to), PieceType::None,
            false, false, false, false));
    }
    captures = possible_moves & enemy_pieces & checking_moves;
    while (captures) {
        const Square to = pop_lsb(captures);
        moves.push_back(Move(from, to, piece, own_color,
            pos.get_piece_type_on_square(other_color, to), PieceType::None,
            false, false, (bit64(to) & direct_moves) != 0,
            (bit64(to) & discovered_moves) != 0));
    }

    if constexpr (!captures_only || with_checks) {
        if constexpr (!captures_only) {
            Bitboard quiets = possible_moves & ~enemy_pieces & ~checking_moves;
            while (quiets) {
                const Square to = pop_lsb(quiets);
                moves.push_back(Move(from, to, piece, own_color,
                    PieceType::None, PieceType::None, false, false,
                    false, false));
            }
        }

        Bitboard quiet_checks =
            possible_moves & ~enemy_pieces & checking_moves;
        while (quiet_checks) {
            const Square to = pop_lsb(quiet_checks);
            moves.push_back(Move(from, to, piece, own_color,
                PieceType::None, PieceType::None, false, false,
                (bit64(to) & direct_moves) != 0,
                (bit64(to) & discovered_moves) != 0));
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
        calculate_discovered_check(pos, check_info);
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
    check_info.rook_checking_squares = rook_attacks(check_info.op_king_sq, check_info.occupied) & ~check_info.own_pieces;
}
void MoveGenerator::bishop_checking_squares(const Position& pos, CheckInfo& check_info) {
	check_info.bishop_checking_squares = bishop_attacks(check_info.op_king_sq, check_info.occupied) & ~check_info.own_pieces;
}
void MoveGenerator::knight_checking_squares(const Position& pos, CheckInfo& check_info) {
    check_info.knight_checking_squares = knight_attacks(check_info.op_king_sq) & ~check_info.own_pieces;
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
template <bool captures_only,bool with_checks>
void MoveGenerator::generate_king_moves(MoveList& moves, const Position& pos, const CheckInfo& check_info) {
    Bitboard possible_moves = KING_ATTACKS[check_info.own_king_sq] & ~check_info.own_pieces;
    Bitboard quiet = possible_moves & ~check_info.op_pieces;
    Bitboard captures = possible_moves & check_info.op_pieces;
    const bool is_discovered_candidate =
        (bit64(check_info.own_king_sq) & check_info.disc_check_info) != 0;
    const Bitboard discovery_line = is_discovered_candidate
        ? Complete_Line(check_info.own_king_sq, check_info.op_king_sq)
        : 0;

    while (captures)
    {
        const Square destination_square = pop_lsb(captures);
        if (pos.attackers_more_than<false>(destination_square, check_info.op_color, 1).count > 0)
        {
            continue;
        }
        const bool discovered_check = is_discovered_candidate
            && (bit64(destination_square) & discovery_line) == 0;
        moves.push_back(Move(check_info.own_king_sq, destination_square,
            PieceType::King, check_info.own_color,
            pos.get_piece_type_on_square(check_info.op_color, destination_square),
            PieceType::None, false, false, false, discovered_check));
    }
    if constexpr (captures_only && !with_checks) return;

    while (quiet) {
        const Square destination_square = pop_lsb(quiet);
        if (pos.attackers_more_than<false>(destination_square, check_info.op_color, 1).count > 0)
        {
            continue;
        }
        const bool discovered_check = is_discovered_candidate
            && (bit64(destination_square) & discovery_line) == 0;
        if constexpr (with_checks) {
			if (!discovered_check) continue;
        }
        moves.push_back(Move(check_info.own_king_sq, destination_square,
            PieceType::King, check_info.own_color, PieceType::None,
            PieceType::None, false, false, false, discovered_check));
    }

    if (pos.attackers_more_than<false>(to_square(check_info.own_king_sq), check_info.op_color, 1).count > 0) return;

    const auto append_castle = [&](Square king_to, Square rook_from,
        Square rook_to) {
        const Bitboard new_occupancy = check_info.occupied
            ^ bit64(check_info.own_king_sq) ^ bit64(king_to)
            ^ bit64(rook_from) ^ bit64(rook_to);
        const bool direct_check =
            (rook_attacks(rook_to, new_occupancy)
                & bit64(check_info.op_king_sq)) != 0;

        Bitboard rook_queen_after =
            pos.get_pieces(check_info.own_color, PieceType::Rook)
            | pos.get_pieces(check_info.own_color, PieceType::Queen);
        rook_queen_after ^= bit64(rook_from) | bit64(rook_to);
        const Bitboard bishop_queen =
            pos.get_pieces(check_info.own_color, PieceType::Bishop)
            | pos.get_pieces(check_info.own_color, PieceType::Queen);
        Bitboard discovered_checkers =
            rook_attacks(check_info.op_king_sq, new_occupancy)
                & rook_queen_after & ~bit64(rook_to);
        discovered_checkers |=
            bishop_attacks(check_info.op_king_sq, new_occupancy)
                & bishop_queen;

        moves.push_back(Move(check_info.own_king_sq, king_to,
            PieceType::King, check_info.own_color, PieceType::None,
            PieceType::None, true, false, direct_check,
            discovered_checkers != 0));
    };

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
                append_castle(check_info.own_king_sq + 2, rook_square,
                    check_info.own_king_sq + 1);
            }

        }

    }
    if ((pos.get_castle_rights() & queen_castle_mask) != 0)
    {
        const Square rook_square = check_info.own_king_sq - 4;
        Bitboard line_between = LINE_BETWEEN(to_square(check_info.own_king_sq - 1), to_square(check_info.own_king_sq - 3));
        if ((line_between & pos.get_all_pieces()) == 0)
        {
            if (pos.attackers_more_than<false>(to_square(check_info.own_king_sq - 1), check_info.op_color, 1).count == 0 && pos.attackers_more_than<false>(to_square(check_info.own_king_sq - 2), check_info.op_color, 1).count == 0)
            {
                append_castle(check_info.own_king_sq - 2, rook_square,
                    check_info.own_king_sq - 1);
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
        const bool is_discovered =
            (bit64(queen_sq) & check_info.disc_check_info) != 0;
        append_piece_moves<captures_only, with_checks>(moves, pos, queen_sq,
            PieceType::Queen, check_info.own_color, check_info.op_color, possible_queen_moves,
            check_info.op_pieces,
            check_info.rook_checking_squares | check_info.bishop_checking_squares,
            is_discovered, check_info.op_king_sq);
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
            check_info.op_pieces, check_info.rook_checking_squares,
            is_discovered, check_info.op_king_sq);
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
            check_info.op_pieces, check_info.bishop_checking_squares,
            is_discovered, check_info.op_king_sq);
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
            check_info.op_pieces, check_info.knight_checking_squares,
            is_discovered, check_info.op_king_sq);
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
    const int push_step = check_info.own_color == Color::White ? 8 : -8;
    const int start_rank = check_info.own_color == Color::White ? 1 : 6;
    const int promotion_rank = check_info.own_color == Color::White ? 6 : 1;

    while (own_pawns) {
        const Square from = pop_lsb(own_pawns);
        const int pawn_rank = ::rank(from);
        const Square one_step = from + push_step;
        if (one_step == NO_SQUARE || (bit64(one_step) & check_info.occupied)) {
            continue;
        }

        Bitboard pinned_mask = BOARD_ALL_SET;
        if (bit64(from) & check_info.pinned_info) {
            pinned_mask = Complete_Line(from, check_info.own_king_sq);
        }
        const bool discovered_candidate =
            (bit64(from) & check_info.disc_check_info) != 0;
        const Bitboard discovery_line = discovered_candidate
            ? Complete_Line(from, check_info.op_king_sq)
            : 0;

        const auto append_push = [&](Square to, PieceType promotion) {
            const bool discovered_check = discovered_candidate
                && (bit64(to) & discovery_line) == 0;
            bool direct_check = false;
            if (promotion == PieceType::None) {
                direct_check =
                    (bit64(to) & check_info.pawn_checking_squares) != 0;
            }
            else {
                const Bitboard new_occupancy =
                    (check_info.occupied ^ bit64(from)) | bit64(to);
                direct_check =
                    (piece_attacks(to, check_info.own_color, new_occupancy,
                        promotion) & bit64(check_info.op_king_sq)) != 0;
            }

            if constexpr (!with_checks) {
                moves.push_back(Move(from, to, PieceType::Pawn,
                    check_info.own_color, PieceType::None, promotion,
                    false, false, direct_check, discovered_check));
            }
            else if (direct_check || discovered_check) {
                moves.push_back(Move(from, to, PieceType::Pawn,
                    check_info.own_color, PieceType::None, promotion,
                    false, false, direct_check, discovered_check));
            }
        };

        if (bit64(one_step) & pinned_mask & check_info.remedy_mask) {
            if (pawn_rank == promotion_rank) {
                append_push(one_step, PieceType::Queen);
                append_push(one_step, PieceType::Rook);
                append_push(one_step, PieceType::Bishop);
                append_push(one_step, PieceType::Knight);
            }
            else {
                append_push(one_step, PieceType::None);
            }
        }

        if (pawn_rank == start_rank) {
            const Square two_steps = from + 2 * push_step;
            if ((bit64(two_steps) & check_info.occupied) == 0
                && (bit64(two_steps) & pinned_mask & check_info.remedy_mask)) {
                append_push(two_steps, PieceType::None);
            }
        }
    }
}
template <bool captures_only, bool with_checks>
void MoveGenerator::generate_pawn_captures(MoveList& moves, const Position& pos, const CheckInfo& check_info) {
    Bitboard own_pawns = pos.get_pieces(check_info.own_color, PieceType::Pawn);
    const Square ep_square = pos.get_en_passant_rights();
    Bitboard ep_remedy = check_info.remedy_mask;
    if (ep_square != NO_SQUARE) {
        ep_remedy |= bit64(ep_square);
    }

    while (own_pawns) {
        const Square from = pop_lsb(own_pawns);
        const Bitboard attack_bb =
            PAWN_ATTACKS[to_int(check_info.own_color)][square_index(from)];
        Bitboard pinned_mask = BOARD_ALL_SET;
        if (bit64(from) & check_info.pinned_info) {
            pinned_mask = Complete_Line(from, check_info.own_king_sq);
        }

        const bool discovered_candidate =
            (bit64(from) & check_info.disc_check_info) != 0;
        const Bitboard discovery_line = discovered_candidate
            ? Complete_Line(from, check_info.op_king_sq)
            : 0;

        Bitboard captures = attack_bb & check_info.op_pieces
            & pinned_mask & check_info.remedy_mask;
        while (captures) {
            const Square to = pop_lsb(captures);
            const PieceType captured =
                pos.get_piece_type_on_square(check_info.op_color, to);
            const bool discovered_check = discovered_candidate
                && (bit64(to) & discovery_line) == 0;
            const bool promotion =
                (check_info.own_color == Color::White && square_index(to) >= 56)
                || (check_info.own_color == Color::Black && square_index(to) <= 7);

            const auto append_capture = [&](PieceType promotion_piece) {
                bool direct_check = false;
                if (promotion_piece == PieceType::None) {
                    direct_check =
                        (bit64(to) & check_info.pawn_checking_squares) != 0;
                }
                else {
                    const Bitboard new_occupancy =
                        (check_info.occupied ^ bit64(from)) | bit64(to);
                    direct_check =
                        (piece_attacks(to, check_info.own_color, new_occupancy,
                            promotion_piece) & bit64(check_info.op_king_sq)) != 0;
                }
                moves.push_back(Move(from, to, PieceType::Pawn,
                    check_info.own_color, captured, promotion_piece,
                    false, false, direct_check, discovered_check));
            };

            if (promotion) {
                append_capture(PieceType::Queen);
                append_capture(PieceType::Rook);
                append_capture(PieceType::Bishop);
                append_capture(PieceType::Knight);
            }
            else {
                append_capture(PieceType::None);
            }
        }

        if (ep_square == NO_SQUARE
            || (attack_bb & bit64(ep_square) & ep_remedy) == 0) {
            continue;
        }

        const Square capture_square = check_info.own_color == Color::White
            ? ep_square - 8
            : ep_square + 8;
        const Bitboard new_occupancy = check_info.occupied
            ^ bit64(from) ^ bit64(capture_square) ^ bit64(ep_square);
        const Bitboard opponent_pawns_after =
            pos.get_pieces(check_info.op_color, PieceType::Pawn)
            ^ bit64(capture_square);
        const Bitboard opponent_bishop_queen =
            pos.get_pieces(check_info.op_color, PieceType::Bishop)
            | pos.get_pieces(check_info.op_color, PieceType::Queen);
        const Bitboard opponent_rook_queen =
            pos.get_pieces(check_info.op_color, PieceType::Rook)
            | pos.get_pieces(check_info.op_color, PieceType::Queen);
        const bool own_king_attacked =
            (opponent_pawns_after
                & pawn_attacks(check_info.own_king_sq, check_info.own_color))
            || (pos.get_pieces(check_info.op_color, PieceType::Knight)
                & knight_attacks(check_info.own_king_sq))
            || (opponent_bishop_queen
                & bishop_attacks(check_info.own_king_sq, new_occupancy))
            || (opponent_rook_queen
                & rook_attacks(check_info.own_king_sq, new_occupancy))
            || (pos.get_pieces(check_info.op_color, PieceType::King)
                & king_attacks(check_info.own_king_sq));
        if (own_king_attacked) {
            continue;
        }

        const bool direct_check =
            (bit64(ep_square) & check_info.pawn_checking_squares) != 0;
        const Bitboard own_bishop_queen =
            pos.get_pieces(check_info.own_color, PieceType::Bishop)
            | pos.get_pieces(check_info.own_color, PieceType::Queen);
        const Bitboard own_rook_queen =
            pos.get_pieces(check_info.own_color, PieceType::Rook)
            | pos.get_pieces(check_info.own_color, PieceType::Queen);
        const bool discovered_check =
            ((own_bishop_queen
                & bishop_attacks(check_info.op_king_sq, new_occupancy))
            | (own_rook_queen
                & rook_attacks(check_info.op_king_sq, new_occupancy))) != 0;
        moves.push_back(Move(from, ep_square, PieceType::Pawn,
            check_info.own_color, PieceType::Pawn, PieceType::None,
            false, true, direct_check, discovered_check));
    }
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
