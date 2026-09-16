#pragma once

#include <cstdint>

#include "position.h"
#include "search.h"

inline constexpr int HISTORY_MAX = 16384;

void sort_moves(MoveList& moves, const Position& pos, int ply, const Move& tt_move,
    bool tt_depth_0 = false, ThreadLocalData* tls = nullptr,
    const Move& previous_move = Move());
void score_moves(const MoveList& moves, int* scores, int ply, const Move& tt_move,
    bool depth_0, const Position& pos, ThreadLocalData* tls,
    const Move& previous_move, bool lazy_see = false);
void score_qsearch_moves(const MoveList& moves, int* scores);
void pick_best(MoveList& moves, int* scores, int start);
void pick_next_staged(MoveList& moves, int* scores, int start, const Position& pos);
int relevant_pawn_push(const Position& pos, const Move& move);
bool is_dangerous_passer_push(const Position& pos, const Move& move);
void perturb_root_order(MoveList& moves, int thread_id, int current_depth,
    uint64_t hash, int thread_count);
void update_history_killer(const Move& move, int depth, int ply,
    ThreadLocalData* tls, const Move& previous_move = Move(),
    const MoveList& searched_quiets = MoveList());
