#pragma once
#include <array>
#include <string>
#include "constants.h"



static const std::array<uint64_t, 64> SQUARE = {0x1ULL, 0x2ULL, 0x4ULL, 0x8ULL, 0x10ULL, 0x20ULL, 0x40ULL, 0x80ULL,
0x100ULL, 0x200ULL, 0x400ULL, 0x800ULL, 0x1000ULL, 0x2000ULL, 0x4000ULL, 0x8000ULL,
0x10000ULL, 0x20000ULL, 0x40000ULL, 0x80000ULL, 0x100000ULL, 0x200000ULL, 0x400000ULL, 0x800000ULL,
0x1000000ULL, 0x2000000ULL, 0x4000000ULL, 0x8000000ULL, 0x10000000ULL, 0x20000000ULL, 0x40000000ULL, 0x80000000ULL,
0x100000000ULL, 0x200000000ULL, 0x400000000ULL, 0x800000000ULL, 0x1000000000ULL, 0x2000000000ULL, 0x4000000000ULL, 0x8000000000ULL,
0x10000000000ULL, 0x20000000000ULL, 0x40000000000ULL, 0x80000000000ULL, 0x100000000000ULL, 0x200000000000ULL, 0x400000000000ULL, 0x800000000000ULL,
0x1000000000000ULL, 0x2000000000000ULL, 0x4000000000000ULL, 0x8000000000000ULL, 0x10000000000000ULL, 0x20000000000000ULL, 0x40000000000000ULL, 0x80000000000000ULL,
0x100000000000000ULL, 0x200000000000000ULL, 0x400000000000000ULL, 0x800000000000000ULL, 0x1000000000000000ULL, 0x2000000000000000ULL, 0x4000000000000000ULL, 0x8000000000000000ULL};
static const std::array<uint64_t, 8> FILES = { 0x101010101010101ULL, 0x202020202020202ULL, 0x404040404040404ULL, 0x808080808080808ULL, 0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL };
static const std::array<uint64_t, 8> RANK = { 0xFFULL, 0xFF00ULL, 0xFF0000ULL, 0xFF000000ULL, 0xFF00000000ULL, 0xFF0000000000ULL, 0xFF000000000000ULL, 0xFF00000000000000ULL };
static const std::array<uint64_t, 15> DIADONAL1 = { 0x8040201008040201ULL,0x80402010080402ULL,0x804020100804ULL,0x8040201008ULL,0x80402010ULL,0x804020ULL,0x8040ULL,0x80ULL,0x4020100804020100ULL,0x2010080402010000ULL,0x1008040201000000ULL,0x804020100000000ULL,0x402010000000000ULL,0x201000000000000ULL,0x100000000000000ULL};
static const std::array<uint64_t, 15> DIADONAL2 = { 0x1ULL,0x102ULL,0x10204ULL,0x1020408ULL,0x102040810ULL,0x10204081020ULL,0x1020408102040ULL,0x102040810204080ULL,0x204081020408000ULL,0x408102040800000ULL,0x810204080000000ULL,0x1020408000000000ULL,0x2040800000000000ULL,0x4080000000000000ULL,0x8000000000000000ULL};
static inline int get_square_number(const std::string& square_str) {
	if (square_str.length() != 2) return NO_SQUARE;
	char file_char = square_str[0];
	char rank_char = square_str[1];
	if (file_char < 'a' || file_char > 'h' || rank_char < '1' || rank_char > '8') return NO_SQUARE;
	int file = file_char - 'a';
	int rank = rank_char - '1';
	return rank * 8 + file;
}
static inline int get_file_number(const std::string& square_str) {
	if (square_str.length() != 2) return NO_SQUARE;
	char file_char = square_str[0];
	if (file_char < 'a' || file_char > 'h') return NO_SQUARE;
	return file_char - 'a';
}
static inline int get_rank_number(const std::string& square_str) {
	if (square_str.length() != 2) return NO_SQUARE;
	char rank_char = square_str[1];
	if (rank_char < '1' || rank_char > '8') return NO_SQUARE;
	return rank_char - '1';
}
static inline uint64_t get_square_bitboard(const std::string& square_str) {
	int square_num = get_square_number(square_str);
	return SQUARE[square_num];
}
static inline uint64_t find_file(int square1, int square2) {
	if (square1 < 0 || square1 >= 64 || square2 < 0 || square2 >= 64) return 0;
	int file1 = square1 % 8;
	int file2 = square2 % 8;
	if (file1 != file2) return 0;
	return FILES[file1];
}
static inline uint64_t find_rank(int square1, int square2) {
	if (square1 < 0 || square1 >= 64 || square2 < 0 || square2 >= 64) return 0;
	int rank1 = square1 / 8;
	int rank2 = square2 / 8;
	if (rank1 != rank2) return 0;
	return RANK[rank1];
}
static inline uint64_t find_diagonal1(int square1, int square2) {
	for(int i=0; i<15; i++) {
		if ((DIADONAL1[i] & SQUARE[square1]) && (DIADONAL1[i] & SQUARE[square2])) {
			return DIADONAL1[i];
		}
	}
	return 0;
}
static inline uint64_t find_diagonal2(int square1, int square2) {
	for(int i=0;i<15; i++) {
		if ((DIADONAL2[i] & SQUARE[square1]) && (DIADONAL2[i] & SQUARE[square2])) {
			return DIADONAL2[i];
		}
	}
	return 0;
}