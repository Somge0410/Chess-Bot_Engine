#pragma once

//evaluation scorees
constexpr int BAD_BISHOP_BLCOKED_MG = -9; //Used in bad bishop eval
constexpr int BAD_BISHOP_BLOCKED_EG = -16; // same
constexpr int BAD_BISHOP_UNBLOCKED_MG = 2; // same
constexpr int BAD_BISHOP_UNBLOCKED_EG = -3; // same
constexpr int FIANCHETTO_BISHOP_BONUS_MG = 2; // used in fianchetto_bishop eval
constexpr int FIANCHETTO_BISHOP_BONUS_EG = 1; // same
constexpr int BROKEN_FIANCHETTO_PENALTY_MG = 7;// same
constexpr int BROKEN_FIANCHETTO_PENALTY_EG = -9; // same
constexpr int TRAPPED_BISHOP_PENALTY_MG = -125; // used in trapped minor pieces
constexpr int TRAPPED_BISHOP_PENALTY_EG = -77; // same
constexpr int TRAPPED_KNIGHT_PENALTY_MG = -23; // same
constexpr int TRAPPED_KNIGHT_PENALTY_EG = -66; // same
const int DOUBLED_PAWN_PENALTY_MG[8] = { 11,6,8,-2,-1,5,-13,-9 };// used in doubled pawn eval
constexpr int DOUBLED_PAWN_PENALTY_EG[8] = { -28,-25,-6,-14,-7,-8,-19,-15 }; //same
const int ISOLATED_PAWN_PENALTY_MG = -10; // used in isolated pawn eval
constexpr int ISOLATED_PAWN_PENALTY_EG = -9; // same
constexpr int BLOCKED_ISO_PENALTY_MG = -24;
constexpr int BLOCKED_ISO_PENALTY_EG = -4;

