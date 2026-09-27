#include "perft.h"


// Add a 'ply' parameter to track how deep in the tree we are
static uint64_t perft(Board& board, int depth, int ply = 0) {
    if (depth == 0) {
        return 1;
    }

    // Grab the pre-allocated move list for this specific depth
    MoveList& moves = perft_lists[ply];
    moves.clear(); // Simply sets count = 0, virtually zero cost

    MoveGenerator::generate_moves(board, moves);

    if (depth == 1) {
        return static_cast<uint64_t>(moves.size());
    }

    uint64_t nodes = 0;
    for (const Move& move : moves) {
        board.make_move(move);
        // Pass ply + 1 to use the next pre-allocated list
        nodes += perft(board, depth - 1, ply + 1);
        board.undo_move(move);
    }

    return nodes;
}

static void run_perft(const Board& root_board, int depth) {
    Board board = root_board;
    auto start = std::chrono::steady_clock::now();

    if (depth < 0) {
        depth = 0;
    }

    if (depth == 0) {
        std::cout << "info string perft depth 0 nodes 1\n";
        std::cout.flush();
        return;
    }

    MoveList moves;
    MoveGenerator::generate_moves(board, moves);

    uint64_t total_nodes = 0;
    for (const Move& move : moves) {
        board.make_move(move);
        uint64_t move_nodes = perft(board, depth - 1);
        board.undo_move(move);

        total_nodes += move_nodes;
        std::cout << move_to_uci(move) << ": " << move_nodes << "\n";
    }

    auto end = std::chrono::steady_clock::now();
    uint64_t elapsed_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
    uint64_t nps = elapsed_ms > 0 ? (total_nodes * 1000ULL) / elapsed_ms : 0;

    std::cout << "\n";
    std::cout << "info string perft depth " << depth
        << " nodes " << total_nodes
        << " time " << elapsed_ms
        << " nps " << nps << "\n";
    std::cout.flush();
}