#pragma once
#include <string>
#include <map>
#include <cstdint>
#include <vector>



//color and piece constants
enum class Color { WHITE, BLACK, NONE };
enum class PieceType { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NONE };
inline const uint8_t WHITE_KING_CASTLE = 1U << 3;
inline const uint8_t WHITE_QUEEN_CASTLE = 1U << 2;
inline const uint8_t BLACK_KING_CASTLE = 1U << 1;
inline const uint8_t BLACK_QUEEN_CASTLE = 1U << 0;


//FEN-constants
const std::string STARTING_FEN="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
const std::map<char, PieceType> PIECE_TYPE_MAP = {
    {'P', PieceType::PAWN}, {'N', PieceType::KNIGHT}, {'B', PieceType::BISHOP}, 
    {'R', PieceType::ROOK}, {'Q', PieceType::QUEEN}, {'K', PieceType::KING},
    {'.',PieceType::NONE},{'p', PieceType::PAWN}, {'n', PieceType::KNIGHT}, {'b', PieceType::BISHOP}, 
    {'r', PieceType::ROOK}, {'q', PieceType::QUEEN}, {'k', PieceType::KING},
};
const std::string PIECE_CHAR_LIST="PNBRQK.";



// constants for evaluation
const int MATE_SCORE=10000000;
const int MAX_PLY=128;
const int MATE_THRESHOLD=MATE_SCORE-MAX_PLY;
const int PIECE_VALUES[2][6] = {
    {100,320,330,500,900,20000},
    {-100,-320,-330,-500,-900,-20000}
};
const int PHASE_WEIGHTS[7]={0,1,1,2,4,0,0};
const int DOUBLED_PAWN_PENALTY=-17;
const int ISOLATED_PAWN_PENALTY=-23;
const int PASSED_PAWN_BONUS[2][8]={{0,10,20,35,50,75,100,0}, {0,100,75,50,35,20,10,0}};
const int PAWN_SHIELD_BONUS=15;
const int OPEN_FILE_PENALTY=-40;
const int SEMI_OPEN_FILE_PENALY=-20;
const int ATTACKER_WEIGHTS[6]={0,10,10,15,25,0};
const int FUTILITY_MARGIN_D1=200;
const int FUTILITY_MARGIN_D2=400;
const int DELTA_MARGIN=200;


// constants for ray_mask
const int DIRECTIONS[]={7,8,9,1,-7,-8,-9,-1};
const std::vector<int> QUEEN_DIR_IND={0,1,2,3,4,5,6,7};
const std::vector<int> ROOK_DIR_IND={1,3,5,7};
const std::vector<int> BISHOP_DIR_IND={0,2,4,6};
const uint64_t BOARD_ALL_SET=0xFFFFFFFFFFFFFFFFULL;
const uint64_t NOT_FILE_A=0xfefefefefefefefe;
const uint64_t NOT_FILE_H=0x7f7f7f7f7f7f7f7f;