constexpr int FORWARD_BLOCKED_BACKWARD_MG = -9;
constexpr int FORWARD_BLOCKED_BACKWARD_EG = -14;
constexpr int FREE_TO_ADVANCE_BACKWARD_MG = -4;
constexpr int FREE_TO_ADVANCE_BACKWARD_EG = 0;
constexpr int FORWARD_CONTROLLED_BACKWARD_PENALTY_MG = -14; // used in backward eval
constexpr int FORWARD_CONTROLLED_BACKWARD_PENALTY_EG = -18;
const int PASSED_PAWN_BONUS_MG[8] = { 0,10,20,35,50,75,100,0 }; // used in is_passed_pawn eval, index = how many ranks the pawn has advanced (0-7)
constexpr int PASSED_PAWN_BONUS_EG[8] = { 0,10,20,35,50,75,100,0 }; //same
const int CANDIDATE_PASSED_PAWN_BONUS_MG[8] = { 0,5,10,15,25,35,50,0 }; //used in candidate eval, same index as above
constexpr int CANDIDATE_PASSED_PAWN_BONUS_EG[8] = { 0,5,10,15,25,35,50,0 };//same
constexpr double WEAK_PIECE_DEFENCE_VALUE_SCALE_MG[8] = { 0,0.1,0.2,0.3,0.4,0.5,0.6,0.7 };//used in eval defence
constexpr double WEAK_PIECE_DEFENCE_VALUE_SCALE_EG[8] = { 0,0.2,0.4,0.5,0.6,0.65,0.66,0.7 };//same
const int PAWN_SHIELD_BONUS_MG = -1; //used in king_safety
constexpr int PAWN_SHIELD_BONUS_EG = -3; //same
const int NEXT_TO_OPEN_FILE_PENALTY_MG = -41; //used in king safets
constexpr int NEXT_TO_OPEN_FILE_PENALTY_EG = 9; //same
constexpr int NEXT_TO_SEMI_OPEN_FILE_PENALTY_MG = -17;
constexpr int NEXT_TO_SEMI_OPEN_FILE_PENALTY_EG = -2;
constexpr int NEXT_TO_OPEN_DIAGONAL_PENALTY_MG[7] = { 3,1,32,45,82,60,60 };
constexpr int NEXT_TO_OPEN_DIAGONAL_PENALTY_EG[7] = { -29,-12,-7,-13,52,52,52 };
constexpr int PAWN_ATTACKS_IN_KING_ZONE_BONUS_MG = 10; //used in eval attack score
constexpr int PAWN_ATTACKS_IN_KING_ZONE_BONUS_EG = 20;
constexpr int PAWN_ATTACKS_IN_SMALL_KING_ZONE_BONUS_MG = 15;
constexpr int PAWN_ATTACKS_IN_SMALL_KING_ZONE_BONUS_EG = 30;
const int ROOK_OPEN_FILE_BONUS_MG = 27;// used in eval rook activity
const int ROOK_OPEN_FILE_BONUS_EG = -5;
const int ROOK_SEMI_OPEN_FILE_BONUS_MG = 10;
const int ROOK_SEMI_OPEN_FILE_BONUS_EG = -7;
constexpr int ROOK_BEHIND_FREE_PAWN_BONUS_MG = 8; // used in eval rook activity
constexpr int ROOK_BEHIND_FREE_PAWN_BONUS_EG = 13; // same
constexpr int CONNECTED_ROOKS_BONUS_MG = -2; // used in eval rook activity
constexpr int CONNECTED_ROOKS_BONUS_EG = 4; // same
constexpr int ROOK_ATTACKING_BONUS_MG = 0; // used in eval rook activity
constexpr int ROOK_ATTACKING_BONUS_EG = 0; // same
constexpr int KING_TROPISM_QUEEN_BONUS_MG[8] = { 0, 60,50,40,30,20,10,10 };// used in eval attack score
constexpr int KING_TROPISM_QUEEN_BONUS_EG[8] = { 0, 120,100,80,60,40,20,10 };
constexpr int KING_TROPISM_OTHER_BONUS_MG[8] = { 0, 30,20,15,10,5,0,10 };
constexpr int KING_TROPISM_OTHER_BONUS_EG[8] = { 0, 60,40,30,20,10,0,10 };
constexpr double KING_ATTACKER_VALUE_SCALE_MG[8] = { 0,0.2,0.4,0.6,0.8,0.9,0.95,0.99 };
constexpr double KING_ATTACKER_VALUE_SCALE_EG[8] = { 0,0.4,0.75,0.88,0.94,0.97,0.99,0.999 };
constexpr double WEAK_PIECE_ATTACK_BONUS_MG[8] = { 0,0.2,0.4,0.6,0.8,0.9,0.95,0.99 };
constexpr double WEAK_PIECE_ATTACK_BONUS_EG[8] = { 0,0.4,0.75,0.88,0.94,0.97,0.99,0.999 };
constexpr int ROOK_ON_OPEN_FILE_NEXT_TO_KING_BONUS_MG = 20;
constexpr int ROOK_ON_OPEN_FILE_NEXT_TO_KING_BONUS_EG = 40;
constexpr int QUEEN_ON_OPEN_FILE_NEXT_TO_KING_BONUS_MG = 40;
constexpr int QUEEN_ON_OPEN_FILE_NEXT_TO_KING_BONUS_EG = 80;
constexpr int ROOK_ON_SEMI_OPEN_FILE_NEXT_TO_KING_BONUS_MG = 10;
constexpr int ROOK_ON_SEMI_OPEN_FILE_NEXT_TO_KING_BONUS_EG = 20;
constexpr int QUEEN_ON_SEMI_OPEN_FILE_NEXT_TO_KING_BONUS_MG = 20;
constexpr int QUEEN_ON_SEMI_OPEN_FILE_NEXT_TO_KING_BONUS_EG = 40;
const int BISHOP_PAIR_BONUS_MG = 14; //used in eval bishop pair
const int BISHOP_PAIR_BONUS_EG = 59;
const int PAWN_ISLAND_PENALTY_MG = -10; // used in eval pawn island
constexpr int PAWN_ISLAND_PENALTY_EG = -20;
const int PAWN_MAJORITY_BONUS_MG = 15; // used in eval pawn maj
constexpr int PAWN_MAJORITY_BONUS_EG = 30;
constexpr int UNDEFENDED_PIECE_PENALTY_MG[5] = { 0,-10,-10,-15,-25 }; // used in eval defence, index = piece type (knight, bishop, rook, queen)
constexpr int UNDEFENDED_PIECE_PENALTY_EG[5] = { 0,-20,-20,-30,-50 };
constexpr int HANGING_PIECE_PENALTY_MG[5] = { 0,-20,-20,-30,-50 }; // same as above
constexpr int HANGING_PIECE_PENALTY_EG[5] = { 0,-40,-40,-60,-100 };
/*const int PIECE_VALUES_MG[6] = {101,548,577,758,1519,20000};
const int PIECE_VALUES_EG[6] = { 164,762,772,1312,2608,20000 };*/
//Knight: mg = 282.54, eg = 402.088
//Bishop: mg = 315.684, eg = 414.115
//Rook : mg = 408.697, eg = 702.447
////Queen : mg = 815.63, eg = 1358.21
//Both
const int PIECE_VALUES_MG[6] = { 100,382,418,508,1107,20000 };
const int PIECE_VALUES_EG[6] = { 100,283,295,502,953,20000 };
////lichess quiet
//const int PIECE_VALUES_MG[6] = { 100,383,423,529,1135,20000 };
//const int PIECE_VALUES_EG[6] = { 100,296,306,510,953,20000 };
////lcihess big
//const int PIECE_VALUES_MG[6] = { 100,383,418,500,1092,20000 };
//const int PIECE_VALUES_EG[6] = { 100,280,293,499,947,20000 };

