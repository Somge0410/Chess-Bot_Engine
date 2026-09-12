#include "fen.h"
#include "bitboard.h"
#include <array>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
namespace fen {
    namespace {

    constexpr PieceType to_piece_type(char c) noexcept{
        switch (c) {
        case 'P':
		case 'p':
			return PieceType::Pawn;
		case 'N':
		case 'n':
			return PieceType::Knight;
		case 'B':
		case 'b':
			return PieceType::Bishop;
		case 'R':
		case 'r':
			return PieceType::Rook;
		case 'Q':
		case 'q':
			return PieceType::Queen;
		case 'K':
		case 'k':
			return PieceType::King;
		default:
			return PieceType::None;
            }

     }
        void parse_fen_pieces(std::string_view field, FenData& data) {
            int rank = 7;
            int file = 0;
            for (char c : field) {
                if (c == '/') {
                    rank--;
                    file = 0;
                }
                else if (isdigit(c)) {
                    file += (c - '0');
                }
                else {
                    const Color color = isupper(c) ? Color::White : Color::Black;

                    const PieceType piece_type = to_piece_type(c);
                    const Square square = to_square(file, rank);

                    data.pieces[to_int(color)][to_int(piece_type)] |= bit64(square);

                    if (piece_type == PieceType::King) {
                        data.king_squares[to_int(color)] = square;
                    }

                    ++file;

                }
            }
        }
        void parse_fen_castling(std::string_view field, FenData& data) {
            data.castling_rights = CastlingRights::None;
            for (char character : field) {
                switch (character) {
                case 'K':
                    data.castling_rights |= CastlingRights::WhiteKingside;
                    break;
                case 'Q':
                    data.castling_rights |= CastlingRights::WhiteQueenside;
                    break;
                case 'k':
                    data.castling_rights |= CastlingRights::BlackKingside;
                    break;
                case 'q':
                    data.castling_rights |= CastlingRights::BlackQueenside;
                    break;
                case '-':
                    break; // No castling rights
                default:
                    throw std::runtime_error{ "Invalid FEN: unexpected character in castling rights: " + std::string{field} };
                }
            }
        }
        void parse_fen_turn(std::string_view field, FenData& data) {
            if (field == "w") {
                data.side_to_move = Color::White;
            }
            else if (field == "b") {
                data.side_to_move = Color::Black;
            }
            else {
                throw std::runtime_error{ "Invalid FEN: unexpected character in turn field: " + std::string{field} };
            }
        }
        void parse_fen_en_passant(std::string_view field, FenData& data) {
            if (field == "-") {
                data.en_passant_square = NO_SQUARE;
            }
            else {
                int file = field[0] - 'a'; // 'a' becomes 0, 'b' becomes 1, etc.
                int rank = field[1] - '1'; // '1' becomes 0, '2' becomes 1, etc.
                data.en_passant_square = to_square(file, rank);
            }
        }
        void parse_fen_half_move(std::string_view field, FenData& data) {
            data.halfmove_clock = std::stoi(std::string{ field });
        }
        void parse_fen_move(std::string_view field, FenData& data) {
            data.full_move_number = std::stoi(std::string{ field });
        }


    }
    FenData parse(std::string_view text) {
        std::istringstream stream{ std::string{text} };
        std::array<std::string, 6> fields;

        for (std::string& field : fields) {
            if (!(stream >> field)) {
                throw std::runtime_error{
                    "Invalid FEN: expected 6 field: " + std::string{text}
                };
            }
        }

        FenData data{};

        parse_fen_pieces(fields[0], data);
        parse_fen_turn(fields[1], data);
        parse_fen_castling(fields[2], data);
        parse_fen_en_passant(fields[3], data);
        parse_fen_half_move(fields[4], data);
        parse_fen_move(fields[5], data);

        return data;
    }
}