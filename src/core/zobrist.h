#pragma once
#include <cstdint>
#include "constants.h"

class Zobrist {
    public:
        static uint64_t piece_keys[2][6][64];
        static uint64_t black_to_move_key;
        static uint64_t castling_keys[16];
        static uint64_t en_passant_keys[8];

        static void initialize_keys();


    
};