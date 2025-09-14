
#include "MoveGenerator.h"
#include <cstdint>
#include "utils.h"
#include "bitboard_masks.h"
#include <array>
#include "attack_rays.h"
#include "constants.h"

MoveGenerator::MoveGenerator()
{
    
}

std::vector<Move> MoveGenerator::generate_moves(const Board& board,bool captures_ony){
    std::vector<Move> legal_moves;
    Color own_color=board.get_turn();
    Color opponent_color=own_color==Color::WHITE ? Color::BLACK:Color::WHITE;
    int king_square=get_lsb(board.get_pieces(own_color,PieceType::KING));
    std::array<uint64_t,64> pinned_info=calculate_pinned_pieces(board,own_color,king_square);
    CheckInfo check_info=board.count_attacker_on_square(king_square,opponent_color);
    uint64_t own_pieces=board.get_color_pieces(own_color);
    if (check_info.count>1)
    { 
        std::vector<Move> king_moves = generate_king_moves(board, own_color,own_pieces,king_square,captures_ony);
        legal_moves.insert(legal_moves.end(), king_moves.begin(), king_moves.end());
    }else if (check_info.count==1)
    {  
        std::vector<Move> king_moves = generate_king_moves(board, own_color,own_pieces ,king_square,captures_ony);
        legal_moves.insert(legal_moves.end(), king_moves.begin(), king_moves.end());
        
        PieceType checker=board.get_piece_on_square(check_info.attacker_square);
        uint64_t remedy_mask=(1ULL<< check_info.attacker_square);
        if (checker==PieceType::QUEEN || checker==PieceType::ROOK|| checker==PieceType::BISHOP) remedy_mask|=LINE_BETWEEN[king_square][check_info.attacker_square];
        
        std::vector<Move> queen_moves = generate_queen_moves(board, own_color, pinned_info, remedy_mask,captures_ony);
        legal_moves.insert(legal_moves.end(), queen_moves.begin(), queen_moves.end());
        
        std::vector<Move> rook_moves = generate_rook_moves(board, own_color, pinned_info, remedy_mask,captures_ony);
        legal_moves.insert(legal_moves.end(), rook_moves.begin(), rook_moves.end());

        std::vector<Move> bishop_moves = generate_bishop_moves(board, own_color, pinned_info, remedy_mask,captures_ony);
        legal_moves.insert(legal_moves.end(), bishop_moves.begin(), bishop_moves.end());

        std::vector<Move> knight_moves = generate_knight_moves(board, own_color, pinned_info, remedy_mask,captures_ony);
        legal_moves.insert(legal_moves.end(), knight_moves.begin(), knight_moves.end());
        
        if (board.get_en_passant_rights() !=-1 && checker == PieceType::PAWN) remedy_mask|=1ULL<<board.get_en_passant_rights();
        std::vector<Move> pawn_moves = generate_pawn_moves(board, own_color,king_square, pinned_info, remedy_mask,captures_ony);
        legal_moves.insert(legal_moves.end(), pawn_moves.begin(), pawn_moves.end());
    }else
    {   
        std::vector<Move> king_moves = generate_king_moves(board, own_color,own_pieces, king_square,captures_ony);
        legal_moves.insert(legal_moves.end(), king_moves.begin(), king_moves.end());
        
        std::vector<Move> queen_moves = generate_queen_moves(board, own_color, pinned_info,BOARD_ALL_SET,captures_ony);
        legal_moves.insert(legal_moves.end(), queen_moves.begin(), queen_moves.end());

        std::vector<Move> rook_moves = generate_rook_moves(board, own_color, pinned_info,BOARD_ALL_SET,captures_ony);
        legal_moves.insert(legal_moves.end(), rook_moves.begin(), rook_moves.end());

        std::vector<Move> bishop_moves = generate_bishop_moves(board, own_color, pinned_info,BOARD_ALL_SET,captures_ony);
        legal_moves.insert(legal_moves.end(), bishop_moves.begin(), bishop_moves.end());
        
        std::vector<Move> knight_moves = generate_knight_moves(board, own_color, pinned_info,BOARD_ALL_SET,captures_ony);
        legal_moves.insert(legal_moves.end(), knight_moves.begin(), knight_moves.end());

        std::vector<Move> pawn_moves = generate_pawn_moves(board, own_color,king_square, pinned_info,BOARD_ALL_SET,captures_ony);
        legal_moves.insert(legal_moves.end(), pawn_moves.begin(), pawn_moves.end());
    }
    return legal_moves;
}
std::array<uint64_t,64> MoveGenerator::calculate_pinned_pieces(const Board& board,const Color friendly_color, const int king_square){
    Color opponent_color=friendly_color==Color::WHITE ? Color::BLACK:Color::WHITE;
    std::array<uint64_t,64> pinned_info;
    pinned_info.fill(BOARD_ALL_SET);
    uint64_t own_pieces=board.get_color_pieces(friendly_color);
    uint64_t opponent_pieces=board.get_color_pieces(opponent_color);
    uint64_t occupied=board.get_all_pieces();
    for (int dir_index = 0; dir_index < 8; ++dir_index)
    {
        uint64_t ray=RAY_MASK[dir_index][king_square];
        int first_block_sq=get_first_blocker_sq(ray,occupied,dir_index<4);
        if (first_block_sq==-1) continue;
        uint64_t first_block_bb=1ULL<<first_block_sq;
        if (first_block_bb & own_pieces)
        {
            int second_block_sq=get_second_blocker_sq(ray,occupied,dir_index<4,first_block_sq);
            if (second_block_sq==-1) continue;
            uint64_t second_block_bb=1ULL<<second_block_sq;
            PieceType rook_or_bishop= dir_index %2 == 0 ? PieceType::BISHOP:PieceType::ROOK;
            uint64_t possible_attackers= board.get_pieces(opponent_color,rook_or_bishop)| board.get_pieces(opponent_color,PieceType::QUEEN);
            if (second_block_bb & possible_attackers)
            {
                pinned_info[first_block_sq]=LINE_BETWEEN[king_square][second_block_sq]^(1ULL<<king_square);
            }
            
        }
        
    }
    return pinned_info;
}
std::vector<Move> MoveGenerator::generate_king_moves(const Board& board,const Color own_color, const uint64_t own_pieces, int king_square, bool captures_only){
        std::vector<Move> moves;
        uint64_t possible_moves=KING_ATTACKS[king_square]&~own_pieces;
        Color other_color=own_color==Color::WHITE ? Color::BLACK:Color::WHITE;
        if (captures_only) possible_moves&=board.get_color_pieces(other_color);
        while (possible_moves)
        {
            int to_square=get_lsb(possible_moves);
            if (board.count_attacker_on_square(to_square,other_color,1,false).count>0)
            {
                possible_moves&=possible_moves-1;
                continue;
            }
            moves.push_back(Move(king_square,to_square,PieceType::KING,own_color,
                board.get_piece_on_square(to_square),board.get_castle_rights(),board.get_en_passant_rights()));
            possible_moves&=possible_moves-1;
        }
        if (board.count_attacker_on_square(king_square,other_color,1,false).count>0) return moves;
        
        char king_char=own_color==Color::WHITE ? 'K':'k';
        char queen_char=own_color==Color::WHITE ? 'Q':'q';
        if (board.get_castle_rights().find(king_char)!=-1)
        {   
            int rook_square=king_square+3;
            uint64_t line_between=LINE_BETWEEN[king_square+1][king_square+2];
    
            if ((line_between & board.get_all_pieces())==0)
            {   
                
                if (board.count_attacker_on_square(king_square+1,other_color,1,false).count==0 && board.count_attacker_on_square(king_square+2,other_color,1,false).count==0)
                {
                    moves.push_back(Move(king_square,king_square+2,PieceType::KING,own_color,
                        PieceType::NONE,board.get_castle_rights(),board.get_en_passant_rights(),PieceType::NONE,true));
                }
                
            }
            
        }
        if (board.get_castle_rights().find(queen_char)!=-1)
        {
            uint64_t line_between=LINE_BETWEEN[king_square-1][king_square-3];
            if ((line_between & board.get_all_pieces())==0)
            {
                if (board.count_attacker_on_square(king_square-1,other_color,1,false).count==0 && board.count_attacker_on_square(king_square-2,other_color,1,false).count==0)
                {
                    moves.push_back(Move(king_square,king_square-2,PieceType::KING,own_color,
                        PieceType::NONE,board.get_castle_rights(),board.get_en_passant_rights(),PieceType::NONE,true));
                }
                
            }
            
        }
    return moves;
}

