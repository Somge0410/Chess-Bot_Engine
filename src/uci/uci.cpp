#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "benchmark.h"
#include "board.h"
#include "engine.h"
#include "Move.h"
#include "MoveGenerator.h"
#include "constants.h"
#include "uci_helpers.h"  // move_to_uci, parse_uci_move
#include "uci.h"
#include "SPSA_parameters.h"

#ifndef GIT_COMMIT
#define GIT_COMMIT "unknown"
#endif
#ifndef GIT_BRANCH
#define GIT_BRANCH "unknown"
#endif

struct NamedPosition {
    const char* name;
    const char* fen;
};

static constexpr NamedPosition DEFAULT_POSITIONS[] = {
    {"Kf7","8/4k2r/3P3q/1pN2pp1/2R1p3/P3P1P1/1Q3PK1/8 b - - 0 42"},
    {"Qe1", "q2r2r1/1k2b3/4P2p/1pp5/3p1B1P/1PQP2P1/2P2P2/R5K1 w - - 0 34"},
    {"h6", "7b/8/3k4/4pK1P/8/6B1/8/8 w - - 3 50" },
    {"g6", "r1b1kb1r/pp2pppp/1q3n2/3p4/PP6/2P2N2/3NPPPP/R2QKB1R b KQkq - 0 1"},
	{"Rb8", "r5k1/pB1b1ppp/3qpb2/8/3n1N2/6P1/4PP1P/2BQ1RK1 b - - 0 1"},
    {"Rd1", "r1bq1rk1/pp2ppb1/3p1np1/6N1/2Pn4/2N4P/PP2BPP1/R3QRK1 w - - 0 1"},
    {"AvoidDraw", "8/p3Q3/2P3k1/1P6/5qpp/8/5PP1/6K1 w - - 8 65"},
    {"Qe3", "r4rk1/1b3ppp/p2pp3/4n1Q1/B1p1P3/P1N4P/1qP2PP1/R4RK1 w - - 0 18"},
    {"a4", "8/8/6k1/ppppp1P1/5pK1/P1PP1P2/1P6/8 b - - 0 41"},
    {"g5", "rnb1k2r/pp4p1/8/2bQPp2/5q1p/1PN4K/PB1PBPP1/R4R2 b kq - 3 21"},
    {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"},
    {"perft2", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"},
    {"perft3", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"},
    {"perft4", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"},
    {"perft5", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"}
};
static constexpr NamedPosition BENCH_POSITIONS[] = {
    {"Kf7","8/4k2r/3P3q/1pN2pp1/2R1p3/P3P1P1/1Q3PK1/8 b - - 0 42"},
    {"Qe1", "q2r2r1/1k2b3/4P2p/1pp5/3p1B1P/1PQP2P1/2P2P2/R5K1 w - - 0 34"},
    {"h6", "7b/8/3k4/4pK1P/8/6B1/8/8 w - - 3 50" },
    {"g6", "r1b1kb1r/pp2pppp/1q3n2/3p4/PP6/2P2N2/3NPPPP/R2QKB1R b KQkq - 0 1"},
    {"Rb8", "r5k1/pB1b1ppp/3qpb2/8/3n1N2/6P1/4PP1P/2BQ1RK1 b - - 0 1"},
    {"Rd1", "r1bq1rk1/pp2ppb1/3p1np1/6N1/2Pn4/2N4P/PP2BPP1/R3QRK1 w - - 0 1"},
    {"Qe3", "r4rk1/1b3ppp/p2pp3/4n1Q1/B1p1P3/P1N4P/1qP2PP1/R4RK1 w - - 0 18"},
    {"a4", "8/8/6k1/ppppp1P1/5pK1/P1PP1P2/1P6/8 b - - 0 41"},
    {"g5", "rnb1k2r/pp4p1/8/2bQPp2/5q1p/1PN4K/PB1PBPP1/R4R2 b kq - 3 21"},
    {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"},
    {"perft2", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"},
    {"perft3", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"},
    {"perft4", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"},
    {"perft5", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"}
};
static constexpr NamedPosition FULL_BENCH_POSITIONS[] = {
    {"Bench1","rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"},
    {"Bench2","r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 10"},
    {"Bench3","8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 11"},
    {"Bench4","4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19"},
    {"Bench5","rq3rk1/ppp2ppp/1bnpN3/3N2B1/4P3/7P/PPPQ1PP1/2KR3R b - - 0 14"},
    {"Bench6","r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4PpP1/1BNP4/PPP2P1P/3R1RK1 b - g3 0 14"},
    {"Bench7","r3r1k1/2p2ppp/p1p1bn2/8/1q2P3/2NPQN2/PPP3PP/R4RK1 b - - 2 15"},
  {"Bench8","r1bbk1nr/pp3p1p/2n5/1N4p1/2Np1B2/8/PPP2PPP/2KR1B1R w kq - 0 13"},
  {"Bench9","r1bq1rk1/ppp1nppp/4n3/3p3Q/3P4/1BP1B3/PP1N2PP/R4RK1 w - - 1 16"},
  {"Bench10","4r1k1/r1q2ppp/ppp2n2/4P3/5Rb1/1N1BQ3/PPP3PP/R5K1 w - - 1 17"},
  {"Bench11","2rqkb1r/ppp2p2/2npb1p1/1N1Nn2p/2P1PP2/8/PP2B1PP/R1BQK2R b KQ - 0 11"},
  {"Bench12","r1bq1r1k/b1p1npp1/p2p3p/1p6/3PP3/1B2NN2/PP3PPP/R2Q1RK1 w - - 1 16"},
  {"Bench13","3r1rk1/p5pp/bpp1pp2/8/q1PP1P2/b3P3/P2NQRPP/1R2B1K1 b - - 6 22"},
  {"Bench14","r1q2rk1/2p1bppp/2Pp4/p6b/Q1PNp3/4B3/PP1R1PPP/2K4R w - - 2 18"},
  {"Bench15","4k2r/1pb2ppp/1p2p3/1R1p4/3P4/2r1PN2/P4PPP/1R4K1 b - - 3 22"},
  {"Bench16","3q2k1/pb3p1p/4pbp1/2r5/PpN2N2/1P2P2P/5PP1/Q2R2K1 b - - 4 26"},
  {"Bench17","6k1/6p1/6Pp/ppp5/3pn2P/1P3K2/1PP2P2/3N4 b - - 0 1"},
  {"Bench18","3b4/5kp1/1p1p1p1p/pP1PpP1P/P1P1P3/3KN3/8/8 w - - 0 1"},
  {"Bench19","2K5/p7/7P/5pR1/8/5k2/r7/8 w - - 4 3"},
  {"Bench20","8/6pk/1p6/8/PP3p1p/5P2/4KP1q/3Q4 w - - 0 1"},
  {"Bench21","7k/3p2pp/4q3/8/4Q3/5Kp1/P6b/8 w - - 0 1"},
  {"Bench22","8/2p5/8/2kPKp1p/2p4P/2P5/3P4/8 w - - 0 1"},
  {"Bench23","8/1p3pp1/7p/5P1P/2k3P1/8/2K2P2/8 w - - 0 1"},
  {"Bench24","8/pp2r1k1/2p1p3/3pP2p/1P1P1P1P/P5KR/8/8 w - - 0 1"},
  {"Bench25","8/3p4/p1bk3p/Pp6/1Kp1PpPp/2P2P1P/2P5/5B2 b - - 0 1"},
  {"Bench26","5k2/7R/4P2p/5K2/p1r2P1p/8/8/8 b - - 0 1"},
  {"Bench27","6k1/6p1/P6p/r1N5/5p2/7P/1b3PP1/4R1K1 w - - 0 1"},
  {"Bench28","1r3k2/4q3/2Pp3b/3Bp3/2Q2p2/1p1P2P1/1P2KP2/3N4 w - - 0 1"},
  {"Bench29","6k1/4pp1p/3p2p1/P1pPb3/R7/1r2P1PP/3B1P2/6K1 w - - 0 1"},
  {"Bench30","8/3p3B/5p2/5P2/p7/PP5b/k7/6K1 w - - 0 1"},
  {"Bench31","5rk1/q6p/2p3bR/1pPp1rP1/1P1Pp3/P3B1Q1/1K3P2/R7 w - - 93 90"},
  {"Bench32","4rrk1/1p1nq3/p7/2p1P1pp/3P2bp/3Q1Bn1/PPPB4/1K2R1NR w - - 40 21"},
  {"Bench33","r3k2r/3nnpbp/q2pp1p1/p7/Pp1PPPP1/4BNN1/1P5P/R2Q1RK1 w kq - 0 16"},
  {"Bench34","3Qb1k1/1r2ppb1/pN1n2q1/Pp1Pp1Pr/4P2p/4BP2/4B1R1/1R5K b - - 11 40"},
  {"Bench35","4k3/3q1r2/1N2r1b1/3ppN2/2nPP3/1B1R2n1/2R1Q3/3K4 w - - 5 1"},
  {"Bench36","1r6/1P4bk/3qr1p1/N6p/3pp2P/6R1/3Q1PP1/1R4K1 w - - 1 42"},
  // Positions with high numbers of changed threats
  //{"Bench37","k7/2n1n3/1nbNbn2/2NbRBn1/1nbRQR2/2NBRBN1/3N1N2/7K w - - 0 1"},
  {"Bench38","K7/8/8/BNQNQNB1/N5N1/R1Q1q2r/n5n1/bnqnqnbk w - - 0 1"},
  // Mate and stalemate positions
  {"Bench46","6k1/3b3r/1p1p4/p1n2p2/1PPNpP1q/P3Q1p1/1R1RB1P1/5K2 b - - 0 1"},
  {"Bench47","r2r1n2/pp2bk2/2p1p2p/3q4/3PN1QP/2P3R1/P4PP1/5RK1 w - - 0 1"},
  {"Bench48","8/8/8/8/8/6k1/6p1/6K1 w - - 0 1"},
  {"Bench49 ","7k/7P/6K1/8/3B4/8/8/8 b - - 0 1"},
  // 5-man positions
  {"Bench39","8/8/8/8/5kp1/P7/8/1K1N4 w - - 0 1"},     // Kc2 - mate
  {"Bench40","8/8/8/5N2/8/p7/8/2NK3k w - - 0 1"},      // Na2 - mate
  {"Bench41","8/3k4/8/8/8/4B3/4KB2/2B5 w - - 0 1"},    // draw

  // 6-man positions
  {"Bench42","8/8/1P6/5pr1/8/4R3/7k/2K5 w - - 0 1"},   // Re5 - mate
  {"Bench43","8/2p4P/8/kr6/6R1/8/8/1K6 w - - 0 1"},    // Ka2 - mate
  {"Bench44","8/8/3P3k/8/1p6/8/1P6/1K3n2 b - - 0 1"},  // Nd2 - draw

  // 7-man positions
  {"Bench45","8/R7/2q5/8/6k1/8/1P5p/K6R w - - 0 124"}, // Draw

};
std::vector<std::pair<std::string, std::string>> get_default_positions() {
    std::vector<std::pair<std::string, std::string>> positions;
    positions.reserve(sizeof(DEFAULT_POSITIONS) / sizeof(DEFAULT_POSITIONS[0]));
    for (const auto& preset : DEFAULT_POSITIONS) {
        positions.emplace_back(preset.name, preset.fen);
    }
    return positions;
}
std::vector<std::pair<std::string, std::string>> get_bench_positions() {
    std::vector<std::pair<std::string, std::string>> positions;
    positions.reserve(sizeof(BENCH_POSITIONS) / sizeof(BENCH_POSITIONS[0]));
    for (const auto& preset : BENCH_POSITIONS) {
        positions.emplace_back(preset.name, preset.fen);
    }
    return positions;
}
std::vector<std::pair<std::string, std::string>> get_full_bench_positions() {
    std::vector<std::pair<std::string, std::string>> positions;
    positions.reserve(sizeof(FULL_BENCH_POSITIONS) / sizeof(FULL_BENCH_POSITIONS[0]));
    for (const auto& preset : FULL_BENCH_POSITIONS) {
        positions.emplace_back(preset.name, preset.fen);
    }
    return positions;
}
static bool try_get_default_position(const std::string& name, std::string& out_fen) {
    for (const auto& preset : DEFAULT_POSITIONS) {
        if (name == preset.name) {
            out_fen = preset.fen;
            return true;
        }
    }
    return false;
}

static void print_default_positions() {
    std::cout << "info string presets";
    for (const auto& preset : DEFAULT_POSITIONS) {
        std::cout << ' ' << preset.name;
    }
    std::cout << "\n";
    std::cout.flush();
}

static void wait_for_search(Engine& engine, std::thread& search_thread) {
    if (search_thread.joinable()) {
        engine.stop_search_and_wait();   // signal stop (non-blocking)
        search_thread.join();            // wait for bestmove output
    }
}

#if ENABLE_QSEARCH_DIAGNOSTICS
static void print_search_diagnostics(Engine& engine) {
    const SearchDiagnostics diagnostics = engine.get_search_diagnostics();
    print_search_diagnostics_summary(diagnostics, "info string qdiag ");
}
#endif

static void print_legal_moves(const Board& board) {
    MoveList moves;
    MoveGenerator::generate_moves(board, moves);

    std::cout << "info string legalmoves";
    for (const Move& move : moves) {
        std::cout << ' ' << move_to_uci(move);
    }
    std::cout << "\n";
    std::cout.flush();
}
// 64 is the maximum depth perft will ever reach
MoveList perft_lists[64];

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
#define SPSA_INT_PARAMS(X) \
    X(FUTILITY_MARGIN_D1, 0, 10000) \
    X(FUTILITY_MARGIN_D2, 0, 10000) \
    X(MAX_QUIET_PLY, 1, 64) \
    X(LMR_MIN_DEPTH, 0, 64) \
    X(LMR_FIRST_REDUCED_MOVE, 1, 256) \
    X(LMR_REDUCTION_AMOUNT, 0, 64) \
    X(NMP_MIN_DEPTH, 0, 64) \
    X(NMP_REDUCTION, 1, 64) \
    X(NMP_DEPTH_DIVISOR, 1, 64) \
    X(NMP_EVAL_DIVISOR, 1, 10000) \
    X(NMP_MAX_EVAL_REDUCTION, 0, 16) \
    X(TT_STAGE, 0, 16) \
    X(PROMO_STAGE, 0, 16) \
    X(MVV_LVA_STAGE, 0, 16) \
    X(KILLER_STAGE, 0, 16) \
    X(COUNTERMOVE_STAGE, 0, 16) \
    X(QUIET_STAGE, 0, 16) \
    X(LOSING_CAPTURE_STAGE, 0, 16) \
    X(CAPTURE_SCORE_TIEBREAK_DIVISOR, 1, 1024) \
    X(HISTORY_BONUS_MULTIPLIER, 0, 1000) \
    X(ASPIRATION_WINDOW_INITIAL, 0, 10000) \
    X(MOVE_OVERHEAD_MS, 0, 10000) \
    X(MOVE_COUNT_THRESHOLD, 0, 200) \
    X(DELTA_BEST_SCORE, 0, 10000) \
    X(ROOT_PERTURBATION_MIN_HELPERS, 0, 256) \
    X(ROOT_PERTURBATION_MIN_BAND_SIZE, 0, 256) \
    X(ROOT_PERTURBATION_MAX_BAND_SIZE, 0, 256) \
    X(REVERSE_FUTILITY_MAX_DEPTH, 0, 64) \
    X(REVERSE_FUTILITY_MARGIN, 0, 100000) \
    X(PAWN_PUSH_SCORE1, -10000, 10000) \
    X(PAWN_PUSH_SCORE2, -10000, 10000) \
    X(PAWN_PUSH_SCORE3, -10000, 10000) \
    X(PAWN_PUSH_SCORE4, -10000, 10000) \
    X(PAWN_PUSH_SCORE5, -10000, 10000) \
    X(PAWN_PUSH_SCORE6, -10000, 10000)

#define SPSA_DOUBLE_PARAMS(X) \
    X(ASPIRATION_WINDOW_MULTIPLIER, 1.0, 100.0) \
    X(MOVES_TO_GO, 1.0, 200.0) \
    X(MOVES_TO_GO_MG, 1.0, 200.0) \
    X(MOVES_TO_GO_EG, 1.0, 200.0) \
    X(MIN_MOVES_TO_GO, 1.0, 200.0) \
    X(MAX_MOVES_TO_GO, 1.0, 400.0) \
    X(INC_USAGE_FACTOR, 0.0, 10.0) \
    X(MOVE_COUNT_WEIGHT, 0.0, 10.0) \
    X(REFERENCE_TIME, 1.0, 10000000.0) \
    X(MAX_MULTIPLIER_FAST, 0.0, 20.0) \
    X(MAX_MULTIPLIER_SLOW, 0.0, 20.0) \
    X(TIME_CHANGES_COUNT_BIG, 0.0, 1.0) \
    X(TIME_CHANGES_COUNT_MEDIUM, 0.0, 1.0) \
    X(TIME_CHANGES_COUNT_SMALL, 0.0, 1.0) \
    X(VOLATILITY_DIV, 1.0, 100000.0) \
    X(EXTRA_BEST_BASE, 0.0, 100000.0) \
    X(EXTRA_BEST_FLIP, 0.0, 100000.0) \
    X(EXTRA_BEST_WEIGHT, 0.0, 100000.0) \
    X(MAX_MOVE_COUNT_REDUCTION, 0.0, 1000.0) \
    X(TIME_MARGIN, -100000.0, 100000.0) \
    X(LOG_BASE, 0.0, 10.0) \
    X(LOG_DIV, 1.0, 1000.0) \
    X(Q_LOG_BASE, 0.0, 10.0) \
    X(Q_LOG_DIV, 1.0, 1000.0)

static void print_spsa_options() {
#define PRINT_INT_OPT(name, minv, maxv) \
    std::cout << "option name " #name " type spin default " << name \
              << " min " << minv << " max " << maxv << std::endl;
#define PRINT_DOUBLE_OPT(name, minv, maxv) \
    std::cout << "option name " #name " type spin default " << static_cast<int>(name) \
              << " min " << static_cast<int>(minv) << " max " << static_cast<int>(maxv) << std::endl;

    SPSA_INT_PARAMS(PRINT_INT_OPT)
        SPSA_DOUBLE_PARAMS(PRINT_DOUBLE_OPT)

#undef PRINT_INT_OPT
#undef PRINT_DOUBLE_OPT
}

static bool try_set_spsa_option(const std::string& opt_name, const std::string& opt_value) {
#define SET_INT_OPT(name, minv, maxv) \
    if (opt_name == #name) { \
        int val = std::stoi(opt_value); \
        name = std::clamp(val, minv, maxv); \
        return true; \
    }

#define SET_DOUBLE_OPT(name, minv, maxv) \
    if (opt_name == #name) { \
        double val = std::stod(opt_value); \
        name = std::clamp(val, minv, maxv); \
        return true; \
    }

    SPSA_INT_PARAMS(SET_INT_OPT)
    SPSA_DOUBLE_PARAMS(SET_DOUBLE_OPT)

#undef SET_INT_OPT
#undef SET_DOUBLE_OPT
        return false;
}

void uci_loop() {
    Board board;     // starts in startpos, thanks to default ctor
    Engine engine;
    std::thread search_thread;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "uci") {
            std::cout << "id name MyEngine 0.1 (" << GIT_BRANCH << " " << GIT_COMMIT << ")\n";
            std::cout << "id author Aaron\n";
            std::cout << "option name Threads type spin default 1 min 1 max 256\n";
            std::cout << "option name Hash type spin default "
                      << MAX_MEMORY_TT_MB << " min 1 max 65536\n";
            print_spsa_options();

            std::cout << "uciok\n";
            std::cout.flush();
        }
        else if (line == "isready") {
            std::cout << "readyok\n";
            std::cout.flush();
        }
        else if (line == "ucinewgame") {
            wait_for_search(engine, search_thread);
            board = Board();  // reset to startpos
        }
        else if (line == "legalmoves") {
            wait_for_search(engine, search_thread);
            print_legal_moves(board);
        }
        else if (line == "presets") {
            wait_for_search(engine, search_thread);
            print_default_positions();
        }
        else if (line.rfind("setoption", 0) == 0) {
            // Format: setoption name <name> value <value>
            wait_for_search(engine, search_thread);

            std::istringstream iss(line);
            std::string token;
            iss >> token; // "setoption"

            std::string name_key;
            iss >> token; // "name"

            // Read option name (may contain spaces, ends before "value")
            std::string opt_name;
            while (iss >> token) {
                if (token == "value") break;
                if (!opt_name.empty()) opt_name += ' ';
                opt_name += token;
            }

            // Read value
            std::string opt_value;
            if (token == "value") {
                std::getline(iss, opt_value);
                // Trim leading whitespace
                auto pos = opt_value.find_first_not_of(' ');
                if (pos != std::string::npos) {
                    opt_value = opt_value.substr(pos);
                }
            }

            if (opt_name == "Threads") {
                int threads = std::stoi(opt_value);
                threads = std::max(1, std::min(threads, static_cast<int>(std::thread::hardware_concurrency())));
                engine.set_threads(threads);
                //std::cerr << "info string Threads set to " << threads << "\n";
            }
            else if (opt_name == "Hash") {
                size_t hash_mb = std::stoull(opt_value);
                hash_mb = std::max<size_t>(1, std::min<size_t>(hash_mb, 65536));
                engine.resize_tt(hash_mb);
                //std::cerr << "info string Hash set to " << hash_mb << " MB\n";
            }
            else if (try_set_spsa_option(opt_name, opt_value)) {
                continue;
			}
        


        }
        else if (line.rfind("position", 0) == 0) {
            std::istringstream iss(line);
            std::string token;
            iss >> token; // "position"

            std::string type;
            iss >> type;

            if (type == "startpos") {
                board = Board();  // start position
            }
            else if (type == "fen") {
                std::string fen, part;
                for (int i = 0; i < 6 && iss >> part; ++i) {
                    if (!fen.empty()) fen += ' ';
                    fen += part;
                }
                board = Board(fen);
            }
            else if (type == "preset") {
                std::string preset_name;
                std::string fen;
                iss >> preset_name;

                if (try_get_default_position(preset_name, fen)) {
                    board = Board(fen);
                    std::cout << "info string loaded preset " << preset_name << "\n";
                    std::cout << "info string fen " << fen << "\n";
                    std::cout.flush();
                }
                else {
                    std::cout << "info string unknown preset " << preset_name << "\n";
                    std::cout.flush();
                    print_default_positions();
                    continue;
                }
            }

            if (iss >> token && token == "moves") {
                std::string move_str;
                while (iss >> move_str) {
                    Move m = parse_uci_move(board, move_str);
                    board.make_move(m);
                }
            }
        }
        else if (line.rfind("go", 0) == 0) {
            // Stop any previous search before starting a new one
            wait_for_search(engine, search_thread);

            SearchLimits limits;
            bool bench_mode = false;
			bool full_bench_mode = false;
            bool tt_bench_mode = false;
            bool bench_game_mode = false;
            bool legalmoves_only = false;
            bool perft_mode = false;
            int perft_depth = -1;

            std::istringstream iss(line);
            std::string token;
            iss >> token; // "go"

            while (iss >> token) {
                if (token == "bench") {
                    bench_mode = true;
                }
                else if (token == "bench_full") {
                    full_bench_mode = true;
                }
                else if (token == "bench_tt") {
#if ENABLE_QSEARCH_DIAGNOSTICS
					tt_bench_mode = true;
#else
                    std::cout << "info string bench_tt is not supported in this build\n";
					std::cout << "starting full_bench instead\n";
					std::cout.flush();
					full_bench_mode = true;
#endif
                }
                else if (token == "bench_game") {
                    bench_game_mode = true;
                }
                else if (token == "depth") {
                    iss >> limits.depth;
                }
                else if (token == "movetime") {
                    iss >> limits.movetime;      // milliseconds
                }
                else if (token == "wtime") {
                    iss >> limits.wtime;         // ms remaining for white
                }
                else if (token == "btime") {
                    iss >> limits.btime;         // ms remaining for black
                }
                else if (token == "winc") {
                    iss >> limits.winc;          // ms increment for white
                }
                else if (token == "binc") {
                    iss >> limits.binc;          // ms increment for black
                }
                else if (token == "nodes") {
                    iss >> limits.nodes;
                }
                else if (token == "infinite") {
                    limits.infinite = true;
                }
                else if (token == "legalmoves") {
                    legalmoves_only = true;
                }
                else if (token == "perft") {
                    perft_mode = true;

                    if (iss >> token) {
                        if (token == "depth") {
                            iss >> perft_depth;
                        }
                        else {
                            perft_depth = std::stoi(token);
                        }
                    }
                }
            }

            if (bench_mode) {
                run_benchmark(get_bench_positions());
                continue;
            }
            if(full_bench_mode) {
                run_benchmark(get_full_bench_positions(),false,10);
                continue;
			}
            if(tt_bench_mode) {
                run_benchmark(get_full_bench_positions(),true,10,16);
                continue;
            }
            if(bench_game_mode) {
                run_benchmark_game(limits.movetime > 0 ? limits.movetime : 200);
				continue;
            }
            if (legalmoves_only) {
                print_legal_moves(board);
                continue;
            }

            if (perft_mode) {
                if (perft_depth < 0) {
                    perft_depth = 1;
                }

                run_perft(board, perft_depth);
                continue;
            }

            // If nothing specified at all, pick a default:
            if (limits.depth == -1 && limits.movetime == -1 &&
                limits.wtime == -1 && !limits.infinite) {
                limits.depth = 6;
            }

            // Launch search on a joinable thread (not detached!)
            search_thread = std::thread([&engine, board, limits]() mutable {
                Move best = engine.search(board, limits);
#if ENABLE_QSEARCH_DIAGNOSTICS
                print_search_diagnostics(engine);
#endif
                std::string best_uci = move_to_uci(best);
                std::cout << "bestmove " << best_uci << "\n";
                std::cout.flush();
                });
        }
        else if (line == "stop") {
            wait_for_search(engine, search_thread);
        }
        else if (line == "quit") {
            wait_for_search(engine, search_thread);
            engine.shutdown();
            break;
        }
    }

    std::cerr << "leaving uci_loop() now\n";
    wait_for_search(engine, search_thread);
    engine.shutdown();
}

