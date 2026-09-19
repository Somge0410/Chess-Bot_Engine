#include "position.h"
#include "fen.h"
#include "bitboard.h"
#include "zobrist.h"
#include "Square.h"
#include "phase.h"
#include "eval_params.h"
#include "attacks.h"
Position::Position() : Position(fen::START_POSITION) {
}

Position::Position(std::string_view fen_text) {
	const FenData data = fen::parse(fen_text);
	pieces = data.pieces;
	side_to_move = data.side_to_move;
	castling_rights = data.castling_rights;
	en_passant_square = data.en_passant_square;
	halfmove_clock = data.halfmove_clock;
	full_move_number = data.full_move_number;

	rebuild_derived_state();
}

void Position::rebuild_derived_state() {
    rebuild_occupancy();
    find_king_squares();
	game_phase = calculate_game_phase();
	zobrist_hash = calculate_zobrist_hash();
	pawn_hash = calculate_pawn_hash();
	material_score = calculate_material_score();
	positional_score = calculate_positional_score();

    history.clear();
	history.reserve(256);
    repetition_tracker.clear();
	repetition_tracker.push(zobrist_hash);

}

int Position::calculate_game_phase() const noexcept
{   
    int phase = 0;
    for(PieceType piece : {PieceType::Knight, PieceType::Bishop, PieceType::Rook, PieceType::Queen}) {
        phase += popcount(
            pieces(Color::White, piece) |
            pieces(Color::Black, piece)
        ) * phase_weight(piece);
	}
	return std::min(phase, MAX_GAME_PHASE);
}
std::uint64_t Position::calculate_zobrist_hash() const noexcept
{
    std::uint64_t hash = 0;

    for (Color color : {Color::White,Color::Black}) {
        for (PieceType piece :{PieceType::Pawn,PieceType::Knight,PieceType::Bishop,PieceType::Rook,PieceType::Queen,PieceType::King}) {
            Bitboard remaining = pieces(color,piece);

            while (remaining) {
                const Square square = pop_lsb(remaining);

                hash ^= Zobrist::piece_keys
                    [color_index(color)]
                    [piece_index(piece)]
                    [square_index(square)];
            }
        }
    }

    if (side_to_move == Color::Black) {
        hash ^= Zobrist::black_to_move_key;
    }

    hash ^= Zobrist::castling_keys[
        static_cast<std::uint8_t>(castling_rights)
    ];

    if (en_passant_square != NO_SQUARE) {
        hash ^= Zobrist::en_passant_keys[
            get_file(en_passant_square)
        ];
    }

    return hash;
}
std::uint64_t Position::calculate_pawn_hash() const noexcept {
    uint64_t pawn_key = 0;
    for (Color color : {Color::White, Color::Black}) {
        Bitboard pawn_bitboard = pieces(color,PieceType::Pawn);
        while (pawn_bitboard) {
            int square_index = lsb(pawn_bitboard);
            pawn_key ^= Zobrist::piece_keys[color_index(color)][piece_index(PieceType::Pawn)][square_index];
            pawn_bitboard &= pawn_bitboard - 1;
        }
    }
    return pawn_key;
}
void Position::rebuild_occupancy() {
    color_pieces(Color::White) = 0;
    color_pieces(Color::Black) = 0;
    all_pieces = 0;
    for (Color color : {Color::White, Color::Black}) {
        for (PieceType piece : {PieceType::Pawn, PieceType::Knight, PieceType::Bishop, PieceType::Rook, PieceType::Queen, PieceType::King}) {
            Bitboard bitboard = pieces(color, piece);
            color_pieces(color) |= bitboard;
            all_pieces |= bitboard;
        }
    }
}
void Position::find_king_squares() {
    for (Color color : {Color::White, Color::Black}) {
        Bitboard king_bitboard = pieces(color,PieceType::King);
        if (king_bitboard) {
            king_squares(color) = lsb(king_bitboard);
        } else {
            king_squares(color) = NO_SQUARE;
        }
    }
}
EvaluationResult Position::calculate_material_score() const noexcept {
    EvaluationResult score{0, 0};
    for (Color color : {Color::White, Color::Black}) {
        for (PieceType piece : {PieceType::Pawn, PieceType::Knight, PieceType::Bishop, PieceType::Rook, PieceType::Queen, PieceType::King}) {
            EvaluationResult piece_score = get_piece_values(color, piece);
            score += piece_score * static_cast<int>(popcount(pieces(color, piece)));
        }
    }
    return score;
}
EvaluationResult Position::calculate_positional_score() const noexcept {
    EvaluationResult score = { 0,0 };
    for (Color color : {Color::White, Color::Black}) {
        for (PieceType piece : {PieceType::Pawn, PieceType::Knight, PieceType::Bishop, PieceType::Rook, PieceType::Queen, PieceType::King}) {
            uint64_t bitboard = pieces(color, piece);
            while (bitboard) {
                int square = pop_lsb(bitboard);

                score.mg_score += get_mg_pos_score(color, piece, square);
                score.eg_score += get_eg_pos_score(color, piece, square);
            }

        }
    }
    return score;
}

