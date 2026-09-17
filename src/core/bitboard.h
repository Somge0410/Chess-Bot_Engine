#pragma once

#include <bit>
#include <cstdint>
#include <iostream>

#include "Square.h"
#include "types.h"

#if defined(_MSC_VER)
#include <intrin.h>
#endif

inline constexpr Bitboard BOARD_ALL_SET = 0xFFFFFFFFFFFFFFFFULL;

inline int get_lsb(uint64_t bitboard) {
    if (bitboard == 0) return NO_SQUARE;
#if defined(_MSC_VER)
    unsigned long index;
    _BitScanForward64(&index, bitboard);
    return index;
#else // For GCC/Clang
    return __builtin_ctzll(bitboard);
#endif
}

inline int get_msb(uint64_t bitboard) {
    if (bitboard == 0) return NO_SQUARE;
#if defined(_MSC_VER)
    unsigned long index;
    _BitScanReverse64(&index, bitboard);
    return index;
#else // For GCC/Clang
    return 63 - __builtin_clzll(bitboard);
#endif
}

inline int poplsb(uint64_t& bitboard) {
    int lsb_index = get_lsb(bitboard);
    if (lsb_index != NO_SQUARE) {
        bitboard &= bitboard - 1; // Clear the least significant bit
    }
    return lsb_index;
}

inline void display_bitboard(uint64_t bitboard) {
    std::cout << std::unitbuf << "\n"
        << "--------------------" << std::endl;
    for (int rank = 7; rank >= 0; --rank) {
        for (int file = 0; file < 8; ++file) {
            int square_index = rank * 8 + file;
            if ((bitboard >> square_index) & 1) {
                std::cout << "1 ";
            }
            else {
                std::cout << ". ";
            }
        }
        std::cout << "  " << rank + 1 << std::endl;
    }
    std::cout << "\na b c d e f g h" << std::endl;
    std::cout << "--------------------" << std::endl;
}

constexpr uint8_t bit8(int square) {
    return static_cast<uint8_t>(1U << square);
}
constexpr int popcount(Bitboard board) noexcept{
	return std::popcount(board);
}

constexpr Square lsb(Bitboard board) noexcept{
    return to_square(std::countr_zero(board));
}
constexpr Square msb(Bitboard board) noexcept{
    return board ==0 ? NO_SQUARE : to_square(63 - std::countl_zero(board));
}
constexpr Square pop_lsb(Bitboard& board) noexcept{
    const Square square = lsb(board);
	board &= board - 1; // Clear the least significant bit
    return square;
}

constexpr Bitboard bit64(Square square) noexcept{
    return Bitboard{ 1 } << square_index(square);
}
constexpr Bitboard bit64(int square) noexcept {
    return Bitboard{ 1 } << square;
}
constexpr bool contains(Bitboard bitboard, Square square) noexcept {
    return (bitboard & bit64(square)) != 0;
}
constexpr bool has_multiple_bits(Bitboard bitboard) noexcept{
    return (bitboard & (bitboard - 1)) != 0;
}

inline bool is_occupied(Square square, Bitboard occupied) {
    return (occupied & bit64(square)) != 0;
}

inline void display_bitboard(Bitboard bitboard) {
    std::cout << std::unitbuf << "\n"
        << "--------------------" << std::endl;
    for (int rank = 7; rank >= 0; --rank) {
        for (int file = 0; file < 8; ++file) {
            int square_index = rank * 8 + file;
            if ((bitboard >> square_index) & 1) {
                std::cout << "1 ";
            }
            else {
                std::cout << ". ";
            }
        }
        std::cout << "  " << rank + 1 << std::endl;
    }
    std::cout << "\na b c d e f g h" << std::endl;
    std::cout << "--------------------" << std::endl;
}
