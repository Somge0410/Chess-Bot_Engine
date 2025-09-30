#pragma once
#include "board.h"
struct KingSafetyScore {
    int mg_score;
    int eg_score;
};
int evaluate(const Board& board);