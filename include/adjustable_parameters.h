#pragma once

//evaluation scorees
constexpr int BAD_BISHOP_BLCOKED_MG = -5; //Used in bad bishop eval
constexpr int BAD_BISHOP_BLOCKED_EG = -11; // same
constexpr int BAD_BISHOP_UNBLOCKED_MG = 0; // same
constexpr int BAD_BISHOP_UNBLOCKED_EG = -3; // same
constexpr int FIANCHETTO_BISHOP_BONUS_MG = 23; // used in fianchetto_bishop eval
constexpr int FIANCHETTO_BISHOP_BONUS_EG = 27; // same
constexpr int BROKEN_FIANCHETTO_PENALTY_MG = 17;// same
constexpr int BROKEN_FIANCHETTO_PENALTY_EG = 22; // same
constexpr int TRAPPED_BISHOP_PENALTY_MG = -107; // used in trapped minor pieces
constexpr int TRAPPED_BISHOP_PENALTY_EG = -71; // same
constexpr int TRAPPED_KNIGHT_PENALTY_MG = -50; // same
constexpr int TRAPPED_KNIGHT_PENALTY_EG = -86; // same
const int DOUBLED_PAWN_PENALTY_MG[8] = { -8, 10, 0, 1, -7, 0, 14, 4 };// used in doubled pawn eval
constexpr int DOUBLED_PAWN_PENALTY_EG[8] = { -25, -14, -17, -16, -10, -19, -16, -31 }; //same
const int ISOLATED_PAWN_PENALTY_MG = -11; // used in isolated pawn eval
constexpr int ISOLATED_PAWN_PENALTY_EG = -4; // same
constexpr int BLOCKED_ISO_PENALTY_MG = -10;
constexpr int BLOCKED_ISO_PENALTY_EG = 3;
constexpr int PROTECTED_PASSED_PAWN_BONUS_MG = 10;
constexpr int PROTECTED_PASSED_PAWN_BONUS_EG = -14;
constexpr int BLOCKED_PASSED_PENALTY_MG = -3;
constexpr int BLOCKED_PASSED_PENALTY_EG = -16;
constexpr int OP_KING_DISTANCE_BONUS_MG[8] = { 0, 1, -7, 0, 14, 4, -30, -70 };
constexpr int OP_KING_DISTANCE_BONUS_EG[8] = { -17, -16, -10, -19, -16, -31, 6, 6 };
constexpr int OWN_KING_DISTANCE_BONUS_MG[8] = { 0, 11, -2, -3, 8, 16, 52, 39 };
constexpr int OWN_KING_DISTANCE_BONUS_EG[8] = { 0, 19, 15, 10, 7, 6, 3, 3 };

