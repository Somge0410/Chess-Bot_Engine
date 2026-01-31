#pragma once

#include <cstdint>
#include <iostream>
#include <algorithm>
#include "constants.h"
#include "pst.h"
#include "Move.h"
#include "notation_utils.h"
#include "bishop_tables.h"
#include "rook_tables.h"
#if defined(_MSC_VER)
#include <intrin.h>
#include "attack_rays.h"
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

static inline uint64_t bit64(int sq) { return 1ULL << sq; }


static inline Color flip_color(Color color) {
    return (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
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
    return static_cast<int8_t>(piece_type);
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
    if (first_blocker_sq==-2)
    {
        first_blocker_sq=get_first_blocker_sq(ray,occupied_mask,forwards);
    }
    if (first_blocker_sq==-1) return -1;
    return get_first_blocker_sq(ray^(1ULL<<first_blocker_sq),occupied_mask,forwards);
    
}
inline Move parse_move(const std::string& move_str, MoveList& move_list){
    for (const Move& move : move_list)
    {
        if (move_str==to_san(move,move_list)) return move;
        
    }
    return Move();
    
}
inline uint64_t get_knight_attacks(int square) {
    return KNIGHT_ATTACKS[square];
}
inline uint64_t get_king_attacks(int square) {
    return KING_ATTACKS[square];
}
inline uint64_t get_bishop_attacks(int from_square, uint64_t occupied) {

    uint64_t bishop_blockers = BISHOP_BLOCKER_MASK[from_square] & occupied;
    uint64_t index = (bishop_blockers * MAGIC_BISHOP_NUMBER[from_square]) >> BISHOP_SHIFT_NUMBERS[from_square];
    return BISHOP_ATTACK_TABLE[BISHOP_ATTACK_OFFSET[from_square] + index];
}
inline uint64_t get_rook_attacks(int from_square, uint64_t occupied) {

    uint64_t rook_blockers = ROOK_BLOCKER_MASK[from_square] & occupied;
    uint64_t index = (rook_blockers * MAGIC_ROOK_NUMBER[from_square]) >> ROOK_SHIFT_NUMBERS[from_square];
    return ROOK_ATTACK_TABLE[ROOK_ATTACK_OFFSET[from_square] + index];
}
inline uint64_t get_queen_attacks(int from_square, uint64_t occupied) {
    return get_bishop_attacks(from_square, occupied) | get_rook_attacks(from_square, occupied);
}
inline uint64_t get_pawn_attacks(uint64_t pawns, Color color) {
    if (Color::WHITE == color) {
        return ((pawns & NOT_FILE_A) << 7) | ((pawns & NOT_FILE_H) << 9);
	}
    else {
		return ((pawns & NOT_FILE_H) >> 7) | ((pawns & NOT_FILE_A) >> 9);
    
    }
}
inline uint64_t get_pawn_attackers(int to_square, Color attacker_color, uint64_t attacker_pawns) {
    uint64_t bb = get_pawn_attacks(bit64(to_square), flip_color(attacker_color));
    return bb & attacker_pawns;
}
inline Move recover_move_from_int(uint16_t m_int) {
    if (m_int == 1u << 15) return Move();
    int from_square = m_int & 0x3F;
    int to_square = (m_int >> 6) & 0x3F;
    int promo_int = (m_int >> 12) & 0x0F;
    return Move(from_square, to_square, PieceType::NONE, Color::WHITE, PieceType::NONE, static_cast<PieceType>(promo_int));
}
static inline int pick_best(MoveList& moves, int* scores, int start) {
    int best = start;
    for(int i=start+1;i<(int)moves.size();++i){
        if (scores[i]>scores[best]){
            best=i;
        }
	}
    if(best!=start){
        std::swap(moves[best],moves[start]);
        std::swap(scores[best],scores[start]);
	}
	return start;
}
static inline int pick_best(MoveList& moves, int* scores, int start, int* see_scores) {
    int best = start;
    for (int i = start + 1; i < (int)moves.size(); ++i) {
        for (int i = start + 1; i < (int)moves.size(); ++i) {
            if (scores[i] > scores[best]) {
                best = i;
            }
        }
    }
        if (best != start) {
            std::swap(moves[best], moves[start]);
            std::swap(scores[best], scores[start]);
            std::swap(see_scores[best], see_scores[start]);
        }
        return start;
}
static inline uint64_t splitmix64(uint64_t& seed) {
    uint64_t z = (seed += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}
static inline bool pick_least_attacker(int tosq,Color side,int& outFromSq,PieceType& outPT, uint64_t occ, uint64_t piecesLocal[2][6]) {
    uint64_t bb = get_pawn_attackers(tosq, side, piecesLocal[to_int(side)][to_int(PieceType::PAWN)]);
    if (bb) { outPT = PieceType::PAWN; outFromSq = get_lsb(bb); return true; }
    bb = get_knight_attacks(tosq) & piecesLocal[to_int(side)][to_int(PieceType::KNIGHT)];
    if (bb) { outPT = PieceType::KNIGHT; outFromSq = get_lsb(bb); return true; }
    bb = get_bishop_attacks(tosq, occ) & piecesLocal[to_int(side)][to_int(PieceType::BISHOP)];
    if (bb) { outPT = PieceType::BISHOP; outFromSq = get_lsb(bb); return true; }
    bb = get_rook_attacks(tosq, occ) & piecesLocal[to_int(side)][to_int(PieceType::ROOK)];
    if (bb) { outPT = PieceType::ROOK; outFromSq = get_lsb(bb); return true; }
    bb = get_queen_attacks(tosq, occ) & piecesLocal[to_int(side)][to_int(PieceType::QUEEN)];
    if (bb) { outPT = PieceType::QUEEN; outFromSq = get_lsb(bb); return true; }
    bb = get_king_attacks(tosq) & piecesLocal[to_int(side)][to_int(PieceType::KING)];
    if (bb) { outPT = PieceType::KING; outFromSq = get_lsb(bb); return true; }

    return false;
}
