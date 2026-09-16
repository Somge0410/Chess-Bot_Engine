#pragma once 
#include "types.h"
#include <array>
struct StateInfo {
    // 1. Largest Types First (8-byte aligned)
    PieceBoards pieces{}; // 96 bytes
    Bitboard color_pieces[2]{};          // 16 bytes
    Bitboard all_pieces{};                           // 8 bytes
    uint64_t zobrist_hash{};                         // 8 bytes
    uint64_t pawn_hash{};                            // 8 bytes

    // 2. Medium Types (Size depends on your implementation, usually 2 to 4 bytes)
    EvaluationResult positional_score{};             // 4 bytes
    EvaluationResult material_score{};               // 4 bytes
    // 3. Bit-Fields (All 10 variables packed into a single 8-byte memory block)
    Color side_to_move : 2;
    uint64_t game_phase : 5;
    Square white_king_square : 6;
    Square black_king_square : 6;
    EnPassantRights en_passant_square : 7;
    CastlingRights castling_rights : 4;
    uint64_t current_twofold_count : 6;
    uint64_t half_moves : 10;
    uint64_t move_count : 9;
    uint64_t current_repetition_tracker_start : 10;
};
