#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include "board.h"
#include <stdexcept>
#include "utils.h"
#include "zobrist.h"
#include "constants.h"
#include "pst.h"
#include <algorithm>
#include <cctype>
#include <array>
#include "attack_rays.h"
#include "bitboard_masks.h"

Board::Board(const std:: string& fen){

     parse_fen(fen);
     initialize_board();
     initialize_game_phase();
     zobrist_hash=initialize_hash();
     material_score=initialize_material_score();
     positional_score=initialize_positional_score();
     history.reserve(256);
     history.push_back(get_board_state());

}
void Board::parse_fen(const std::string& fen){
    std::stringstream ss(fen);
    std::string item;
    std::vector<std::string> fen_parts;

    while (std::getline(ss,item,' ')){
        fen_parts.push_back(item);
    }
    if (fen_parts.size()==6){
        
    parse_fen_pieces(fen_parts[0]);
     parse_fen_turn(fen_parts[1]);
     parse_fen_castling(fen_parts[2]);
     parse_fen_en_passant(fen_parts[3]);
     parse_fen_half_move(fen_parts[4]);
     parse_fen_move(fen_parts[5]);

    }else{
        throw std::runtime_error("Invalid FEN string: does not have 6 parts. FEN:"+fen);
    }
}

void Board::parse_fen_pieces(const std::string& piece_data){
    std::cout << piece_data << std::endl;
    int rank=7;
    int file=0;
    for(char c : piece_data){
        if (c=='/'){
            rank--;
            file=0;
        } else if (isdigit(c)){
            file+=(c-'0');
        }else {
            int square_index=rank*8+file;
            int color_index=isupper(c) ? to_int(Color::WHITE):to_int(Color::BLACK);

            int piece_type_index=to_int(PIECE_TYPE_MAP.at(toupper(c)));

            pieces[color_index][piece_type_index]|=(1ULL<< square_index);

            file++;


        }
        if(c=='K' || c == 'k') {
            if (isupper(c)){
                white_king_square=rank*8+file-1;
            }else{
                black_king_square=rank*8+file-1;
            }
		}
    }
}

void Board::parse_fen_turn(const  std::string& turn_data){
     turn= turn_data=="w" ? to_int(Color::WHITE):to_int(Color::BLACK); 
}
void Board::parse_fen_castling(const std::string& castling_data){
	castling_rights = 0;
    if (castling_data!="-"){
		if (castling_data.find('K') != std::string::npos) castling_rights |= (1U << 3); // White kingside
		if (castling_data.find('Q') != std::string::npos) castling_rights |= (1U << 2); // White queenside
		if (castling_data.find('k') != std::string::npos) castling_rights |= (1U << 1); // Black kingside
		if (castling_data.find('q') != std::string::npos) castling_rights |= (1U << 0); // Black queenside
    }
}

