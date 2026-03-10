#pragma once

//evaluation scorees
constexpr int BAD_BISHOP_BLCOKED_MG = -5; //Used in bad bishop eval
constexpr int BAD_BISHOP_BLOCKED_EG = -12; // same
constexpr int BAD_BISHOP_UNBLOCKED_MG = -3; // same
constexpr int BAD_BISHOP_UNBLOCKED_EG = 0; // same
constexpr int FIANCHETTO_BISHOP_BONUS_MG = 1; // used in fianchetto_bishop eval
constexpr int FIANCHETTO_BISHOP_BONUS_EG = 8; // same
constexpr int BROKEN_FIANCHETTO_PENALTY_MG = -6;// same
constexpr int BROKEN_FIANCHETTO_PENALTY_EG = 2; // same
constexpr int TRAPPED_BISHOP_PENALTY_MG = -123; // used in trapped minor pieces
constexpr int TRAPPED_BISHOP_PENALTY_EG = -73; // same
constexpr int TRAPPED_KNIGHT_PENALTY_MG = -56; // same
constexpr int TRAPPED_KNIGHT_PENALTY_EG = -81; // same
const int DOUBLED_PAWN_PENALTY_MG[8] = { -11,0,-4,0,-9,-3,3,-6};// used in doubled pawn eval
constexpr int DOUBLED_PAWN_PENALTY_EG[8] = { -35,-22,-25,-22,15,-28,25,-38 }; //same
const int ISOLATED_PAWN_PENALTY_MG = -10; // used in isolated pawn eval
constexpr int ISOLATED_PAWN_PENALTY_EG = -8; // same
constexpr int BLOCKED_ISO_PENALTY_MG = -25;
constexpr int BLOCKED_ISO_PENALTY_EG = -3;

