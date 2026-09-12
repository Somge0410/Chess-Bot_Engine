#include "zobrist.h"
#include <random>

uint64_t Zobrist::piece_keys[2][6][64];
uint64_t Zobrist::black_to_move_key;
uint64_t Zobrist::castling_keys[16];
uint64_t Zobrist::en_passant_keys[8];

void Zobrist:: initialize_keys(){
    std::mt19937_64 gen(123456789);
    std::uniform_int_distribution<uint64_t> dist;

    for (int color=0; color<2; ++color){
        for (int piece=0; piece<6;++piece){
            for (int square=0; square<64;++square){
                piece_keys[color][piece][square]=dist(gen);
            }
        }
    }

    black_to_move_key =dist(gen);

    for (int i=0; i<16; ++i){
        castling_keys[i]=dist(gen);
    }
    for (int i=0;i<8;++i){
        en_passant_keys[i]=dist(gen);
    }

}