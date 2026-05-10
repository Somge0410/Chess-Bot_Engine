#include <iostream>
#include "board.h"
#include "utils.h"    
#include "constants.h" 
#include "zobrist.h"
#include "attack_rays.h"
#include "bitboard_masks.h"
#include "MoveGenerator.h"
#include "notation_utils.h"
#include "evaluation.h"
#include "engine.h"
#include "rook_tables.h"
#include "bishop_tables.h"
#include <chrono>
#include "prepare_data.h"
#include <vector>
#include "uci.h"
#include "see.h"
#include "Squares.h"

#include <iostream>
int main(int argc, char* argv[]) {
    Zobrist::initialize_keys();
  

        std::ios::sync_with_stdio(false);
        std::cin.tie(nullptr);
        uci_loop();
        return 0;
}