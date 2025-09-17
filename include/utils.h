#pragma once

#include <cstdint>
#include <iostream>
#include <algorithm>
#include "constants.h"
#include "pst.h"
#include "Move.h"
#include "notation_utils.h"
#if defined(_MSC_VER)
#include <intrin.h>
#endif

inline int get_lsb(uint64_t bitboard) {
    if (bitboard == 0) return -1;
#if defined(_MSC_VER)
    unsigned long index;
    _BitScanForward64(&index, bitboard);
    return index;
#else // For GCC/Clang
    return __builtin_ctzll(bitboard);
#endif
}

inline int get_msb(uint64_t bitboard) {
    if (bitboard == 0) return -1;
#if defined(_MSC_VER)
    unsigned long index;
    _BitScanReverse64(&index, bitboard);
    return index;
#else // For GCC/Clang
    return 63 - __builtin_clzll(bitboard);
#endif
}
    
inline void display_bitboard(uint64_t bitboard){
    std::cout <<"--------------------"<< std::endl;
    for (int rank=7; rank>=0;--rank){
        for (int file=0;file<8;++file){
            int square_index=rank*8+file;
            if ((bitboard>>square_index)& 1){
                std::cout<<"1 ";
            }else{
                std::cout <<". ";
            }
        }
        std::cout<< "  "<< rank+1<< std::endl;
    }
    std::cout << "\na b c d e f g h" << std::endl;
    std::cout << "--------------------" << std::endl;
}

inline int popcount(uint64_t bitboard){
#ifdef _MSC_VER
    return __popcnt64(bitboard);
#else 
    return __builtin_popcountll(bitboard);
#endif // _MSC_VER
}

inline int to_int(Color color){
    return static_cast<int>(color);
}

inline int to_int(PieceType piece_type){
    return static_cast<int>(piece_type);
}

inline void remove_castling_right(std::string& rights, char right_to_remove){
    rights.erase(
        std::remove(rights.begin(),rights.end(),right_to_remove),
        rights.end()
    );
}

inline char get_piece_char(const PieceType& piece,const Color& color){
    return color==Color::WHITE ? PIECE_CHAR_LIST[to_int(piece)]:tolower(PIECE_CHAR_LIST[to_int(piece)]);
}

inline int get_piece_values(const Color& color, const PieceType& piece){
    return PIECE_VALUES[to_int(color)][to_int(piece)];
}
inline int get_mg_pos_score(const Color& color, const PieceType& piece,const int& square){
    return MG_PST[to_int(color)][to_int(piece)][square];
}
inline int get_eg_pos_score(const Color& color, const PieceType& piece,const int& square){
    return EG_PST[to_int(color)][to_int(piece)][square];
}
inline int get_first_blocker_sq(const uint64_t& ray, const uint64_t& occupied_mask,bool forwards=true){
    uint64_t blocker=ray & occupied_mask;
    return forwards ? get_lsb(blocker):get_msb(blocker);
}
inline int get_second_blocker_sq(const uint64_t& ray, const uint64_t& occupied_mask,bool forwards=true,int first_blocker_sq=-2){
    if (first_blocker_sq=-2)
    {
        first_blocker_sq=get_first_blocker_sq(ray,occupied_mask,forwards);
    }
    if (first_blocker_sq==-1) return -1;
    return get_first_blocker_sq(ray^(1ULL<<first_blocker_sq),occupied_mask,forwards);
    
}
inline Move parse_move(const std::string& move_str, std::vector<Move> move_list){
    for (const Move& move : move_list)
    {
        if (move_str==to_san(move,move_list)) return move;
        
    }
    return Move();
    
}