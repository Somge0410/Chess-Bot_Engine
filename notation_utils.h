#pragma once
#include <string>
#include <vector>
#include "move.h"

std::string to_san(const Move & move,const std::vector<Move>& all_legal_moves);