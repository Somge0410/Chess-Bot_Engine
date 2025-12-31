#pragma once
#include "board.h"
#include "Move.h"
#include <vector>
#include <map>
#include <exception>
#include <chrono>
#include <utility>
#include "MoveGenerator.h"
enum TTFlag {
    EXACT,
    LOWERBOUND,
    UPPERBOUND
};
struct TTEntry{
    double score;
    int depth;
    TTFlag flag;
    Move best_move;
};
struct PawnEvalEntry {
    uint64_t key;
	int pawn_structure_score;
    int king_safety_white;
    int king_safety_black;
    bool valid;
};
struct PerftRes {
    double duration;
    uint64_t nodes;
};
struct SearchLimits {
    int depth = -1;
    int movetime = -1;
    int wtime = -1;
	int btime = -1;
    int winc = 0;
    int binc = 0;
    int nodes = -1;
	int mate = -1;
    bool infinite = false;
};
class Engine {
    public:
        Move search(const Board& board, const SearchLimits& limits);
        PerftRes perft_test(Board& board, int depth);
        int checks_count;
        int ep_count;
        int capture_count;
        int checkmate_count;
    private:
        bool stop_search;
        std::pair<double,Move> negamax(Board & board, int depth, double alpha, double beta, int ply);
        double quiescence_search(Board& board, double alpha, double beta, int ply);
        Move best_move_this_iteration;
        std::map<uint64_t,TTEntry> transposition_table;
        Move killer_moves[128][2];
        int history_scores[2][6][64]={};
        std::chrono::steady_clock::time_point start_time;
        std::chrono::duration<double> time_limit;
        void sort_moves(std::vector<Move>& moves, const Board& board, int ply);
        int score_move(const Move& move, const Board& board, int ply);
        uint64_t perft_driver(Board& board, int depth, int orignal_depth);
};