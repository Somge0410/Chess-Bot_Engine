#include "evaluation.h"
#include "pst.h"
EvaluationResult EvalWeights[PARAM_COUNT] = {
        {100,100},
    {340,255},
    {280,242},
    {334,416},
    {782,889},
    // Pawns
        {   0,    0}, {   0,    0}, {   0,    0}, {   0,    0}, {   0,    0}, {   0,    0}, {   0,    0}, {   0,    0},
        { -33,    2}, { -32,    7}, { -25,   11}, { -30,   13}, { -14,   18}, {  -9,   14}, {  -5,    1}, { -38,   -5},
        { -35,    0}, { -40,    4}, { -21,    3}, { -21,    8}, {  -7,    7}, { -14,    8}, { -10,   -2}, { -21,   -8},
        { -33,    6}, { -28,    7}, { -12,    4}, {   1,    3}, {   6,    1}, {  -3,    4}, { -21,    3}, { -28,   -2},
        { -24,   17}, { -16,   10}, {  -8,    9}, {  -4,    1}, {  23,   -4}, {   8,    4}, { -10,    8}, { -14,    5},
        {  -7,   22}, { -15,   28}, {  21,   17}, {  27,   21}, {  44,    6}, {  66,   11}, {  32,   37}, {  10,   24},
        {  74,  111}, {  95,  106}, {  84,  102}, { 121,   75}, { 101,   72}, {  83,   83}, { -31,  107}, { -57,  115},
        {   0,    0}, {   0,    0}, {   0,    0}, {   0,    0}, {   0,    0}, {   0,    0}, {   0,    0}, {   0,    0},
        // Knights
            { -114,   44 }, { -72,   25}, { -76,   50}, { -59,   49}, { -55,   51}, { -44,   42}, { -69,   31}, { -75,   31},
            { -83,   42}, { -70,   52}, { -50,   65}, { -34,   63}, { -35,   62}, { -30,   59}, { -42,   42}, { -49,   45},
            { -66,   56}, { -41,   72}, { -27,   80}, { -21,   86}, {  -7,   83}, { -15,   79}, { -15,   68}, { -47,   56},
            { -58,   55}, { -42,   70}, { -17,   85}, { -20,   87}, {  -6,   90}, { -11,   80}, { -12,   66}, { -42,   50},
            { -48,   59}, { -31,   74}, {  -9,   83}, {  15,   85}, {   1,   85}, {  28,   79}, { -13,   74}, {  -7,   46},
            { -54,   47}, { -21,   63}, {   1,   80}, {  10,   81}, {  54,   65}, {  52,   56}, {  11,   52}, { -20,   38},
            { -74,   37}, { -49,   55}, { -20,   64}, {  10,   61}, { -25,   59}, {  49,   41}, { -43,   48}, { -32,   21},
            {-173,    9}, {-158,   39}, {-105,   59}, { -76,   51}, { -30,   51}, { -99,   32}, {-133,   36}, {-112,  -11},
    // Bishops
        { 3,   45 }, {  18,   57}, {   6,   49}, {  -1,   57}, {   6,   60}, {  -3,   60}, {   9,   49}, {  16,   27},
        {   8,   57}, {  -2,   38}, {  21,   49}, {   3,   65}, {  13,   60}, {  23,   56}, {  15,   36}, {  18,   38},
        {   8,   68}, {  22,   74}, {  19,   73}, {  10,   69}, {  16,   75}, {  24,   72}, {  27,   69}, {  24,   59},
        {  -5,   61}, {  -4,   70}, {   1,   68}, {  25,   68}, {  21,   65}, {   6,   69}, {   2,   65}, {   9,   52},
        {  -8,   65}, {   0,   67}, {  11,   66}, {  17,   70}, {  26,   64}, {  16,   66}, {   9,   62}, {  -5,   62},
        {  -6,   64}, {   7,   63}, {   3,   64}, {  19,   60}, {  17,   59}, {  43,   62}, {  33,   55}, {  26,   59},
        { -11,   55}, {  -4,   56}, {  -7,   62}, { -18,   64}, {   4,   56}, {  11,   53}, {  -7,   59}, {  -1,   48},
        { -28,   58}, { -50,   74}, { -44,   64}, { -89,   74}, { -65,   66}, { -55,   64}, { -16,   57}, { -57,   56},
    // Rooks
        { 22,  116 }, {  21,  114}, {  23,  120}, {  32,  112}, {  38,  108}, {  41,  112}, {  49,  103}, {  35,  100},
        {   2,  113}, {   4,  116}, {  15,  116}, {  16,  115}, {  25,  109}, {  43,  101}, {  56,   92}, {  20,  100},
        {   0,  121}, {   0,  119}, {   5,  118}, {   7,  117}, {  21,  111}, {  34,  107}, {  69,   93}, {  45,   95},
        {  -2,  126}, {  -8,  125}, {   2,  124}, {   7,  122}, {  11,  119}, {  17,  117}, {  44,  106}, {  25,  108},
        {   2,  130}, {  11,  127}, {  12,  130}, {  17,  124}, {  17,  115}, {  45,  108}, {  52,  111}, {  44,  108},
        {   2,  129}, {  27,  127}, {  21,  128}, {  24,  123}, {  54,  112}, {  73,  107}, { 117,  102}, {  79,  100},
        {  20,  132}, {  18,  142}, {  37,  142}, {  57,  133}, {  43,  134}, {  86,  121}, {  82,  120}, { 105,  109},
        {  41,  128}, {  24,  134}, {  32,  139}, {  32,  136}, {  44,  129}, {  76,  125}, {  74,  123}, {  85,  121},
    // Queens
        { 41,   76 }, {  41,   76}, {  49,   77}, {  58,   84}, {  55,   74}, {  43,   74}, {  56,   59}, {  54,   49},
        {  42,   84}, {  50,   80}, {  57,   76}, {  60,   83}, {  58,   87}, {  69,   68}, {  73,   46}, {  80,   32},
        {  46,   86}, {  52,   97}, {  49,   99}, {  47,   94}, {  51,  101}, {  61,  112}, {  75,  103}, {  68,   91},
        {  38,   96}, {  29,  115}, {  35,  101}, {  41,  119}, {  44,  116}, {  44,  120}, {  51,  118}, {  60,  114},
        {  27,  101}, {  36,  103}, {  36,  113}, {  33,  126}, {  36,  139}, {  51,  132}, {  52,  136}, {  60,  124},
        {  40,   89}, {  37,   97}, {  31,  124}, {  47,  127}, {  58,  141}, {  92,  131}, { 100,  110}, { 103,  104},
        {  35,   89}, {  13,  116}, {  15,  145}, {  14,  154}, {  17,  171}, {  64,  137}, {  43,  132}, {  95,  115},
        {  19,  103}, {   8,  122}, {  34,  132}, {  67,  118}, {  63,  127}, {  86,  119}, { 117,   77}, {  51,  111},
    // Kings
        { 38,  -61 }, {  63,  -40}, {  58,  -26}, { -19,  -18}, {  45,  -39}, {   7,  -21}, {  49,  -35}, {  51,  -65},
        {  53,  -30}, {  20,   -5}, {   8,    5}, { -12,    9}, { -17,   12}, {   0,    4}, {  26,   -8}, {  28,  -22},
        { -27,  -15}, {  -9,    9}, { -59,   29}, { -69,   34}, { -64,   34}, { -58,   27}, { -33,   16}, { -52,    3},
        { -91,   -3}, { -79,   21}, { -96,   42}, {-128,   54}, {-121,   52}, { -91,   41}, {-107,   31}, {-151,   18},
        { -81,    2}, { -69,   30}, { -94,   49}, {-137,   61}, {-119,   60}, { -91,   53}, { -96,   43}, {-127,   19},
        {-118,   11}, {  20,   28}, { -35,   44}, { -73,   56}, { -33,   54}, {  46,   45}, {  28,   41}, { -41,   16},
        { -79,   -4}, { -10,   22}, { -40,   34}, {  36,   23}, { -17,   40}, { -13,   49}, {  33,   36}, {   5,    5},
        {  71, -100}, {  87,  -54}, {  90,  -35}, { -56,    7}, { -34,   -1}, {  34,   -5}, { 108,  -21}, { 209, -114},
         {0,0},  // PASSED_PAWNS_START+0
        {0,0},  // PASSED_PAWNS_START+1
        {0,0},  // PASSED_PAWNS_START+2
        {0,0},  // PASSED_PAWNS_START+3
        {0,0},  // PASSED_PAWNS_START+4
        {0,0},  // PASSED_PAWNS_START+5
        {0,0},  // PASSED_PAWNS_START+6
        {0,0},  // PASSED_PAWNS_START+7
        {-21,-7},       // PASSED_PAWNS_START+8
        {-11,-5},       // PASSED_PAWNS_START+9
        {-13,-10},      // PASSED_PAWNS_START+10
        {-36,-11},      // PASSED_PAWNS_START+11
        {-15,-10},      // PASSED_PAWNS_START+12
        {-6,-11},       // PASSED_PAWNS_START+13
        {-11,-13},      // PASSED_PAWNS_START+14
        {0,-16},        // PASSED_PAWNS_START+15
        {-15,-12},      // PASSED_PAWNS_START+16
        {-4,-11},       // PASSED_PAWNS_START+17
        {-13,-9},       // PASSED_PAWNS_START+18
        {-24,-9},       // PASSED_PAWNS_START+19
        {-20,-10},      // PASSED_PAWNS_START+20
        {-12,-10},      // PASSED_PAWNS_START+21
        {-7,-12},       // PASSED_PAWNS_START+22
        {-1,-16},       // PASSED_PAWNS_START+23
        {-10,-13},      // PASSED_PAWNS_START+24
        {-5,-11},       // PASSED_PAWNS_START+25
        {-10,-12},      // PASSED_PAWNS_START+26
        {-14,-13},      // PASSED_PAWNS_START+27
        {-14,-12},      // PASSED_PAWNS_START+28
        {-9,-12},       // PASSED_PAWNS_START+29
        {-8,-14},       // PASSED_PAWNS_START+30
        {-1,-14},       // PASSED_PAWNS_START+31
        {-3,-17},       // PASSED_PAWNS_START+32
        {-6,-14},       // PASSED_PAWNS_START+33
        {0,-18},        // PASSED_PAWNS_START+34
        {-2,-18},       // PASSED_PAWNS_START+35
        {0,-16},        // PASSED_PAWNS_START+36
        {0,-16},        // PASSED_PAWNS_START+37
        {-5,-12},       // PASSED_PAWNS_START+38
        {-2,-13},       // PASSED_PAWNS_START+39
        {5,-19},        // PASSED_PAWNS_START+40
        {-2,-18},       // PASSED_PAWNS_START+41
        {-8,-19},       // PASSED_PAWNS_START+42
        {12,-21},       // PASSED_PAWNS_START+43
        {11,-20},       // PASSED_PAWNS_START+44
        {-9,-19},       // PASSED_PAWNS_START+45
        {-6,-16},       // PASSED_PAWNS_START+46
        {8,-16},        // PASSED_PAWNS_START+47
        {8,-32},        // PASSED_PAWNS_START+48
        {7,-35},        // PASSED_PAWNS_START+49
        {-8,-27},       // PASSED_PAWNS_START+50
        {-3,-32},       // PASSED_PAWNS_START+51
        {-12,-25},      // PASSED_PAWNS_START+52
        {-6,-36},       // PASSED_PAWNS_START+53
        {0,-26},        // PASSED_PAWNS_START+54
        {4,-23},        // PASSED_PAWNS_START+55
        {0,0},  // PASSED_PAWNS_START+56
        {0,0},  // PASSED_PAWNS_START+57
        {0,0},  // PASSED_PAWNS_START+58
        {0,0},  // PASSED_PAWNS_START+59
        {0,0},  // PASSED_PAWNS_START+60
        {0,0},  // PASSED_PAWNS_START+61
        {0,0},  // PASSED_PAWNS_START+62
        {0,0},  // PASSED_PAWNS_START+63
        {0,0},  // ISOLANI_START+0
        {0,0},  // ISOLANI_START+1
        {0,0},  // ISOLANI_START+2
        {0,0},  // ISOLANI_START+3
        {0,0},  // ISOLANI_START+4
        {0,0},  // ISOLANI_START+5
        {0,0},  // ISOLANI_START+6
        {0,0},  // ISOLANI_START+7
        {5,-3}, // ISOLANI_START+8
        {16,-1},        // ISOLANI_START+9
        {8,1},  // ISOLANI_START+10
        {3,8},  // ISOLANI_START+11
        {14,14},        // ISOLANI_START+12
        {15,-1},        // ISOLANI_START+13
        {5,5},  // ISOLANI_START+14
        {-12,1},        // ISOLANI_START+15
        {5,-5}, // ISOLANI_START+16
        {27,0}, // ISOLANI_START+17
        {11,3}, // ISOLANI_START+18
        {10,18},        // ISOLANI_START+19
        {3,12}, // ISOLANI_START+20
        {5,1},  // ISOLANI_START+21
        {17,11},        // ISOLANI_START+22
        {-15,2},        // ISOLANI_START+23
        {-1,-7},        // ISOLANI_START+24
        {21,-1},        // ISOLANI_START+25
        {2,-2}, // ISOLANI_START+26
        {9,6},  // ISOLANI_START+27
        {-4,8}, // ISOLANI_START+28
        {-9,1}, // ISOLANI_START+29
        {0,10}, // ISOLANI_START+30
        {-31,3},        // ISOLANI_START+31
        {-9,14},        // ISOLANI_START+32
        {8,14}, // ISOLANI_START+33
        {-2,5}, // ISOLANI_START+34
        {3,17}, // ISOLANI_START+35
        {-8,12},        // ISOLANI_START+36
        {-15,7},        // ISOLANI_START+37
        {-7,21},        // ISOLANI_START+38
        {-49,19},       // ISOLANI_START+39
        {-33,51},       // ISOLANI_START+40
        {3,69}, // ISOLANI_START+41
        {-19,65},       // ISOLANI_START+42
        {10,30},        // ISOLANI_START+43
        {-3,48},        // ISOLANI_START+44
        {-24,18},       // ISOLANI_START+45
        {-55,85},       // ISOLANI_START+46
        {-90,76},       // ISOLANI_START+47
        {0,0},  // ISOLANI_START+48
        {0,0},  // ISOLANI_START+49
        {0,0},  // ISOLANI_START+50
        {0,0},  // ISOLANI_START+51
        {0,0},  // ISOLANI_START+52
        {0,0},  // ISOLANI_START+53
        {0,0},  // ISOLANI_START+54
        {0,0},  // ISOLANI_START+55
        {0,0},  // ISOLANI_START+56
        {0,0},  // ISOLANI_START+57
        {0,0},  // ISOLANI_START+58
        {0,0},  // ISOLANI_START+59
        {0,0},  // ISOLANI_START+60
        {0,0},  // ISOLANI_START+61
        {0,0},  // ISOLANI_START+62
        {0,0},  // ISOLANI_START+63
        {0,0},  // BLOCKED_ISOLANI_START+0
        {0,0},  // BLOCKED_ISOLANI_START+1
        {0,0},  // BLOCKED_ISOLANI_START+2
        {0,0},  // BLOCKED_ISOLANI_START+3
        {0,0},  // BLOCKED_ISOLANI_START+4
        {0,0},  // BLOCKED_ISOLANI_START+5
        {0,0},  // BLOCKED_ISOLANI_START+6
        {0,0},  // BLOCKED_ISOLANI_START+7
        {-15,-24},      // BLOCKED_ISOLANI_START+8
        {-7,-19},       // BLOCKED_ISOLANI_START+9
        {3,-29},        // BLOCKED_ISOLANI_START+10
        {-18,-19},      // BLOCKED_ISOLANI_START+11
        {-10,-9},       // BLOCKED_ISOLANI_START+12
        {6,-18},        // BLOCKED_ISOLANI_START+13
        {3,-19},        // BLOCKED_ISOLANI_START+14
        {-27,-19},      // BLOCKED_ISOLANI_START+15
        {-3,-18},       // BLOCKED_ISOLANI_START+16
        {16,-10},       // BLOCKED_ISOLANI_START+17
        {0,-5}, // BLOCKED_ISOLANI_START+18
        {4,8},  // BLOCKED_ISOLANI_START+19
        {5,7},  // BLOCKED_ISOLANI_START+20
        {-1,3}, // BLOCKED_ISOLANI_START+21
        {20,0}, // BLOCKED_ISOLANI_START+22
        {-18,-9},       // BLOCKED_ISOLANI_START+23
        {-3,-12},       // BLOCKED_ISOLANI_START+24
        {12,-6},        // BLOCKED_ISOLANI_START+25
        {0,-3}, // BLOCKED_ISOLANI_START+26
        {10,8}, // BLOCKED_ISOLANI_START+27
        {3,7},  // BLOCKED_ISOLANI_START+28
        {-2,-4},        // BLOCKED_ISOLANI_START+29
        {3,-1}, // BLOCKED_ISOLANI_START+30
        {-27,-8},       // BLOCKED_ISOLANI_START+31
        {-4,1}, // BLOCKED_ISOLANI_START+32
        {13,6}, // BLOCKED_ISOLANI_START+33
        {10,11},        // BLOCKED_ISOLANI_START+34
        {11,11},        // BLOCKED_ISOLANI_START+35
        {-2,15},        // BLOCKED_ISOLANI_START+36
        {14,5}, // BLOCKED_ISOLANI_START+37
        {-7,10},        // BLOCKED_ISOLANI_START+38
        {-29,8},        // BLOCKED_ISOLANI_START+39
        {-4,76},        // BLOCKED_ISOLANI_START+40
        {0,95}, // BLOCKED_ISOLANI_START+41
        {8,78}, // BLOCKED_ISOLANI_START+42
        {37,50},        // BLOCKED_ISOLANI_START+43
        {-3,67},        // BLOCKED_ISOLANI_START+44
        {-18,43},       // BLOCKED_ISOLANI_START+45
        {-32,51},       // BLOCKED_ISOLANI_START+46
        {-58,67},       // BLOCKED_ISOLANI_START+47
        {0,0},  // BLOCKED_ISOLANI_START+48
        {0,0},  // BLOCKED_ISOLANI_START+49
        {0,0},  // BLOCKED_ISOLANI_START+50
        {0,0},  // BLOCKED_ISOLANI_START+51
        {0,0},  // BLOCKED_ISOLANI_START+52
        {0,0},  // BLOCKED_ISOLANI_START+53
        {0,0},  // BLOCKED_ISOLANI_START+54
        {0,0},  // BLOCKED_ISOLANI_START+55
        {0,0},  // BLOCKED_ISOLANI_START+56
        {0,0},  // BLOCKED_ISOLANI_START+57
        {0,0},  // BLOCKED_ISOLANI_START+58
        {0,0},  // BLOCKED_ISOLANI_START+59
        {0,0},  // BLOCKED_ISOLANI_START+60
        {0,0},  // BLOCKED_ISOLANI_START+61
        {0,0},  // BLOCKED_ISOLANI_START+62
        {0,0},  // BLOCKED_ISOLANI_START+63
        {-3,-1},        // FORWARD_BLOCKED_BACKWARD
        {0,2},  // FORWARD_CONTROLLED_BACKWARD
        {4,5},  // FREE_TO_ADV_BACKWARD
        {-11,30},       // DOUBLE_PAWN_FILE_START+0
        {5,17}, // DOUBLE_PAWN_FILE_START+1
        {11,6}, // DOUBLE_PAWN_FILE_START+2
        {-4,8}, // DOUBLE_PAWN_FILE_START+3
        {-4,3}, // DOUBLE_PAWN_FILE_START+4
        {12,0}, // DOUBLE_PAWN_FILE_START+5
        {8,14}, // DOUBLE_PAWN_FILE_START+6
        {32,15},        // DOUBLE_PAWN_FILE_START+7
        { 6,-8 }, // PAWN_SHIELD_BONUS
        { -95,-7 },       // DIRECTLY_ON_OPEN_FILE_NEXT_TO_OPEN_PENALTY
        { -92,3 },        // DIRECTLY_ON_OPEN_FILE_NOT_NEXT_TO_OPEN_PENALTY
        { -36,-9 },       // NEXT_TO_OPEN_FILE_PENALTY
        { 6,8 },  // DIRECTLY_ON_SEMI_OPEN_FILE_NEXT_TO_OPEN_PENALTY
        { 2,-12 },        // DIRECTLY_ON_SEMI_OPEN_FILE_NOT_NEXT_TO_OPEN_PENALTY
        { 0,-3 }, // NEXT_TO_SEMI_OPEN_FILE_PENALTY
        { -24,-58 },      // NEXT_TO_OPEN_DIAGONAL_PENALTY_START+0
        { -1,-13 },       // NEXT_TO_OPEN_DIAGONAL_PENALTY_START+1
        { 16,18 },        // NEXT_TO_OPEN_DIAGONAL_PENALTY_START+2
        { 44,-6 },        // NEXT_TO_OPEN_DIAGONAL_PENALTY_START+3
        { 127,107 },      // NEXT_TO_OPEN_DIAGONAL_PENALTY_START+4
        { 0,0 },  // NEXT_TO_OPEN_DIAGONAL_PENALTY_START+5
        { 0,0 },  // NEXT_TO_OPEN_DIAGONAL_PENALTY_START+6
        { 0,0 },  // MOBILITY_START+0
        { 2,3 },  // MOBILITY_START+1
        { 3,3 },  // MOBILITY_START+2
        { 1,3 },  // MOBILITY_START+3
        { 0,5 },  // ROOK_BEHIND_FREE_PAWN_START+0
        { -17,4 },        // ROOK_BEHIND_FREE_PAWN_START+1
        { -21,-3 },       // ROOK_BEHIND_FREE_PAWN_START+2
        { -1,5 }, // ROOK_BEHIND_FREE_PAWN_START+3
        { 9,10 }, // ROOK_BEHIND_FREE_PAWN_START+4
        { 14,3 }, // ROOK_BEHIND_FREE_PAWN_START+5
        { 6,-18 },        // ROOK_BEHIND_FREE_PAWN_START+6
        { 0,0 },  // ROOK_BEHIND_FREE_PAWN_START+7
        { 30,-1 },        // ROOK_ON_OPEN_FILE
        { 13,-10 },       // ROOK_ON_SEMI_OPEN_FILE
        { 0,2 },  // CONNECTED_ROOKS
        { 13,77 },        // BISHOP_PAIR
        { -5,-20 },       // BAD_BISHOP_BLOCKED
        { -3,-2 },        // BAD_BISHOP_UNBLOCKED
        { -130,-98 },     // TRAPPED_BISHOP
        { -24,-88 },      // TRAPPED_KNIGHT
        { 5,-4 }, // FIANCHETTO_BISHOP
        { 1,-9 }, // BROKEN_FIANCHETTO
        { 35,0 }, // BISHOP_OUTPOST_NO_OPPOSITE_BISHOP
        { 19,15 },        // BISHOP_OUTPOST_WITH_OPPOSITE_BISHOP
        { 35,9 }, // KNIGHT_OUTPOST_NO_OPPOSITE_BISHOP
        { 26,17 },        // KNIGHT_OUTPOST_WITH_OPPOSITE_BISHOP
};