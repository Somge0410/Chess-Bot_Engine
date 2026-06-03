#pragma once

// For Pruning and search

const int FUTILITY_MARGIN_D1 = 200;
const int FUTILITY_MARGIN_D2 = 400;
const int DELTA_MARGIN = 200;
constexpr int MAX_QUIET_PLY = 7;

// --- NEU: Late Move Reduction (LMR) ---
constexpr int LMR_MIN_DEPTH = 3;
constexpr int LMR_MIN_MOVES_SEARCHED = 3;
constexpr int LMR_REDUCTION_AMOUNT = 2;

// --- NEU: Null Move Pruning (NMP) ---
constexpr int NMP_MIN_DEPTH = 3;
constexpr int NMP_REDUCTION = 3;

// --- NEU: Move Ordering ---

constexpr int TT_STAGE = 7;
constexpr int PROMO_STAGE = 6;
constexpr int MVV_LVA_STAGE = 5;
constexpr int KILLER_STAGE = 4;
constexpr int COUNTERMOVE_STAGE = 3;
constexpr int QUIET_STAGE = 2;
constexpr int LOSING_CAPTURE_STAGE = 1;

constexpr int CAPTURE_SCORE_TIEBREAK_DIVISOR = 16;

// --- NEU: History Heuristic ---
constexpr int HISTORY_BONUS_MULTIPLIER = 1;  // bonus = depth * depth * multiplier

// --- NEU: Aspiration Window ---
constexpr int ASPIRATION_WINDOW_INITIAL = 50;
constexpr int ASPIRATION_WINDOW_MULTIPLIER = 2;

// --- NEU: Time Management ---
constexpr int OPT_TIME_ALLOCATION_DIVISOR = 40;  // time_left / divisor
constexpr int OPT_TIME_ALLOCATION_DIVISOR_MG = 40;  // time_left / divisor
constexpr int OPT_TIME_ALLOCATION_DIVISOR_EG = 40;  // time_left / divisor
constexpr int MAX_TIME_ALLOCATION_DIVISOR = 40;// 20;
constexpr int MAX_TIME_ALLOCATION_DIVISOR_MG = 40;// 15;
constexpr int MAX_TIME_ALLOCATION_DIVISOR_EG = 40;// 25;
constexpr int MAX_RECENT_BEST_COUNT = 1; //6;
constexpr int TIME_CHANGES_COUNT_BIG = 0;//0.5;
constexpr int TIME_CHANGES_COUNT_MEDIUM = 0;// 0.3;
constexpr int TIME_CHANGES_COUNT_SMALL = 0;// 0.2;
constexpr int INCREMENT_DIVISOR = 1;
constexpr int DELTA_BEST_SCORE = -1; //50;
constexpr double VOLATILITY_DIV = 1;//250.0;
constexpr double EXTRA_BEST_BASE = 0; //0.08;
constexpr double EXTRA_BEST_FLIP = 0;//0.1;
constexpr double EXTRA_BEST_WEIGHT = 0; //0.22;
constexpr double TIME_MARGIN = 0;
constexpr int MAX_TIME_FRACTION = 2;  // max time = time_left / divisor

// --- NEU: Root Move Perturbation (Multi-Threading) ---
constexpr int ROOT_PERTURBATION_MIN_HELPERS = 2;
constexpr int ROOT_PERTURBATION_MIN_BAND_SIZE = 6;
constexpr int ROOT_PERTURBATION_MAX_BAND_SIZE = 16;
