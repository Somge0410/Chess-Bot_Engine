#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "board.h"
#include "Engine.h"
#include "Move.h"
#include "MoveGenerator.h"
#include "constants.h"
#include "uci_helpers.h"  // move_to_uci, parse_uci_move
#include "uci.h"
void uci_loop() {
    Board board;     // starts in startpos, thanks to default ctor
    Engine engine;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "uci") {
            std::cout << "id name MyEngine 0.1\n";
            std::cout << "id author Aaron\n";
            // you can add "option name ..." lines here later
            std::cout << "uciok\n";
            std::cout.flush();
        }
        else if (line == "isready") {
            std::cout << "readyok\n";
            std::cout.flush();
        }
        else if (line == "ucinewgame") {
            // clear internal stuff for a new game
            // (optional, but recommended)
            // engine.clear();
            board = Board();  // reset to startpos
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
                std::cout << fen;
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
                // you can also add "mate N", etc. later
            }

            // If nothing specified at all, pick a default:
            if (limits.depth == -1 && limits.movetime == -1 &&
                limits.wtime == -1 && !limits.infinite) {
                limits.depth = 6; // or 8, or whatever you like
            }
            Move best = engine.search(board, limits);

            std::string best_uci = move_to_uci(best);
            std::cout << "bestmove " << best_uci << "\n";
            std::cout.flush();
        }
        else if (line == "stop") {
            // Only needed if your search runs in a separate thread or checks a stop flag.
            // If your search is synchronous and you don't support pondering yet,
            // you can ignore this for now.
            // engine.stop_search();
        }
        else if (line == "quit") {
            break;
        }
        // `setoption` etc. can be added later
    }
}
