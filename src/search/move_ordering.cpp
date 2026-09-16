#include "move_ordering.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "bitboard.h"
#include "bitboard_masks.h"
#include "search_parameters.h"
#include "see.h"

namespace {
constexpr int PIECE_VALUES_MG[7] = { 100, 320, 320, 500, 900, 10000, 0 };

void gravity_update(int& value, int bonus) {
    bonus = std::clamp(bonus, -HISTORY_MAX, HISTORY_MAX);
    value += bonus - value * std::abs(bonus) / HISTORY_MAX;
    value = std::clamp(value, -HISTORY_MAX, HISTORY_MAX);
}

uint64_t splitmix64(uint64_t& seed) {
    uint64_t value = (seed += 0x9E3779B97F4A7C15ULL);
    value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
    value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
    return value ^ (value >> 31);
}
}

static int score_move(const Move& move, int ply,const Move& tt_move,bool depth_0,const Position& pos, ThreadLocalData* tls, const Move& previous_move) {
   

    int stage = 0;
    int sub = 0;

    if (move == tt_move && !depth_0) { stage = TT_STAGE; sub = 0; }
    else if (move.promotion_piece != PieceType::None) {
        stage = PROMO_STAGE; sub = PIECE_VALUES_MG[piece_index(move.promotion_piece)] - PIECE_VALUES_MG[piece_index(move.piece_captured)];
    }
    else if (move.piece_captured != PieceType::None) {
        int see = see_move(pos, move);

        int attacker_val = PIECE_VALUES_MG[piece_index(move.piece_moved)];
        int victim_val = PIECE_VALUES_MG[piece_index(move.piece_captured)];
        int tiebreak = (victim_val - attacker_val) / CAPTURE_SCORE_TIEBREAK_DIVISOR;
        if(see>=0) { stage = MVV_LVA_STAGE; sub = see+tiebreak; }
		else { stage = LOSING_CAPTURE_STAGE; sub = see; }
    }
    else if(move == tls->killer_moves[ply][0] || move == tls->killer_moves[ply][1]) { stage = KILLER_STAGE; sub = 0; }
    else if (previous_move.from_square != NO_SQUARE &&
        move == tls->counter_moves[color_index(previous_move.move_color)]
        [piece_index(previous_move.piece_moved)]
        [previous_move.to_square]
        ) {
        stage = COUNTERMOVE_STAGE;
        sub = 0;
    }
    else {
        stage = QUIET_STAGE; sub = tls->history_scores[color_index(move.move_color)][piece_index(move.piece_moved)][move.to_square] +relevant_pawn_push(pos,move);
	}
	return stage * 100000 + sub;
}

void sort_moves(MoveList& moves,const Position& pos, int ply,const Move& tt_move,bool tt_depth_0, ThreadLocalData* tls, const Move& previous_move){
    std::vector<std::pair<int, Move>> scored;
    scored.reserve(moves.size());
    for (const auto& m : moves) scored.emplace_back(score_move(m, ply, tt_move,tt_depth_0,pos,tls,previous_move), m);

    std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) {
        if(a.first != b.first)
            return a.first > b.first;
		return a.second.get_int() < b.second.get_int();
        });

    for (size_t i = 0; i < moves.size(); ++i) moves[i]=scored[i].second;
}

static void add_history(ThreadLocalData* tls, const Move& move, int bonus) {
	int& h = tls->history_scores[color_index(move.move_color)][piece_index(move.piece_moved)][move.to_square];
    gravity_update(h, bonus);
}

void update_history_killer(const Move& move, int depth, int ply,ThreadLocalData* tls,const Move& previous_move,
    const MoveList& searched_quiets) {
    if (!tls) return;
	bool quiet = move.piece_captured == PieceType::None && move.promotion_piece == PieceType::None;
    if (quiet)
    {
        tls->killer_moves[ply][1] = tls->killer_moves[ply][0];
        tls->killer_moves[ply][0] = move;

        if(previous_move.from_square != NO_SQUARE) {
            tls->counter_moves[color_index(previous_move.move_color)]
                [piece_index(previous_move.piece_moved)]
                [previous_move.to_square] = move;
		}
    }
    int bonus = depth * depth*HISTORY_BONUS_MULTIPLIER;
    add_history(tls, move, bonus);
    for(int i=0;i<(int)searched_quiets.size();++i){
        const Move& m = searched_quiets[i];
		if (m == move) continue;
        add_history(tls, m, -bonus);
	}
}

void score_moves(const MoveList& moves, int* scores,
    int ply, const Move& tt_move, bool tt_depth_0,const Position& pos,ThreadLocalData* tls,
    const Move& previous_move, bool lazy_see) {
    for (int i = 0; i < (int)moves.size(); ++i) {
        const Move& move = moves[i];
        const bool is_tt_move = move == tt_move && !tt_depth_0;
        if (lazy_see && !is_tt_move && move.promotion_piece == PieceType::None &&
            move.piece_captured != PieceType::None) {
            const int attacker = PIECE_VALUES_MG[piece_index(move.piece_moved)];
            const int victim = PIECE_VALUES_MG[piece_index(move.piece_captured)];
            // The offset marks captures whose SEE has not been evaluated yet while
            // keeping them between promotions and killers in the staged ordering.
            scores[i] = MVV_LVA_STAGE * 100000 + 50000 +
                (victim - attacker) / CAPTURE_SCORE_TIEBREAK_DIVISOR;
        }
        else {
            scores[i] = score_move(move, ply, tt_move, tt_depth_0,pos, tls,previous_move);
        }
    }
}