constexpr int FORWARD_BLOCKED_BACKWARD_MG = -6;
constexpr int FORWARD_BLOCKED_BACKWARD_EG = -10;
constexpr int FREE_TO_ADVANCE_BACKWARD_MG = -3;
constexpr int FREE_TO_ADVANCE_BACKWARD_EG = -2;
constexpr int FORWARD_CONTROLLED_BACKWARD_PENALTY_MG = -8; // used in backward eval
constexpr int FORWARD_CONTROLLED_BACKWARD_PENALTY_EG = -12;
const int PASSED_PAWN_BONUS_MG[8] = { 0,10,20,35,50,75,100,0 }; // used in is_passed_pawn eval, index = how many ranks the pawn has advanced (0-7)
constexpr int PASSED_PAWN_BONUS_EG[8] = { 0,10,20,35,50,75,100,0 }; //same
const int CANDIDATE_PASSED_PAWN_BONUS_MG[8] = { 0,5,10,15,25,35,50,0 }; //used in candidate eval, same index as above
constexpr int CANDIDATE_PASSED_PAWN_BONUS_EG[8] = { 0,5,10,15,25,35,50,0 };//same
constexpr double WEAK_PIECE_DEFENCE_VALUE_SCALE_MG[8] = { 0,0.1,0.2,0.3,0.4,0.5,0.6,0.7 };//used in eval defence
constexpr double WEAK_PIECE_DEFENCE_VALUE_SCALE_EG[8] = { 0,0.2,0.4,0.5,0.6,0.65,0.66,0.7 };//same
const int PAWN_SHIELD_BONUS_MG = 17; //used in king_safety
constexpr int PAWN_SHIELD_BONUS_EG = -7; //same
const int NEXT_TO_OPEN_FILE_PENALTY_MG = -31; //used in king safets
constexpr int NEXT_TO_OPEN_FILE_PENALTY_EG = -2; //same
constexpr int NEXT_TO_SEMI_OPEN_FILE_PENALTY_MG = -22;
constexpr int NEXT_TO_SEMI_OPEN_FILE_PENALTY_EG = 12;
constexpr int NEXT_TO_OPEN_DIAGONAL_PENALTY_MG[7] = { -5, -3, 14, 26, 80, 86, 86 };
constexpr int NEXT_TO_OPEN_DIAGONAL_PENALTY_EG[7] = { -24, -11, -4, -9, 29, 10, 10 };
constexpr int PAWN_ATTACKS_IN_KING_ZONE_BONUS_MG = 15; //used in eval attack score
constexpr int PAWN_ATTACKS_IN_KING_ZONE_BONUS_EG = 3;
constexpr int PAWN_ATTACKS_IN_SMALL_KING_ZONE_BONUS_MG = 21;
constexpr int PAWN_ATTACKS_IN_SMALL_KING_ZONE_BONUS_EG = 6;
constexpr int KING_SAFETY_TABLE_MG[25] = { 0, -15, -30, -45, -58, -73, -87, -103, -116, -133, -146, -160, -175, -190, -205, -220, -236, -248, -263, -277, -293, -309, -323, -337, -352 };
constexpr int KING_SAFETY_TABLE_EG[25] = { 0, -2, -3, -6, -10, -12, -16, -21, -26, -30, -37, -43, -50, -57, -64, -73, -83, -91, -101, -112, -122, -133, -144, -157, -171 };
const int ROOK_OPEN_FILE_BONUS_MG = 36;// used in eval rook activity
const int ROOK_OPEN_FILE_BONUS_EG = 6;
const int ROOK_SEMI_OPEN_FILE_BONUS_MG = 15;
const int ROOK_SEMI_OPEN_FILE_BONUS_EG = 8;
constexpr int ROOK_BEHIND_FREE_PAWN_BONUS_MG = 17; // used in eval rook activity
constexpr int ROOK_BEHIND_FREE_PAWN_BONUS_EG = -1; // same
constexpr int CONNECTED_ROOKS_BONUS_MG = 1; // used in eval rook activity
constexpr int CONNECTED_ROOKS_BONUS_EG = 0; // same
constexpr int ROOK_ATTACKING_BONUS_MG = 0; // used in eval rook activity
constexpr int ROOK_ATTACKING_BONUS_EG = 0; // same
constexpr int KING_TROPISM_QUEEN_BONUS_MG[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };// used in eval attack score
constexpr int KING_TROPISM_QUEEN_BONUS_EG[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
constexpr int KING_TROPISM_OTHER_BONUS_MG[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
constexpr int KING_TROPISM_OTHER_BONUS_EG[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
constexpr double KING_ATTACKER_VALUE_SCALE_MG[8] = { 0, 0.282843, 0.565686, 0.84853, 1.13138, 1.27279, 1.34351, 1.40008 };
constexpr double KING_ATTACKER_VALUE_SCALE_EG[8] = { 0, 0.0745113, 0.139708, 0.163924, 0.175101, 0.180689, 0.184415, 0.186093 };
constexpr double WEAK_PIECE_ATTACK_BONUS_MG[8] = { 0, 0.282843, 0.565686, 0.84853, 1.13138, 1.27279, 1.34351, 1.40008 };
constexpr double WEAK_PIECE_ATTACK_BONUS_EG[8] = { 0, 0.0745113, 0.139708, 0.163924, 0.175101, 0.180689, 0.184415, 0.186093 };
constexpr int ROOK_ON_OPEN_FILE_NEXT_TO_KING_BONUS_MG = 28;
constexpr int ROOK_ON_OPEN_FILE_NEXT_TO_KING_BONUS_EG = 7;
constexpr int QUEEN_ON_OPEN_FILE_NEXT_TO_KING_BONUS_MG = 56;
constexpr int QUEEN_ON_OPEN_FILE_NEXT_TO_KING_BONUS_EG = 15;
constexpr int ROOK_ON_SEMI_OPEN_FILE_NEXT_TO_KING_BONUS_MG = 15;
constexpr int ROOK_ON_SEMI_OPEN_FILE_NEXT_TO_KING_BONUS_EG = 3;
constexpr int QUEEN_ON_SEMI_OPEN_FILE_NEXT_TO_KING_BONUS_MG = 28;
constexpr int QUEEN_ON_SEMI_OPEN_FILE_NEXT_TO_KING_BONUS_EG = 7;
const int BISHOP_PAIR_BONUS_MG = 22; //used in eval bishop pair
const int BISHOP_PAIR_BONUS_EG = 52;
const int PAWN_ISLAND_PENALTY_MG = -10; // used in eval pawn island
constexpr int PAWN_ISLAND_PENALTY_EG = -20;
const int PAWN_MAJORITY_BONUS_MG = 15; // used in eval pawn maj
constexpr int PAWN_MAJORITY_BONUS_EG = 30;
constexpr int UNDEFENDED_PIECE_PENALTY_MG[5] = { 0, -15, -15, -21, -36 }; // used in eval defence, index = piece type (knight, bishop, rook, queen)
constexpr int UNDEFENDED_PIECE_PENALTY_EG[5] = { 0, -3, -3, -6, -10 };
constexpr int HANGING_PIECE_PENALTY_MG[5] = { 0, -28, -28, -43, -70 }; // same as above
constexpr int HANGING_PIECE_PENALTY_EG[5] = { 0, -7, -7, -10, -18 };
const int PIECE_VALUES_MG[6] = { 100, 340, 280, 334, 782, 20000 };
const int PIECE_VALUES_EG[6] = { 100, 255, 242, 416, 889, 20000 };

// --- NEU: Mobility Bonuses ---
constexpr int MOBILITY_BONUS_MG[6] = { 0, 0, 5, 2, 1, 0 }; // Pawn, Knight, Bishop, Rook, Queen, King
constexpr int MOBILITY_BONUS_EG[6] = { 0, -1, 3, 2, 4, 0 }; // Pawn, Knight, Bishop, Rook, Queen, King

// --- NEU: Outpost Bonuses ---
constexpr int BISHOP_OUTPOST_BONUS_WITH_OPPOSITE_BISHOP_MG = 33;
constexpr int BISHOP_OUTPOST_BONUS_WITH_OPPOSITE_BISHOP_EG = 14;
constexpr int BISHOP_OUTPOST_BONUS_NO_OPPOSITE_BISHOP_MG = 51;
constexpr int BISHOP_OUTPOST_BONUS_NO_OPPOSITE_BISHOP_EG = 3;
constexpr int KNIGHT_OUTPOST_BONUS_WITH_OPPOSITE_BISHOP_MG = 31;
constexpr int KNIGHT_OUTPOST_BONUS_WITH_OPPOSITE_BISHOP_EG = 15;
constexpr int KNIGHT_OUTPOST_BONUS_NO_OPPOSITE_BISHOP_MG = 39;
constexpr int KNIGHT_OUTPOST_BONUS_NO_OPPOSITE_BISHOP_EG = 14;



// --- NEU: King Zone Attack Weights ---
constexpr int KNIGHT_ATTACK_VALUE = 20;
constexpr int BISHOP_ATTACK_VALUE = 20;
constexpr int ROOK_ATTACK_VALUE = 40;
constexpr int QUEEN_ATTACK_VALUE = 80;


// For Pruning and search

inline int FUTILITY_MARGIN_D1 = 200;
inline int FUTILITY_MARGIN_D2 = 400;
inline int DELTA_MARGIN = 200;
inline int DEFAULT_SEARCH_WINDOW = 50;
inline int MAX_QUIET_PLY = 7;

// --- NEU: Late Move Reduction (LMR) ---
constexpr int LMR_MIN_DEPTH = 3;
constexpr int LMR_MIN_MOVES_SEARCHED = 3;
constexpr int LMR_REDUCTION_AMOUNT = 2;

// --- NEU: Null Move Pruning (NMP) ---
constexpr int NMP_MIN_DEPTH = 3;
constexpr int NMP_REDUCTION = 3;
// --- NEU: Reverse Futility Pruning (RFP) ---
inline int REVERSE_FUTILITY_MAX_DEPTH = 5;
inline int REVERSE_FUTILITY_MARGIN = 125;
// --- NEU: Move Ordering ---
constexpr int CAPTURE_SCORE_TIEBREAK_DIVISOR = 16;
constexpr int MVV_LVA_STAGE = 4;
constexpr int LOSING_CAPTURE_STAGE = 1;
constexpr int KILLER_STAGE = 3;
constexpr int QUIET_STAGE = 2;
constexpr int TT_STAGE = 6;
constexpr int PROMO_STAGE = 5;

// --- NEU: History Heuristic ---
constexpr int HISTORY_BONUS_MULTIPLIER = 1;  // bonus = depth * depth * multiplier

// --- NEU: Aspiration Window ---
constexpr int ASPIRATION_WINDOW_INITIAL = 50;
constexpr int ASPIRATION_WINDOW_MULTIPLIER = 2;

// --- NEU: Time Management ---
constexpr int TIME_ALLOCATION_DIVISOR = 40;  // time_left / divisor
constexpr int MAX_TIME_FRACTION = 2;  // max time = time_left / divisor

// --- NEU: Root Move Perturbation (Multi-Threading) ---
constexpr int ROOT_PERTURBATION_MIN_HELPERS = 2;
constexpr int ROOT_PERTURBATION_MIN_BAND_SIZE = 6;
constexpr int ROOT_PERTURBATION_MAX_BAND_SIZE = 16;
