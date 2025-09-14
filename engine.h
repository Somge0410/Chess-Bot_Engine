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

class Engine {
    public:
        Move search(const Board& board, int depth,int time_limit=100);
        void perft_test(Board& board, int depth);
        bool load_tt(const std::string& filename);
        bool save_tt(const std::string& filename) const;
        int checks_count;
        int ep_count;
        int capture_count;
        int checkmate_count;
        std::vector<Board> special_boards;

    private:
        bool stop_search;
        std::pair<double,Move> negamax(Board & board, int depth, double alpha, double beta, int ply);
        double quiescence_search(Board& board, double alpha, double beta, int ply);

        std::map<uint64_t,TTEntry> transposition_table;
        Move killer_moves[128][2];
        int history_scores[2][6][64]={};
        std::chrono::steady_clock::time_point start_time;
        std::chrono::duration<double> time_limit;
        void sort_moves(std::vector<Move>& moves, const Board& board, int ply);
        int score_move(const Move& move, const Board& board, int ply);
        uint64_t perft_driver(Board& board, int depth, int orignal_depth);
};