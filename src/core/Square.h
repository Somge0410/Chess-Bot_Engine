#pragma once
#include <algorithm>
#include <cstdlib>

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
constexpr inline bool is_single_push_area(Square square, Color color) noexcept {
    int rank = get_rank(square);
    return (color == Color::White && rank < 6) || (color == Color::Black && rank >1);
}
inline int king_distance(int sq1, int sq2) {
    int file1 = sq1 % 8;
    int rank1 = sq1 / 8;
    int file2 = sq2 % 8;
    int rank2 = sq2 / 8;
    return std::max(std::abs(file1 - file2), std::abs(rank1 - rank2));
}

inline int rank(int square) {
    return square / 8;
}

inline int file(int square) {
    return square % 8;
}

inline int flip_rank(int square) {
    return 7 - rank(square);
}

inline int get_forward_square(int square, Color color) {
    if (color == Color::White) {
        return square + 8;
    }
    else {
        return square - 8;
    }
}

inline int get_promotion_square(int square, Color color) {
    if (color == Color::White) {
        return square + 8 * (7 - rank(square));
    }
    else {
        return square - 8 * rank(square);
    }
}

constexpr Bitboard NOT_FILE_A = 0xfefefefefefefefe;
constexpr Bitboard NOT_FILE_H = 0x7f7f7f7f7f7f7f7f;