// --- NEU: Mobility Bonuses ---
constexpr int MOBILITY_BONUS_MG[6] = { 0, 2, 3, 3, 0, 0 }; // Pawn, Knight, Bishop, Rook, Queen, King
constexpr int MOBILITY_BONUS_EG[6] = { 0, 3, 2, 3, 5, 0 }; // Pawn, Knight, Bishop, Rook, Queen, King

// --- NEU: Outpost Bonuses ---
constexpr int BISHOP_OUTPOST_BONUS_WITH_OPPOSITE_BISHOP_MG = 21;
constexpr int BISHOP_OUTPOST_BONUS_WITH_OPPOSITE_BISHOP_EG = 12;
constexpr int BISHOP_OUTPOST_BONUS_NO_OPPOSITE_BISHOP_MG = 32;
constexpr int BISHOP_OUTPOST_BONUS_NO_OPPOSITE_BISHOP_EG = 3;
constexpr int KNIGHT_OUTPOST_BONUS_WITH_OPPOSITE_BISHOP_MG = 24;
constexpr int KNIGHT_OUTPOST_BONUS_WITH_OPPOSITE_BISHOP_EG = 14;
constexpr int KNIGHT_OUTPOST_BONUS_NO_OPPOSITE_BISHOP_MG = 30;
constexpr int KNIGHT_OUTPOST_BONUS_NO_OPPOSITE_BISHOP_EG = 11;



// --- NEU: King Zone Attack Weights ---
constexpr int KNIGHT_ATTACK_VALUE = 20;
constexpr int BISHOP_ATTACK_VALUE = 20;
constexpr int ROOK_ATTACK_VALUE = 40;
constexpr int QUEEN_ATTACK_VALUE = 80;


// For Pruning and search

const int FUTILITY_MARGIN_D1 = 200;
const int FUTILITY_MARGIN_D2 = 400;
const int DELTA_MARGIN = 200;
constexpr int DEFAULT_SEARCH_WINDOW = 50;
constexpr int MAX_QUIET_PLY = 7;

// --- NEU: Late Move Reduction (LMR) ---
constexpr int LMR_MIN_DEPTH = 3;
constexpr int LMR_MIN_MOVES_SEARCHED = 3;
constexpr int LMR_REDUCTION_AMOUNT = 2;

// --- NEU: Null Move Pruning (NMP) ---
constexpr int NMP_MIN_DEPTH = 3;
constexpr int NMP_REDUCTION = 3;

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