void Position::push_current_state_to_history() {
    history.emplace_back();
    StateInfo& current_state = history.back();
    current_state.zobrist_hash = zobrist_hash;
    current_state.pawn_hash = pawn_hash;
    current_state.castling_rights = castling_rights;
    current_state.en_passant_square = en_passant_square;
    current_state.game_phase = game_phase;
    current_state.side_to_move = side_to_move;
    current_state.white_king_square = king_squares(Color::White);
    current_state.black_king_square = king_squares(Color::Black);
    current_state.pieces = pieces;
    current_state.color_pieces[color_index(Color::White)] = color_pieces(Color::White);
    current_state.color_pieces[color_index(Color::Black)] = color_pieces(Color::Black);
    current_state.all_pieces = all_pieces;
    current_state.positional_score = positional_score;
    current_state.material_score = material_score;
    current_state.half_moves = halfmove_clock;
    current_state.move_count = full_move_number;
    current_state.current_twofold_count = repetition_tracker.get_twofold();
    current_state.current_repetition_tracker_start = repetition_tracker.get_start();
}
void Position::make_move(const Move& move) {

    push_current_state_to_history();
    update_material_score(move);
    update_positional_score(move);
    update_game_phase(move);
    update_castle_rights(move);
    update_en_passsant_rights(move);
    update_king_square(move);
    update_pieces(move);
    update_pieces_hash(move);
    update_turn_rights(move);
    update_move_count(move);
    update_repetition_tracker();
}
void Position::undo_move() {
    recover_position_state(history.back());
    history.pop_back();
}
bool Position::is_square_attacked_by_pawn(Square square, Color attacker_color) const {
    Bitboard pawn_attackers = pieces(attacker_color, PieceType::Pawn) & PAWN_ATTACKS[color_index(attacker_color)][square_index(square)];
    return pawn_attackers != 0;
}
void Position::update_material_score(const Move& move) {
    if (move.piece_captured != PieceType::None) {
        material_score -= get_piece_values(move.get_capture_color(), move.piece_captured);
    }
    if (move.promotion_piece != PieceType::None) {
        material_score += get_piece_values(move.move_color, move.promotion_piece);
        material_score -= get_piece_values(move.move_color, PieceType::Pawn);
    }
}
void Position::update_positional_score(const Move& move) {
    PieceType piece_moved = move.piece_moved;
    PieceType piece_reached = move.promotion_piece == PieceType::None ? move.piece_moved : move.promotion_piece;
    positional_score.mg_score -= get_mg_pos_score(move.move_color, piece_moved, move.from_square);
    positional_score.mg_score += get_mg_pos_score(move.move_color, piece_reached, move.to_square);

    positional_score.eg_score -= get_eg_pos_score(move.move_color, piece_moved, move.from_square);
    positional_score.eg_score += get_eg_pos_score(move.move_color, piece_reached, move.to_square);
    if (move.piece_captured != PieceType::None) {
        positional_score.mg_score -= get_mg_pos_score(move.get_capture_color(), move.piece_captured, move.get_capture_square());
        positional_score.eg_score -= get_eg_pos_score(move.get_capture_color(), move.piece_captured, move.get_capture_square());
    }
    if (move.is_castle)
    {
        bool king_side = move.to_square > move.from_square;
        int old_rook_square = king_side ? move.to_square + 1 : move.to_square - 2;
        int new_rook_square = king_side ? move.to_square - 1 : move.to_square + 1;

        positional_score.mg_score -= get_mg_pos_score(move.move_color, PieceType::Rook, old_rook_square);
        positional_score.mg_score += get_mg_pos_score(move.move_color, PieceType::Rook, new_rook_square);


        positional_score.eg_score -= get_eg_pos_score(move.move_color, PieceType::Rook, old_rook_square);
        positional_score.eg_score += get_eg_pos_score(move.move_color, PieceType::Rook, new_rook_square);

    }

}
void Position::update_game_phase(const Move& move) {
    if (move.piece_captured != PieceType::None) {
        int piece_weight = phase_weight(move.piece_captured);
        game_phase -= piece_weight;
    }
    if (move.promotion_piece != PieceType::None) {
        int piece_weight = phase_weight(move.promotion_piece);
        game_phase += piece_weight;
    }
}
void Position::update_castle_rights(const Move& move){

zobrist_hash ^= Zobrist::castling_keys[castling_rights]; // Remove old rights from hash

// if King moves
if (move.piece_moved == PieceType::King)
{
    if (move.move_color == Color::White)
    {
        castling_rights &= ~CastlingRights::WhiteKingside;
        castling_rights &= ~CastlingRights::WhiteQueenside;
    }
else
{
    castling_rights &= ~CastlingRights::BlackKingside;
    castling_rights &= ~CastlingRights::BlackQueenside;
}


}
// 2. If a rook moves FROM its starting square, remove that one right
if (move.from_square == 7)  castling_rights &= ~CastlingRights::WhiteKingside;
if (move.from_square == 0)  castling_rights &= ~CastlingRights::WhiteQueenside;
if (move.from_square == 63) castling_rights &= ~CastlingRights::BlackKingside;
if (move.from_square == 56) castling_rights &= ~CastlingRights::BlackQueenside;

// 3. If an enemy rook is captured ON its starting square, remove that right
if (move.to_square == 7)   castling_rights &= ~CastlingRights::WhiteKingside;
if (move.to_square == 0)   castling_rights &= ~CastlingRights::WhiteQueenside;
if (move.to_square == 63)  castling_rights &= ~CastlingRights::BlackKingside;
if (move.to_square == 56)  castling_rights &= ~CastlingRights::BlackQueenside;


zobrist_hash ^= Zobrist::castling_keys[castling_rights]; // Add new rights to hash
}
void Position::update_en_passsant_rights(const Move& move) {
    if (en_passant_square != Square::NO_SQUARE) {
        zobrist_hash ^= Zobrist::en_passant_keys[get_file(en_passant_square)];
    }
    en_passant_square = Square::NO_SQUARE;
    if (move.is_double_pawn_move())
    {
        en_passant_square = move.move_color == Color::White ? move.to_square - 8 : move.to_square + 8;
    }
    if (en_passant_square != Square::NO_SQUARE) {
		//In Fute maybe only do if there is a pawn that could take en passant.
            zobrist_hash ^= Zobrist::en_passant_keys[get_file(en_passant_square)];
    }
}
void Position::update_king_square(const Move& move) {
    if (move.piece_moved == PieceType::King) {
        king_squares(move.move_color) = move.to_square;
    }
}
void Position::update_pieces(const Move& move) {
    PieceType piece_reached = move.promotion_piece == PieceType::None ? move.piece_moved : move.promotion_piece;
    pieces(move.move_color,move.piece_moved) ^= bit64(move.from_square);
    color_pieces(move.move_color) ^= bit64(move.from_square);
    all_pieces ^= bit64(move.from_square);

    pieces(move.move_color,piece_reached) ^= bit64(move.to_square);
    color_pieces(move.move_color) ^= bit64(move.to_square);
    all_pieces ^= bit64(move.to_square);

    if (move.piece_captured != PieceType::None)
    {
        Color other_color = move.get_capture_color();
        Square capture_square = move.is_en_passant ? (move.move_color == Color::White ? move.to_square - 8 : move.to_square + 8) : move.to_square;

        pieces(other_color,move.piece_captured) ^= bit64(capture_square);
        color_pieces(other_color) ^= bit64(capture_square);
        all_pieces ^= bit64(capture_square);

    }

    if (move.is_castle)
    {
        bool king_side = move.to_square > move.from_square;
        Square old_rook_square = king_side ? move.to_square + 1 : move.to_square - 2;
        Square new_rook_square = king_side ? move.to_square - 1 : move.to_square + 1;


        pieces(move.move_color,PieceType::Rook) ^= bit64(old_rook_square) | bit64(new_rook_square);
        color_pieces(move.move_color) ^= bit64(old_rook_square) | bit64(new_rook_square);
        all_pieces ^= bit64(old_rook_square) | bit64(new_rook_square);
    }

}
void Position::update_pieces_hash(const Move& move) {

    PieceType piece_reached = move.promotion_piece == PieceType::None ? move.piece_moved : move.promotion_piece;
    int move_color_index = color_index(move.move_color);
    zobrist_hash ^= Zobrist::piece_keys[move_color_index][piece_index(move.piece_moved)][square_index(move.from_square)];
    zobrist_hash ^= Zobrist::piece_keys[move_color_index][piece_index(piece_reached)][square_index(move.to_square)];
    if (move.piece_moved == PieceType::Pawn) {
        pawn_hash ^= Zobrist::piece_keys[move_color_index][piece_index(move.piece_moved)][square_index(move.from_square)];
    }
    if (piece_reached == PieceType::Pawn) {
        pawn_hash ^= Zobrist::piece_keys[move_color_index][piece_index(piece_reached)][square_index(move.to_square)];
    }

    if (move.piece_captured != PieceType::None)
    {
        int other_color = color_index(move.get_capture_color());
        Square capture_square = move.is_en_passant ? (move.move_color == Color::White ? move.to_square - 8 : move.to_square + 8) : move.to_square;
        zobrist_hash ^= Zobrist::piece_keys[other_color][piece_index(move.piece_captured)][square_index(capture_square)];
        if (move.piece_captured == PieceType::Pawn) {
            pawn_hash ^= Zobrist::piece_keys[other_color][piece_index(move.piece_captured)][square_index(capture_square)];
        }
    }
    if (move.is_castle)
    {
        bool king_side = move.to_square > move.from_square;
        int rook = piece_index(PieceType::Rook);
        Square old_rook_square = king_side ? move.to_square + 1 : move.to_square - 2;
        Square new_rook_square = king_side ? move.to_square - 1 : move.to_square + 1;
        zobrist_hash ^= Zobrist::piece_keys[move_color_index][rook][square_index(old_rook_square)];
        zobrist_hash ^= Zobrist::piece_keys[move_color_index][rook][square_index(new_rook_square)];
    }

}
void Position::update_turn_rights(const Move& move) {
    side_to_move = flip_color(side_to_move);
    zobrist_hash ^= Zobrist::black_to_move_key;
}
void Position::update_move_count(const Move& move) {
    if (move.move_color == Color::Black) {
        full_move_number++;
    }
    if (move.piece_captured != PieceType::None || move.piece_moved == PieceType::Pawn) {
        halfmove_clock = 0;
    }
    else
    {
        halfmove_clock = std::min(100, halfmove_clock + 1);
    }
}
void Position::update_repetition_tracker() {

    if (halfmove_clock == 0) {
        repetition_tracker.reset(zobrist_hash);
    }
    else {
        repetition_tracker.push(zobrist_hash);
    }
}
void Position::recover_position_state(const StateInfo& previous_state) {

    repetition_tracker.recover_from_old(zobrist_hash, previous_state.current_repetition_tracker_start, previous_state.current_twofold_count);
    zobrist_hash = previous_state.zobrist_hash;
    pawn_hash = previous_state.pawn_hash;
    castling_rights = previous_state.castling_rights;
    en_passant_square = previous_state.en_passant_square;
    game_phase = previous_state.game_phase;
    side_to_move = previous_state.side_to_move;
    king_squares(Color::White) = previous_state.white_king_square;
    king_squares(Color::Black) = previous_state.black_king_square;
    pieces = previous_state.pieces;
    color_pieces(Color::White) = previous_state.color_pieces[color_index(Color::White)];
    color_pieces(Color::Black) = previous_state.color_pieces[color_index(Color::Black)];
    all_pieces = previous_state.all_pieces;
    positional_score = previous_state.positional_score;
    material_score = previous_state.material_score;
    halfmove_clock = previous_state.half_moves;
    full_move_number = previous_state.move_count;
}
Square Position::make_null_move() {
    Square original_ep_square = en_passant_square;

    if (en_passant_square != NO_SQUARE) zobrist_hash ^= Zobrist::en_passant_keys[get_file(en_passant_square)];
    en_passant_square = NO_SQUARE;
	side_to_move = flip_color(side_to_move);
    zobrist_hash ^= Zobrist::black_to_move_key;
    return original_ep_square;
}
void Position::undo_null_move(Square original_ep_square) {
	side_to_move = flip_color(side_to_move);
    zobrist_hash ^= Zobrist::black_to_move_key;
    en_passant_square = original_ep_square;

    if (en_passant_square != NO_SQUARE) zobrist_hash ^= Zobrist::en_passant_keys[get_file(en_passant_square)];
}
Bitboard Position::get_attacks_for_color_piece(Color color, PieceType piece_type) const {
    Bitboard attacks = 0;
    switch(piece_type) {
        case PieceType::Pawn: {
            uint64_t pawns = pieces(color, PieceType::Pawn);
			return pawn_attacks(pawns,color);
        }
        default: {
            Bitboard pieces_bb = pieces(color, piece_type);
            while (pieces_bb) {
                Square square = pop_lsb(pieces_bb);
                attacks |= piece_attacks(square, color, all_pieces, piece_type);
            }
			break;
        }
	}
   return attacks;
}
Bitboard Position::get_attacks_for_color(Color color) const {
    Bitboard attacks = 0;
    for(PieceType piece : {PieceType::Pawn, PieceType::Knight, PieceType::Bishop, PieceType::Rook, PieceType::Queen, PieceType::King}) {
        attacks |= get_attacks_for_color_piece(color, piece);
    }
    return attacks;
}
bool Position::is_square_attacked(Square square, Color attacker_color) const {
    for(PieceType piece : {PieceType::Pawn, PieceType::Knight, PieceType::Bishop, PieceType::Rook, PieceType::Queen, PieceType::King}) {
        if(pieces(attacker_color, piece) & piece_attacks(square, attacker_color, all_pieces, piece)) {
            return true;
        }
    }
    return false;
}
Bitboard Position::get_square_attackers(Square square, Color attacker_color) const {
    Bitboard attackers = pieces(attacker_color, PieceType::Pawn)
        & pawn_attacks(square, flip_color(attacker_color));
    attackers |= pieces(attacker_color, PieceType::Knight) & knight_attacks(square);
    attackers |= pieces(attacker_color, PieceType::King) & king_attacks(square);
    attackers |= (pieces(attacker_color, PieceType::Bishop)
        | pieces(attacker_color, PieceType::Queen)) & bishop_attacks(square, all_pieces);
    attackers |= (pieces(attacker_color, PieceType::Rook)
        | pieces(attacker_color, PieceType::Queen)) & rook_attacks(square, all_pieces);
    return attackers;
}