std::vector<Move> MoveGenerator::generate_queen_moves(const Board& board, Color own_color,const std::array<uint64_t,64>& pinned_info,const uint64_t& remedy_mask, bool captures_only) {
    return generate_sliding_moves(PieceType::QUEEN,board,own_color,pinned_info,remedy_mask,captures_only);
}

std::vector<Move> MoveGenerator::generate_rook_moves(const Board& board, Color own_color, const std::array<uint64_t, 64>& pinned_info, const uint64_t& remedy_mask,bool captures_only) {
    return generate_sliding_moves(PieceType::ROOK,board,own_color,pinned_info,remedy_mask,captures_only);
}

std::vector<Move> MoveGenerator::generate_bishop_moves(const Board& board, Color own_color, const std::array<uint64_t, 64>& pinned_info, const uint64_t& remedy_mask,bool captures_only) {
    return generate_sliding_moves(PieceType::BISHOP,board,own_color,pinned_info,remedy_mask,captures_only);
}

std::vector<Move> MoveGenerator::generate_knight_moves(const Board& board, Color own_color, const std::array<uint64_t, 64>& pinned_info, const uint64_t& remedy_mask, bool captures_only) {
    std::vector<Move> moves;
    uint64_t knight_bitboard=board.get_pieces(own_color,PieceType::KNIGHT);
    while (knight_bitboard)
    {
        int from_square=get_lsb(knight_bitboard);
        if (pinned_info[from_square]!=BOARD_ALL_SET)
        {
            knight_bitboard&=knight_bitboard-1;
            continue;
        }
        uint64_t possible_moves=KNIGHT_ATTACKS[from_square]&~board.get_color_pieces(own_color)&remedy_mask;
        if (captures_only){
            Color other_color=own_color==Color::WHITE ? Color::BLACK:Color::WHITE;
            possible_moves &= board.get_color_pieces(other_color);
        }
        while (possible_moves)
        {
            int to_square=get_lsb(possible_moves);
            moves.push_back(Move(from_square,to_square,PieceType::KNIGHT,own_color,board.get_piece_on_square(to_square),board.get_castle_rights(),board.get_en_passant_rights()));
            possible_moves&=possible_moves-1;
        }
        knight_bitboard&=knight_bitboard-1;
    }
    return moves;
}

