#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "board.h"
#include "engine.h"
#include "Move.h"
#include "MoveGenerator.h"
#include "constants.h"
#include "uci_helpers.h"  // move_to_uci, parse_uci_move
#include "uci.h"
#include "adjustable_parameters.h"

static void wait_for_search(Engine& engine, std::thread& search_thread) {
    if (search_thread.joinable()) {
        engine.stop_search_and_wait();   // signal stop (non-blocking)
        search_thread.join();        // wait for bestmove output
    }
}

void uci_loop() {
    Board board;     // starts in startpos, thanks to default ctor
    Engine engine;
    std::thread search_thread;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "uci") {
            std::cout << "id name MyEngine 0.1\n";
            std::cout << "id author Aaron\n";
            std::cout << "option name Threads type spin default 1 min 1 max 256\n";
            std::cout << "option name Hash type spin default "
                      << MAX_MEMORY_TT_MB << " min 1 max 65536\n";
            std::cout << "option name RevFut type spin default 125 min 0 max 500" << std::endl;
            std::cout << "option name RevFutDepth type spin default 5 min 1 max 20" << std::endl;
            std::cout << "option name FutilityMarginD1 type spin default 200 min 0 max 1000" << std::endl;
            std::cout << "option name FutilityMarginD2 type spin default 400 min 0 max 1000" << std::endl;
            std::cout << "option name DeltaMargin type spin default 200 min 0 max 1000" << std::endl;
            std::cout << "option name MaxQuietPly type spin default 7 min 1 max 20" << std::endl;
            std::cout << "option name LmrMinDepth type spin default 3 min 1 max 10" << std::endl;
            std::cout << "option name LmrMinMoves type spin default 3 min 1 max 10" << std::endl;
            std::cout << "option name LmrRedAm type spin default 2 min 0 max 10" << std::endl;
            std::cout << "option name NmpReduction type spin default 3 min 1 max 10" << std::endl;
            std::cout << "option name HistoryBonusMultiplier type spin default 1 min 0 max 10" << std::endl;
            std::cout << "option name AspirationWindowInitial type spin default 50 min 1 max 500" << std::endl;
            std::cout << "option name AspirationWindowMultiplier type spin default 2 min 1 max 10" << std::endl;
            std::cout << "option name TimeAllocationDivisor type spin default 40 min 1 max 100" << std::endl;
            std::cout << "option name MaxTimeFraction type spin default 2 min 1 max 10" << std::endl;

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
                if (pos != std::string::npos)
                    opt_value = opt_value.substr(pos);
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
            else if(opt_name == "RevFut"){
                int val=std::stoi(opt_value);
                REVERSE_FUTILITY_MARGIN = std::max(0,val);
                std::cerr << "info string RevFut set to " << val << "\n";
			}
            else if (opt_name == "RevFutDepth") {
                int val = std::stoi(opt_value);
                REVERSE_FUTILITY_MAX_DEPTH = std::max(0, val);
                std::cerr << "info string RevFutDepth set to " << val << "\n";
            }
            else if(opt_name =="FutilityMarginD1"){
                int val=std::stoi(opt_value);
                FUTILITY_MARGIN_D1=std::max(0,val);
                std::cerr << "info string FutilityMarginD1 set to " << val << "\n";
            }
            else if (opt_name == "FutilityMarginD2") {
                int val = std::stoi(opt_value);
                FUTILITY_MARGIN_D2 = std::max(0, val);
                std::cerr << "info string FutilityMarginD2 set to " << val << "\n";
			}
            else if(opt_name == "DeltaMargin"){
                int val=std::stoi(opt_value);
                DELTA_MARGIN=std::max(0,val);
                std::cerr << "info string DeltaMargin set to " << val << "\n";
			}
            else if (opt_name == "MaxQuietPly") {
                int val = std::stoi(opt_value);
                MAX_QUIET_PLY = std::max(0, val);
                std::cerr << "info string MaxQuietPly set to " << val << "\n";
            }
            else if (opt_name == "LmrMinDepth") {
                int val = std::stoi(opt_value);
                LMR_MIN_DEPTH = std::max(0, val);
                std::cerr << "info string LmrMinDepth set to " << val << "\n";
            }
            else if (opt_name == "LmrMinMoves") {
                int val = std::stoi(opt_value);
                LMR_MIN_MOVES_SEARCHED = std::max(0, val);
                std::cerr << "info string LMR_MIN_MOVES set to " << val << "\n";
            }
            else if (opt_name == "LmrRedAm") {
                int val = std::stoi(opt_value);
                LMR_REDUCTION_AMOUNT = std::max(0, val);
                std::cerr << "info string LmrRedAm set to " << val << "\n";
            }
            else if (opt_name == "NmpMinDepth") {
                int val = std::stoi(opt_value);
                NMP_MIN_DEPTH = std::max(0, val);
                std::cerr << "info string NmpMinDepth set to " << val << "\n";
            }
            else if (opt_name == "NmpReduction") {
                int val = std::stoi(opt_value);
                NMP_REDUCTION = std::max(0, val);
                std::cerr << "info string NmpReduction set to " << val << "\n";
            }
            else if (opt_name == "HistoryBonusMultiplier") {
                int val = std::stoi(opt_value);
                HISTORY_BONUS_MULTIPLIER = std::max(1, val);
                std::cerr << "info string HISTORY_BONUS_MULTIPLIER set to " << val << "\n";
            }
            else if (opt_name == "AspirationWindowInitial") {
                int val = std::stoi(opt_value);
                ASPIRATION_WINDOW_INITIAL = std::max(0, val);
                std::cerr << "info string AspirationWindowInitial set to " << val << "\n";
            }
            else if(opt_name == "AspirationWindowMultiplier"){
                int val=std::stoi(opt_value);
                ASPIRATION_WINDOW_MULTIPLIER=std::max(1,val);
                std::cerr << "info string AspirationWindowMultiplier set to " << val << "\n";
            }
             else if (opt_name == "TimeAllocationDivisor") {
                int val = std::stoi(opt_value);
                TIME_ALLOCATION_DIVISOR = std::max(1, val);
                std::cerr << "info string TimeAllocationDivisor set to " << val << "\n";
            }
            else if (opt_name == "MaxTimeFraction") {
                int val = std::stoi(opt_value);
                MAX_TIME_FRACTION = std::max(1, val);
                std::cerr << "info string MaxTimeFraction set to " << val << "\n";
			}
        


        }
        else if (line.rfind("position", 0) == 0) {
            std::istringstream iss(line);
            std::string token;
            iss >> token; // "position"

            std::string type;
            iss >> type;

            if (type == "startpos") {
                board =Board();  // start position
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
                    int c = board.get_move_count();
                    Move m = parse_uci_move(board, move_str);
                    bool ka=board.is_repetition_draw(2);
                    board.make_move(m);
                }
            }
        }
        else if (line.rfind("go", 0) == 0) {
            // Stop any previous search before starting a new one
            wait_for_search(engine, search_thread);

            SearchLimits limits;

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
