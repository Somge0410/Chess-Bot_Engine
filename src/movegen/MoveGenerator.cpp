
#include "MoveGenerator.h"
#include <cstdint>
#include "bitboard.h"
#include "bitboard_masks.h"
#include <array>
#include "attack_rays.h"
#include "bitboard_masks.h"
#include "rook_tables.h"
#include "bishop_tables.h"
#include "attacks.h"

const int DIRECTIONS[]={7,8,9,1,-7,-8,-9,-1};
const std::vector<int> QUEEN_DIR_IND={0,1,2,3,4,5,6,7};
const std::vector<int> ROOK_DIR_IND={1,3,5,7};
const std::vector<int> BISHOP_DIR_IND={0,2,4,6};
MoveGenerator::MoveGenerator()
{
    
}
template <bool captures_only, bool with_checks>
void MoveGenerator::generate_moves(const Position& pos,MoveList& move_list){
	generate_moves<captures_only, with_checks>(pos, move_list, pos.get_checkers());
}

template <bool captures_only, bool with_checks>
void MoveGenerator::generate_moves(const Position& pos, MoveList& move_list, Bitboard checkers){
	//if (pos.is_fifty_move_rule_draw() || pos.is_repetition_draw()) return move_list;
    Color own_color=pos.get_turn();
    Color opponent_color=own_color==Color::White ? Color::Black:Color::White;
    Square king_square=pos.get_king_square(own_color);
	Bitboard pinned_info = calculate_pinned_pieces(pos, own_color,opponent_color,king_square);
	const int check_count = popcount(checkers);
    Bitboard own_pieces=pos.get_color_pieces(own_color);
    if (check_count>1)
    { 
        generate_king_moves<captures_only>(move_list, pos, own_color, own_pieces, king_square);
    }else if (check_count==1)
    {  
        generate_king_moves<captures_only>(move_list,pos, own_color,own_pieces ,king_square);
        
        const Square checker_square = lsb(checkers);
        PieceType checker=pos.get_piece_type_on_square(checker_square);
        Bitboard remedy_mask=bit64(checker_square);
        if (checker==PieceType::Queen || checker==PieceType::Rook|| checker==PieceType::Bishop) remedy_mask |= LINE_BETWEEN(king_square, checker_square);
        
        generate_queen_moves<captures_only,with_checks>(move_list,pos, own_color, pinned_info, remedy_mask);
        
        generate_rook_moves<captures_only,with_checks>(move_list,pos, own_color, pinned_info, remedy_mask);

        generate_bishop_moves<captures_only,with_checks>(move_list,pos, own_color, pinned_info, remedy_mask);
        generate_knight_moves<captures_only,with_checks>(move_list,pos, own_color, pinned_info, remedy_mask);
        
        if (pos.get_en_passant_rights() !=NO_SQUARE && checker == PieceType::Pawn) remedy_mask|=1ULL<<pos.get_en_passant_rights();
        generate_pawn_moves<captures_only, with_checks>(move_list,pos, own_color,king_square, pinned_info, remedy_mask);
    }
    else
    {
        generate_queen_moves<captures_only,with_checks>(move_list, pos, own_color, pinned_info, BOARD_ALL_SET);

        generate_rook_moves<captures_only,with_checks>(move_list, pos, own_color, pinned_info, BOARD_ALL_SET);

        generate_bishop_moves<captures_only,with_checks>(move_list, pos, own_color, pinned_info, BOARD_ALL_SET);
        generate_knight_moves<captures_only,with_checks>(move_list, pos, own_color, pinned_info, BOARD_ALL_SET);

        generate_pawn_moves<captures_only,with_checks>(move_list, pos, own_color, king_square, pinned_info, BOARD_ALL_SET);

        generate_king_moves<captures_only>(move_list, pos, own_color, own_pieces, king_square);
    }
}
Bitboard MoveGenerator::calculate_pinned_pieces(const Position& pos, const Color friendly_color,const Color opponent_color, Square king_square) {
	Bitboard all_rook_bockers = rook_attacks(to_square(king_square), pos.get_all_pieces());
	Bitboard all_bishop_blockers = bishop_attacks(to_square(king_square), pos.get_all_pieces());
    Bitboard possible_rook_pinned = all_rook_bockers & pos.get_color_pieces(friendly_color) & ~FOUR_CORNER_MASK;
    Bitboard possible_bishop_pinned = all_bishop_blockers & pos.get_color_pieces(friendly_color) & ~FOUR_CORNER_MASK;
    Bitboard pinned_info=0;
    Bitboard opponent_rooks_queens = pos.get_pieces(opponent_color, PieceType::Rook) | pos.get_pieces(opponent_color, PieceType::Queen);
    Bitboard opponent_bishops_queens = pos.get_pieces(opponent_color, PieceType::Bishop) | pos.get_pieces(opponent_color, PieceType::Queen);
    if (opponent_rooks_queens != 0) {
		Bitboard second_blockers = rook_attacks(to_square(king_square), pos.get_all_pieces()^possible_rook_pinned) & opponent_rooks_queens&~all_rook_bockers;
        while (second_blockers) {
            Square second_blocker_sq = lsb(second_blockers);
            Bitboard between_mask = LINE_BETWEEN(king_square, second_blocker_sq);
            Square first_blocker_sq = lsb(between_mask & possible_rook_pinned);
            pinned_info |= bit64(first_blocker_sq);
            second_blockers &= second_blockers - 1;
        }
    }
    if (opponent_bishops_queens != 0) {
        Bitboard second_blockers = bishop_attacks(to_square(king_square), pos.get_all_pieces() ^possible_bishop_pinned) & opponent_bishops_queens & ~all_bishop_blockers;
        while (second_blockers) {
            Square second_blocker_sq = lsb(second_blockers);
            Bitboard between_mask = LINE_BETWEEN(king_square, second_blocker_sq);
            Square first_blocker_sq = lsb(between_mask & possible_bishop_pinned);
			pinned_info |= bit64(first_blocker_sq);
            second_blockers &= second_blockers - 1;
        }
    }
	return pinned_info;
      
}

