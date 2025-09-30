#include "evaluation.h"
#include "constants.h"
#include "pst.h"
#include "utils.h"
#include "bitboard_masks.h"
#include "Evaluation_data.h"
#include "attack_rays.h"
// --- Forward Declarations for static helper functions ---
static int evaluate_material(const Board& board);
static int evaluate_positional(const Board& board);
static int evaluate_pawn_structure_score(const Board& board,const uint64_t& white_pawns, const uint64_t& black_pawns);
static int eval_king_safety_score(const Board& board, const uint64_t& white_pawns, const uint64_t& black_pawns);
static int eval_iso_passed_pawns(const Board& board, Color color,const uint64_t& my_pawns,const uint64_t& opponent_pawns);
static KingSafetyScore eval_king_safety_by_color(const Board& board, int king_sq, Color color, const uint64_t& friendly_pawns);

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
static int eval_king_safety_score(const Board& board, const uint64_t& white_pawns, const uint64_t& black_pawns,const int& game_phase){
    int score=0;
    int white_king_square=board.get_king_square(Color::WHITE);
	int black_king_square = board.get_king_square(Color::BLACK);
    KingSafetyScore white_ks=eval_king_safety_by_color(board, white_king_square,Color::WHITE,white_pawns);
    KingSafetyScore black_ks=eval_king_safety_by_color(board,black_king_square,Color::BLACK,black_pawns);
	int mg_king_safety = white_ks.mg_score - black_ks.mg_score;
	int eg_king_safety = white_ks.eg_score - black_ks.eg_score;
    int king_safety_score = (mg_king_safety * game_phase + eg_king_safety * (24 - game_phase)) / 24;

    return king_safety_score;
}
static KingSafetyScore eval_king_safety_by_color(const Board& board,int king_sq,Color color,const uint64_t& friendly_pawns){
    if (king_sq == -1) return { 0,0 };
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
        return { safety_score - attacker_score,-(attacker_score) / 2 };
        


        
}
static int evaluate_rook_activity(const Board& board,const uint64_t white_pawns, const uint64_t& black_pawns, const uint64_t& white_rooks, const uint64_t& black_rooks){
    int score=0;
    score += popcount(white_rooks & RANK_MASK[6]) * ROOK_ON_SEVENTH_BONUS;
	score -= popcount(black_rooks & RANK_MASK[1]) * ROOK_ON_SEVENTH_BONUS;
    for (uint64_t file : FILE_MASK) {
		bool white_has_pawns = (white_pawns & file) != 0;
		bool black_has_pawns = (black_pawns & file) != 0;
        if (!white_has_pawns) {
            int white_rooks_on_file = popcount(white_rooks & file);
            if (!black_has_pawns) {
                score += OPEN_FILE_BONUS * white_rooks_on_file;
            }
            else {
                score += SEMI_OPEN_FILE_BONUS * white_rooks_on_file;
            }
        }
        if (!black_has_pawns) {
            int black_rooks_on_file = popcount(black_rooks & file);
            if (!white_has_pawns) {
                score -= OPEN_FILE_BONUS * black_rooks_on_file;
            }
            else {
                score -= SEMI_OPEN_FILE_BONUS * black_rooks_on_file;
            }
		}
    }
	return score;
}
static int evaluate_bishop_pair(const Board& board,const uint64_t& white_bishops, const uint64_t& black_bishops){
    int score=0;
    if (popcount(white_bishops)>=2)
    {
        score+=BISHOP_PAIR_BONUS;
    }
    if (popcount(black_bishops)>=2)
    {
        score-=BISHOP_PAIR_BONUS;
    }
    return score;
}
static int evaluate_mobility(uint64_t white_knights, uint64_t black_knights, uint64_t white_bishops, uint64_t black_bishops, uint64_t white_rooks, 
    uint64_t black_rooks, uint64_t white_queens, uint64_t black_queens, uint64_t white_pieces, uint64_t black_pieces, uint64_t all_pieces,const int& game_phase){
    int mobility=0;
    while (white_knights)
    {
        int knight_sq=get_lsb(white_knights);
        mobility+=popcount(KNIGHT_ATTACKS[knight_sq] & ~white_pieces)*4;
        white_knights&=white_knights-1;
    }
    while (black_knights)
    {
        int knight_sq=get_lsb(black_knights);
        mobility-=popcount(KNIGHT_ATTACKS[knight_sq] & ~black_pieces)*4;
        black_knights&=black_knights-1;
	}
    while (white_bishops)
    {
        int bishop_sq=get_lsb(white_bishops);
        mobility+=popcount(get_bishop_attacks(bishop_sq,all_pieces) & ~white_pieces)*5;
        white_bishops&=white_bishops-1;
    }
    while (black_bishops)
    {
        int bishop_sq=get_lsb(black_bishops);
        mobility-=popcount(get_bishop_attacks(bishop_sq,all_pieces) & ~black_pieces)*5;
        black_bishops&=black_bishops-1;
	}
    while (white_rooks)
    {
        int rook_sq=get_lsb(white_rooks);
        mobility+=popcount(get_rook_attacks(rook_sq,all_pieces) & ~white_pieces)*(2*game_phase+4*(24-game_phase))/24;
        white_rooks&=white_rooks-1;
    }
    while (black_rooks)
    {
        int rook_sq = get_lsb(black_rooks);
        mobility -= popcount(get_rook_attacks(rook_sq, all_pieces) & ~black_pieces)* (2 * game_phase + 4 * (24 - game_phase)) / 24;
        black_rooks &= black_rooks - 1;
    }
    while (white_queens)
    {
            int queen_sq = get_lsb(white_queens);
            mobility += popcount(get_queen_attacks(queen_sq, all_pieces) & ~white_pieces)*(1 * game_phase + 2 * (24 - game_phase)) / 24;
            white_queens &= white_queens - 1;
    }
    while (black_queens)
    {
            int queen_sq = get_lsb(black_queens);
            mobility -= popcount(get_queen_attacks(queen_sq, all_pieces) & ~black_pieces)*(1 * game_phase + 2 * (24 - game_phase)) / 24;
            black_queens &= black_queens - 1;
    }
    return mobility;
    }
