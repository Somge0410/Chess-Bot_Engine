#pragma once

#include <bit>

#include "Square.h"
#include "types.h"

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