constexpr int FORWARD_BLOCKED_BACKWARD_MG = -17;
constexpr int FORWARD_BLOCKED_BACKWARD_EG = -13;
constexpr int FREE_TO_ADVANCE_BACKWARD_MG = -11;
constexpr int FREE_TO_ADVANCE_BACKWARD_EG = -3;
constexpr int FORWARD_CONTROLLED_BACKWARD_PENALTY_MG = -23; // used in backward eval
constexpr int FORWARD_CONTROLLED_BACKWARD_PENALTY_EG = -18;
const int PASSED_PAWN_BONUS_MG[8] = { 0,10,20,35,50,75,100,0 }; // used in is_passed_pawn eval, index = how many ranks the pawn has advanced (0-7)
constexpr int PASSED_PAWN_BONUS_EG[8] = { 0,10,20,35,50,75,100,0 }; //same
const int CANDIDATE_PASSED_PAWN_BONUS_MG[8] = { 0,5,10,15,25,35,50,0 }; //used in candidate eval, same index as above
constexpr int CANDIDATE_PASSED_PAWN_BONUS_EG[8] = { 0,5,10,15,25,35,50,0 };//same
constexpr double WEAK_PIECE_DEFENCE_VALUE_SCALE_MG[8] = { 0,0.1,0.2,0.3,0.4,0.5,0.6,0.7 };//used in eval defence
constexpr double WEAK_PIECE_DEFENCE_VALUE_SCALE_EG[8] = { 0,0.2,0.4,0.5,0.6,0.65,0.66,0.7 };//same
const int PAWN_SHIELD_BONUS_MG = 19; //used in king_safety
constexpr int PAWN_SHIELD_BONUS_EG = -7; //same
const int NEXT_TO_OPEN_FILE_PENALTY_MG = -3; //used in king safets
constexpr int NEXT_TO_OPEN_FILE_PENALTY_EG = -34; //same
constexpr int NEXT_TO_SEMI_OPEN_FILE_PENALTY_MG = -25;
constexpr int NEXT_TO_SEMI_OPEN_FILE_PENALTY_EG = 16;
constexpr int NEXT_TO_OPEN_DIAGONAL_PENALTY_MG[7] = { -5,-3,16,28,74,62,62 };
constexpr int NEXT_TO_OPEN_DIAGONAL_PENALTY_EG[7] = { -23,-11,-5,-7,49,40,40 };
constexpr int PAWN_ATTACKS_IN_KING_ZONE_BONUS_MG = 10; //used in eval attack score
constexpr int PAWN_ATTACKS_IN_KING_ZONE_BONUS_EG = 20;
constexpr int PAWN_ATTACKS_IN_SMALL_KING_ZONE_BONUS_MG = 15;
constexpr int PAWN_ATTACKS_IN_SMALL_KING_ZONE_BONUS_EG = 30;
const int ROOK_OPEN_FILE_BONUS_MG = 39;// used in eval rook activity
const int ROOK_OPEN_FILE_BONUS_EG = 11;
const int ROOK_SEMI_OPEN_FILE_BONUS_MG = 17;
const int ROOK_SEMI_OPEN_FILE_BONUS_EG = 10;
constexpr int ROOK_BEHIND_FREE_PAWN_BONUS_MG = 18; // used in eval rook activity
constexpr int ROOK_BEHIND_FREE_PAWN_BONUS_EG = 14; // same
constexpr int CONNECTED_ROOKS_BONUS_MG = 1; // used in eval rook activity
constexpr int CONNECTED_ROOKS_BONUS_EG = 0; // same
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
const int BISHOP_PAIR_BONUS_MG = 23; //used in eval bishop pair
const int BISHOP_PAIR_BONUS_EG = 51;
const int PAWN_ISLAND_PENALTY_MG = -10; // used in eval pawn island
constexpr int PAWN_ISLAND_PENALTY_EG = -20;
const int PAWN_MAJORITY_BONUS_MG = 15; // used in eval pawn maj
constexpr int PAWN_MAJORITY_BONUS_EG = 30;
constexpr int UNDEFENDED_PIECE_PENALTY_MG[5] = { 0,-10,-10,-15,-26 }; // used in eval defence, index = piece type (knight, bishop, rook, queen)
constexpr int UNDEFENDED_PIECE_PENALTY_EG[5] = { 0,-15,-15,-22,-37 };
constexpr int HANGING_PIECE_PENALTY_MG[5] = { 0,-21,-21,-31,-52 }; // same as above
constexpr int HANGING_PIECE_PENALTY_EG[5] = { 0,-30,-30,-45,-75 };
const int PIECE_VALUES_MG[6] = { 100,388,392,481,1101,20000 };
const int PIECE_VALUES_EG[6] = { 100,239,240,410,779,20000 };

// --- NEU: Mobility Bonuses ---
constexpr int MOBILITY_BONUS_MG[6] = { 0, 0, 5, 3, 1, 0 }; // Pawn, Knight, Bishop, Rook, Queen, King
constexpr int MOBILITY_BONUS_EG[6] = { 0, -1, 2, 2, 4, 0 }; // Pawn, Knight, Bishop, Rook, Queen, King

// --- NEU: Outpost Bonuses ---
constexpr int BISHOP_OUTPOST_BONUS_WITH_OPPOSITE_BISHOP_MG = 37;
constexpr int BISHOP_OUTPOST_BONUS_WITH_OPPOSITE_BISHOP_EG = 15;
constexpr int BISHOP_OUTPOST_BONUS_NO_OPPOSITE_BISHOP_MG = 55;
constexpr int BISHOP_OUTPOST_BONUS_NO_OPPOSITE_BISHOP_EG = 6;
constexpr int KNIGHT_OUTPOST_BONUS_WITH_OPPOSITE_BISHOP_MG = 34;
constexpr int KNIGHT_OUTPOST_BONUS_WITH_OPPOSITE_BISHOP_EG = 14;
constexpr int KNIGHT_OUTPOST_BONUS_NO_OPPOSITE_BISHOP_MG = 43;
constexpr int KNIGHT_OUTPOST_BONUS_NO_OPPOSITE_BISHOP_EG = 13;



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
