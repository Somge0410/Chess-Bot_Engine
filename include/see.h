#pragma once
#include "board.h"

static int see_capture(
    const Board& board,
    int from_sq,
    int to_sq,
    Color stm,
    PieceType movingPT,
    PieceType capturedPT
) {
    uint64_t piecesLocal[2][6];
    for (int c = 0; c < 2; ++c)
        for (int p = 0; p < 6; ++p)
            piecesLocal[c][p] = board.get_pieces(static_cast<Color>(c), static_cast<PieceType>(p));
    uint64_t occ = board.get_all_pieces();
    const int enemy = to_int(flip_color(stm));
    if(capturedPT!=PieceType::NONE){

        piecesLocal[enemy][to_int(capturedPT)] &= ~bit64(to_sq);
	}

    const int us = to_int(stm);
    piecesLocal[us][to_int(movingPT)] &= ~bit64(from_sq);
    occ &= ~bit64(from_sq);

    int gain[32];
    int d = 0;

    gain[0] = PIECE_VALUES[0][to_int(capturedPT)];

    int victimValue = PIECE_VALUES[0][to_int(movingPT)];

    Color side = flip_color(stm);

    while (true) {
        PieceType attPT;
        int attFrom;

        bool found = pick_least_attacker(
            to_sq, side, attFrom, attPT, occ, piecesLocal);
        if (!found) break;
        ++d;
        gain[d] = victimValue - gain[d - 1];
        piecesLocal[to_int(side)][to_int(attPT)] &= ~bit64(attFrom);
        occ &= ~bit64(attFrom);

        victimValue = PIECE_VALUES[0][to_int(attPT)];
        side = flip_color(side);

    }
    for (int i = d - 1; i >= 0; --i) {
        gain[i] = -std::max(-gain[i], gain[i + 1]);
    }
    return gain[0];
}
static int see_move(
    const Board& board,
    const Move& move
) {
    if (move.piece_moved == PieceType::KING) return 0;
    return see_capture(
        board,
        move.from_square,
        move.to_square,
        move.move_color,
        move.piece_moved,
        move.piece_captured
    );
}