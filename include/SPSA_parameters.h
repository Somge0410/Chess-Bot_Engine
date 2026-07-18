#pragma once

// For Pruning and search
inline int FUTILITY_MARGIN_D1 = 244;
inline int FUTILITY_MARGIN_D2 = 460;
inline int DELTA_MARGIN = 179;
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
inline int HISTORY_BONUS_MULTIPLIER = 8;  // bonus = depth * depth * multiplier

// --- NEU: Aspiration Window ---
inline int ASPIRATION_WINDOW_INITIAL = 45;
inline double ASPIRATION_WINDOW_MULTIPLIER = 2.662;

// --- NEU: Time Management ---
inline double OPT_TIME_ALLOCATION_DIVISOR = 33.69;     // time_left / divisor
inline double OPT_TIME_ALLOCATION_DIVISOR_MG = 12.9;  // time_left / divisor
inline double OPT_TIME_ALLOCATION_DIVISOR_EG = 29.6;  // time_left / divisor
inline double MAX_TIME_ALLOCATION_DIVISOR = 28.7;
inline double MAX_TIME_ALLOCATION_DIVISOR_MG = 32.27;
inline double MAX_TIME_ALLOCATION_DIVISOR_EG = 28.5;
constexpr int MAX_RECENT_BEST_COUNT = 5;
inline double TIME_CHANGES_COUNT_BIG = 0.1;
inline double TIME_CHANGES_COUNT_MEDIUM = 0.0404;
inline double TIME_CHANGES_COUNT_SMALL = 0.0017;
inline double INCREMENT_DIVISOR = 1.1238;
inline int DELTA_BEST_SCORE = 270;
inline double VOLATILITY_DIV = 202.38;
inline double EXTRA_BEST_BASE = 0.0731;
inline double EXTRA_BEST_FLIP = 0.0015;
inline double EXTRA_BEST_WEIGHT = 1.0081;
inline double NO_TIME_TRIGGER_DIV = 2.01;
inline double NO_TIME_ALLOC_DIV = 42.9;
inline double MAX_NO_TIME_ALLOC_DIV = 40.34;
inline double TIME_MARGIN = 0.3916;
inline double LOG_BASE = 0.209;
inline double LOG_DIV = 3.36;
inline double Q_LOG_BASE = 1.37;
inline double Q_LOG_DIV = 3.356;

// --- NEU: Root Move Perturbation (Multi-Threading) ---
inline int ROOT_PERTURBATION_MIN_HELPERS = 2;
inline int ROOT_PERTURBATION_MIN_BAND_SIZE = 6;
inline int ROOT_PERTURBATION_MAX_BAND_SIZE = 16;

inline int REVERSE_FUTILITY_MAX_DEPTH = 7;
inline int REVERSE_FUTILITY_MARGIN = 138;

inline int PAWN_PUSH_SCORE1 = 40;
inline int PAWN_PUSH_SCORE2 = 20;
inline int PAWN_PUSH_SCORE3 = 50;
inline int PAWN_PUSH_SCORE4 = 160;
inline int PAWN_PUSH_SCORE5 = 25;
inline int PAWN_PUSH_SCORE6 = 60;

