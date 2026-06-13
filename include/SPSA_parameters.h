#pragma once

// For Pruning and search
inline int FUTILITY_MARGIN_D1 = 204;
inline int FUTILITY_MARGIN_D2 = 434;
inline int DELTA_MARGIN = 180;
inline int MAX_QUIET_PLY = 7;

// --- NEU: Late Move Reduction (LMR) ---
inline int LMR_MIN_DEPTH = 1;
inline int LMR_MIN_MOVES_SEARCHED = 3;
inline int LMR_REDUCTION_AMOUNT = 2;

// --- NEU: Null Move Pruning (NMP) ---
inline int NMP_MIN_DEPTH = 5;
inline int NMP_REDUCTION = 1;

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
inline int HISTORY_BONUS_MULTIPLIER = 6;  // bonus = depth * depth * multiplier

// --- NEU: Aspiration Window ---
inline int ASPIRATION_WINDOW_INITIAL = 52;
inline double ASPIRATION_WINDOW_MULTIPLIER = 2.7;

// --- NEU: Time Management ---
inline double OPT_TIME_ALLOCATION_DIVISOR = 42.374;     // time_left / divisor
inline double OPT_TIME_ALLOCATION_DIVISOR_MG = 34.3507;  // time_left / divisor
inline double OPT_TIME_ALLOCATION_DIVISOR_EG = 37.0596;  // time_left / divisor
inline double MAX_TIME_ALLOCATION_DIVISOR = 29.2642;
inline double MAX_TIME_ALLOCATION_DIVISOR_MG = 30.4544;
inline double MAX_TIME_ALLOCATION_DIVISOR_EG = 28.8201;
constexpr int MAX_RECENT_BEST_COUNT = 5;
inline double TIME_CHANGES_COUNT_BIG = 0.1;
inline double TIME_CHANGES_COUNT_MEDIUM = 0.0497 ;
inline double TIME_CHANGES_COUNT_SMALL = 0.006;
inline double INCREMENT_DIVISOR = 1.13;
inline int DELTA_BEST_SCORE = 284;
inline double VOLATILITY_DIV = 267;
inline double EXTRA_BEST_BASE = 0.0541;
inline double EXTRA_BEST_FLIP = 0.0823;
inline double EXTRA_BEST_WEIGHT = 0.998;
inline double NO_TIME_TRIGGER_DIV = 1.9373;
inline double NO_TIME_ALLOC_DIV = 40;
inline double MAX_NO_TIME_ALLOC_DIV = 40;
inline double TIME_MARGIN = 0.3942;
inline double LOG_BASE = 0.206;
inline double LOG_DIV = 3.3591;
inline double Q_LOG_BASE = 1.36;
inline double Q_LOG_DIV = 3.356;

// --- NEU: Root Move Perturbation (Multi-Threading) ---
inline int ROOT_PERTURBATION_MIN_HELPERS = 2;
inline int ROOT_PERTURBATION_MIN_BAND_SIZE = 6;
inline int ROOT_PERTURBATION_MAX_BAND_SIZE = 16;

inline int REVERSE_FUTILITY_MAX_DEPTH = 6;
inline int REVERSE_FUTILITY_MARGIN = 137;

inline int PAWN_PUSH_SCORE1 = 40;
inline int PAWN_PUSH_SCORE2 = 20;
inline int PAWN_PUSH_SCORE3 = 50;
inline int PAWN_PUSH_SCORE4 = 160;
inline int PAWN_PUSH_SCORE5 = 25;
inline int PAWN_PUSH_SCORE6 = 60;