template <bool captures_only>
void MoveGenerator::generate_king_moves(MoveList& moves,const Position& pos,const Color own_color, const Bitboard& own_pieces, Square king_square){
        Bitboard possible_moves=KING_ATTACKS[king_square]&~own_pieces;
		Color other_color = flip_color(own_color);
		Bitboard enemy_pieces = pos.get_color_pieces(other_color);
		Bitboard quiet = possible_moves & ~enemy_pieces;
		Bitboard captures = possible_moves & enemy_pieces;
       
        while (captures)
        {
            Square destination_square = pop_lsb(captures);
            if (pos.attackers_more_than<false>(destination_square, other_color, 1).count > 0)
            {
                continue;
            }
            moves.push_back(Move(king_square, destination_square, PieceType::King, own_color,
                pos.get_piece_type_on_square(other_color,destination_square)));
        }
		if (captures_only) return;
        
        while (quiet) {

            Square destination_square = pop_lsb(quiet);
            if (pos.attackers_more_than<false>(destination_square, other_color, 1).count > 0)
            {
                continue;
            }
            moves.push_back(Move(king_square, destination_square, PieceType::King, own_color,PieceType::None));
        }

        if (pos.attackers_more_than<false>(to_square(king_square),other_color,1).count>0) return;

        CastlingRights king_castle_mask = own_color == Color::White ? CastlingRights::WhiteKingside : CastlingRights::BlackKingside;
		CastlingRights queen_castle_mask = own_color == Color::White ? CastlingRights::WhiteQueenside : CastlingRights::BlackQueenside;
        if ((pos.get_castle_rights() & king_castle_mask)!= 0)
        {   
            Square rook_square=king_square+3;
            Bitboard line_between = LINE_BETWEEN(king_square + 1, king_square + 2);
    
            if ((line_between & pos.get_all_pieces())==0)
            {   
                
                 if (pos.attackers_more_than<false>(to_square(king_square + 1), other_color, 1).count == 0 && pos.attackers_more_than<false>(to_square(king_square + 2), other_color, 1).count == 0)
                {
                    moves.push_back(Move(king_square,king_square+2,PieceType::King,own_color,
                        PieceType::None,PieceType::None,true));
                }
                
            }
            
        }
        if ((pos.get_castle_rights() & queen_castle_mask) != 0)
        {
            Bitboard line_between = LINE_BETWEEN(to_square(king_square - 1), to_square(king_square - 3));
            if ((line_between & pos.get_all_pieces()) == 0)
            {
                if (pos.attackers_more_than<false>(to_square(king_square - 1), other_color, 1).count == 0 && pos.attackers_more_than<false>(to_square(king_square - 2), other_color, 1).count == 0)
                {
                    moves.push_back(Move(king_square, king_square - 2, PieceType::King, own_color,
                        PieceType::None, PieceType::None, true));
                }

            }

        }
    return;
}
template <bool captures_only, bool with_checks>
void MoveGenerator::generate_queen_moves(MoveList& moves, const Position& pos, Color own_color, const Bitboard& pinned_info, Bitboard remedy_mask) {
    Color other_color = flip_color(own_color);
	Bitboard own_pieces = pos.get_color_pieces(own_color);
	Bitboard enemy_pieces = pos.get_color_pieces(other_color);
	Bitboard occupied = pos.get_all_pieces();
	Bitboard king_checking_squares = queen_attacks(pos.get_king_square(other_color),occupied) &~own_pieces;
	Bitboard queens = pos.get_pieces(own_color, PieceType::Queen);
    while (queens) {
        Square queen_sq = pop_lsb(queens);
        Bitboard possible_queen_moves = queen_attacks(queen_sq, occupied) & remedy_mask& pinned_info & ~own_pieces;
        Bitboard captures = possible_queen_moves & enemy_pieces & ~king_checking_squares;
        Bitboard quiets = possible_queen_moves & ~enemy_pieces & ~king_checking_squares;
        Bitboard checks = possible_queen_moves & king_checking_squares & ~enemy_pieces;
        Bitboard capture_checks = possible_queen_moves & king_checking_squares & enemy_pieces;
        while (captures) {
            Square to_sq = pop_lsb(captures);
            moves.push_back(Move(queen_sq, to_sq, PieceType::Queen, own_color, pos.get_piece_type_on_square(other_color, to_sq)));
        }
        if constexpr (captures_only) {
            if constexpr (!with_checks) continue;
            while (capture_checks) {
                Square to_sq = pop_lsb(capture_checks);
                moves.push_back(Move(queen_sq, to_sq, PieceType::Queen, own_color, pos.get_piece_type_on_square(other_color, to_sq)));
            }
        }
        while (capture_checks) {
            Square to_sq = pop_lsb(capture_checks);
            moves.push_back(Move(queen_sq, to_sq, PieceType::Queen, own_color, pos.get_piece_type_on_square(other_color, to_sq)));
        }
        while (checks) {
            Square to_sq = pop_lsb(checks);
            moves.push_back(Move(queen_sq, to_sq, PieceType::Queen, own_color, PieceType::None));
        }
        while (quiets) {
            Square to_sq = pop_lsb(quiets);
			moves.push_back(Move(queen_sq, to_sq, PieceType::Queen, own_color, PieceType::None));
        }
    }
    
}
template <bool captures_only, bool with_checks>
void MoveGenerator::generate_rook_moves(MoveList& moves,const Position& pos, Color own_color, const Bitboard& pinned_info,Bitboard remedy_mask) {
    //return generate_sliding_moves(moves,PieceType::ROOK,pos,own_color,pinned_info,remedy_mask,captures_only);
    Bitboard rooks = pos.get_pieces(own_color, PieceType::Rook);
    Bitboard occupied = pos.get_all_pieces();
    Bitboard own_pieces = pos.get_color_pieces(own_color); 
    if constexpr (captures_only) {
        Color other_color = own_color == Color::White ? Color::Black : Color::White;
        Bitboard mask_changer = pos.get_color_pieces(other_color);
        if constexpr (with_checks) {
            Square op_king_square = pos.get_king_square(other_color);
            mask_changer |= rook_attacks(op_king_square,occupied);
        }
        remedy_mask &= mask_changer;
    }
    while (rooks) {
        int from_square = square_index(lsb(rooks));
        Bitboard attacks=0;
        Bitboard blockers = ROOK_BLOCKER_MASK[from_square] & occupied;
        Bitboard index = (blockers * MAGIC_ROOK_NUMBER[from_square]) >> ROOK_SHIFT_NUMBERS[from_square];
        attacks = ROOK_ATTACK_TABLE[ROOK_ATTACK_OFFSET[from_square] + index] & ~own_pieces & remedy_mask;
        if (bit64(from_square) & pinned_info) {
            attacks &= Complete_Line(to_square(from_square), pos.get_king_square(own_color));
        }
        while (attacks) {
            Square to_square = lsb(attacks);
            moves.push_back(Move(from_square, to_square, PieceType::Rook, own_color, pos.get_piece_type_on_square(to_square)));
            attacks &= attacks - 1;

        }
        rooks &= rooks - 1;

    }

}
template <bool captures_only, bool with_checks>
void MoveGenerator::generate_bishop_moves(MoveList& moves,const Position& pos, Color own_color, const Bitboard& pinned_info, Bitboard remedy_mask) {
    //return generate_sliding_moves(moves,PieceType::BISHOP,pos,own_color,pinned_info,remedy_mask,captures_only);
    Bitboard bishops = pos.get_pieces(own_color, PieceType::Bishop);
    Bitboard occupied = pos.get_all_pieces();
    Bitboard own_pieces = pos.get_color_pieces(own_color); 
    if constexpr (captures_only) {
        Color other_color = own_color == Color::White ? Color::Black : Color::White;
        Bitboard mask_changer = pos.get_color_pieces(other_color);
        if constexpr (with_checks) {
            Square op_king_square = pos.get_king_square(other_color);
            mask_changer |= bishop_attacks(op_king_square, occupied);
        }
        remedy_mask &= mask_changer;
    }
    while (bishops) {
        int from_square = square_index(lsb(bishops));
        Bitboard attacks = 0;
        Bitboard blockers = BISHOP_BLOCKER_MASK[from_square] & occupied;
        Bitboard index = (blockers * MAGIC_BISHOP_NUMBER[from_square]) >> BISHOP_SHIFT_NUMBERS[from_square];
        attacks = BISHOP_ATTACK_TABLE[BISHOP_ATTACK_OFFSET[from_square] + index] & ~own_pieces & remedy_mask;
        if(bit64(from_square) & pinned_info) {
          attacks &= Complete_Line(to_square(from_square), pos.get_king_square(own_color));
		}
        while (attacks) {
            Square to_square = lsb(attacks);
            moves.push_back(Move(from_square, to_square, PieceType::Bishop, own_color, pos.get_piece_type_on_square(to_square)));
            attacks &= attacks - 1;

        }
        bishops &= bishops - 1;

    }

}
template <bool captures_only, bool with_checks>
void MoveGenerator::generate_knight_moves(MoveList& moves,const Position& pos, Color own_color, const Bitboard& pinned_info, Bitboard remedy_mask) {
    Bitboard knight_bitpos=pos.get_pieces(own_color,PieceType::Knight);
    Bitboard own_pieces = pos.get_color_pieces(own_color);
    if constexpr (captures_only) {
         Color other_color = own_color == Color::White ? Color::Black : Color::White;
        Bitboard mask_changer= pos.get_color_pieces(other_color);
        if constexpr (with_checks) {
                        Square op_king_square = pos.get_king_square(other_color);
                     mask_changer |= knight_attacks(op_king_square);
        }
		remedy_mask &= mask_changer;
    }
    while (knight_bitpos)
    {
        int from_square=square_index(lsb(knight_bitpos));
        if (bit64(from_square) & pinned_info)
        {
            knight_bitpos&=knight_bitpos-1;
            continue;
        }
        Bitboard possible_moves=KNIGHT_ATTACKS[from_square]&~own_pieces&remedy_mask;
        while (possible_moves)
        {
            Square to_square=lsb(possible_moves);
            moves.push_back(Move(from_square,to_square,PieceType::Knight,own_color,pos.get_piece_type_on_square(to_square)));
            possible_moves&=possible_moves-1;
        }
        knight_bitpos&=knight_bitpos-1; 
    }
    return;
}
template <bool captures_only,bool with_checks>
void MoveGenerator::generate_pawn_moves(MoveList& moves,const Position& pos, Color own_color,const Square king_square, const Bitboard& pinned_info, const Bitboard& remedy_mask) {
    
    if constexpr (!captures_only || (captures_only&&with_checks))
    {
        generate_pawn_pushes<with_checks>(moves,pos,own_color,pinned_info,remedy_mask);
    }
    
    generate_pawn_captures<captures_only,with_checks>(moves,pos,own_color,king_square,pinned_info,remedy_mask);
    return;
}
template <bool captures_only,bool with_checks>
void MoveGenerator::generate_sliding_moves(
    MoveList& moves,
    PieceType piece,
    const Position& pos,
    Color own_color,
    const Bitboard& pinned_info, 
    const Bitboard& remedy_mask){

    Bitboard piece_bitpos=pos.get_pieces(own_color,piece);    
    while (piece_bitpos)
    {
        int from_square=square_index(lsb(piece_bitpos));
        Bitboard possible_moves=0;
        std::vector<int> direction=piece==PieceType::Queen ? QUEEN_DIR_IND:(piece==PieceType::Rook ? ROOK_DIR_IND:BISHOP_DIR_IND);
        
        for (int  dir_index : direction)
        {   
            Bitboard ray_mask=RAY_MASK[dir_index][from_square];
            Bitboard blockers=ray_mask&pos.get_all_pieces();
            if (blockers!=0)
            {
                
            possible_moves|=ray_mask^RAY_MASK[dir_index][dir_index<4 ? lsb(blockers):msb(blockers)];
            }else
            {
                possible_moves|=ray_mask;
            }
            
            

        }
        possible_moves&=remedy_mask&~pos.get_color_pieces(own_color);
        if (bit64(from_square) & pinned_info) {
            possible_moves &= Complete_Line(to_square(from_square), pos.get_king_square(own_color));
        }
        if constexpr (captures_only){
            Color other_color=own_color==Color::White ? Color::Black:Color::White;
            possible_moves &= pos.get_color_pieces(other_color);
        }
        while (possible_moves)
        {
            Square to_square=lsb(possible_moves);
            moves.push_back(Move(from_square,to_square,piece,own_color,pos.get_piece_type_on_square(to_square)));
            possible_moves&=possible_moves-1;
        }
        piece_bitpos&=piece_bitpos-1;
    }
    return;
}
template <bool with_checks>
void MoveGenerator::generate_pawn_pushes(MoveList& moves,const Position& pos,Color own_color,const Bitboard& pinned_info,Bitboard remedy_mask){

        Bitboard own_pawns=pos.get_pieces(own_color,PieceType::Pawn);
        Bitboard all_pieces=pos.get_all_pieces();
        int push_step=(own_color==Color::White) ? 8:-8;
        int start_rank=(own_color==Color::White) ? 1:6;
        int promotion_rank=(own_color==Color::White) ? 6:1;
		CastlingRights castle_rights = pos.get_castle_rights();
		Square en_passant_square = pos.get_en_passant_rights();
        if constexpr (with_checks) {
			Color other_color = own_color == Color::White ? Color::Black : Color::White;
            int op_king_square = square_index(pos.get_king_square(other_color));
            remedy_mask &= PAWN_ATTACKS[to_int(other_color)][op_king_square];
        }
        while (own_pawns)
        {
            Square from_square = lsb(own_pawns);
            int rank =::rank(from_square);
            Square go_to_square=from_square+push_step;
            if (go_to_square!=NO_SQUARE)
            {
                if (!(bit64(go_to_square) & all_pieces))
                {   
                    Bitboard pinned_mask = BOARD_ALL_SET;
                    if(bit64(from_square)& pinned_info) {
                       pinned_mask = Complete_Line(from_square, pos.get_king_square(own_color));
					}
                    if (pinned_mask & bit64(go_to_square) & remedy_mask)
                    {
                        bool is_promotion=(rank==promotion_rank);
                        if (is_promotion)
                        {
                            
                        moves.push_back(Move(from_square, go_to_square, PieceType::Pawn, own_color, PieceType::None,PieceType::Queen));
                        moves.push_back(Move(from_square, go_to_square, PieceType::Pawn, own_color, PieceType::None,PieceType::Rook));
                        moves.push_back(Move(from_square, go_to_square, PieceType::Pawn, own_color, PieceType::None,PieceType::Bishop));
                        moves.push_back(Move(from_square, go_to_square, PieceType::Pawn, own_color, PieceType::None,PieceType::Knight));
                        }else
                        {
                            moves.push_back(Move(from_square,go_to_square,PieceType::Pawn,own_color,PieceType::None));
                        }
                    }

                    if (rank==start_rank)
                    {
                        Square to_square2=from_square+2*push_step;
                        if (!(bit64(to_square2)& all_pieces))
                        {
                            if (pinned_mask & bit64(to_square2) & remedy_mask)
                            {
                                moves.push_back(Move(from_square,to_square2,PieceType::Pawn,own_color,PieceType::None));
                            }
                            
                        }
                        
                    }
                    
                    
                }
                
            }
            own_pawns &=own_pawns-1;  
        }
        return;
}
template <bool captures_only, bool with_checks>
void MoveGenerator::generate_pawn_captures(MoveList& moves,const Position& pos, Color own_color,const Square king_square,const Bitboard& pinned_info,const Bitboard& remedy_mask){

        Bitboard own_pawns=pos.get_pieces(own_color,PieceType::Pawn);
        Color opponent_color =(own_color==Color::White) ? Color::Black:Color::White;
        Bitboard enemy_pieces=pos.get_color_pieces(opponent_color);
        Bitboard new_remedy=remedy_mask;
        if (pos.get_en_passant_rights()!=NO_SQUARE) new_remedy |=bit64(pos.get_en_passant_rights());
        while (own_pawns)
        {
            int from_square=square_index(lsb(own_pawns));

            Bitboard attack_bb=PAWN_ATTACKS[to_int(own_color)][from_square];

            Bitboard capture_bb=attack_bb & enemy_pieces;

            if(bit64(from_square)& pinned_info) {
                capture_bb &= Complete_Line(to_square(from_square), pos.get_king_square(own_color));
            }
            capture_bb&=remedy_mask;

            while (capture_bb)
            {
                Square to_square=lsb(capture_bb);
                PieceType captured_piece=pos.get_piece_type_on_square(to_square);

                bool is_promotion =(own_color==Color::White && square_index(to_square)>=56) || (own_color==Color::Black && square_index(to_square)<=7);

                if (is_promotion
                )
                {
                    moves.push_back(Move(from_square,to_square,PieceType::Pawn,own_color,captured_piece,PieceType::Queen));
                    moves.push_back(Move(from_square,to_square,PieceType::Pawn,own_color,captured_piece,PieceType::Rook));
                    moves.push_back(Move(from_square,to_square,PieceType::Pawn,own_color,captured_piece,PieceType::Bishop));
                    moves.push_back(Move(from_square,to_square,PieceType::Pawn,own_color,captured_piece,PieceType::Knight));
                }else{
                    moves.push_back(Move(from_square,to_square,PieceType::Pawn,own_color,captured_piece));
                }
                capture_bb&=capture_bb-1;
            }
            
            Square ep_square=pos.get_en_passant_rights();
            if (ep_square!=NO_SQUARE)
            {   
				Bitboard pinned_mask = BOARD_ALL_SET;
                if (bit64(from_square) & pinned_info) {
                    pinned_mask = Complete_Line(to_square(from_square), pos.get_king_square(own_color));
                }
                if (attack_bb & bit64(ep_square) & pinned_mask & remedy_mask)
                {       
                    if (rank(king_square) != rank(from_square))
                    {
                        moves.push_back(Move(from_square,ep_square,PieceType::Pawn,own_color,PieceType::Pawn,PieceType::None,false,true));
                    }else
                    {   
                        int dir_index= (king_square>from_square) ? 7:3;
                        Square capture_square= (own_color==Color::White) ? ep_square-8:ep_square+8;
                        Bitboard two_pawns_mask=bit64(from_square) | bit64(capture_square);
                        Bitboard opponent_rook_queen=pos.get_pieces(opponent_color,PieceType::Rook) | pos.get_pieces(opponent_color,PieceType::Queen);
                        Bitboard ray=RAY_MASK[dir_index][square_index(king_square)]^two_pawns_mask;
                        ray&= pos.get_all_pieces();
                        Square next_piece_square= (dir_index==7) ? msb(ray) : lsb(ray);
                        
                        if ((next_piece_square==NO_SQUARE) || (opponent_rook_queen & bit64(next_piece_square)) == 0)
                        {
                            moves.push_back(Move(from_square,ep_square,PieceType::Pawn,own_color,PieceType::Pawn,PieceType::None,false,true));
                        }else{
                            own_pawns&=own_pawns-1;
                            continue;
                        }
                        
                    }
                    

                }
                
            }

            own_pawns&=own_pawns-1;
            
        }
        return;
        
}

void MoveGenerator::generate_captures(const Position& pos, MoveList& moves){
    return generate_moves<true,false>(pos,moves);
}
void MoveGenerator::generate_captures(const Position& pos, MoveList& moves, Bitboard checkers){
    return generate_moves<true, false>(pos, moves, checkers);
}
void MoveGenerator::generate_captures_with_checks(const Position& pos,MoveList& moves, Bitboard checkers){
    return generate_moves<true,true>(pos,moves, checkers);
}



// Force the compiler to generate these specific versions of the template
template void MoveGenerator::generate_moves<false, false>(const Position&, MoveList&);
template void MoveGenerator::generate_moves<true, false>(const Position&, MoveList&);
template void MoveGenerator::generate_moves<false, true>(const Position&, MoveList&);
template void MoveGenerator::generate_moves<true, true>(const Position&, MoveList&);
template void MoveGenerator::generate_moves<false, false>(const Position&, MoveList&, Bitboard);
template void MoveGenerator::generate_moves<true, false>(const Position&, MoveList&, Bitboard);
template void MoveGenerator::generate_moves<false, true>(const Position&, MoveList&, Bitboard);
template void MoveGenerator::generate_moves<true, true>(const Position&, MoveList&, Bitboard);
