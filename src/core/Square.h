#pragma once
#include "types.h"

constexpr int square_index(Square square) noexcept {
	return static_cast<int>(square);
}

constexpr Square to_square(int index) noexcept {
	return static_cast<Square>(index);
}

constexpr Square to_square(int file, int rank) noexcept {
	return static_cast<Square>(rank * 8 + file);
}

constexpr int get_file(Square square) noexcept {
	return square_index(square) % 8;
}

constexpr int get_rank(Square square) noexcept {
	return square_index(square) / 8;
}

constexpr bool is_valid_square(Square square) noexcept {
	return square_index(square)<64;
}
constexpr int flip_square(int square) noexcept {
	return square ^ 56;
}
constexpr Square flip_square(Square square) noexcept {
	return to_square(flip_square(square_index(square)));
}
constexpr inline Square operator+(Square square, int offset) noexcept {
	return to_square(square_index(square) + offset);
}
constexpr inline Square operator-(Square square, int offset) noexcept {
	return to_square(square_index(square) - offset);
}

constexpr Bitboard NOT_FILE_A = 0xfefefefefefefefe;
constexpr Bitboard NOT_FILE_H = 0x7f7f7f7f7f7f7f7f;