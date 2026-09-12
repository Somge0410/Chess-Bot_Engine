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