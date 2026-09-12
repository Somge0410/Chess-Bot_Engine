#include <string>
#include "Move.h"
#include "constants.h"  // for PieceType

inline std::string move_to_uci(const Move& m) {
    auto sq_to_str = [](int sq) -> std::string {
        int file = sq % 8;
        int rank = sq / 8;
        std::string s;
        s += char('a' + file);
        s += char('1' + rank);
        return s;
        };

    std::string out = sq_to_str(m.from_square) + sq_to_str(m.to_square);

    if (m.promotion_piece != PieceType::NONE) {
        char c = 'q'; // default
        switch (m.promotion_piece) {
        case PieceType::QUEEN:  c = 'q'; break;
        case PieceType::ROOK:   c = 'r'; break;
        case PieceType::BISHOP: c = 'b'; break;
        case PieceType::KNIGHT: c = 'n'; break;
        default: break;
        }
        out += c;
    }

    return out;
}
#include <string>
#include <vector>
#include <stdexcept>

#include "Move.h"
#include "MoveGenerator.h"
#include "board.h"
#include "constants.h"

inline Move parse_uci_move(const Board& board, const std::string& s) {
    if (s.size() < 4)
        throw std::runtime_error("Invalid UCI move: " + s);

    int from_file = s[0] - 'a';
    int from_rank = s[1] - '1';
    int to_file = s[2] - 'a';
    int to_rank = s[3] - '1';

    int from_sq = from_rank * 8 + from_file;
    int to_sq = to_rank * 8 + to_file;

    PieceType promo = PieceType::NONE;
    if (s.size() == 5) {
        char pc = s[4];
        switch (pc) {
        case 'q': promo = PieceType::QUEEN;  break;
        case 'r': promo = PieceType::ROOK;   break;
        case 'b': promo = PieceType::BISHOP; break;
        case 'n': promo = PieceType::KNIGHT; break;
        default:
            throw std::runtime_error("Invalid promotion piece in UCI move: " + s);
        }
    }

    MoveList moves;
    MoveGenerator gen;
    gen.generate_moves(board,moves);   // use your legal move gen here

    for (const Move& m : moves) {
        if (m.from_square == from_sq && m.to_square == to_sq) {
            if (promo == PieceType::NONE || m.promotion_piece == promo) {
                return m;
            }
        }
    }

    throw std::runtime_error("No legal move matching UCI: " + s);
}
