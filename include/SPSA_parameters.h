#pragma once

// For Pruning and search
inline int FUTILITY_MARGIN_D1 = 200;
inline int FUTILITY_MARGIN_D2 = 400;
inline int DELTA_MARGIN = 200;
inline int MAX_QUIET_PLY = 7;

// --- NEU: Late Move Reduction (LMR) ---
inline int LMR_MIN_DEPTH = 3;
inline int LMR_MIN_MOVES_SEARCHED = 3;
inline int LMR_REDUCTION_AMOUNT = 2;

// --- NEU: Null Move Pruning (NMP) ---
inline int NMP_MIN_DEPTH = 3;
inline int NMP_REDUCTION = 3;

// --- NEU: Move Ordering ---
inline int TT_STAGE = 7;
inline int PROMO_STAGE = 6;
inline int MVV_LVA_STAGE = 5;
inline int KILLER_STAGE = 4;
inline int COUNTERMOVE_STAGE = 3;
inline int QUIET_STAGE = 2;
inline int LOSING_CAPTURE_STAGE = 1;

inline int CAPTURE_SCORE_TIEBREAK_DIVISOR = 16;

// --- NEU: History Heuristic ---
inline int HISTORY_BONUS_MULTIPLIER = 1;  // bonus = depth * depth * multiplier

// --- NEU: Aspiration Window ---
inline int ASPIRATION_WINDOW_INITIAL = 50;
inline double ASPIRATION_WINDOW_MULTIPLIER = 2;

// --- NEU: Time Management ---
inline double OPT_TIME_ALLOCATION_DIVISOR = 40;     // time_left / divisor
inline double OPT_TIME_ALLOCATION_DIVISOR_MG = 40;  // time_left / divisor
inline double OPT_TIME_ALLOCATION_DIVISOR_EG = 40;  // time_left / divisor
inline double MAX_TIME_ALLOCATION_DIVISOR = 30;
inline double MAX_TIME_ALLOCATION_DIVISOR_MG = 30;
inline double MAX_TIME_ALLOCATION_DIVISOR_EG = 30;
constexpr int MAX_RECENT_BEST_COUNT = 5;
inline double TIME_CHANGES_COUNT_BIG = 0.1;
inline double TIME_CHANGES_COUNT_MEDIUM = 0.05 ;
inline double TIME_CHANGES_COUNT_SMALL = 0.01;
inline double INCREMENT_DIVISOR = 1.1;
inline int DELTA_BEST_SCORE = 300;
inline double VOLATILITY_DIV = 250;
inline double EXTRA_BEST_BASE = 0.05;
inline double EXTRA_BEST_FLIP = 0.1;
inline double EXTRA_BEST_WEIGHT = 1;
inline double NO_TIME_TRIGGER_DIV = 2;
inline double NO_TIME_ALLOC_DIV = 40;
inline double MAX_NO_TIME_ALLOC_DIV = 40;
inline double TIME_MARGIN = 0.5;
inline double LOG_BASE = 0.2;
inline double LOG_DIV = 3.35;
inline double Q_LOG_BASE = 1.35;
inline double Q_LOG_DIV = 3.35;

// --- NEU: Root Move Perturbation (Multi-Threading) ---
inline int ROOT_PERTURBATION_MIN_HELPERS = 2;
inline int ROOT_PERTURBATION_MIN_BAND_SIZE = 6;
inline int ROOT_PERTURBATION_MAX_BAND_SIZE = 16;

inline int REVERSE_FUTILITY_MAX_DEPTH = 5;
inline int REVERSE_FUTILITY_MARGIN = 112;

inline int PAWN_PUSH_SCORE1 = 40;
inline int PAWN_PUSH_SCORE2 = 20;
inline int PAWN_PUSH_SCORE3 = 50;
inline int PAWN_PUSH_SCORE4 = 160;
inline int PAWN_PUSH_SCORE5 = 25;
inline int PAWN_PUSH_SCORE6 = 60;

