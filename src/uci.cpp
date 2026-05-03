#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "board.h"
#include "engine.h"
#include "Move.h"
#include "MoveGenerator.h"
#include "constants.h"
#include "uci_helpers.h"  // move_to_uci, parse_uci_move
#include "uci.h"

#ifndef GIT_COMMIT
#define GIT_COMMIT "unknown"
#endif
#ifndef GIT_BRANCH
#define GIT_BRANCH "unknown"
#endif

static void wait_for_search(Engine& engine, std::thread& search_thread) {
    if (search_thread.joinable()) {
        engine.stop_search_and_wait();   // signal stop (non-blocking)
        search_thread.join();            // wait for bestmove output
    }
}

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

static uint64_t perft(Board& board, int depth) {
    if (depth == 0) {
        return 1;
    }

    MoveList moves;
    MoveGenerator::generate_moves(board, moves);

    if (depth == 1) {
        return static_cast<uint64_t>(moves.size());
    }

    uint64_t nodes = 0;
    for (const Move& move : moves) {
        board.make_move(move);
        nodes += perft(board, depth - 1);
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
                std::cerr << "info string Threads set to " << threads << "\n";
            }
            else if (opt_name == "Hash") {
                size_t hash_mb = std::stoull(opt_value);
                hash_mb = std::max<size_t>(1, std::min<size_t>(hash_mb, 65536));
                engine.resize_tt(hash_mb);
                std::cerr << "info string Hash set to " << hash_mb << " MB\n";
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
                // FEN is 6 space-separated parts
                for (int i = 0; i < 6 && iss >> part; ++i) {
                    if (!fen.empty()) fen += ' ';
                    fen += part;
                }
                board = Board(fen);
            }

            // Optional moves part
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
            bool legalmoves_only = false;
            bool perft_mode = false;
            int perft_depth = -1;

            std::istringstream iss(line);
            std::string token;
            iss >> token; // "go"

            while (iss >> token) {
                if (token == "depth") {
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
