#include "evaluation.h"
#include "constants.h"
#include "pst.h"
#include "utils.h"
#include "bitboard_masks.h"
#include "Evaluation_data.h"
// --- Forward Declarations for static helper functions ---
static int evaluate_material(const Board& board);
static int evaluate_positional(const Board& board);
static int evaluate_pawn_structure_score(const Board& board,const uint64_t& white_pawns, const uint64_t& black_pawns);
static int eval_king_safety_score(const Board& board, const uint64_t& white_pawns, const uint64_t& black_pawns);
static int eval_iso_passed_pawns(const Board& board, Color color,const uint64_t& my_pawns,const uint64_t& opponent_pawns);
static int eval_king_safety_by_color(const Board& board, int king_sq, Color color, const uint64_t& friendly_pawns);

static int evaluate_material(const Board& board){
    return board.get_material_score();
}
static int evaluate_positional(const Board& board){
    return board.get_positional_score();
}

static int eval_iso_passed_pawns(const Board& board, const Color color, const uint64_t& my_pawns, const uint64_t& op_pawns){
    int score=0;
        uint64_t pawns=my_pawns;
        Color other_color=color==Color::WHITE ? Color::BLACK : Color::WHITE;
        int weight=color==Color::BLACK ? -1:1;
        while (pawns)
        {
            int pawn_square=get_lsb(pawns);
            int file_index=pawn_square % 8;
            if ((my_pawns & ADJACENT_FILE_MASK[file_index])==0)
            {
                score+=ISOLATED_PAWN_PENALTY*weight;

            }
            if ((op_pawns & PASSED_PAWN_MASK[to_int(color)][pawn_square])==0)
            {
                score+=PASSED_PAWN_BONUS[to_int(color)][pawn_square/8]*weight;
            }
            pawns&=pawns-1;           
        }
    return score;   
}
static int evaluate_pawn_structure_score(const Board& board, const uint64_t& white_pawns,const uint64_t& black_pawns){
    int score=0;
    for (size_t file = 0; file < 8; ++file)
    {   
        uint64_t file_mask=FILE_MASK[file];
        int white_doubled=popcount(white_pawns & file_mask);
        if (white_doubled>1)
        {
            score+=DOUBLED_PAWN_PENALTY*(white_doubled-1);
            
            
        }
        int black_doubled=popcount(black_pawns& file_mask);
        if (black_doubled>1)
        {
            score-=DOUBLED_PAWN_PENALTY*(black_doubled-1);
        }
        
    }
    score+=eval_iso_passed_pawns(board,Color::WHITE,white_pawns,black_pawns);
    score+=eval_iso_passed_pawns(board,Color::BLACK,black_pawns,white_pawns);
    return score;
    
}
static int eval_king_safety_score(const Board& board, const uint64_t& white_pawns, const uint64_t& black_pawns){
    int score=0;
    int white_king_square=board.get_king_square(Color::WHITE);
	int black_king_square = board.get_king_square(Color::BLACK);
    score+=eval_king_safety_by_color(board, white_king_square,Color::WHITE,white_pawns);
    score-=eval_king_safety_by_color(board,black_king_square,Color::BLACK,black_pawns);
    return (score*board.get_game_phase())/24;
}
static int eval_king_safety_by_color(const Board& board,int king_sq,Color color,const uint64_t& friendly_pawns){
        if (king_sq==-1) return 0;
        int score=0;
        uint64_t shield_mask=KING_SHIELD[to_int(color)][king_sq];
        int shield_pawns_count=popcount(friendly_pawns & shield_mask);
        int safety_score= shield_pawns_count*PAWN_SHIELD_BONUS;

        uint64_t zone_mask=KING_ZONE[king_sq];
        uint64_t enemy_pieces=board.get_color_pieces(color==Color::WHITE ? Color::BLACK : Color::WHITE);
        uint64_t attacker_in_zone=zone_mask & enemy_pieces;
        int attacker_score=0;

        while (attacker_in_zone)
        {   
            int att_sq=get_lsb(attacker_in_zone);
            int piece=to_int(board.get_piece_on_square(att_sq));
            attacker_score+=(ATTACKER_WEIGHTS[piece]*DISTANCE_BONUS[king_sq][att_sq]) >> 1;
            attacker_in_zone &=attacker_in_zone-1;
        }
        return safety_score-attacker_score;
        


        
}
int evaluate(const Board& board){
	uint64_t white_pawns = board.get_pieces(Color::WHITE, PieceType::PAWN);
	uint64_t black_pawns = board.get_pieces(Color::BLACK, PieceType::PAWN);
    int material_score=evaluate_material(board);
    int positional_score=evaluate_positional(board);
    int pawn_struct_score=evaluate_pawn_structure_score(board,white_pawns,black_pawns);
    int king_safety_score=eval_king_safety_score(board,white_pawns,black_pawns);

    int final_score=material_score+positional_score+pawn_struct_score+king_safety_score;

    return final_score;
}