// dont use this
static int evaluate_center(const uint64_t& white_knights, const uint64_t& black_knights, const uint64_t& white_bishops, const uint64_t& black_bishops, const uint64_t& white_rooks,
    const uint64_t& black_rooks, const uint64_t& white_queens, const uint64_t& black_queens, const uint64_t& white_pieces, const uint64_t& black_pieces, const uint64_t& all_pieces) {
	int score = 0;
	uint64_t d4_mask = 1ULL << 27;
	uint64_t e4_mask = 1ULL << 28;
	uint64_t d5_mask = 1ULL << 35;
	uint64_t e5_mask = 1ULL << 36;
    uint64_t rook_attackersd4 = get_rook_attacks(27, all_pieces);
	uint64_t rook_attackerse4 = get_rook_attacks(28, all_pieces);
	uint64_t rook_attackersd5 = get_rook_attacks(35, all_pieces);
	uint64_t rook_attackerse5 = get_rook_attacks(36, all_pieces);
	uint64_t bishop_attackersd4 = get_bishop_attacks(27, all_pieces);
	uint64_t bishop_attackerse4 = get_bishop_attacks(28, all_pieces);
	uint64_t bishop_attackersd5 = get_bishop_attacks(35, all_pieces);
	uint64_t bishop_attackerse5 = get_bishop_attacks(36, all_pieces);
	uint64_t queen_attackersd4 = rook_attackersd4 | bishop_attackersd4;
	uint64_t queen_attackerse4 = rook_attackerse4 | bishop_attackerse4;
	uint64_t queen_attackersd5 = rook_attackersd5 | bishop_attackersd5;
	uint64_t queen_attackerse5 = rook_attackerse5 | bishop_attackerse5;
	uint64_t knight_attackersd4 = KNIGHT_ATTACKS[27];
	uint64_t knight_attackerse4 = KNIGHT_ATTACKS[28];
	uint64_t knight_attackersd5 = KNIGHT_ATTACKS[35];
	uint64_t knight_attackerse5 = KNIGHT_ATTACKS[36];
	score += popcount(rook_attackersd4 & white_rooks) * 2;
	score -= popcount(rook_attackersd4 & black_rooks) * 2;
	score += popcount(rook_attackerse4 & white_rooks) * 2;
	score -= popcount(rook_attackerse4 & black_rooks) * 2;
	score += popcount(rook_attackersd5 & white_rooks) * 2;
	score -= popcount(rook_attackersd5 & black_rooks) * 2;
	score += popcount(rook_attackerse5 & white_rooks) * 2;
	score -= popcount(rook_attackerse5 & black_rooks) * 2;
	score += popcount(bishop_attackersd4 & white_bishops) * 3;
	score -= popcount(bishop_attackersd4 & black_bishops) * 3;
	score += popcount(bishop_attackerse4 & white_bishops) * 3;
	score -= popcount(bishop_attackerse4 & black_bishops) * 3;
	score += popcount(bishop_attackersd5 & white_bishops) * 3;
	score -= popcount(bishop_attackersd5 & black_bishops) * 3;
	score += popcount(bishop_attackerse5 & white_bishops) * 3;
	score -= popcount(bishop_attackerse5 & black_bishops) * 3;
	score += popcount(queen_attackersd4 & white_queens) * 4;
	score -= popcount(queen_attackersd4 & black_queens) * 4;
	score += popcount(queen_attackerse4 & white_queens) * 4;
	score -= popcount(queen_attackerse4 & black_queens) * 4;
	score += popcount(queen_attackersd5 & white_queens) * 4;
	score -= popcount(queen_attackersd5 & black_queens) * 4;
	score += popcount(queen_attackerse5 & white_queens) * 4;
	score -= popcount(queen_attackerse5 & black_queens) * 4;
	score += popcount(knight_attackersd4 & white_knights) * 3;
	score -= popcount(knight_attackersd4 & black_knights) * 3;
	score += popcount(knight_attackerse4 & white_knights) * 3;
	score -= popcount(knight_attackerse4 & black_knights) * 3;
	score += popcount(knight_attackersd5 & white_knights) * 3;
	score -= popcount(knight_attackersd5 & black_knights) * 3;
	score += popcount(knight_attackerse5 & white_knights) * 3;
	score -= popcount(knight_attackerse5 & black_knights) * 3;
	return score;

}
static int evaluate_outpost(const uint64_t& white_knights, const uint64_t& black_knights, const uint64_t& white_bishops, const uint64_t& black_bishops,
    const uint64_t& white_pawns, const uint64_t& black_pawns) {
    int score = 0;
	uint64_t white_pawn_attacks = get_pawn_attacks(white_pawns,Color::WHITE);
	uint64_t black_pawn_attacks = get_pawn_attacks(black_pawns,Color::BLACK);
	uint64_t possible_light_white_outpost = white_pawn_attacks & WHITE_OUTPOST_MASK & (white_bishops | white_knights) & LIGHT_SQUARES;
	uint64_t possible_dark_white_outpost = white_pawn_attacks & WHITE_OUTPOST_MASK & (white_bishops | white_knights) & DARK_SQUARES;
	uint64_t possible_light_black_outpost = black_pawn_attacks & BLACK_OUTPOST_MASK & (black_bishops | black_knights) & LIGHT_SQUARES;
	uint64_t possible_dark_black_outpost = black_pawn_attacks & BLACK_OUTPOST_MASK & (black_bishops | black_knights) & DARK_SQUARES;
	bool white_light_bishop = (white_bishops & LIGHT_SQUARES) != 0;
	bool white_dark_bishop = (white_bishops & DARK_SQUARES) != 0;
	bool black_light_bishop = (black_bishops & LIGHT_SQUARES) != 0;
	bool black_dark_bishop = (black_bishops & DARK_SQUARES) != 0;
    while (possible_light_white_outpost)
    {
        int square = get_lsb(possible_light_white_outpost);
        if((black_pawns & OUTPOST_MASK[to_int(Color::WHITE)][square]) == 0) {
            if(black_light_bishop) {
                score += 20;
            } else {
                score += 30;
			}
        } 
		possible_light_white_outpost &= possible_light_white_outpost - 1;
    }
    while (possible_dark_white_outpost)
    {
        int square = get_lsb(possible_dark_white_outpost);
        if ((black_pawns & OUTPOST_MASK[to_int(Color::WHITE)][square]) == 0) {
            if (black_dark_bishop) {
                score += 20;
            }
            else {
                score += 30;
            }
        }
		possible_dark_white_outpost &= possible_dark_white_outpost - 1;
    }
    while (possible_light_black_outpost)
    {
        int square = get_lsb(possible_light_black_outpost);
        if ((white_pawns & OUTPOST_MASK[to_int(Color::BLACK)][square]) == 0) {
            if (white_light_bishop) {
                score -= 20;
            }
            else {
                score -= 30;
            }
        }
        possible_light_black_outpost &= possible_light_black_outpost - 1;
	}
    while (possible_dark_black_outpost)
    {
        int square = get_lsb(possible_dark_black_outpost);
        if ((white_pawns & OUTPOST_MASK[to_int(Color::BLACK)][square]) == 0) {
            if (white_dark_bishop) {
                score -= 20;
            }
            else {
                score -= 30;
            }
        }
        possible_dark_black_outpost &= possible_dark_black_outpost - 1;
	}



    return score;
}

