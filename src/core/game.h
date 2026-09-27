#pragma once

#include <initializer_list>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Move.h"
#include "position.h"
#include "uci_helpers.h"

struct Game {
    Game() = default;

    Game(std::string_view starting_fen,
        std::span<const std::string_view> uci_moves) {
        if (uci_moves.size() > std::size(moves.moves)) {
            throw std::length_error("A Game cannot contain more than 256 moves");
        }

        Position position(starting_fen);
        for (std::size_t ply = 0; ply < uci_moves.size(); ++ply) {
            try {
                const Move move = parse_uci_move(position, uci_moves[ply]);
                moves.push_back(move);
                position.make_move(move);
            }
            catch (const std::exception& error) {
                throw std::invalid_argument(
                    "Invalid move at ply " + std::to_string(ply + 1)
                    + " ('" + std::string(uci_moves[ply]) + "'): " + error.what());
            }
        }
    }

    Game(std::string_view starting_fen,
        std::initializer_list<std::string_view> uci_moves)
        : Game(starting_fen,
            std::span<const std::string_view>(uci_moves.begin(), uci_moves.size())) {}

    MoveList moves = {};
    std::string_view Event = "";
    std::string_view Site = "";
    std::string_view Date = "";
    std::string_view Round = "-";
    std::string_view White = "";
    std::string_view Black = "";
    std::string_view Result = "";
    std::string_view GameId = "";
    std::string_view UTCDate = "";
    std::string_view UTCTime = "";
    std::string_view WhiteElo = "";
    std::string_view BlackElo = "";
    std::string_view WhiteRatingDiff = "";
    std::string_view BlackRatingDiff = "";
    std::string_view WhiteTitle = "";
    std::string_view BlackTitle = "";
    std::string_view Variant = "";
    std::string_view TimeControl = "";
    std::string_view ECO = "";
    std::string_view Opening = "";
    std::string_view Termination = "";
};
