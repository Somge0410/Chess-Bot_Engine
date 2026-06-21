#include "evaluation.h"
#include "pst.h"

EvaluationResult EvalWeights[PARAM_COUNT1] = {
        {100,178},      // PAWN
        {383,499},      // KNIGHT
        {418,522},      // BISHOP
        {501,891},      // ROOK
        {1093,1688},    // QUEEN
        {0,0},  // PAWN_PST_START+0
        {0,0},  // PAWN_PST_START+1
        {0,0},  // PAWN_PST_START+2
        {0,0},  // PAWN_PST_START+3
        {0,0},  // PAWN_PST_START+4
        {0,0},  // PAWN_PST_START+5
        {0,0},  // PAWN_PST_START+6
        {0,0},  // PAWN_PST_START+7
        {-57,33},       // PAWN_PST_START+8
        {-19,35},       // PAWN_PST_START+9
        {-26,14},       // PAWN_PST_START+10
        {-41,22},       // PAWN_PST_START+11
        {-11,31},       // PAWN_PST_START+12
        {10,14},        // PAWN_PST_START+13
        {44,14},        // PAWN_PST_START+14
        {-26,0},        // PAWN_PST_START+15
        {-57,25},       // PAWN_PST_START+16
        {-20,29},       // PAWN_PST_START+17
        {-21,1},        // PAWN_PST_START+18
        {-19,20},       // PAWN_PST_START+19
        {3,9},  // PAWN_PST_START+20
        {-15,6},        // PAWN_PST_START+21
        {30,15},        // PAWN_PST_START+22
        {-14,-1},       // PAWN_PST_START+23
        {-56,35},       // PAWN_PST_START+24
        {-14,31},       // PAWN_PST_START+25
        {-16,4},        // PAWN_PST_START+26
        {9,-1}, // PAWN_PST_START+27
        {9,-4}, // PAWN_PST_START+28
        {-4,-1},        // PAWN_PST_START+29
        {8,18}, // PAWN_PST_START+30
        {-26,5},        // PAWN_PST_START+31
        {-40,75},       // PAWN_PST_START+32
        {-2,58},        // PAWN_PST_START+33
        {3,27}, // PAWN_PST_START+34
        {6,13}, // PAWN_PST_START+35
        {38,-1},        // PAWN_PST_START+36
        {24,4}, // PAWN_PST_START+37
        {29,35},        // PAWN_PST_START+38
        {-7,36},        // PAWN_PST_START+39
        {-18,187},      // PAWN_PST_START+40
        {4,200},        // PAWN_PST_START+41
        {53,146},       // PAWN_PST_START+42
        {60,113},       // PAWN_PST_START+43
        {65,100},       // PAWN_PST_START+44
        {99,76},        // PAWN_PST_START+45
        {66,152},       // PAWN_PST_START+46
        {1,147},        // PAWN_PST_START+47
        {107,274},      // PAWN_PST_START+48
        {133,264},      // PAWN_PST_START+49
        {105,261},      // PAWN_PST_START+50
        {134,197},      // PAWN_PST_START+51
        {111,188},      // PAWN_PST_START+52
        {91,202},       // PAWN_PST_START+53
        {11,258},       // PAWN_PST_START+54
        {-26,279},      // PAWN_PST_START+55
        {0,0},  // PAWN_PST_START+56
        {0,0},  // PAWN_PST_START+57
        {0,0},  // PAWN_PST_START+58
        {0,0},  // PAWN_PST_START+59
        {0,0},  // PAWN_PST_START+60
        {0,0},  // PAWN_PST_START+61
        {0,0},  // PAWN_PST_START+62
        {0,0},  // PAWN_PST_START+63
        {-152,34},      // KNIGHT_PST_START+0
        {-66,15},       // KNIGHT_PST_START+1
        {-88,60},       // KNIGHT_PST_START+2
        {-66,67},       // KNIGHT_PST_START+3
        {-59,68},       // KNIGHT_PST_START+4
        {-39,51},       // KNIGHT_PST_START+5
        {-61,26},       // KNIGHT_PST_START+6
        {-108,20},      // KNIGHT_PST_START+7
        {-84,47},       // KNIGHT_PST_START+8
        {-67,73},       // KNIGHT_PST_START+9
        {-41,85},       // KNIGHT_PST_START+10
        {-22,92},       // KNIGHT_PST_START+11
        {-20,90},       // KNIGHT_PST_START+12
        {-17,83},       // KNIGHT_PST_START+13
        {-38,59},       // KNIGHT_PST_START+14
        {-41,65},       // KNIGHT_PST_START+15
        {-63,61},       // KNIGHT_PST_START+16
        {-29,90},       // KNIGHT_PST_START+17
        {-8,107},       // KNIGHT_PST_START+18
        {-4,128},       // KNIGHT_PST_START+19
        {12,126},       // KNIGHT_PST_START+20
        {-2,101},       // KNIGHT_PST_START+21
        {3,83}, // KNIGHT_PST_START+22
        {-38,64},       // KNIGHT_PST_START+23
        {-35,86},       // KNIGHT_PST_START+24
        {-12,101},      // KNIGHT_PST_START+25
        {10,139},       // KNIGHT_PST_START+26
        {13,139},       // KNIGHT_PST_START+27
        {26,145},       // KNIGHT_PST_START+28
        {18,129},       // KNIGHT_PST_START+29
        {15,107},       // KNIGHT_PST_START+30
        {-19,74},       // KNIGHT_PST_START+31
        {-16,84},       // KNIGHT_PST_START+32
        {5,118},        // KNIGHT_PST_START+33
        {42,137},       // KNIGHT_PST_START+34
        {72,143},       // KNIGHT_PST_START+35
        {47,143},       // KNIGHT_PST_START+36
        {82,136},       // KNIGHT_PST_START+37
        {20,118},       // KNIGHT_PST_START+38
        {34,76},        // KNIGHT_PST_START+39
        {-15,70},       // KNIGHT_PST_START+40
        {39,94},        // KNIGHT_PST_START+41
        {66,121},       // KNIGHT_PST_START+42
        {82,123},       // KNIGHT_PST_START+43
        {128,105},      // KNIGHT_PST_START+44
        {124,99},       // KNIGHT_PST_START+45
        {70,82},        // KNIGHT_PST_START+46
        {24,56},        // KNIGHT_PST_START+47
        {-35,43},       // KNIGHT_PST_START+48
        {-6,76},        // KNIGHT_PST_START+49
        {28,92},        // KNIGHT_PST_START+50
        {43,95},        // KNIGHT_PST_START+51
        {26,81},        // KNIGHT_PST_START+52
        {109,64},       // KNIGHT_PST_START+53
        {-16,74},       // KNIGHT_PST_START+54
        {15,25},        // KNIGHT_PST_START+55
        {-232,-41},     // KNIGHT_PST_START+56
        {-179,48},      // KNIGHT_PST_START+57
        {-99,78},       // KNIGHT_PST_START+58
        {-64,71},       // KNIGHT_PST_START+59
        {-27,80},       // KNIGHT_PST_START+60
        {-93,35},       // KNIGHT_PST_START+61
        {-154,61},      // KNIGHT_PST_START+62
        {-162,-58},     // KNIGHT_PST_START+63
        {-34,50},       // BISHOP_PST_START+0
        {-3,78},        // BISHOP_PST_START+1
        {-25,49},       // BISHOP_PST_START+2
        {-41,83},       // BISHOP_PST_START+3
        {-34,76},       // BISHOP_PST_START+4
        {-33,77},       // BISHOP_PST_START+5
        {2,56}, // BISHOP_PST_START+6
        {-19,33},       // BISHOP_PST_START+7
        {0,77}, // BISHOP_PST_START+8
        {3,78}, // BISHOP_PST_START+9
        {20,75},        // BISHOP_PST_START+10
        {-13,99},       // BISHOP_PST_START+11
        {-2,102},       // BISHOP_PST_START+12
        {18,83},        // BISHOP_PST_START+13
        {27,88},        // BISHOP_PST_START+14
        {7,48}, // BISHOP_PST_START+15
        {-3,87},        // BISHOP_PST_START+16
        {8,103},        // BISHOP_PST_START+17
        {8,114},        // BISHOP_PST_START+18
        {13,114},       // BISHOP_PST_START+19
        {15,120},       // BISHOP_PST_START+20
        {6,116},        // BISHOP_PST_START+21
        {11,88},        // BISHOP_PST_START+22
        {16,71},        // BISHOP_PST_START+23
        {-20,87},       // BISHOP_PST_START+24
        {2,114},        // BISHOP_PST_START+25
        {11,126},       // BISHOP_PST_START+26
        {42,120},       // BISHOP_PST_START+27
        {38,120},       // BISHOP_PST_START+28
        {14,118},       // BISHOP_PST_START+29
        {2,109},        // BISHOP_PST_START+30
        {-7,72},        // BISHOP_PST_START+31
        {-8,93},        // BISHOP_PST_START+32
        {13,118},       // BISHOP_PST_START+33
        {47,109},       // BISHOP_PST_START+34
        {61,131},       // BISHOP_PST_START+35
        {59,118},       // BISHOP_PST_START+36
        {52,114},       // BISHOP_PST_START+37
        {14,114},       // BISHOP_PST_START+38
        {-7,93},        // BISHOP_PST_START+39
        {4,100},        // BISHOP_PST_START+40
        {40,92},        // BISHOP_PST_START+41
        {43,107},       // BISHOP_PST_START+42
        {77,91},        // BISHOP_PST_START+43
        {55,100},       // BISHOP_PST_START+44
        {105,100},      // BISHOP_PST_START+45
        {66,89},        // BISHOP_PST_START+46
        {49,90},        // BISHOP_PST_START+47
        {-10,53},       // BISHOP_PST_START+48
        {28,84},        // BISHOP_PST_START+49
        {17,91},        // BISHOP_PST_START+50
        {-3,92},        // BISHOP_PST_START+51
        {36,80},        // BISHOP_PST_START+52
        {35,78},        // BISHOP_PST_START+53
        {25,89},        // BISHOP_PST_START+54
        {7,50}, // BISHOP_PST_START+55
        {-31,72},       // BISHOP_PST_START+56
        {-44,86},       // BISHOP_PST_START+57
        {-30,82},       // BISHOP_PST_START+58
        {-82,99},       // BISHOP_PST_START+59
        {-67,90},       // BISHOP_PST_START+60
        {-49,78},       // BISHOP_PST_START+61
        {-3,69},        // BISHOP_PST_START+62
        {-60,64},       // BISHOP_PST_START+63
        {-17,143},      // ROOK_PST_START+0
        {-16,159},      // ROOK_PST_START+1
        {-2,171},       // ROOK_PST_START+2
        {6,169},        // ROOK_PST_START+3
        {12,157},       // ROOK_PST_START+4
        {-2,149},       // ROOK_PST_START+5
        {19,143},       // ROOK_PST_START+6
        {-14,129},      // ROOK_PST_START+7
        {-47,152},      // ROOK_PST_START+8
        {-30,158},      // ROOK_PST_START+9
        {-7,159},       // ROOK_PST_START+10
        {-12,162},      // ROOK_PST_START+11
        {-5,149},       // ROOK_PST_START+12
        {-2,142},       // ROOK_PST_START+13
        {24,127},       // ROOK_PST_START+14
        {-20,136},      // ROOK_PST_START+15
        {-43,160},      // ROOK_PST_START+16
        {-29,160},      // ROOK_PST_START+17
        {-16,159},      // ROOK_PST_START+18
        {-16,166},      // ROOK_PST_START+19
        {-9,159},       // ROOK_PST_START+20
        {-12,147},      // ROOK_PST_START+21
        {39,116},       // ROOK_PST_START+22
        {8,116},        // ROOK_PST_START+23
        {-32,166},      // ROOK_PST_START+24
        {-29,174},      // ROOK_PST_START+25
        {-14,178},      // ROOK_PST_START+26
        {6,176},        // ROOK_PST_START+27
        {5,169},        // ROOK_PST_START+28
        {-18,166},      // ROOK_PST_START+29
        {18,145},       // ROOK_PST_START+30
        {6,137},        // ROOK_PST_START+31
        {-4,178},       // ROOK_PST_START+32
        {16,177},       // ROOK_PST_START+33
        {20,191},       // ROOK_PST_START+34
        {34,185},       // ROOK_PST_START+35
        {43,160},       // ROOK_PST_START+36
        {43,151},       // ROOK_PST_START+37
        {54,147},       // ROOK_PST_START+38
        {61,136},       // ROOK_PST_START+39
        {21,175},       // ROOK_PST_START+40
        {52,181},       // ROOK_PST_START+41
        {56,182},       // ROOK_PST_START+42
        {61,180},       // ROOK_PST_START+43
        {103,159},      // ROOK_PST_START+44
        {106,150},      // ROOK_PST_START+45
        {157,139},      // ROOK_PST_START+46
        {126,130},      // ROOK_PST_START+47
        {51,176},       // ROOK_PST_START+48
        {50,195},       // ROOK_PST_START+49
        {78,202},       // ROOK_PST_START+50
        {109,188},      // ROOK_PST_START+51
        {88,188},       // ROOK_PST_START+52
        {131,166},      // ROOK_PST_START+53
        {111,159},      // ROOK_PST_START+54
        {156,139},      // ROOK_PST_START+55
        {77,177},       // ROOK_PST_START+56
        {64,189},       // ROOK_PST_START+57
        {75,203},       // ROOK_PST_START+58
        {81,197},       // ROOK_PST_START+59
        {108,183},      // ROOK_PST_START+60
        {124,169},      // ROOK_PST_START+61
        {104,171},      // ROOK_PST_START+62
        {133,164},      // ROOK_PST_START+63
        {49,175},       // QUEEN_PST_START+0
        {33,188},       // QUEEN_PST_START+1
        {44,193},       // QUEEN_PST_START+2
        {69,179},       // QUEEN_PST_START+3
        {54,186},       // QUEEN_PST_START+4
        {34,183},       // QUEEN_PST_START+5
        {66,146},       // QUEEN_PST_START+6
        {54,144},       // QUEEN_PST_START+7
        {51,188},       // QUEEN_PST_START+8
        {60,193},       // QUEEN_PST_START+9
        {77,188},       // QUEEN_PST_START+10
        {76,203},       // QUEEN_PST_START+11
        {73,208},       // QUEEN_PST_START+12
        {87,167},       // QUEEN_PST_START+13
        {94,128},       // QUEEN_PST_START+14
        {105,97},       // QUEEN_PST_START+15
        {55,194},       // QUEEN_PST_START+16
        {67,219},       // QUEEN_PST_START+17
        {58,256},       // QUEEN_PST_START+18
        {57,253},       // QUEEN_PST_START+19
        {61,258},       // QUEEN_PST_START+20
        {72,246},       // QUEEN_PST_START+21
        {91,211},       // QUEEN_PST_START+22
        {81,194},       // QUEEN_PST_START+23
        {59,210},       // QUEEN_PST_START+24
        {57,255},       // QUEEN_PST_START+25
        {54,272},       // QUEEN_PST_START+26
        {68,301},       // QUEEN_PST_START+27
        {67,298},       // QUEEN_PST_START+28
        {63,286},       // QUEEN_PST_START+29
        {81,255},       // QUEEN_PST_START+30
        {85,236},       // QUEEN_PST_START+31
        {58,215},       // QUEEN_PST_START+32
        {63,252},       // QUEEN_PST_START+33
        {71,273},       // QUEEN_PST_START+34
        {71,308},       // QUEEN_PST_START+35
        {74,328},       // QUEEN_PST_START+36
        {92,308},       // QUEEN_PST_START+37
        {89,287},       // QUEEN_PST_START+38
        {99,254},       // QUEEN_PST_START+39
        {83,201},       // QUEEN_PST_START+40
        {80,228},       // QUEEN_PST_START+41
        {80,289},       // QUEEN_PST_START+42
        {103,294},      // QUEEN_PST_START+43
        {110,315},      // QUEEN_PST_START+44
        {172,288},      // QUEEN_PST_START+45
        {167,237},      // QUEEN_PST_START+46
        {163,220},      // QUEEN_PST_START+47
        {83,183},       // QUEEN_PST_START+48
        {50,249},       // QUEEN_PST_START+49
        {61,299},       // QUEEN_PST_START+50
        {58,319},       // QUEEN_PST_START+51
        {69,342},       // QUEEN_PST_START+52
        {115,292},      // QUEEN_PST_START+53
        {85,265},       // QUEEN_PST_START+54
        {144,237},      // QUEEN_PST_START+55
        {26,238},       // QUEEN_PST_START+56
        {42,255},       // QUEEN_PST_START+57
        {91,280},       // QUEEN_PST_START+58
        {132,268},      // QUEEN_PST_START+59
        {130,267},      // QUEEN_PST_START+60
        {142,249},      // QUEEN_PST_START+61
        {155,196},      // QUEEN_PST_START+62
        {86,232},       // QUEEN_PST_START+63
        {93,-117},      // KING_PST_START+0
        {129,-87},      // KING_PST_START+1
        {89,-56},       // KING_PST_START+2
        {-61,-31},      // KING_PST_START+3
        {36,-69},       // KING_PST_START+4
        {-25,-32},      // KING_PST_START+5
        {101,-73},      // KING_PST_START+6
        {102,-118},     // KING_PST_START+7
        {98,-62},       // KING_PST_START+8
        {37,-21},       // KING_PST_START+9
        {17,-2},        // KING_PST_START+10
        {-36,16},       // KING_PST_START+11
        {-38,21},       // KING_PST_START+12
        {-8,5}, // KING_PST_START+13
        {64,-23},       // KING_PST_START+14
        {78,-51},       // KING_PST_START+15
        {-30,-32},      // KING_PST_START+16
        {-11,6},        // KING_PST_START+17
        {-95,39},       // KING_PST_START+18
        {-114,58},      // KING_PST_START+19
        {-106,58},      // KING_PST_START+20
        {-104,45},      // KING_PST_START+21
        {-33,14},       // KING_PST_START+22
        {-59,-5},       // KING_PST_START+23
        {-78,-20},      // KING_PST_START+24
        {-93,30},       // KING_PST_START+25
        {-138,69},      // KING_PST_START+26
        {-175,91},      // KING_PST_START+27
        {-177,91},      // KING_PST_START+28
        {-129,72},      // KING_PST_START+29
        {-134,52},      // KING_PST_START+30
        {-157,22},      // KING_PST_START+31
        {-78,-1},       // KING_PST_START+32
        {-91,54},       // KING_PST_START+33
        {-111,82},      // KING_PST_START+34
        {-164,100},     // KING_PST_START+35
        {-150,99},      // KING_PST_START+36
        {-103,91},      // KING_PST_START+37
        {-103,77},      // KING_PST_START+38
        {-129,34},      // KING_PST_START+39
        {-111,12},      // KING_PST_START+40
        {7,50}, // KING_PST_START+41
        {-70,76},       // KING_PST_START+42
        {-88,91},       // KING_PST_START+43
        {-43,92},       // KING_PST_START+44
        {43,86},        // KING_PST_START+45
        {22,82},        // KING_PST_START+46
        {-23,33},       // KING_PST_START+47
        {-87,-11},      // KING_PST_START+48
        {-41,38},       // KING_PST_START+49
        {-86,55},       // KING_PST_START+50
        {25,35},        // KING_PST_START+51
        {-25,63},       // KING_PST_START+52
        {-17,81},       // KING_PST_START+53
        {25,68},        // KING_PST_START+54
        {2,17}, // KING_PST_START+55
        {38,-130},      // KING_PST_START+56
        {22,-69},       // KING_PST_START+57
        {49,-51},       // KING_PST_START+58
        {-85,10},       // KING_PST_START+59
        {-40,-13},      // KING_PST_START+60
        {21,-5},        // KING_PST_START+61
        {83,-12},       // KING_PST_START+62
        {192,-143},     // KING_PST_START+63
        //PAWN EVAL
    { 0,0 },  // PASSED_PAWNS_START+0
    { 0,0 },  // PASSED_PAWNS_START+1
    { 0,0 },  // PASSED_PAWNS_START+2
    { 0,0 },  // PASSED_PAWNS_START+3
    { 0,0 },  // PASSED_PAWNS_START+4
    { 0,0 },  // PASSED_PAWNS_START+5
    { 0,0 },  // PASSED_PAWNS_START+6
    { 0,0 },  // PASSED_PAWNS_START+7
    { -60,-43 },      // PASSED_PAWNS_START+8
    { -26,-22 },      // PASSED_PAWNS_START+9
    { -46,-23 },      // PASSED_PAWNS_START+10
    { -55,-29 },      // PASSED_PAWNS_START+11
    { 12,-66 },       // PASSED_PAWNS_START+12
    { 36,-52 },       // PASSED_PAWNS_START+13
    { 93,-70 },       // PASSED_PAWNS_START+14
    { 2,-71 },        // PASSED_PAWNS_START+15
    { -31,-48 },      // PASSED_PAWNS_START+16
    { -56,-14 },      // PASSED_PAWNS_START+17
    { -53,-19 },      // PASSED_PAWNS_START+18
    { -45,-40 },      // PASSED_PAWNS_START+19
    { 0,-31 },        // PASSED_PAWNS_START+20
    { 0,-31 },        // PASSED_PAWNS_START+21
    { 1,4 },  // PASSED_PAWNS_START+22
    { 44,-77 },       // PASSED_PAWNS_START+23
    { -44,71 },       // PASSED_PAWNS_START+24
    { -29,68 },       // PASSED_PAWNS_START+25
    { -49,44 },       // PASSED_PAWNS_START+26
    { -8,15 },        // PASSED_PAWNS_START+27
    { -14,21 },       // PASSED_PAWNS_START+28
    { 2,36 }, // PASSED_PAWNS_START+29
    { -26,74 },       // PASSED_PAWNS_START+30
    { -28,39 },       // PASSED_PAWNS_START+31
    { -5,170 },       // PASSED_PAWNS_START+32
    { 32,162 },       // PASSED_PAWNS_START+33
    { 35,109 },       // PASSED_PAWNS_START+34
    { 32,65 },        // PASSED_PAWNS_START+35
    { 33,74 },        // PASSED_PAWNS_START+36
    { 36,110 },       // PASSED_PAWNS_START+37
    { -6,166 },       // PASSED_PAWNS_START+38
    { -42,156 },      // PASSED_PAWNS_START+39
    { -13,281 },      // PASSED_PAWNS_START+40
    { 54,270 },       // PASSED_PAWNS_START+41
    { 44,184 },       // PASSED_PAWNS_START+42
    { 4,105 },        // PASSED_PAWNS_START+43
    { 33,119 },       // PASSED_PAWNS_START+44
    { 82,209 },       // PASSED_PAWNS_START+45
    { 20,252 },       // PASSED_PAWNS_START+46
    { -115,298 },     // PASSED_PAWNS_START+47
    { -10,299 },      // PASSED_PAWNS_START+48
    { 71,276 },       // PASSED_PAWNS_START+49
    { 28,279 },       // PASSED_PAWNS_START+50
    { 60,178 },       // PASSED_PAWNS_START+51
    { 59,197 },       // PASSED_PAWNS_START+52
    { 52,232 },       // PASSED_PAWNS_START+53
    { -46,294 },      // PASSED_PAWNS_START+54
    { -124,326 },     // PASSED_PAWNS_START+55
    { 0,0 },  // PASSED_PAWNS_START+56
    { 0,0 },  // PASSED_PAWNS_START+57
    { 0,0 },  // PASSED_PAWNS_START+58
    { 0,0 },  // PASSED_PAWNS_START+59
    { 0,0 },  // PASSED_PAWNS_START+60
    { 0,0 },  // PASSED_PAWNS_START+61
    { 0,0 },  // PASSED_PAWNS_START+62
    { 0,0 },  // PASSED_PAWNS_START+63
    { 0,0 },  // ISOLANI_START+0
    { 0,0 },  // ISOLANI_START+1
    { 0,0 },  // ISOLANI_START+2
    { 0,0 },  // ISOLANI_START+3
    { 0,0 },  // ISOLANI_START+4
    { 0,0 },  // ISOLANI_START+5
    { 0,0 },  // ISOLANI_START+6
    { 0,0 },  // ISOLANI_START+7
    { 7,-32 },        // ISOLANI_START+8
    { -32,-33 },      // ISOLANI_START+9
    { -46,-38 },      // ISOLANI_START+10
    { -111,-35 },     // ISOLANI_START+11
    { -143,-23 },     // ISOLANI_START+12
    { -56,-20 },      // ISOLANI_START+13
    { -19,-32 },      // ISOLANI_START+14
    { -35,1 },        // ISOLANI_START+15
    { -7,-38 },       // ISOLANI_START+16
    { -44,-60 },      // ISOLANI_START+17
    { -88,-50 },      // ISOLANI_START+18
    { -87,-79 },      // ISOLANI_START+19
    { -119,-44 },     // ISOLANI_START+20
    { -71,-34 },      // ISOLANI_START+21
    { -95,-29 },      // ISOLANI_START+22
    { -43,-14 },      // ISOLANI_START+23
    { 5,-29 },        // ISOLANI_START+24
    { -22,-36 },      // ISOLANI_START+25
    { -82,-47 },      // ISOLANI_START+26
    { -71,-71 },      // ISOLANI_START+27
    { -72,-68 },      // ISOLANI_START+28
    { -59,-43 },      // ISOLANI_START+29
    { -62,-16 },      // ISOLANI_START+30
    { -2,-23 },       // ISOLANI_START+31
    { 39,-33 },       // ISOLANI_START+32
    { 18,-76 },       // ISOLANI_START+33
    { -19,-53 },      // ISOLANI_START+34
    { 3,-85 },        // ISOLANI_START+35
    { -22,-73 },      // ISOLANI_START+36
    { 29,-63 },       // ISOLANI_START+37
    { 96,-83 },       // ISOLANI_START+38
    { 88,-28 },       // ISOLANI_START+39
    { 133,142 },      // ISOLANI_START+40
    { 86,58 },        // ISOLANI_START+41
    { 76,56 },        // ISOLANI_START+42
    { 84,83 },        // ISOLANI_START+43
    { 49,77 },        // ISOLANI_START+44
    { 61,38 },        // ISOLANI_START+45
    { 136,36 },       // ISOLANI_START+46
    { 206,45 },       // ISOLANI_START+47
    { 0,0 },  // ISOLANI_START+48
    { 0,0 },  // ISOLANI_START+49
    { 0,0 },  // ISOLANI_START+50
    { 0,0 },  // ISOLANI_START+51
    { 0,0 },  // ISOLANI_START+52
    { 0,0 },  // ISOLANI_START+53
    { 0,0 },  // ISOLANI_START+54
    { 0,0 },  // ISOLANI_START+55
    { 0,0 },  // ISOLANI_START+56
    { 0,0 },  // ISOLANI_START+57
    { 0,0 },  // ISOLANI_START+58
    { 0,0 },  // ISOLANI_START+59
    { 0,0 },  // ISOLANI_START+60
    { 0,0 },  // ISOLANI_START+61
    { 0,0 },  // ISOLANI_START+62
    { 0,0 },  // ISOLANI_START+63
    { 0,0 },  // BLOCKED_ISOLANI_START+0
    { 0,0 },  // BLOCKED_ISOLANI_START+1
    { 0,0 },  // BLOCKED_ISOLANI_START+2
    { 0,0 },  // BLOCKED_ISOLANI_START+3
    { 0,0 },  // BLOCKED_ISOLANI_START+4
    { 0,0 },  // BLOCKED_ISOLANI_START+5
    { 0,0 },  // BLOCKED_ISOLANI_START+6
    { 0,0 },  // BLOCKED_ISOLANI_START+7
    { 34,-52 },       // BLOCKED_ISOLANI_START+8
    { -10,-31 },      // BLOCKED_ISOLANI_START+9
    { -44,-11 },      // BLOCKED_ISOLANI_START+10
    { -113,7 },       // BLOCKED_ISOLANI_START+11
    { -116,3 },       // BLOCKED_ISOLANI_START+12
    { -136,15 },      // BLOCKED_ISOLANI_START+13
    { -118,1 },       // BLOCKED_ISOLANI_START+14
    { -82,12 },       // BLOCKED_ISOLANI_START+15
    { -3,17 },        // BLOCKED_ISOLANI_START+16
    { -31,-5 },       // BLOCKED_ISOLANI_START+17
    { -89,-12 },      // BLOCKED_ISOLANI_START+18
    { -28,-12 },      // BLOCKED_ISOLANI_START+19
    { -93,-34 },      // BLOCKED_ISOLANI_START+20
    { -84,-17 },      // BLOCKED_ISOLANI_START+21
    { -173,-4 },      // BLOCKED_ISOLANI_START+22
    { -60,15 },       // BLOCKED_ISOLANI_START+23
    { 13,29 },        // BLOCKED_ISOLANI_START+24
    { -33,16 },       // BLOCKED_ISOLANI_START+25
    { -71,-4 },       // BLOCKED_ISOLANI_START+26
    { -41,-53 },      // BLOCKED_ISOLANI_START+27
    { -52,-46 },      // BLOCKED_ISOLANI_START+28
    { -57,-4 },       // BLOCKED_ISOLANI_START+29
    { -70,37 },       // BLOCKED_ISOLANI_START+30
    { -9,45 },        // BLOCKED_ISOLANI_START+31
    { -8,6 }, // BLOCKED_ISOLANI_START+32
    { 3,-35 },        // BLOCKED_ISOLANI_START+33
    { -72,-45 },      // BLOCKED_ISOLANI_START+34
    { -52,-57 },      // BLOCKED_ISOLANI_START+35
    { -57,-64 },      // BLOCKED_ISOLANI_START+36
    { -30,-45 },      // BLOCKED_ISOLANI_START+37
    { 28,-23 },       // BLOCKED_ISOLANI_START+38
    { -30,5 },        // BLOCKED_ISOLANI_START+39
    { 67,-162 },      // BLOCKED_ISOLANI_START+40
    { 19,-221 },      // BLOCKED_ISOLANI_START+41
    { -38,-160 },     // BLOCKED_ISOLANI_START+42
    { -13,-119 },     // BLOCKED_ISOLANI_START+43
    { -45,-167 },     // BLOCKED_ISOLANI_START+44
    { 46,-123 },      // BLOCKED_ISOLANI_START+45
    { 73,-69 },       // BLOCKED_ISOLANI_START+46
    { 58,-86 },       // BLOCKED_ISOLANI_START+47
    { 0,0 },  // BLOCKED_ISOLANI_START+48
    { 0,0 },  // BLOCKED_ISOLANI_START+49
    { 0,0 },  // BLOCKED_ISOLANI_START+50
    { 0,0 },  // BLOCKED_ISOLANI_START+51
    { 0,0 },  // BLOCKED_ISOLANI_START+52
    { 0,0 },  // BLOCKED_ISOLANI_START+53
    { 0,0 },  // BLOCKED_ISOLANI_START+54
    { 0,0 },  // BLOCKED_ISOLANI_START+55
    { 0,0 },  // BLOCKED_ISOLANI_START+56
    { 0,0 },  // BLOCKED_ISOLANI_START+57
    { 0,0 },  // BLOCKED_ISOLANI_START+58
    { 0,0 },  // BLOCKED_ISOLANI_START+59
    { 0,0 },  // BLOCKED_ISOLANI_START+60
    { 0,0 },  // BLOCKED_ISOLANI_START+61
    { 0,0 },  // BLOCKED_ISOLANI_START+62
    { 0,0 },  // BLOCKED_ISOLANI_START+63
    { -17,-32 },      // FORWARD_BLOCKED_BACKWARD
    { -19,-31 },      // FORWARD_CONTROLLED_BACKWARD
    { -12,-21 },      // FREE_TO_ADV_BACKWARD
    { -79,-120 },     // DOUBLE_PAWN_FILE_START+0
    { 5,-78 },        // DOUBLE_PAWN_FILE_START+1
    { 7,-67 },        // DOUBLE_PAWN_FILE_START+2
    { 15,-60 },       // DOUBLE_PAWN_FILE_START+3
    { 16,-43 },       // DOUBLE_PAWN_FILE_START+4
    { 12,-69 },       // DOUBLE_PAWN_FILE_START+5
    { 16,-76 },       // DOUBLE_PAWN_FILE_START+6
    { -45,-170 },     // DOUBLE_PAWN_FILE_START+7
    { 38,-27 },       // PAWN_SHIELD_BONUS
    { -227,3 },       // DIRECTLY_ON_OPEN_FILE_NEXT_TO_OPEN_PENALTY
    { -253,21 },      // DIRECTLY_ON_OPEN_FILE_NOT_NEXT_TO_OPEN_PENALTY
    { -130,-4 },      // NEXT_TO_OPEN_FILE_PENALTY
    { 31,13 },        // DIRECTLY_ON_SEMI_OPEN_FILE_NEXT_TO_OPEN_PENALTY
    { -20,-41 },      // DIRECTLY_ON_SEMI_OPEN_FILE_NOT_NEXT_TO_OPEN_PENALTY
    { 1,-9 }, // NEXT_TO_SEMI_OPEN_FILE_PENALTY
    { -69,-138 },     // NEXT_TO_OPEN_DIAGONAL_PENALTY_START+0
    { -17,-52 },      // NEXT_TO_OPEN_DIAGONAL_PENALTY_START+1
    { 20,25 },        // NEXT_TO_OPEN_DIAGONAL_PENALTY_START+2
    { 97,-17 },       // NEXT_TO_OPEN_DIAGONAL_PENALTY_START+3
    { 112,47 },       // NEXT_TO_OPEN_DIAGONAL_PENALTY_START+4
    { 0,0 },  // NEXT_TO_OPEN_DIAGONAL_PENALTY_START+5
    { 0,0 },  // NEXT_TO_OPEN_DIAGONAL_PENALTY_START+6
    // MOBILITY values
{ 0,0 },  // MOBILITY_START+0
{ 11,9 }, // MOBILITY_START+1
{ 11,7 }, // MOBILITY_START+2
{ 7,6 },  // MOBILITY_START+3
{ 2,14 }, // ROOK_BEHIND_FREE_PAWN_START+0
{ -41,-8 },       // ROOK_BEHIND_FREE_PAWN_START+1
{ -47,-36 },      // ROOK_BEHIND_FREE_PAWN_START+2
{ 4,-13 },        // ROOK_BEHIND_FREE_PAWN_START+3
{ 40,55 },        // ROOK_BEHIND_FREE_PAWN_START+4
{ 81,144 },       // ROOK_BEHIND_FREE_PAWN_START+5
{ 35,155 },       // ROOK_BEHIND_FREE_PAWN_START+6
{ 0,0 },  // ROOK_BEHIND_FREE_PAWN_START+7
{ 65,23 },        // ROOK_ON_OPEN_FILE
{ 29,-13 },       // ROOK_ON_SEMI_OPEN_FILE
{ 8,0 },  // CONNECTED_ROOKS
{ 30,184 },       // BISHOP_PAIR
{ -16,-54 },      // BAD_BISHOP_BLOCKED
{ -4,-6 },        // BAD_BISHOP_UNBLOCKED
{ -319,-288 },    // TRAPPED_BISHOP
{ -167,-359 },    // TRAPPED_KNIGHT
{ 27,4 }, // FIANCHETTO_BISHOP
{ 9,-29 },        // BROKEN_FIANCHETTO
{ 101,24 },       // BISHOP_OUTPOST_NO_OPPOSITE_BISHOP
{ 56,52 },        // BISHOP_OUTPOST_WITH_OPPOSITE_BISHOP
{ 102,29 },       // KNIGHT_OUTPOST_NO_OPPOSITE_BISHOP
{ 72,49 },        // KNIGHT_OUTPOST_WITH_OPPOSITE_BISHOP
};

