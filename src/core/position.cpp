#include "position.h"
#include "fen.h"
#include "bitboard.h"
#include "zobrist.h"
#include "Square.h"
Position::Position() : Position(fen::START_POSITION) {
}

Position::Position(std::string_view fen_text) {
	const FenData data = fen::parse(fen_text);
	pieces = data.pieces;
	side_to_move = data.side_to_move;
	castling_rights = data.castling_rights;
	en_passant_square = data.en_passant_square;
	halfmove_clock = data.halfmove_clock;
	full_move_number = data.full_move_number;

	rebuild_derived_state();
}

void Position::rebuild_derived_state() {
	game_phase = calculate_game_phase();
	zobrist_hash = calculate_zobrist_hash();
	pawn_hash = calculate_pawn_hash();
}

int Position::calculate_game_phase() const noexcept
{
    constexpr int MAX_GAME_PHASE = 24;

    constexpr int KNIGHT_PHASE = 1;
    constexpr int BISHOP_PHASE = 1;
    constexpr int ROOK_PHASE = 2;
    constexpr int QUEEN_PHASE = 4;

    const int knight_phase =
        popcount(
            pieces[color_index(Color::White)]
            [piece_index(PieceType::Knight)]
            |
            pieces[color_index(Color::Black)]
            [piece_index(PieceType::Knight)]
        ) * KNIGHT_PHASE;

    const int bishop_phase =
        popcount(
            pieces[color_index(Color::White)]
            [piece_index(PieceType::Bishop)]
            |
            pieces[color_index(Color::Black)]
            [piece_index(PieceType::Bishop)]
        ) * BISHOP_PHASE;

    const int rook_phase =
        popcount(
            pieces[color_index(Color::White)]
            [piece_index(PieceType::Rook)]
            |
            pieces[color_index(Color::Black)]
            [piece_index(PieceType::Rook)]
        ) * ROOK_PHASE;

    const int queen_phase =
        popcount(
            pieces[color_index(Color::White)]
            [piece_index(PieceType::Queen)]
            |
            pieces[color_index(Color::Black)]
            [piece_index(PieceType::Queen)]
        ) * QUEEN_PHASE;

    const int phase =
        knight_phase +
        bishop_phase +
        rook_phase +
        queen_phase;

    return std::min(phase, MAX_GAME_PHASE);
}
std::uint64_t Position::calculate_zobrist_hash() const noexcept
{
    std::uint64_t hash = 0;

    for (int color = 0; color < 2; ++color) {
        for (int piece = 0; piece < 6; ++piece) {
            Bitboard remaining = pieces[color][piece];

            while (remaining) {
                const Square square = pop_lsb(remaining);

                hash ^= Zobrist::piece_keys
                    [color]
                    [piece]
                    [square_index(square)];
            }
        }
    }

    if (side_to_move == Color::Black) {
        hash ^= Zobrist::black_to_move_key;
    }

    hash ^= Zobrist::castling_keys[
        static_cast<std::uint8_t>(castling_rights)
    ];

    if (en_passant_square != NO_SQUARE) {
        hash ^= Zobrist::en_passant_keys[
            file_of(en_passant_square)
        ];
    }

    return hash;
}