std::vector<Move> MoveGenerator::generate_pawn_moves(const Board& board, Color own_color,const int king_square, const std::array<uint64_t, 64>& pinned_info, const uint64_t& remedy_mask, bool captures_only) {
    std::vector<Move> moves;
    
    if (!captures_only)
    {
        std::vector<Move> pushes_moves=generate_pawn_pushes(board,own_color,pinned_info,remedy_mask);
        moves.insert(
        moves.end(),
        pushes_moves.begin(),
        pushes_moves.end()
        );
    }
    
    std::vector<Move> capture_moves=generate_pawn_captures(board,own_color,king_square,pinned_info,remedy_mask);
    moves.insert(
        moves.end(),
        capture_moves.begin(),
        capture_moves.end()
    );
    return moves;
}

std::vector<Move> MoveGenerator::generate_sliding_moves(
    PieceType piece,
    const Board& board,
    Color own_color,
    const std::array<uint64_t,64>& pinned_info, 
    const uint64_t& remedy_mask,
    bool captures_only){

    std::vector<Move> moves;
    uint64_t piece_bitboard=board.get_pieces(own_color,piece);    
    while (piece_bitboard)
    {
        int from_square=get_lsb(piece_bitboard);
        uint64_t possible_moves=0;
        std::vector<int> direction=piece==PieceType::QUEEN ? QUEEN_DIR_IND:(piece==PieceType::ROOK ? ROOK_DIR_IND:BISHOP_DIR_IND);
        
        for (int  dir_index : direction)
        {   
            uint64_t ray_mask=RAY_MASK[dir_index][from_square];
            uint64_t blockers=ray_mask&board.get_all_pieces();
            if (blockers!=0)
            {
                
            possible_moves|=ray_mask^RAY_MASK[dir_index][dir_index<4 ? get_lsb(blockers):get_msb(blockers)];
            }else
            {
                possible_moves|=ray_mask;
            }
            
            

        }
        possible_moves&=pinned_info[from_square] & remedy_mask&~board.get_color_pieces(own_color);
        if (captures_only){
            Color other_color=own_color==Color::WHITE ? Color::BLACK:Color::WHITE;
            possible_moves &= board.get_color_pieces(other_color);
        }
        while (possible_moves)
        {
            int to_square=get_lsb(possible_moves);
            moves.push_back(Move(from_square,to_square,piece,own_color,board.get_piece_on_square(to_square),board.get_castle_rights(),board.get_en_passant_rights()));
            possible_moves&=possible_moves-1;
        }
        piece_bitboard&=piece_bitboard-1;
    }
    return moves;
}

