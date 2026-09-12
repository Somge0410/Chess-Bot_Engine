#pragma once
#include "board.h"
inline constexpr int PIECE_VALUES[7] = { 100,320,330,500,900,10000,0 };

static inline bool find_least_see_attacker(
    int to_sq,
    Color side,
    int& out_from_sq,
    PieceType& out_piece_type,
    uint64_t occupancy,
    const std::array<std::array<uint64_t, 6>, 2>& pieces
) {
    const int color = to_int(side);
    uint64_t attackers = get_pawn_attackers(
        to_sq, side, pieces[color][to_int(PieceType::PAWN)] & occupancy);
    if (attackers) {
        out_piece_type = PieceType::PAWN;
        out_from_sq = get_lsb(attackers);
        return true;
    }

    attackers = get_knight_attacks(to_sq)
        & pieces[color][to_int(PieceType::KNIGHT)] & occupancy;
    if (attackers) {
        out_piece_type = PieceType::KNIGHT;
        out_from_sq = get_lsb(attackers);
        return true;
    }

    attackers = get_bishop_attacks(to_sq, occupancy)
        & pieces[color][to_int(PieceType::BISHOP)] & occupancy;
    if (attackers) {
        out_piece_type = PieceType::BISHOP;
        out_from_sq = get_lsb(attackers);
        return true;
    }

    attackers = get_rook_attacks(to_sq, occupancy)
        & pieces[color][to_int(PieceType::ROOK)] & occupancy;
    if (attackers) {
        out_piece_type = PieceType::ROOK;
        out_from_sq = get_lsb(attackers);
        return true;
    }

    attackers = get_queen_attacks(to_sq, occupancy)
        & pieces[color][to_int(PieceType::QUEEN)] & occupancy;
    if (attackers) {
        out_piece_type = PieceType::QUEEN;
        out_from_sq = get_lsb(attackers);
        return true;
    }

    attackers = get_king_attacks(to_sq)
        & pieces[color][to_int(PieceType::KING)] & occupancy;
    if (attackers) {
        out_piece_type = PieceType::KING;
        out_from_sq = get_lsb(attackers);
        return true;
    }

    return false;
}

static int see_capture_impl(
    const Board& board,
    int from_sq,
    int to_sq,
    int capture_sq,
    Color stm,
    PieceType capturedPT,
    PieceType resultingPT
) {
    const auto& pieces = board.get_pieces_table();
    uint64_t occ = board.get_all_pieces();
    occ &= ~bit64(from_sq);
    if (capturedPT != PieceType::NONE) {
        occ &= ~bit64(capture_sq);
	}
    occ |= bit64(to_sq);

    int gain[32];
    int d = 0;

    gain[0] = PIECE_VALUES[to_int(capturedPT)];
    int victimValue = PIECE_VALUES[to_int(resultingPT)];

    Color side = flip_color(stm);

    while (d < 31) {
        PieceType attPT;
        int attFrom;

        bool found = find_least_see_attacker(
            to_sq, side, attFrom, attPT, occ, pieces);
        if (!found) break;
        ++d;
        gain[d] = victimValue - gain[d - 1];
        occ &= ~bit64(attFrom);

        victimValue = PIECE_VALUES[to_int(attPT)];
        side = flip_color(side);

    }
    for (int i = d - 1; i >= 0; --i) {
        gain[i] = -std::max(-gain[i], gain[i + 1]);
    }
    return gain[0];
}

static int see_capture(
    const Board& board,
    int from_sq,
    int to_sq,
    Color stm,
    PieceType movingPT,
    PieceType capturedPT
) {
    return see_capture_impl(
        board,
        from_sq,
        to_sq,
        to_sq,
        stm,
        capturedPT,
        movingPT
    );
}

static int see_move(
    const Board& board,
    const Move& move
) {
    if (move.piece_moved == PieceType::KING) return 0;
    return see_capture_impl(
        board,
        move.from_square,
        move.to_square,
        move.to_square,
        move.move_color,
        move.piece_captured,
        move.piece_moved
    );
}

static bool see_capture_ge(
    const Board& board,
    int from_sq,
    int to_sq,
    Color stm,
    PieceType movingPT,
    PieceType capturedPT,
    int threshold
) {
    const int captured_value = PIECE_VALUES[to_int(capturedPT)];
    if (captured_value < threshold) {
        return false;
    }
    if (PIECE_VALUES[to_int(movingPT)] <= captured_value - threshold) {
        return true;
    }
    return see_capture(board, from_sq, to_sq, stm, movingPT, capturedPT) >= threshold;
}

static bool see_move_ge(
    const Board& board,
    const Move& move,
    int threshold
) {
    if (move.piece_moved == PieceType::KING) {
        return threshold <= 0;
    }
    const int captured_value = PIECE_VALUES[to_int(move.piece_captured)];
    if (captured_value < threshold) {
        return false;
    }
    if (PIECE_VALUES[to_int(move.piece_moved)] <= captured_value - threshold) {
        return true;
    }
    return see_move(board, move) >= threshold;
}
