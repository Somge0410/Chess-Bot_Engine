#pragma once

#include <string>
#include <vector>
#include <array>
#include "position.h"

#include "base.h" 

// 1. Define your parameters enum
// This keeps your indices perfectly organized.
struct PstNormEntry {
    int pst_start;
    int value_param;
};

class TuneEval
{
public:
    // We set this to true so we can pass our fixed 100-point pawns via the static score
    constexpr static bool includes_additional_score = false;

    // We set this to true because using Chess::Board directly is much faster than FEN parsing
    constexpr static bool supports_external_chess_eval = true;

    // Tuning configuration
    constexpr static bool retune_from_zero = false;  // Start with current values
    constexpr static double preferred_k = 0;  // Auto-determine K
    constexpr static int32_t max_epoch = 20000;  // Maximum epochs
    constexpr static bool enable_qsearch = false;  // No quiescence search
    constexpr static bool filter_in_check = false;  // Skip positions in check
    
    constexpr static double initial_learning_rate = 10;
    constexpr static int32_t learning_rate_drop_interval = 100;
    constexpr static double learning_rate_drop_ratio = 0.8;  // No LR drops
    
    constexpr static int32_t data_load_print_interval = 10000;

    static parameters_t get_initial_parameters();
    static EvalResult get_fen_eval_result(const std::string& fen);
    static EvalResult get_external_eval_result(const Position& position);
    static void print_parameters(parameters_t& parameters);
    static void write_parameters(const parameters_t& parameters, const std::string& path);
    static void normalize_pst(parameters_t& params);
};
