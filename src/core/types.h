#pragma once

#include <array>
#include <cstdint>

using Bitboard = std::uint64_t;

enum class Color : std::uint8_t {
	White,
	Black,
	None,
};
constexpr Color flip_color(Color color) {
	return (color == Color::White) ? Color::Black : Color::White;
}
enum class PieceType : std::uint8_t {
	Pawn,
	Knight,
	Bishop,
	Rook,
	Queen,
	King,
	None,
};
enum CastlingRights : std::uint8_t {
	None = 0,
	BlackQueenside = 1U << 0,
	BlackKingside = 1U << 1,
	WhiteQueenside = 1U << 2,
	WhiteKingside = 1U << 3
};

constexpr CastlingRights operator|(CastlingRights a, CastlingRights b) {
	return static_cast<CastlingRights>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}
constexpr CastlingRights& operator|=(CastlingRights& a, CastlingRights b) {
	a = a | b;
	return a;
}
constexpr CastlingRights operator&(CastlingRights a, CastlingRights b) {
	return static_cast<CastlingRights>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
}
constexpr CastlingRights& operator&=(CastlingRights& a, CastlingRights b) {
	a = a & b;
	return a;
}
constexpr bool has_castling_right(CastlingRights rights, CastlingRights check) {
	return (static_cast<std::uint8_t>(rights) & static_cast<std::uint8_t>(check)) != 0;
}
constexpr CastlingRights operator~(CastlingRights a) {
	return static_cast<CastlingRights>(~static_cast<std::uint8_t>(a));
}
constexpr CastlingRights operator-(CastlingRights a, CastlingRights b) {
	return static_cast<CastlingRights>(static_cast<std::uint8_t>(a) & ~static_cast<std::uint8_t>(b));
}
enum Square : std::uint8_t {
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8,
	NO_SQUARE
};
using EnPassantRights = Square;

constexpr int piece_index(PieceType pt) {
	return static_cast<int>(pt);
}
constexpr int color_index(Color color) {
	return static_cast<int>(color);
}
constexpr int to_int(Color color) {
	return color_index(color);
}
constexpr int to_int(PieceType piece_type) {
	return piece_index(piece_type);
}

struct PieceBoards {
	std::array<std::array<Bitboard, 6>, 2> data{}; // [color][piece_type]
	constexpr Bitboard& operator()(Color color, PieceType piece_type) {
		return data[color_index(color)][piece_index(piece_type)];
	}
	constexpr const Bitboard& operator()(Color color, PieceType piece_type) const {
		return data[color_index(color)][piece_index(piece_type)];
	}
};
struct ColorBoards {
	std::array<Bitboard, 2> data{}; // [color]
	constexpr Bitboard& operator()(Color color) {
		return data[color_index(color)];
	}
	constexpr const Bitboard& operator()(Color color) const {
		return data[color_index(color)];
	}
};
struct KingSquares {
	std::array<Square, 2> data{}; // [color]
	constexpr Square& operator()(Color color) {
		return data[color_index(color)];
	}
	constexpr const Square& operator()(Color color) const {
		return data[color_index(color)];
	}
};
struct EvaluationResult {
	int16_t mg_score;
	int16_t eg_score;
	EvaluationResult& operator+=(const EvaluationResult& other) {
		this->mg_score += other.mg_score;
		this->eg_score += other.eg_score;
		return *this;
	}
	EvaluationResult& operator-=(const EvaluationResult& other) {
		this->mg_score -= other.mg_score;
		this->eg_score -= other.eg_score;
		return *this;
	}
	EvaluationResult& operator*=(int multiplier) {
		this->mg_score = static_cast<int16_t>(this->mg_score * multiplier);
		this->eg_score = static_cast<int16_t>(this->eg_score * multiplier);
		return *this;
	}
};
inline EvaluationResult operator+(EvaluationResult lhs, const EvaluationResult& rhs) {
	lhs += rhs;
	return lhs;
}
inline EvaluationResult operator-(EvaluationResult lhs, const EvaluationResult& rhs) {
	lhs.mg_score -= rhs.mg_score;
	lhs.eg_score -= rhs.eg_score;
	return lhs;
}
inline EvaluationResult operator*(EvaluationResult lhs, int multiplier) {
	lhs *= multiplier;
	return lhs;
}
