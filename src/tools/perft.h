#pragma once
#include <string_view>
#include <vector>
#include <cstdint>
#include "fen.h"
#include "Default_positions.h"


// 64 is the maximum depth perft will ever reach
MoveList perft_lists[64];

// Add a 'ply' parameter to track how deep in the tree we are
static uint64_t perft(Board& board, int depth, int ply = 0);

static void run_perft(const Board& root_board, int depth);