StateInfo Position::get_state_info() const {
    StateInfo state{};
    state.zobrist_hash = zobrist_hash;
    state.pawn_hash = pawn_hash;
    state.castling_rights = castling_rights;
    state.en_passant_square = en_passant_square;
    state.game_phase = game_phase;
    state.side_to_move = side_to_move;
    state.white_king_square = king_squares(Color::White);
    state.black_king_square = king_squares(Color::Black);
    state.pieces = pieces;
    state.color_pieces[color_index(Color::White)] = color_pieces(Color::White);
    state.color_pieces[color_index(Color::Black)] = color_pieces(Color::Black);
    state.all_pieces = all_pieces;
    state.positional_score = positional_score;
    state.material_score = material_score;
    state.half_moves = halfmove_clock;
    state.move_count = full_move_number;
    state.current_twofold_count = repetition_tracker.get_twofold();
    state.current_repetition_tracker_start = repetition_tracker.get_start();
	return state;
}
bool Position::any_appeared_more_than(int count) const {
    if (count == 2) {
        return repetition_tracker.has_any_twofold();
    }
    else if (count == 3) {
        return repetition_tracker.has_any_threefold();
    }
    else return false;
}
template <const bool need_sq>
AttackerInfo Position::attackers_more_than(const Square square, const Color attacker_color, const int bound) const {
    AttackerInfo info{};
    Bitboard attackers = 0;
    int count = 0;

    const auto add_attackers = [&](Bitboard piece_attackers) {
        count += popcount(piece_attackers);
        attackers |= piece_attackers;
        if (count >= bound) {
            return true;
        }
        return false;
        };

    const auto finish = [&]() {
        info.count = count;
        if constexpr (need_sq) {
            info.first_attacker_square = attackers ? lsb(attackers) : NO_SQUARE;
        }
        return info;
        };

    if (add_attackers(pieces(attacker_color, PieceType::Pawn)
        & pawn_attacks(square, flip_color(attacker_color)))) return finish();
    if (add_attackers(pieces(attacker_color, PieceType::Knight)
        & knight_attacks(square))) return finish();
    if (add_attackers(pieces(attacker_color, PieceType::King)
        & king_attacks(square))) return finish();

    const Bitboard occupancy = all_pieces
        & ~pieces(flip_color(attacker_color), PieceType::King);
    if (add_attackers((pieces(attacker_color, PieceType::Bishop)
        | pieces(attacker_color, PieceType::Queen))
        & bishop_attacks(square, occupancy))) return finish();
    add_attackers((pieces(attacker_color, PieceType::Rook)
        | pieces(attacker_color, PieceType::Queen))
        & rook_attacks(square, occupancy));
    return finish();
}
template AttackerInfo Position::attackers_more_than<false>(Square, Color, int) const;

