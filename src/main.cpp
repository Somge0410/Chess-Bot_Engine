#include <iostream>
#include "Board.h"
#include "utils.h"    
#include "constants.h" 
#include "zobrist.h"
#include "attack_rays.h"
#include "bitboard_masks.h"
#include "MoveGenerator.h"
#include "notation_utils.h"
#include "evaluation.h"
#include "engine.h"
#include "rook_tables.h"
#include "bishop_tables.h"
#include <chrono>
#include "prepare_data.h"
#include <vector>
#include <iostream>
#include "uci.h"
#include "see.h"
int main(int argc, char* argv[]) {
    Zobrist::initialize_keys();
    if (argc > 1 && std::string(argv[1]) == "perft")
    {
        Engine engine;
        std::string pos2 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
        std::string pos3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
        std::string pos4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
        std::string pos5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
        std::string pos6 = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";
        Board board(pos4);
        int depth = std::stoi(argv[2]);
        for (int i = 1; i <= depth; ++i) {

            PerftRes result = engine.perft_test(board, i);
            std::cout << "Perft result for depth " << depth << ": " << result.nodes << " nodes in " << result.duration << " ms" << std::endl;
        }
        return 0;
    }
    else if (argc > 1 && std::string(argv[1]) == "nodes")
    {
        Engine engine;
        Board board;

        double duration = 0;
        uint64_t nodes = 0;
        for (size_t i = 0; i < 100; ++i)
        {
            PerftRes x = engine.perft_test(board, 5);
            duration += x.duration;
            nodes += x.nodes;
        }
        std::cout << "Average duration for depth 5 over 100 runs: " << duration / 100 << " ms" << std::endl;
        std::cout << "Nodes per second:" << nodes / duration << std::endl;
        return 0;
    }
    else if (argc > 1 && std::string(argv[1]) == "game")
    {
        Engine engine;
        Board board;
        std::cout << "Choose a color, w for white, b for black" << std::endl;
        char color_choice;
        std::cin >> color_choice;
        Color player_color = (color_choice == 'w') ? Color::WHITE : Color::BLACK;

        bool game_running = true;
        std::vector<Move> move_history;
        move_history.reserve(256);
        while (game_running)
        {
            board.display();
            MoveList legal_moves;
            MoveGenerator::generate_moves(board,legal_moves);

            if (legal_moves.empty())
            {
                if (board.in_check())
                {
                    std::string winner = board.get_turn() == Color::WHITE ? "Black" : "Whte";
                    std::cout << "Checkmate! " << winner << " wins." << std::endl;
                }
                else
                {
                    std::cout << "Stalement! It's a draw." << std::endl;
                }
                game_running = false;
                continue;
            }
            if (board.get_turn() == player_color)
            {
                while (true)
                {
                    std::cout << "Your Move: ";
                    std::string move_str;
                    std::cin >> move_str;

                    if (move_str == "exit")
                    {
                        game_running = false;
                        break;
                    }
                    if (move_str == "undo")
                    {
                        if (!move_history.empty()) {
                            Move last_move = move_history.back();
                            board.undo_move(last_move);
                            move_history.pop_back();
                            if (!move_history.empty()) {
                                last_move = move_history.back();
                                board.undo_move(last_move);
                                move_history.pop_back();
                                std::cout << "Undo move" << std::endl;
                                board.display();
                                continue;
                            }
                            else {

                                std::cout << "Undo move" << std::endl;
                                board.display();
                                break;
                            }
                        }
                        else {
                            std::cout << "No moves to undo." << std::endl;
                            board.display();
                            continue;
                        }
                        board.display();
                        continue;
                    }

                    Move move_object = parse_move(move_str, legal_moves);
                    if (move_object.from_square != -1)
                    {
                        board.make_move(move_object);
                        move_history.push_back(move_object);
                        break;
                    }
                    else
                    {
                        std::cout << "\n!!! Invalid Move. Please try again. !!!\n" << std::endl;
                    }
                }

            }
            else
            {
                std::cout << "\nComputer is thinking..." << std::endl;
                SearchLimits limits;
                limits.movetime = 15000;
                Move best_move = engine.search(board, limits);
                if (best_move.from_square != -1)
                {
                    std::cout << "Computer plays:" << to_san(best_move, legal_moves) << std::endl;
                    board.make_move(best_move);
                    move_history.push_back(best_move);
                }
                else
                {
                    std::cout << "Engine cannot find a move. Game over." << std::endl;
                    game_running = false;
                }
            }
        }
        return 0;

    }
    /*else {
        std::cout << "profiler";
        Engine engine(512);
        std::string Qe3 = "r4rk1/1b3ppp/p2pp3/4n1Q1/B1p1P3/P1N4P/1qP2PP1/R4RK1 w - - 0 18"; // Best move: Qe3
        std::string a4 = "8/8/6k1/ppppp1P1/5pK1/P1PP1P2/1P6/8 b - - 0 41"; //Best move: a4
        std::string g5 = "rnb1k2r/pp4p1/8/2bQPp2/5q1p/1PN4K/PB1PBPP1/R4R2 b kq - 3 21"; //Best  move: g5
        std::string draws = "1K5k/PN1p4/P1pP4/P1P5/8/8/8/8 w - - 0 1";
        std::string illegal = "8/8/8/2K4P/2P2Q2/p2k2P1/P7/8 b - - 0 63";
        std::string see_captures="k2r4/8/5n2/8/1N6/8/3Q4/K7 b - - 0 1";
        std::string g5debug = "rnb1k2r/pp6/8/2bQPpp1/5q1p/1PN4K/PB1PBPP1/R4R2 w kq - 0 22";
        std::string input_fen=g5;
        
        Board board=Board(input_fen);
        SearchLimits limits;
        //limits.movetime=15000;
        limits.depth = 13;
  	    Move best_move = engine.search(board, limits);
        return 0;
	}*/
    else {
        #include <iostream>
        std::ios::sync_with_stdio(false);
        std::cin.tie(nullptr);
        uci_loop();
        return 0;
    }
}
// position fen 8/8/8/8/7n/5k2/5b2/7K w - - 18 80 moves h1h2
// position fen 8/8/6K1/3k1P2/8/2b2n2/8/8 b - - 11 62
// position fen 8/8/6K1/3k1P2/8/2b2n2/8/8 b - - 11 62 moves d5e4 f5f6 f3e5 g6h7 e4f5 h7g8 f5g6 g8f8 g6f6 f8e8 c3a5 e8f8 e5f7 f8e8 f6e6 e8f8 a5c3 f8g8 e6e7 g8h7 e7f6 h7g8 c3b4 g8h7 b4a3 h7g8 f7e5 g8h7 a3f8 h7g8 f8c5 g8h7 c5f8 h7g8 f8c5 g8h7
// position fen 8/R7/1p6/p7/4n3/4k3/1r6/6K1 b - - 7 68