void pick_next_staged(MoveList& moves, int* scores, int start, const Position& pos) {
    while (true) {
        int best = start;
        for (int i = start + 1; i < static_cast<int>(moves.size()); ++i) {
            if (scores[i] > scores[best]) best = i;
        }

        const Move& candidate = moves[best];
        const bool untested_capture = candidate.piece_captured != PieceType::None &&
            candidate.promotion_piece == PieceType::None &&
            scores[best] >= MVV_LVA_STAGE * 100000 + 40000 &&
            scores[best] < MVV_LVA_STAGE * 100000 + 60000;
        if (untested_capture) {
            const int see = see_move(pos, candidate);
            if (see < 0) {
                scores[best] = LOSING_CAPTURE_STAGE * 100000 + see;
                continue;
            }
            const int attacker = PIECE_VALUES_MG[piece_index(candidate.piece_moved)];
            const int victim = PIECE_VALUES_MG[piece_index(candidate.piece_captured)];
            scores[best] = MVV_LVA_STAGE * 100000 + see +
                (victim - attacker) / CAPTURE_SCORE_TIEBREAK_DIVISOR;
        }

        if (best != start) {
            std::swap(moves[best], moves[start]);
            std::swap(scores[best], scores[start]);
        }
        return;
    }
}

void score_qsearch_moves(const MoveList& moves, int* scores) {
    for (int i = 0; i < (int)moves.size(); ++i) {
        scores[i] = 0;
		const Move& m = moves[i];
        int victim = PIECE_VALUES_MG[piece_index(m.piece_captured)]/100;
        int attacker = PIECE_VALUES_MG[piece_index(m.piece_moved)]/100;
        scores[i] += victim - attacker;
    }
}

void pick_best(MoveList& moves, int* scores, int start) {
    int best = start;
    for (int i = start + 1; i < static_cast<int>(moves.size()); ++i) {
        if (scores[i] > scores[best]
            || (scores[i] == scores[best]
                && moves[i].get_int() < moves[best].get_int())) {
            best = i;
        }
    }
    if (best != start) {
        moves.swap_items(start, best);
        std::swap(scores[start], scores[best]);
    }
}

int relevant_pawn_push(const Position& pos, const Move& move) {
    if (move.piece_moved != PieceType::Pawn) return 0;
    int score = 0;
    Color us = pos.get_turn();

    int to = move.to_square;
	int rank = to / 8;
	int relative_rank = (us == Color::White) ? rank : 7 - rank;

    const Bitboard enemy_pawns =
        pos.get_pieces(flip_color(us), PieceType::Pawn);
    const bool passed =
        (enemy_pawns & PASSED_PAWN_MASK[color_index(us)][square_index(move.to_square)]) == 0;

    if (passed) {
        score += PAWN_PUSH_SCORE1;
        if (relative_rank >= 4) score += PAWN_PUSH_SCORE2;
        if (relative_rank >= 5) score += PAWN_PUSH_SCORE3;
        if (relative_rank >= 6) score += PAWN_PUSH_SCORE4;

        if (!pos.is_square_attacked(static_cast<Square>(to), flip_color(us))) {
            score += PAWN_PUSH_SCORE5;
        }
    }
    int king_square = pos.get_king_square(flip_color(us));
    if (KING_ZONE[king_square] & bit64(to)) {
        score += PAWN_PUSH_SCORE6;
    }
    return score;

}

bool is_dangerous_passer_push(const Position& pos, const Move& move) {
    if (move.piece_moved != PieceType::Pawn) return false;
    if (move.piece_captured != PieceType::None) return false;
    if (move.promotion_piece != PieceType::None) return true;

    const int relative_rank = move.move_color == Color::White
        ? square_index(move.to_square) / 8
        : 7 - square_index(move.to_square) / 8;
    if (relative_rank < 6) return false;

    const Bitboard enemy_pawns =
        pos.get_pieces(flip_color(move.move_color), PieceType::Pawn);
    return (enemy_pawns
        & PASSED_PAWN_MASK[color_index(move.move_color)][square_index(move.to_square)]) == 0;
}

void perturb_root_order(MoveList& moves, int thread_id, int depth, uint64_t hash, int thread_count) {
    if (thread_id == 0) return;
    if (moves.size() <= ROOT_PERTURBATION_MIN_HELPERS) return;

    int helpers = std::max(0, thread_count - 1);
    // Treat crossed SPSA endpoints as an unordered interval. This avoids an
    // invalid std::clamp range and makes the result independent of set order.
    const int min_band_size = std::min(ROOT_PERTURBATION_MIN_BAND_SIZE,
        ROOT_PERTURBATION_MAX_BAND_SIZE);
    const int max_band_size = std::max(ROOT_PERTURBATION_MIN_BAND_SIZE,
        ROOT_PERTURBATION_MAX_BAND_SIZE);
    int K = std::clamp(2 * helpers, min_band_size, max_band_size);
    int bandSize = std::min<int>(K, (int)moves.size() - 1);
    if (bandSize <= 1) return;

    //Mix position+thread+depth int a pseudo-random deterministic value
    uint64_t seed = hash
        ^ (uint64_t(thread_id) * 0xD1B54A32D192ED03ULL)
        ^ (uint64_t(depth) * 0x9E3779B97F4A7C15ULL);

    size_t shift = (size_t)(splitmix64(seed) % (uint64_t)bandSize);
    if (shift == 0) return;
    auto first = moves.begin() + 1;
    auto mid = first + shift;
    auto last = first + bandSize;

    std::rotate(first, mid, last);

    //// Keep the first move as-is (usually TT/PV), rotate the rest.
    //size_t shift = (size_t)((thread_id * 7 + depth * 3) % (moves.size() - 1));
    //if (shift == 0) return;
    //std::rotate(moves.begin() + 1, moves.begin() + 1 + shift, moves.end());

    }