bool Position::has_enough_material_for_nmp() const {
    for(PieceType piece :{PieceType::Knight,PieceType::Bishop, PieceType::Rook, PieceType::Queen}) {
        if(pieces(side_to_move, piece)) {
            return true;
		}
	}
    return false;
}
PieceType Position::get_piece_type_on_square(Square square) const noexcept{
    if (square == Square::NO_SQUARE) return PieceType::None;
    for (Color color : {Color::White, Color::Black}) {
        for (PieceType piece : {PieceType::Pawn, PieceType::Knight, PieceType::Bishop, PieceType::Rook, PieceType::Queen, PieceType::King}) {
            if (is_occupied(square, pieces(color, piece))) return piece;
        }
    }
    return PieceType::None;
}
PieceType Position::get_piece_type_on_square(Color color, Square square) const noexcept{
    const Bitboard mask = bit64(square);

    if (!(color_pieces(color) & mask))
        return PieceType::None;

    if (pieces(color, PieceType::Pawn) & mask) return PieceType::Pawn;
    if (pieces(color, PieceType::Knight) & mask) return PieceType::Knight;
    if (pieces(color, PieceType::Bishop) & mask) return PieceType::Bishop;
    if (pieces(color, PieceType::Rook) & mask) return PieceType::Rook;
    if (pieces(color, PieceType::Queen) & mask) return PieceType::Queen;
    return PieceType::King;
}
Color Position::get_color_on_square(Square square) const {
    if (is_occupied(square, color_pieces(Color::White))) return Color::White;
    if (is_occupied(square, color_pieces(Color::Black))) return Color::Black;
    return Color::None;
}
void Position::display() const{
    std::cout << "\n--- Current Position---" << std::endl;
    std::cout << "Turn: " << (side_to_move == Color::White ? "WHITE" : "BLACK") << std::endl;
    std::cout << "   a b c d e f g h" << std::endl;
    std::cout << "-------------------" << std::endl;

    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1ULL << "  ";

        for (int file = 0; file < 8; ++file) {
            Square square_index =to_square(file,rank);

            char piece = get_char_on_square(square_index);
            std::cout << piece << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "-------------------" << std::endl;
    std::cout << "   a b c d e f g h\n" << std::endl;

}
char Position::get_char_on_square(Square square) const {
    char c = '.';
    PieceType piece = get_piece_type_on_square(square);
    if (piece == PieceType::None) return c;
    switch (piece) {
    case PieceType::Pawn:
        c = 'p';
    case PieceType::Knight:
        c = 'n';
    case PieceType::Bishop:
        c = 'b';
    case PieceType::Rook:
        c = 'r';
    case PieceType::Queen:
        c = 'q';
    case PieceType::King:
        c = 'k';
    }
    Color color = get_color_on_square(square);
    switch (color) {
    case Color::White:
        return std::toupper(c);
    case Color::Black:
        return c;
    }
    return c;

}