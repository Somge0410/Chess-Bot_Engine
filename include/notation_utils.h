#pragma once
#include <string>
#include <vector>
#include "move.h"

std::string to_san(const Move & move,const MoveList& all_legal_moves);