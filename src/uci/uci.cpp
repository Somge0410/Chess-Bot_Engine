#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "benchmark.h"
#include "position.h"
#include "engine.h"
#include "Move.h"
#include "MoveGenerator.h"
#include "uci_helpers.h"  // move_to_uci, parse_uci_move
#include "uci.h"
#include "search_parameters.h"
#include "perft.h"
#include "Default_positions.h"

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

#if ENABLE_QSEARCH_DIAGNOSTICS
static void print_search_diagnostics(Engine& engine) {
    const SearchDiagnostics diagnostics = engine.get_search_diagnostics();
    print_search_diagnostics_summary(diagnostics, "info string qdiag ");
}
#endif

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
    Position pos;     // starts in startpos, thanks to default ctor
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
            pos = Position();  // reset to startpos
        }
        else if (line == "legalmoves") {
            wait_for_search(engine, search_thread);
            print_legal_moves(pos);
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
                pos = Position();  // start position
            }
            else if (type == "fen") {
                std::string fen, part;
                for (int i = 0; i < 6 && iss >> part; ++i) {
                    if (!fen.empty()) fen += ' ';
                    fen += part;
                }
                pos = Position(fen);
            }
            else if (type == "preset") {
                std::string preset_name;
                std::string fen;
                iss >> preset_name;

                if (try_get_default_position(preset_name, fen)) {
                    pos = Position(fen);
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
                    Move m = parse_uci_move(pos, move_str);
                    pos.make_move(m);
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
                run_benchmark(get_defaults("bench"), false,
                    limits.depth > 0 ? limits.depth : 12);
                continue;
            }
            if(full_bench_mode) {
                run_benchmark(get_defaults("full_bench"), false,
                    limits.depth > 0 ? limits.depth : 10);
                continue;
			}
            if(tt_bench_mode) {
                run_benchmark(get_defaults("full_bench"), true,
                    limits.depth > 0 ? limits.depth : 10, 16);
                continue;
            }
            if(bench_game_mode) {
                run_benchmark_game(limits.movetime > 0 ? limits.movetime : 200);
				continue;
            }
            if (legalmoves_only) {
                print_legal_moves(pos);
                continue;
            }

            if (perft_mode) {
                if (perft_depth < 0) {
                    perft_depth = 1;
                }

                run_perft(pos, perft_depth);
                continue;
            }

            // If nothing specified at all, pick a default:
            if (limits.depth == -1 && limits.movetime == -1 &&
                limits.wtime == -1 && !limits.infinite) {
                limits.depth = 6;
            }

            // Launch search on a joinable thread (not detached!)
            search_thread = std::thread([&engine, pos, limits]() mutable {
                Move best = engine.search(pos, limits);
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

