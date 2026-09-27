#include "perft.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <stdexcept>

#include "MoveGenerator.h"
#include "position.h"
#include "uci_helpers.h"

namespace {
constexpr int MAX_PERFT_DEPTH = 64;
using PerftMoveLists = std::array<MoveList, MAX_PERFT_DEPTH>;

std::uint64_t perft_impl(Position& position, int depth, int ply,
    PerftMoveLists& move_lists) {
    if (depth == 0) {
        return 1;
    }
    if (ply >= MAX_PERFT_DEPTH) {
        throw std::out_of_range("perft depth exceeds the supported maximum of 64");
    }

    MoveList& moves = move_lists[static_cast<std::size_t>(ply)];
    moves.clear();
    MoveGenerator::generate_moves(position, moves);

    if (depth == 1) {
        return static_cast<std::uint64_t>(moves.size());
    }

    std::uint64_t nodes = 0;
    for (const Move& move : moves) {
        position.make_move(move);
        nodes += perft_impl(position, depth - 1, ply + 1, move_lists);
        position.undo_move();
    }
    return nodes;
}
}

std::uint64_t perft(Position& position, int depth) {
    if (depth < 0) {
        throw std::invalid_argument("perft depth cannot be negative");
    }

    PerftMoveLists move_lists{};
    return perft_impl(position, depth, 0, move_lists);
}

void run_perft(const Position& root_position, int depth) {
    depth = std::max(0, depth);
    Position position = root_position;
    const auto start = std::chrono::steady_clock::now();

    if (depth == 0) {
        std::cout << "info string perft depth 0 nodes 1\n";
        std::cout.flush();
        return;
    }

    MoveList moves;
    MoveGenerator::generate_moves(position, moves);

    std::uint64_t total_nodes = 0;
    for (const Move& move : moves) {
        position.make_move(move);
        const std::uint64_t move_nodes = perft(position, depth - 1);
        position.undo_move();

        total_nodes += move_nodes;
        std::cout << move_to_uci(move) << ": " << move_nodes << '\n';
    }

    const auto elapsed_ms = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count());
    const std::uint64_t nps = elapsed_ms > 0
        ? total_nodes * 1000ULL / elapsed_ms
        : 0;

    std::cout << '\n'
              << "info string perft depth " << depth
              << " nodes " << total_nodes
              << " time " << elapsed_ms
              << " nps " << nps << '\n';
    std::cout.flush();
}

void print_legal_moves(const Position& position) {
    MoveList moves;
    MoveGenerator::generate_moves(position, moves);

    std::cout << "info string legalmoves";
    for (const Move& move : moves) {
        std::cout << ' ' << move_to_uci(move);
    }
    std::cout << '\n';
    std::cout.flush();
}