std::vector<Move> MoveGenerator::generate_pawn_pushes(const Board& board,Color own_color,const std::array<uint64_t,64>&pinned_info,const uint64_t& remedy_mask){
        std::vector<Move> moves;

        uint64_t own_pawns=board.get_pieces(own_color,PieceType::PAWN);
        uint64_t all_pieces=board.get_all_pieces();
        int push_step=(own_color==Color::WHITE) ? 8:-8;
        int start_rank=(own_color==Color::WHITE) ? 1:6;
        int promotion_rank=(own_color==Color::WHITE) ? 6:1;
        
        while (own_pawns)
        {
            int from_square = get_lsb(own_pawns);
            int rank = from_square /8;

            int to_square=from_square+push_step;
            if (to_square>=0 && to_square<64)
            {
                if (!((1ULL<<to_square) &all_pieces))
                {
                    if (pinned_info[from_square] &(1ULL<< to_square) & remedy_mask)
                    {
                        bool is_promotion=(rank==promotion_rank);
                        if (is_promotion)
                        {
                            
                        moves.emplace_back(from_square, to_square, PieceType::PAWN, own_color, PieceType::NONE, board.get_castle_rights(), board.get_en_passant_rights(),PieceType::QUEEN);
                        moves.emplace_back(from_square, to_square, PieceType::PAWN, own_color, PieceType::NONE, board.get_castle_rights(), board.get_en_passant_rights(),PieceType::ROOK);
                        moves.emplace_back(from_square, to_square, PieceType::PAWN, own_color, PieceType::NONE, board.get_castle_rights(), board.get_en_passant_rights(),PieceType::BISHOP);
                        moves.emplace_back(from_square, to_square, PieceType::PAWN, own_color, PieceType::NONE, board.get_castle_rights(), board.get_en_passant_rights(),PieceType::KNIGHT);
                        }else
                        {
                            moves.emplace_back(from_square,to_square,PieceType::PAWN,own_color,PieceType::NONE,board.get_castle_rights(),board.get_en_passant_rights());
                        }
                    }

                    if (rank==start_rank)
                    {
                        int to_square2=from_square+2*push_step;
                        if (!((1ULL<< to_square2)& all_pieces))
                        {
                            if (pinned_info[from_square] &(1ULL<<to_square2)& remedy_mask)
                            {
                                moves.emplace_back(from_square,to_square2,PieceType::PAWN,own_color,PieceType::NONE,board.get_castle_rights(),board.get_en_passant_rights());
                            }
                            
                        }
                        
                    }
                    
                    
                }
                
            }
            own_pawns &=own_pawns-1;  
        }
        return moves;
}
std::vector<Move> MoveGenerator::generate_pawn_captures(const Board& board, Color own_color,const int king_square,const std::array<uint64_t,64>& pinned_info,const uint64_t& remedy_mask){
        std::vector<Move> moves;

        uint64_t own_pawns=board.get_pieces(own_color,PieceType::PAWN);
        Color opponent_color =(own_color==Color::WHITE) ? Color::BLACK:Color::WHITE;
        uint64_t enemy_pieces=board.get_color_pieces(opponent_color);
        uint64_t new_remedy=remedy_mask;
        if (board.get_en_passant_rights()!=-1) new_remedy |=(1ULL<<board.get_en_passant_rights());
        while (own_pawns)
        {
            int from_square=get_lsb(own_pawns);

            uint64_t attack_bb=PAWN_ATTACKS[to_int(own_color)][from_square];

            uint64_t capture_bb=attack_bb & enemy_pieces;

            capture_bb&=pinned_info[from_square];
            capture_bb&=remedy_mask;

            while (capture_bb)
            {
                int to_square=get_lsb(capture_bb);
                PieceType captured_piece=board.get_piece_on_square(to_square);

                bool is_promotion =(own_color==Color::WHITE && to_square>=56) || (own_color==Color::BLACK && to_square<=7);

                if (is_promotion
                )
                {
                    moves.emplace_back(from_square,to_square,PieceType::PAWN,own_color,captured_piece,board.get_castle_rights(),board.get_en_passant_rights(),PieceType::QUEEN);
                    moves.emplace_back(from_square,to_square,PieceType::PAWN,own_color,captured_piece,board.get_castle_rights(),board.get_en_passant_rights(),PieceType::ROOK);
                    moves.emplace_back(from_square,to_square,PieceType::PAWN,own_color,captured_piece,board.get_castle_rights(),board.get_en_passant_rights(),PieceType::BISHOP);
                    moves.emplace_back(from_square,to_square,PieceType::PAWN,own_color,captured_piece,board.get_castle_rights(),board.get_en_passant_rights(),PieceType::KNIGHT);
                }else{
                    moves.emplace_back(from_square,to_square,PieceType::PAWN,own_color,captured_piece,board.get_castle_rights(),board.get_en_passant_rights());
                }
                capture_bb&=capture_bb-1;
            }
            
            int ep_square=board.get_en_passant_rights();
            if (ep_square!=-1)
            {  
                if (attack_bb & (1ULL<<ep_square) & pinned_info[from_square] & remedy_mask)
                {       
                    if (king_square /8 != from_square /8)
                    {
                        moves.emplace_back(from_square,ep_square,PieceType::PAWN,own_color,PieceType::PAWN,board.get_castle_rights(),board.get_en_passant_rights(),PieceType::NONE,false,true);
                    }else
                    {   
                        int dir_index= (king_square>from_square) ? 7:3;
                        int capture_square= (own_color==Color::WHITE) ? ep_square-8:ep_square+8;
                        uint64_t two_pawns_mask=(1ULL<<from_square) | (1ULL<<capture_square);
                        uint64_t opponent_rook_queen=board.get_pieces(opponent_color,PieceType::ROOK) | board.get_pieces(opponent_color,PieceType::QUEEN);
                        uint64_t ray=RAY_MASK[dir_index][king_square]^two_pawns_mask;
                        ray&= board.get_all_pieces();
                        int next_piece_square= (dir_index==7) ? get_msb(ray) : get_lsb(ray);
                        
                        if ((next_piece_square==-1) | (opponent_rook_queen & (1ULL<<next_piece_square)) == 0)
                        {
                            moves.emplace_back(from_square,ep_square,PieceType::PAWN,own_color,PieceType::PAWN,board.get_castle_rights(),board.get_en_passant_rights(),PieceType::NONE,false,true);
                        }else{
                            own_pawns&=own_pawns-1;
                            continue;
                        }
                        
                    }
                    

                }
                
            }

            own_pawns&=own_pawns-1;
            
        }
        return moves;
        
}
std::vector<Move> MoveGenerator::generate_captures(const Board& board){
    return generate_moves(board,true);
}