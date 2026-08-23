#pragma once

// Search pruning and reductions

// Futility pruning
inline int FUTILITY_MARGIN_D1 = 244;
inline int FUTILITY_MARGIN_D2 = 460;

// Reverse futility pruning (RFP)
inline int REVERSE_FUTILITY_MAX_DEPTH = 7;
inline int REVERSE_FUTILITY_MARGIN = 138;

// Null-move pruning (NMP)
inline int NMP_MIN_DEPTH = 5;
inline int NMP_REDUCTION = 2;
inline int NMP_DEPTH_DIVISOR = 6;
inline int NMP_EVAL_DIVISOR = 200;
inline int NMP_MAX_EVAL_REDUCTION = 2;

// ProbCut
inline int PROBCUT_MIN_DEPTH = 5;
inline int PROBCUT_REDUCTION = 4;
inline int PROBCUT_MARGIN = 150;

// Late-move reductions (LMR)
inline int LMR_MIN_DEPTH = 1;
inline int LMR_MIN_MOVES_SEARCHED = 3;
inline int LMR_REDUCTION_AMOUNT = 2;
inline double LOG_BASE = 0.209;
inline double LOG_DIV = 3.36;
inline double Q_LOG_BASE = 1.37;
inline double Q_LOG_DIV = 3.356;

// Quiescence search
inline int DELTA_MARGIN = 179;
inline int MAX_QUIET_PLY = 7;

// Move ordering and search heuristics

// Move-ordering stages
inline int TT_STAGE = 7;
inline int PROMO_STAGE = 6;
inline int MVV_LVA_STAGE = 5;
inline int KILLER_STAGE = 4;
inline int COUNTERMOVE_STAGE = 3;
inline int QUIET_STAGE = 2;
inline int LOSING_CAPTURE_STAGE = 1;
inline int CAPTURE_SCORE_TIEBREAK_DIVISOR = 16;

// History heuristic
inline int HISTORY_BONUS_MULTIPLIER = 8;  // bonus = depth * depth * multiplier

// Relevant pawn-push scoring
inline int PAWN_PUSH_SCORE1 = 40;
inline int PAWN_PUSH_SCORE2 = 20;
inline int PAWN_PUSH_SCORE3 = 50;
inline int PAWN_PUSH_SCORE4 = 160;
inline int PAWN_PUSH_SCORE5 = 25;
inline int PAWN_PUSH_SCORE6 = 60;

// Aspiration windows
inline int ASPIRATION_WINDOW_INITIAL = 45;
inline double ASPIRATION_WINDOW_MULTIPLIER = 2.662;

// Time management

// Base time allocation
inline int MOVE_OVERHEAD_MS = 50;
inline double MOVES_TO_GO = 40;
inline double MOVES_TO_GO_MG = 30;
inline double MOVES_TO_GO_EG = 33;
inline int MOVE_COUNT_THRESHOLD = 20;
inline double MIN_MOVES_TO_GO = 10;
inline double MAX_MOVES_TO_GO = 70;
inline double INC_USAGE_FACTOR = 0.9;
inline double MOVE_COUNT_WEIGHT = 0.1;
inline double MAX_MOVE_COUNT_REDUCTION = 8.0;
inline double REFERENCE_TIME = 300000.0;  // 5 minutes
inline double MAX_MULTIPLIER_FAST = 1.5;
inline double MAX_MULTIPLIER_SLOW = 2.5;

// Iterative-deepening stability and time extensions
constexpr int MAX_RECENT_BEST_COUNT = 5;
inline double TIME_CHANGES_COUNT_BIG = 0.1;
inline double TIME_CHANGES_COUNT_MEDIUM = 0.0404;
inline double TIME_CHANGES_COUNT_SMALL = 0.0017;
inline int DELTA_BEST_SCORE = 270;
inline double VOLATILITY_DIV = 202.38;
inline double EXTRA_BEST_BASE = 0.0731;
inline double EXTRA_BEST_FLIP = 0.0015;
inline double EXTRA_BEST_WEIGHT = 1.0081;
inline double TIME_MARGIN = 0.3916;

// Multi-threaded root search
inline int ROOT_PERTURBATION_MIN_HELPERS = 2;
inline int ROOT_PERTURBATION_MIN_BAND_SIZE = 6;
inline int ROOT_PERTURBATION_MAX_BAND_SIZE = 16;