int evaluate(const Board& board){
	uint64_t white_pawns = board.get_pieces(Color::WHITE, PieceType::PAWN);
	uint64_t black_pawns = board.get_pieces(Color::BLACK, PieceType::PAWN);
	uint64_t white_knights = board.get_pieces(Color::WHITE, PieceType::KNIGHT);
	uint64_t black_knights = board.get_pieces(Color::BLACK, PieceType::KNIGHT);
	uint64_t white_bishops = board.get_pieces(Color::WHITE, PieceType::BISHOP);
	uint64_t black_bishops = board.get_pieces(Color::BLACK, PieceType::BISHOP);
	uint64_t white_rooks = board.get_pieces(Color::WHITE, PieceType::ROOK);
	uint64_t black_rooks = board.get_pieces(Color::BLACK, PieceType::ROOK);
	uint64_t white_queens = board.get_pieces(Color::WHITE, PieceType::QUEEN);
	uint64_t black_queens = board.get_pieces(Color::BLACK, PieceType::QUEEN);
	uint64_t white_pieces = board.get_color_pieces(Color::WHITE);
	uint64_t black_pieces = board.get_color_pieces(Color::BLACK);
    int game_phase = board.get_game_phase();
	uint64_t all_pieces = white_pieces | black_pieces;
    int material_score=evaluate_material(board);
    int positional_score=evaluate_positional(board);
    int pawn_struct_score=evaluate_pawn_structure_score(board,white_pawns,black_pawns);
    int king_safety_score=eval_king_safety_score(board,white_pawns,black_pawns,game_phase);
	int rook_activity_score = evaluate_rook_activity(board,white_pawns,black_pawns,white_rooks,black_rooks);
	int bishop_pair_score = evaluate_bishop_pair(board,white_bishops,black_bishops);
    int mobility_score= evaluate_mobility(white_knights,black_knights,white_bishops,black_bishops,
		white_rooks, black_rooks, white_queens, black_queens, white_pieces, black_pieces, all_pieces,game_phase);
    //int outpost_score = evaluate_outpost(white_knights, black_knights, white_bishops, black_bishops, white_pawns, black_pawns);
    int final_score = material_score + positional_score + pawn_struct_score + king_safety_score
        + rook_activity_score + bishop_pair_score + mobility_score;// +outpost_score;

    return final_score;
}