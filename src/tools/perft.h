#pragma once

#include <cstdint>

class Position;

[[nodiscard]] std::uint64_t perft(Position& position, int depth);
void run_perft(const Position& root_position, int depth);
void print_legal_moves(const Position& position);
