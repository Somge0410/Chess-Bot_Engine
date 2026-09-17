#pragma once

#include <cstdint>
#include <iostream>
#include <algorithm>
#include "constants.h"
#include "Move.h"
#include "notation_utils.h"
#include "bishop_tables.h"
#include "rook_tables.h"
#include "attack_rays.h"
#include "bitboard_masks.h"
#include "eval_params.h"
#if defined(_MSC_VER)
#include <intrin.h>
#endif





inline int get_mg_pos_score(const Color& color, const PieceType& piece, const int& square) {
    if (color == Color::White) {
        if (piece == PieceType::Pawn) return EvalWeights[PAWN_PST_START + square].mg_score;
        else if (piece == PieceType::Knight) return EvalWeights[KNIGHT_PST_START + square].mg_score;
        else if (piece == PieceType::Bishop) return EvalWeights[BISHOP_PST_START + square].mg_score;
        else if (piece == PieceType::Rook) return EvalWeights[ROOK_PST_START + square].mg_score;
        else if (piece == PieceType::Queen) return EvalWeights[QUEEN_PST_START + square].mg_score;
        else if (piece == PieceType::King) return EvalWeights[KING_PST_START + square].mg_score;
        else return 0;
    }
    else {
        if (piece == PieceType::Pawn) return -EvalWeights[PAWN_PST_START + flip_square(square)].mg_score;
        else if (piece == PieceType::Knight) return -EvalWeights[KNIGHT_PST_START + flip_square(square)].mg_score;
        else if (piece == PieceType::Bishop) return -EvalWeights[BISHOP_PST_START + flip_square(square)].mg_score;
        else if (piece == PieceType::Rook) return -EvalWeights[ROOK_PST_START + flip_square(square)].mg_score;
        else if (piece == PieceType::Queen) return -EvalWeights[QUEEN_PST_START + flip_square(square)].mg_score;
        else if (piece == PieceType::King) return -EvalWeights[KING_PST_START + flip_square(square)].mg_score;
        else return 0;
    }
}
inline int get_eg_pos_score(const Color& color, const PieceType& piece, const int& square) {
    if (color == Color::White) {
        if (piece == PieceType::Pawn) return EvalWeights[PAWN_PST_START + square].eg_score;
        else if (piece == PieceType::Knight) return EvalWeights[KNIGHT_PST_START + square].eg_score;
        else if (piece == PieceType::Bishop) return EvalWeights[BISHOP_PST_START + square].eg_score;
        else if (piece == PieceType::Rook) return EvalWeights[ROOK_PST_START + square].eg_score;
        else if (piece == PieceType::Queen) return EvalWeights[QUEEN_PST_START + square].eg_score;
        else if (piece == PieceType::King) return EvalWeights[KING_PST_START + square].eg_score;
        else return 0;
    }
    else {
        if (piece == PieceType::Pawn) return -EvalWeights[PAWN_PST_START + flip_square(square)].eg_score;
        else if (piece == PieceType::Knight) return -EvalWeights[KNIGHT_PST_START + flip_square(square)].eg_score;
        else if (piece == PieceType::Bishop) return -EvalWeights[BISHOP_PST_START + flip_square(square)].eg_score;
        else if (piece == PieceType::Rook) return -EvalWeights[ROOK_PST_START + flip_square(square)].eg_score;
        else if (piece == PieceType::Queen) return -EvalWeights[QUEEN_PST_START + flip_square(square)].eg_score;
        else if (piece == PieceType::King) return -EvalWeights[KING_PST_START + flip_square(square)].eg_score;
        else return 0;
    }
}
inline Move parse_move(const std::string& move_str, MoveList& move_list) {
    for (const Move& move : move_list)
    {
        if (move_str == to_san(move, move_list)) return move;

    }
    return Move();

}
inline Move recover_move_from_int(uint16_t m_int) {
    if (m_int == 1u << 15) return Move();
    int from_square = m_int & 0x3F;
    int to_square = (m_int >> 6) & 0x3F;
    int promo_int = (m_int >> 12) & 0x0F;
    return Move(from_square, to_square, PieceType::None, Color::White, PieceType::None, static_cast<PieceType>(promo_int));
}
static inline int pick_best(MoveList& moves, int* scores, int start) {
    int best = start;
    for (int i = start + 1; i < (int)moves.size(); ++i) {
        if (scores[i] > scores[best]) {
            best = i;
        }
    }
    if (best != start) {
        std::swap(moves[best], moves[start]);
        std::swap(scores[best], scores[start]);
    }
    return start;
}
static inline int pick_best(MoveList& moves, int* scores, int start, int* see_scores) {
    int best = start;
    for (int i = start + 1; i < (int)moves.size(); ++i) {
        for (int i = start + 1; i < (int)moves.size(); ++i) {
            if (scores[i] > scores[best]) {
                best = i;
            }
        }
    }
    if (best != start) {
        std::swap(moves[best], moves[start]);
        std::swap(scores[best], scores[start]);
        std::swap(see_scores[best], see_scores[start]);
    }
    return start;
}
static inline uint64_t splitmix64(uint64_t& seed) {
    uint64_t z = (seed += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}
static inline bool pick_least_attacker(int tosq, Color side, int& outFromSq, PieceType& outPT, uint64_t occ, uint64_t piecesLocal[2][6]) {
    uint64_t bb = get_pawn_attackers(tosq, side, piecesLocal[to_int(side)][to_int(PieceType::Pawn)]);
    if (bb) { outPT = PieceType::Pawn; outFromSq = get_lsb(bb); return true; }
    bb = get_knight_attacks(tosq) & piecesLocal[to_int(side)][to_int(PieceType::Knight)];
    if (bb) { outPT = PieceType::Knight; outFromSq = get_lsb(bb); return true; }
    bb = get_bishop_attacks(tosq, occ) & piecesLocal[to_int(side)][to_int(PieceType::Bishop)];
    if (bb) { outPT = PieceType::Bishop; outFromSq = get_lsb(bb); return true; }
    bb = get_rook_attacks(tosq, occ) & piecesLocal[to_int(side)][to_int(PieceType::Rook)];
    if (bb) { outPT = PieceType::Rook; outFromSq = get_lsb(bb); return true; }
    bb = get_queen_attacks(tosq, occ) & piecesLocal[to_int(side)][to_int(PieceType::Queen)];
    if (bb) { outPT = PieceType::Queen; outFromSq = get_lsb(bb); return true; }
    bb = get_king_attacks(tosq) & piecesLocal[to_int(side)][to_int(PieceType::King)];
    if (bb) { outPT = PieceType::King; outFromSq = get_lsb(bb); return true; }

    return false;
}
inline bool has_castling_rights(Color color, uint8_t castle_rights) {
    if (color == Color::White) {
        return (castle_rights & (WHITE_KING_CASTLE | WHITE_QUEEN_CASTLE)) != 0;
    }
    if (color == Color::Black) {
        return (castle_rights & (BLACK_KING_CASTLE | BLACK_QUEEN_CASTLE)) != 0;
    }
    return false;
}
inline bool has_castling_rights(int color, uint8_t castle_rights) {
    if (color == to_int(Color::White)) {
        return (castle_rights & (WHITE_KING_CASTLE | WHITE_QUEEN_CASTLE)) != 0;
    }
    if (color == to_int(Color::Black)) {
        return (castle_rights & (BLACK_KING_CASTLE | BLACK_QUEEN_CASTLE)) != 0;
    }
    return false;
}
inline int king_distance(int sq1, int sq2) {
    int file1 = sq1 % 8;
    int rank1 = sq1 / 8;
    int file2 = sq2 % 8;
    int rank2 = sq2 / 8;
    return std::max(std::abs(file1 - file2), std::abs(rank1 - rank2));
}
inline int rank(int square) {
    return square / 8;
}
inline int file(int square) {
    return square % 8;
}
inline int flip_rank(int square) {
    return 7 - rank(square);
}
inline bool is_on_center_files(int king_square) {
    uint64_t center_file_mask = FILE_MASK[3] | FILE_MASK[4] | FILE_MASK[5];
    return (bit64(king_square) & center_file_mask) != 0;
}
inline void gravity_update(int& h, int bonus) {
    bonus = std::clamp(bonus, -HISTORY_MAX, HISTORY_MAX);
    h += bonus - h * std::abs(bonus) / HISTORY_MAX;
    h = std::clamp(h, -HISTORY_MAX, HISTORY_MAX);
}
inline int get_forward_square(int square, Color color) {
    if (color == Color::White) {
        return square + 8;
    }
    else {
        return square - 8;
    }
}
inline bool is_occupied(int square, uint64_t occupied) {
    return (occupied & bit64(square)) != 0;
}
inline int get_promotion_square(int square, Color color) {
    if (color == Color::White) {
        return square + 8 * (7 - rank(square));
    }
    else {
        return square - 8 * rank(square);
    }
}
constexpr uint8_t PASSED_PAWN_BUCKET[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 2, 2, 3, 3,
    0, 0, 1, 1, 2, 2, 3, 3,
    4, 4, 5, 5, 6, 6, 7, 7,
    4, 4, 5, 5, 6, 6, 7, 7,
    8, 8, 9, 9, 10, 10, 11, 11,
    12, 12, 13, 13, 14, 14, 15, 15,
    0,0,0,0,0,0,0,0
};
constexpr uint8_t ISOLATED_PAWN_BUCKET[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 2, 3, 3, 2, 1, 0,
    0, 1, 2, 3, 3, 2, 1, 0,
    4, 5, 6, 7, 7, 6, 5, 4,
    8, 9, 10, 11, 11, 10, 9, 8,
    12, 13, 14, 15, 15, 14, 13, 12,
    12, 13 ,14 ,15 ,15, 14 ,13 ,12 ,
    0 ,0 ,0 ,0 ,0 ,0 ,0 ,0
};