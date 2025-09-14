#include "evaluation.h"
#include "constants.h"
#include "pst.h"
#include "utils.h"
#include "bitboard_masks.h"

// --- Forward Declarations for static helper functions ---
static int evaluate_material(const Board& board);
static double evaluate_positional(const Board& board);
static double evaluate_pawn_structure_score(const Board& board);
static double eval_king_safety_score(const Board& board);
static double eval_iso_passed_pawns(const Board& board, Color color);
static double eval_king_safety_by_color(const Board& board, int king_sq, Color color);

static int evaluate_material(const Board& board){
    return board.get_material_score();
}
static double evaluate_positional(const Board& board){
    return board.get_positional_score();
}

static double eval_iso_passed_pawns(const Board& board, const Color color){
    double score=0;
        uint64_t pawns=board.get_pieces(color,PieceType::PAWN);
        Color other_color=color==Color::WHITE ? Color::BLACK : Color::WHITE;
        int weight=color==Color::BLACK ? -1:1;
        while (pawns)
        {
            int pawn_square=get_lsb(pawns);
            int file_index=pawn_square % 8;
            if ((board.get_pieces(color,PieceType::PAWN) & ADJACENT_FILE_MASK[file_index])==0)
            {
                score+=ISOLATED_PAWN_PENALTY*weight;

            }
            if ((board.get_pieces(other_color,PieceType::PAWN)& PASSED_PAWN_MASK[to_int(color)][pawn_square])==0)
            {
                score+=PASSED_PAWN_BONUS[to_int(color)][pawn_square/8]*weight;
            }
            pawns&=pawns-1;           
        }
    return score;   
}
static double evaluate_pawn_structure_score(const Board& board){
    double score=0;
    for (size_t file = 0; file < 8; ++file)
    {   
        uint64_t file_mask=FILE_MASK[file];
        int white_doubled=popcount(board.get_pieces(Color::WHITE,PieceType::PAWN) & file_mask);
        if (white_doubled>1)
        {
            score+=DOUBLED_PAWN_PENALTY*(white_doubled-1);
            
            
        }
        int black_doubled=popcount(board.get_pieces(Color::BLACK,PieceType::PAWN)& file_mask);
        if (black_doubled>1)
        {
            score-=DOUBLED_PAWN_PENALTY*(black_doubled-1);
        }
        
    }
    score+=eval_iso_passed_pawns(board,Color::WHITE);
    score+=eval_iso_passed_pawns(board,Color::BLACK);
    return score;
    
}
static double eval_king_safety_score(const Board& board){
    double score=0;
    int white_king_square=get_lsb(board.get_pieces(Color::WHITE,PieceType::KING));
    int black_king_square=get_lsb(board.get_pieces(Color::BLACK,PieceType::KING));
    score+=eval_king_safety_by_color(board, white_king_square,Color::WHITE);
    score-=eval_king_safety_by_color(board,black_king_square,Color::BLACK);
    return score*board.get_game_phase();
}
static double eval_king_safety_by_color(const Board& board,int king_sq,Color color){
        if (king_sq==-1) return 0;
        double score=0;
        uint64_t friendly_pawns=board.get_pieces(color,PieceType::PAWN);
        uint64_t shield_mask=KING_SHIELD[to_int(color)][king_sq];
        int shield_pawns_count=popcount(friendly_pawns & shield_mask);
        double safety_score= shield_pawns_count*PAWN_SHIELD_BONUS;

        uint64_t zone_mask=KING_ZONE[king_sq];
        uint64_t enemy_pieces=board.get_color_pieces(color==Color::WHITE ? Color::BLACK : Color::WHITE);
        uint64_t attacker_in_zone=zone_mask & enemy_pieces;
        double attacker_score=0;

        while (attacker_in_zone)
        {   
            int att_sq=get_lsb(attacker_in_zone);
            int piece=to_int(board.get_piece_on_square(att_sq));
            attacker_score+=ATTACKER_WEIGHTS[piece]*(6-std::abs(att_sq % 8-king_sq %8)-std::abs(att_sq/8-king_sq/8))/4;
            attacker_in_zone &=attacker_in_zone-1;
        }
        return safety_score-attacker_score;
        


        
}
double evaluate(const Board& board){
    int material_score=evaluate_material(board);
    double positional_score=evaluate_positional(board);
    double pawn_struct_score=evaluate_pawn_structure_score(board);
    double king_safety_score=eval_king_safety_score(board);

    double final_score=material_score+positional_score+pawn_struct_score+king_safety_score;

    return final_score;
}