void Board::parse_fen_en_passant(const std::string& en_passant_data){
    if (en_passant_data == "-") {
        en_passant_square = -1; 
    } else {
        // Character arithmetic is a fast way to get indices
        int file = en_passant_data[0] - 'a'; // 'a' becomes 0, 'b' becomes 1, etc.
        int rank = en_passant_data[1] - '1'; // '1' becomes 0, '2' becomes 1, etc.
        
        en_passant_square = rank * 8 + file;
    }
}
void Board::parse_fen_half_move(const std::string& half_move_data){
     half_moves=std::stoi(half_move_data);
}
void Board::parse_fen_move(const std::string& move_data){
     move_count=std::stoi(move_data);
}
void Board::display() const{
    std::cout<<"\n--- Current Position---"<< std::endl;
    std::cout<< "Turn: " <<(turn==to_int(Color::WHITE) ? "WHITE":"BLACK")<<std::endl;
    std::cout <<"   a b c d e f g h" << std::endl;
    std::cout<< "-------------------"<< std::endl;

    for (int rank=7; rank>=0;--rank){
        std::cout << rank+1ULL<< "  ";

        for (int file=0;file<8;++file){
            int square_index=rank*8+file;

            char piece=get_char_on_square(square_index);
            std::cout << piece << " ";
        }
        std::cout << std::endl;
    }
    std::cout<< "-------------------"<< std::endl;
    std::cout << "   a b c d e f g h\n"<< std::endl;

}
void Board::initialize_board(){
     color_pieces[to_int(Color::WHITE)]=0;
     color_pieces[to_int(Color::BLACK)]=0;
    for (int piece=to_int(PieceType::PAWN); piece<=to_int(PieceType::KING);++piece){
        color_pieces[to_int(Color::WHITE)]|=pieces[to_int(Color::WHITE)][piece];
         color_pieces[to_int(Color::BLACK)]|= pieces[to_int(Color::BLACK)][piece];

    }
    this -> all_pieces=color_pieces[to_int(Color::WHITE)]| color_pieces[to_int(Color::BLACK)];

}
void Board::initialize_game_phase() {
    const int INITIAL_PHASE_VALUE = 24;
    
    // Define the phase value for each piece type
    const int KNIGHT_PHASE = 1;
    const int BISHOP_PHASE = 1;
    const int ROOK_PHASE = 2;
    const int QUEEN_PHASE = 4;

    // Use popcount on the combined bitboard for each piece type
    int knight_phase = popcount(pieces[to_int(Color::WHITE)][to_int(PieceType::KNIGHT)] | pieces[to_int(Color::BLACK)][to_int(PieceType::KNIGHT)]) * KNIGHT_PHASE;
    int bishop_phase = popcount(pieces[to_int(Color::WHITE)][to_int(PieceType::BISHOP)] | pieces[to_int(Color::BLACK)][to_int(PieceType::BISHOP)]) * BISHOP_PHASE;
    int rook_phase   = popcount(pieces[to_int(Color::WHITE)][to_int(PieceType::ROOK)]   | pieces[to_int(Color::BLACK)][to_int(PieceType::ROOK)])   * ROOK_PHASE;
    int queen_phase  = popcount(pieces[to_int(Color::WHITE)][to_int(PieceType::QUEEN)]  | pieces[to_int(Color::BLACK)][to_int(PieceType::QUEEN)])  * QUEEN_PHASE;

    game_phase = knight_phase + bishop_phase + rook_phase + queen_phase;
    
}
uint64_t Board::initialize_hash() const {
    uint64_t h=0;

    for (int color=0;color<2;++color){
        for (int piece=0;piece<6;++piece){
            uint64_t bitboard=pieces[color][piece];
            while (bitboard){
                int square_index =get_lsb(bitboard);
                h^=Zobrist::piece_keys[color][piece][square_index];
                bitboard &= bitboard-1;
            }

            
        }
    }

    if (turn== to_int(Color::BLACK)){
        h^=Zobrist::black_to_move_key;
    }
    h ^= Zobrist::castling_keys[this->castling_rights];

    // Hash en passant square
    if (this->en_passant_square != -1) {
        h ^= Zobrist::en_passant_keys[this->en_passant_square % 8]; // Use the file of the square
    }

    return h;
}
MaterialScore Board::initialize_material_score()const {
    int score=0;
    for (int color=0;color<2;++color){
        for (int piece=0;piece<6;++piece){
            score+=popcount(pieces[color][piece]) * PIECE_VALUES[color][piece];
        }
    }
    return {score};
}
PositionalScore Board::initialize_positional_score()const {
        int score_mg=0;
        int score_eg=0;
        for (int color=0;color<2;++color){
            for (int piece=0;piece<6;++piece){
                uint64_t bitboard=pieces[color][piece];

                while (bitboard){
                    int square=get_lsb(bitboard);
                    score_mg+=MG_PST[color][piece][square];
                    score_eg+=EG_PST[color][piece][square];

                    bitboard &=bitboard-1;
                }
            }
        }
        return {score_mg,score_eg};
}
void Board::make_move(const Move& move){

    history.push_back(get_board_state());
    update_material_score(move);
    update_positional_score(move);
    update_game_phase(move);
    update_castle_rights(move );
    update_en_passsant_rights(move);
    update_king_square(move);
    update_pieces(move);
    update_pieces_hash(move);
    update_turn_rights(move);
}
void Board::undo_move(const Move& move){
    recover_board_state(history.back());
    history.pop_back();

}
void Board::update_material_score(const Move& move){
    if (move.piece_captured!=PieceType::NONE){
        material_score.score-=get_piece_values(move.get_capture_color(),move.piece_captured);
    }
    if (move.promotion_piece!=PieceType::NONE){
        material_score.score+=get_piece_values(move.move_color,move.promotion_piece);
        material_score.score-=get_piece_values(move.move_color,PieceType::PAWN);
    }
}
void Board::update_positional_score(const Move& move){
    PieceType piece_moved=move.piece_moved;
    PieceType piece_reached=move.promotion_piece==PieceType::NONE ? move.piece_moved:move.promotion_piece;
    positional_score.mg-=get_mg_pos_score(move.move_color,piece_moved,move.from_square);
    positional_score.mg+=get_mg_pos_score(move.move_color,piece_reached,move.to_square);
    
    positional_score.eg-=get_eg_pos_score(move.move_color,piece_moved,move.from_square);
    positional_score.eg+=get_eg_pos_score(move.move_color,piece_reached,move.to_square);
    if (move.piece_captured!=PieceType::NONE){
        positional_score.mg-=get_mg_pos_score(move.get_capture_color(),move.piece_captured,move.get_capture_square());
        positional_score.eg-=get_eg_pos_score(move.get_capture_color(),move.piece_captured,move.get_capture_square());
    }
    if (move.is_castle)
    {
        bool king_side=move.to_square>move.from_square;
        int old_rook_square=king_side ? move.to_square+1:move.to_square-2;
        int new_rook_square=king_side ? move.to_square-1:move.to_square+1;

        positional_score.mg-=get_mg_pos_score(move.move_color,PieceType::ROOK,old_rook_square);
        positional_score.mg+=get_mg_pos_score(move.move_color,PieceType::ROOK,new_rook_square);

        
        positional_score.eg-=get_eg_pos_score(move.move_color,PieceType::ROOK,old_rook_square);
        positional_score.eg+=get_eg_pos_score(move.move_color,PieceType::ROOK,new_rook_square);

    }
    
}
void Board::update_turn_rights(const Move& move){
        turn=turn==0 ? 1:0;
        zobrist_hash^=Zobrist::black_to_move_key;
        if (move.piece_moved == PieceType::PAWN || move.piece_captured != PieceType::NONE) {
            half_moves = 0;
		}
		else {
            half_moves++;
        }
        if (turn == to_int(Color::WHITE)) {
            move_count++;
        }

}
void Board::update_game_phase(const Move& move){
    if (move.piece_captured!=PieceType::NONE){
        int piece_weight=PHASE_WEIGHTS[to_int(move.piece_captured)];
        game_phase-=piece_weight;
    }
    if (move.promotion_piece!=PieceType::NONE){
        int piece_weight=PHASE_WEIGHTS[to_int(move.promotion_piece)];
        game_phase+=piece_weight;
    }
}
void Board::update_castle_rights(const Move& move){
    this->zobrist_hash ^= Zobrist::castling_keys[this->castling_rights]; // Remove old rights from hash

        // if King moves
    if (move.piece_moved==PieceType::KING)
    {
        if (move.move_color==Color::WHITE)
        {  
            castling_rights &= ~WHITE_KING_CASTLE;
			castling_rights &= ~WHITE_QUEEN_CASTLE;
        }else
        {   
			castling_rights &= ~BLACK_KING_CASTLE;
			castling_rights &= ~BLACK_QUEEN_CASTLE;
        }
        
        
    }
    // 2. If a rook moves FROM its starting square, remove that one right
	if (move.from_square == 7)  castling_rights &= ~WHITE_KING_CASTLE;
	if (move.from_square == 0)  castling_rights &= ~WHITE_QUEEN_CASTLE;
    if (move.from_square == 63) castling_rights&= ~BLACK_KING_CASTLE; 
	if (move.from_square == 56) castling_rights &= ~BLACK_QUEEN_CASTLE;

    // 3. If an enemy rook is captured ON its starting square, remove that right
	if (move.to_square == 7)   castling_rights &= ~WHITE_KING_CASTLE;
	if (move.to_square == 0)   castling_rights &= ~WHITE_QUEEN_CASTLE;
	if (move.to_square == 63)  castling_rights &= ~BLACK_KING_CASTLE;
	if (move.to_square == 56)  castling_rights &= ~BLACK_QUEEN_CASTLE;


	this->zobrist_hash ^= Zobrist::castling_keys[this->castling_rights]; // Add new rights to hash
}
void Board::update_en_passsant_rights(const Move& move){
    
        if (en_passant_square != -1) {
            zobrist_hash ^= Zobrist::en_passant_keys[en_passant_square % 8];
        }
        en_passant_square=-1;
        if (move.is_double_pawn_move())
        {
            en_passant_square=move.move_color==Color::WHITE ? move.to_square-8:move.to_square+8;
        }
        if (en_passant_square != -1) {
            zobrist_hash ^= Zobrist::en_passant_keys[en_passant_square % 8];
        }
    
    
    
    
}
void Board::update_pieces_hash(const Move& move){
    PieceType piece_reached= move.promotion_piece==PieceType::NONE? move.piece_moved:move.promotion_piece;
    int move_color=to_int(move.move_color);
    zobrist_hash^=Zobrist::piece_keys[move_color][to_int(move.piece_moved)][move.from_square];
    zobrist_hash^=Zobrist::piece_keys[move_color][to_int(piece_reached)][move.to_square];
    if (move.piece_captured!=PieceType::NONE)
    {
        int other_color=to_int(move.get_capture_color());
        int capture_square=move.is_en_passant ? (move.move_color==Color::WHITE ? move.to_square-8:move.to_square+8):move.to_square;
        zobrist_hash^=Zobrist::piece_keys[other_color][to_int(move.piece_captured)][capture_square];
    }
    if (move.is_castle)
    {
        bool king_side=move.to_square>move.from_square;
        int rook=to_int(PieceType::ROOK);
        int old_rook_square=king_side ? move.to_square+1:move.to_square-2;
        int new_rook_square=king_side ? move.to_square-1:move.to_square+1;
        zobrist_hash^=Zobrist::piece_keys[move_color][rook][old_rook_square];
        zobrist_hash^=Zobrist::piece_keys[move_color][rook][new_rook_square];

    }
    
    
}
void Board::update_king_square(const Move& move){
    
        if (move.piece_moved==PieceType::KING)
        {
            if (move.move_color==Color::WHITE)
            {
                white_king_square=move.to_square;
            }else
            {
                black_king_square=move.to_square;
            }
            
        }
    
    
}
void Board::update_pieces(const Move& move){
    PieceType piece_reached= move.promotion_piece==PieceType::NONE ? move.piece_moved: move.promotion_piece;
    pieces[to_int(move.move_color)][to_int(move.piece_moved)]^=1ULL<< move.from_square;
    color_pieces[to_int(move.move_color)]^=1ULL<<move.from_square;
    all_pieces^=1ULL<<move.from_square;

    pieces[to_int(move.move_color)][to_int(piece_reached)]^=1ULL<< move.to_square;
    color_pieces[to_int(move.move_color)]^=1ULL<<move.to_square;
    all_pieces^=1ULL<<move.to_square;
    
    if (move.piece_captured!=PieceType::NONE)
    {   
        Color other_color=move.get_capture_color();
        int capture_square=move.is_en_passant ? (move.move_color==Color::WHITE ? move.to_square-8:move.to_square+8):move.to_square;
        
        pieces[to_int(other_color)][to_int(move.piece_captured)]^=1ULL<< capture_square;
        color_pieces[to_int(other_color)]^=1ULL<<capture_square;
        all_pieces^=1ULL<<capture_square;
        
    }

    if (move.is_castle)
    {
        bool king_side= move.to_square>move.from_square;
        int old_rook_square=king_side ? move.to_square+1: move.to_square-2;
        int new_rook_square= king_side ? move.to_square-1:move.to_square+1;

        
        pieces[to_int(move.move_color)][to_int(PieceType::ROOK)]^=1ULL<<old_rook_square| 1ULL<<new_rook_square;
        color_pieces[to_int(move.move_color)]^=1ULL<<old_rook_square| 1ULL<<new_rook_square;
        all_pieces^=1ULL<<old_rook_square|1ULL<<new_rook_square;
    }
    
    
    




}
CheckInfo Board::count_attacker_on_square(const int square, const Color attacker_color,const int bound,const bool need_square)const {
    CheckInfo info={0,-1};
    int other_color=attacker_color==Color::BLACK ? to_int(Color::WHITE):to_int(Color::BLACK);
    // Check if PAWNS Attack the square
    uint64_t pawns=pieces[to_int(attacker_color)][to_int(PieceType::PAWN)];
    uint64_t attack_squares=PAWN_ATTACKS[other_color][square];
    int number=popcount(pawns & attack_squares);
    info.count+=number;
    if (number>0 && need_square) info.attacker_square=get_lsb(pawns & attack_squares);
    if (info.count>=bound) return info;
    uint64_t knights=pieces[to_int(attacker_color)][to_int(PieceType::KNIGHT)];
    attack_squares=KNIGHT_ATTACKS[square];
    number=popcount(knights & attack_squares);
    
    if (number>0 && need_square) info.attacker_square=get_lsb(knights & attack_squares);
    info.count+=number;
    if (info.count>=bound) return info;

    //Check for Bishop or Queen attack
    for (int dir_index = 0; dir_index < 8; dir_index+=2)
    {
        uint64_t possible_atackers=pieces[to_int(attacker_color)][to_int(PieceType::QUEEN)]| pieces[to_int(attacker_color)][to_int(PieceType::BISHOP)];
        int blocker_sq=get_first_blocker_sq(RAY_MASK[dir_index][square],all_pieces^(pieces[other_color][to_int(PieceType::KING)]),dir_index<4);
        if (blocker_sq==-1) continue;
        if (possible_atackers & (1ULL << blocker_sq)){
            info.count+=1;
            info.attacker_square=blocker_sq;
        }
        if (info.count>=bound) return info;
    }
    //Check for ROOk or Queen attack
    for (int dir_index = 1; dir_index < 8; dir_index+=2)
    {   
    
        uint64_t possible_atackers=pieces[to_int(attacker_color)][to_int(PieceType::QUEEN)]| pieces[to_int(attacker_color)][to_int(PieceType::ROOK)];
        int blocker_sq=get_first_blocker_sq(RAY_MASK[dir_index][square],all_pieces^(pieces[other_color][to_int(PieceType::KING)]),dir_index<4);
        if (blocker_sq==-1) continue;
        if (possible_atackers & (1ULL << blocker_sq)){
            info.count+=1;
            info.attacker_square=blocker_sq;
        }
        if (info.count>=bound) return info;
    }
    return info;

}
Color Board::get_turn() const {
        return static_cast<Color>(this->turn);
}
uint64_t Board::get_pieces(const Color color, const PieceType piece) const {
    return pieces[to_int(color)][to_int(piece)];
}
uint64_t Board::get_color_pieces(const Color color) const {
    return color_pieces[to_int(color)];
}
uint64_t Board::get_all_pieces() const{
    return all_pieces;
}
PieceType Board::get_piece_on_square(int square) const {
	PieceType piece_type = PieceType::NONE;
    if ((all_pieces & (1ULL << square)) == 0) {
        return piece_type;
	}
	int piece_type_index = 0;
    uint64_t piece_mask = 0;
    while (piece_type==PieceType::NONE && piece_type_index<6) {
		piece_mask = pieces[0][piece_type_index] | pieces[1][piece_type_index];
        if (piece_mask & (1ULL << square)) {
            piece_type = static_cast<PieceType>(piece_type_index);
		}
		piece_type_index++;
	}

    return piece_type;
}
Color Board::get_color_on_square(int square) const {
    
    for (int color = 0; color < 2; ++color) {
            if (color_pieces[color] & (1ULL << square)) {
                return static_cast<Color>(color);
            }
    }
	return Color::NONE;
}
char Board::get_char_on_square(int square) const {
	char piece_char = PIECE_CHAR_LIST[to_int(get_piece_on_square(square))];
    if (piece_char == '.') {
        return piece_char; // Empty square
	}
    Color color = get_color_on_square(square);
    if (color == Color::WHITE) {
        return piece_char; // Uppercase for white pieces
    } else if (color == Color::BLACK) {
        return std::tolower(piece_char); // Lowercase for black pieces
	}
}
int Board::get_en_passant_rights() const{
    return en_passant_square;
}
uint8_t Board::get_castle_rights() const{
    return castling_rights;
}
uint64_t Board::get_hash() const{
    return zobrist_hash;
}
int Board::get_material_score() const{
    return material_score.score;
}
double Board::get_positional_score() const{
    return (positional_score.mg*game_phase+positional_score.eg*(1-game_phase))/24.0;
}
double Board::get_game_phase() const{
    return game_phase/24.0;
}
int Board::get_king_square(Color color) const{
    return color==Color::WHITE ? white_king_square:black_king_square;
}
BoardState Board::get_board_state() const {
    BoardState current_state;
    current_state.zobrist_hash=this->zobrist_hash;
    current_state.castling_rights=this->castling_rights;
    current_state.en_passant_square = this->en_passant_square;
    current_state.game_phase = this->game_phase;
    current_state.turn = this->turn;
    current_state.white_king_square = this->white_king_square;
    current_state.black_king_square = this->black_king_square;
    current_state.pieces = this->pieces;
    current_state.color_pieces = this->color_pieces;
    current_state.all_pieces = this->all_pieces;
    current_state.positional_score = this->positional_score;
    current_state.material_score = this->material_score;
    current_state.half_moves = this->half_moves;
    current_state.move_count = this->move_count;
    return current_state;
}
bool Board::has_enough_material_for_nmp() const {
    // Get the bitboard of all non-pawn/king pieces for the current side to move
    uint64_t pieces = this->pieces[this->turn][to_int(PieceType::KNIGHT)] |
                      this->pieces[this->turn][to_int(PieceType::BISHOP)] |
                      this->pieces[this->turn][to_int(PieceType::ROOK)]   |
                      this->pieces[this->turn][to_int(PieceType::QUEEN)];
    
    // NMP is generally safe if there is at least one piece other than pawns or the king
    return (pieces != 0);
}
bool Board::in_check() const{
    Color color=turn==0 ? Color::WHITE:Color::BLACK;
    Color attacker_color=turn== 0 ? Color::BLACK:Color::WHITE;
    int king_sq=color==Color::WHITE ? white_king_square : black_king_square;
    return count_attacker_on_square(king_sq,attacker_color,1,false).count>=1;
}
int Board::make_null_move(){
    int original_ep_square=en_passant_square;

    if (en_passant_square!=-1) zobrist_hash^=Zobrist::en_passant_keys[en_passant_square %8];
    en_passant_square=-1;
    turn= turn==0 ? 1:0;
    zobrist_hash^=Zobrist::black_to_move_key;
    return original_ep_square;
}
void Board::undo_null_move(int original_ep_square){
        turn= turn==0? 1:0;
        zobrist_hash^=Zobrist::black_to_move_key;
        en_passant_square=original_ep_square;

        if (en_passant_square!=-1) zobrist_hash^=Zobrist::en_passant_keys[en_passant_square % 8];
        
}
void Board::recover_board_state(const BoardState& previous_state) {

    this->zobrist_hash = previous_state.zobrist_hash;
    this->castling_rights = previous_state.castling_rights;
    this->en_passant_square = previous_state.en_passant_square;
    this->game_phase = previous_state.game_phase;
    this->turn = previous_state.turn;
    this->white_king_square = previous_state.white_king_square;
    this->black_king_square = previous_state.black_king_square;
    this->pieces = previous_state.pieces;
    this->color_pieces = previous_state.color_pieces;
    this->all_pieces = previous_state.all_pieces;
    this->positional_score = previous_state.positional_score;
    this->material_score = previous_state.material_score;
    this->half_moves = previous_state.half_moves;
    this->move_count = previous_state.move_count;
}
bool Board::is_repetition_draw() const {
    int repetition_count = 0;
    for (int i = history.size()-2; i >=0 &&i>=(int)history.size()-half_moves; i-=2)
    {
        if (history[i].zobrist_hash == this->zobrist_hash) {
            repetition_count++;
			if (repetition_count >= 2) return true;
        }
    }
	return false;
}
bool Board::is_fifty_move_rule_draw() const {
    return half_moves >= 100;
}