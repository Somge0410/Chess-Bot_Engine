#include "make_undo_test.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "engine.h"
#include "fen.h"
#include "game.h"
#include "MoveGenerator.h"
#include "position.h"
#include "search.h"
#include "uci_helpers.h"

namespace {
constexpr int SELF_PLAY_DEPTH = 3;
constexpr int SELF_PLAY_MAX_PLIES = 120;
constexpr std::size_t SELF_PLAY_TT_MB = 16;

struct PositionSnapshot {
    RegressionTestState derived_state;
    PieceBoards pieces;
    Color side_to_move;
    CastlingRights castling_rights;
    Square en_passant_square;
    int halfmove_clock;
    int fullmove_number;
    Square white_king_square;
    Square black_king_square;
    std::size_t history_size;
    int repetition_count;
    bool has_twofold;
};

class ScopedSearchOutputSilencer {
public:
    ScopedSearchOutputSilencer()
        : previous_buffer_(std::cout.rdbuf(discarded_output_.rdbuf())) {}

    ~ScopedSearchOutputSilencer() {
        std::cout.rdbuf(previous_buffer_);
    }

    ScopedSearchOutputSilencer(const ScopedSearchOutputSilencer&) = delete;
    ScopedSearchOutputSilencer& operator=(const ScopedSearchOutputSilencer&) = delete;

private:
    std::ostringstream discarded_output_;
    std::streambuf* previous_buffer_;
};

PositionSnapshot snapshot(const Position& position) {
    return {
        position.get_regression_test_state(),
        position.get_pieces_table(),
        position.get_turn(),
        position.get_castle_rights(),
        position.get_en_passant_rights(),
        position.get_half_moves(),
        position.get_move_count(),
        position.get_king_square(Color::White),
        position.get_king_square(Color::Black),
        position.get_history().size(),
        position.get_position_repeat_count(),
        position.has_twofold(),
    };
}

bool validate_incremental_state(const Position& position,
    std::string_view operation, std::size_t ply) {
    const RegressionTestState actual = position.get_regression_test_state();
    const RegressionTestState recalculated = position.calculate_regression_test_state();
    if (actual == recalculated) {
        return true;
    }

    std::cerr << "[FAIL] make/undo " << operation << " at ply " << ply
              << ": incremental state differs from recalculated state\n"
              << "  hash: " << actual.zobrist << " vs " << recalculated.zobrist << '\n'
              << "  pawn hash: " << actual.pawn_key << " vs " << recalculated.pawn_key << '\n'
              << "  raw phase: " << actual.game_phase << " vs "
              << recalculated.game_phase << '\n'
              << "  material: (" << actual.material_score.mg_score << ','
              << actual.material_score.eg_score << ") vs ("
              << recalculated.material_score.mg_score << ','
              << recalculated.material_score.eg_score << ")\n"
              << "  positional: (" << actual.positional_score.mg_score << ','
              << actual.positional_score.eg_score << ") vs ("
              << recalculated.positional_score.mg_score << ','
              << recalculated.positional_score.eg_score << ")\n"
              << "  checkers: " << actual.checkers << " vs "
              << recalculated.checkers << '\n';
    return false;
}

bool matches_snapshot(const Position& position, const PositionSnapshot& expected,
    std::size_t ply) {
    const PositionSnapshot actual = snapshot(position);
    if (actual.derived_state == expected.derived_state
        && actual.pieces.data == expected.pieces.data
        && actual.side_to_move == expected.side_to_move
        && actual.castling_rights == expected.castling_rights
        && actual.en_passant_square == expected.en_passant_square
        && actual.halfmove_clock == expected.halfmove_clock
        && actual.fullmove_number == expected.fullmove_number
        && actual.white_king_square == expected.white_king_square
        && actual.black_king_square == expected.black_king_square
        && actual.history_size == expected.history_size
        && actual.repetition_count == expected.repetition_count
        && actual.has_twofold == expected.has_twofold) {
        return true;
    }

    std::cerr << "[FAIL] undo at ply " << ply
              << " did not restore the exact previous position\n"
              << "  turn: " << static_cast<int>(actual.side_to_move) << " vs "
              << static_cast<int>(expected.side_to_move) << '\n'
              << "  castling: " << static_cast<int>(actual.castling_rights) << " vs "
              << static_cast<int>(expected.castling_rights) << '\n'
              << "  en passant: " << static_cast<int>(actual.en_passant_square) << " vs "
              << static_cast<int>(expected.en_passant_square) << '\n'
              << "  clocks: " << actual.halfmove_clock << '/' << actual.fullmove_number
              << " vs " << expected.halfmove_clock << '/' << expected.fullmove_number << '\n'
              << "  history: " << actual.history_size << " vs "
              << expected.history_size << '\n'
              << "  repetition count: " << actual.repetition_count << " vs "
              << expected.repetition_count << '\n'
              << "  twofold: " << actual.has_twofold << " vs "
              << expected.has_twofold << '\n';
    return false;
}

Game make_regression_game() {
    return Game(fen::START_POSITION, {
        "e2e4", "d7d5", "e4e5", "f7f5", "e5f6", "e7e6",
        "f6f7", "e8d7", "f7g8q", "h8g8", "b1c3", "c7c5",
        "d2d4", "c5d4", "d1d4", "b8c6", "c3d5", "h7h6",
        "d5b6", "d7e7", "d4d8", "e7d8", "c1e3", "a7b6",
        "e1c1", "d8c7", "f1b5", "a8a2", "g1e2", "a2a1",
        "c1d2", "f8b4", "c2c3", "a1d1", "h1d1", "b4c5",
        "e3c5", "b6c5", "b5c6", "c7d8", "d2e1", "d8e7",
        "c6b7", "c8b7", "d1d7", "e7d7", "e1f1", "b7a6",
        "f1e1", "a6b7", "e1f1", "b7a6", "f1e1", "a6b7",
        "e1f1",
    });
}

bool undo_and_validate(Position& position, const MoveList& moves,
    const std::vector<PositionSnapshot>& previous_positions,
    std::string_view test_name) {
    bool passed = true;
    for (std::size_t index = moves.size(); index > 0; --index) {
        position.undo_move();
        if (!validate_incremental_state(position, "after undo", index)
            || !matches_snapshot(position, previous_positions[index - 1], index)) {
            std::cerr << "  test: " << test_name << '\n'
                      << "  move: "
                      << move_to_uci(moves[static_cast<int>(index - 1)]) << '\n';
            passed = false;
        }
    }
    return passed;
}

void print_uci_game(const MoveList& moves, std::string_view termination,
    std::string_view result) {
    std::cout << "Self-play game (UCI, depth " << SELF_PLAY_DEPTH << "):\n";
    for (std::size_t ply = 0; ply < moves.size(); ++ply) {
        if (ply % 16 == 0 && ply != 0) {
            std::cout << '\n';
        }
        if (ply % 2 == 0) {
            std::cout << (ply / 2 + 1) << ". ";
        }
        std::cout << move_to_uci(moves[static_cast<int>(ply)]) << ' ';
    }
    std::cout << result << "\nTermination: " << termination
              << ", plies: " << moves.size() << '\n';
}

bool run_fixed_game_test() {
    const Game game = make_regression_game();
    Position position(fen::START_POSITION);
    std::vector<PositionSnapshot> previous_positions;
    previous_positions.reserve(game.moves.size());

    std::size_t ply = 0;
    for (const Move& move : game.moves) {
        previous_positions.push_back(snapshot(position));
        position.make_move(move);
        ++ply;

        if (!validate_incremental_state(position, "after make", ply)) {
            std::cerr << "  test: fixed move sequence\n"
                      << "  move: " << move_to_uci(move) << '\n';
            return false;
        }
    }

    if (!undo_and_validate(position, game.moves, previous_positions,
            "fixed move sequence")) {
        return false;
    }

    std::cout << "Fixed make/undo regression passed: " << game.moves.size()
              << " moves made and undone\n";
    return true;
}

bool run_self_play_game_test() {
    SearchLimits limits;
    limits.depth = SELF_PLAY_DEPTH;

    Position position(fen::START_POSITION);
    // Engine owns large per-ply search buffers, so keep both instances off the
    // relatively small Windows test-thread stack.
    auto white = std::make_unique<Engine>(SELF_PLAY_TT_MB);
    auto black = std::make_unique<Engine>(SELF_PLAY_TT_MB);
    MoveList played_moves;
    std::vector<PositionSnapshot> previous_positions;
    previous_positions.reserve(SELF_PLAY_MAX_PLIES);

    std::string termination = "move-limit";
    std::string result = "*";
    bool passed = true;

    for (int ply = 1; ply <= SELF_PLAY_MAX_PLIES; ++ply) {
        if (position.is_fifty_move_rule_draw()) {
            termination = "fifty-move rule";
            result = "1/2-1/2";
            break;
        }
        if (position.is_repetition_draw(3)) {
            termination = "threefold repetition";
            result = "1/2-1/2";
            break;
        }

        MoveList legal_moves;
        MoveGenerator::generate_moves(position, legal_moves);
        if (legal_moves.empty()) {
            if (position.in_check()) {
                termination = "checkmate";
                result = position.is_white_to_move() ? "0-1" : "1-0";
            }
            else {
                termination = "stalemate";
                result = "1/2-1/2";
            }
            break;
        }

        Engine& engine = position.is_white_to_move() ? *white : *black;
        Move best;
        {
            ScopedSearchOutputSilencer silence_search_output;
            best = engine.search(position, limits);
        }

        const Move* legal_best = std::find(legal_moves.begin(), legal_moves.end(), best);
        if (legal_best == legal_moves.end()) {
            termination = "illegal or missing best move";
            const std::string best_uci = best.from_square == NO_SQUARE
                || best.to_square == NO_SQUARE ? "0000" : move_to_uci(best);
            std::cerr << "[FAIL] self-play returned an illegal move at ply "
                      << ply << ": " << best_uci << '\n';
            passed = false;
            break;
        }

        previous_positions.push_back(snapshot(position));
        played_moves.push_back(*legal_best);
        position.make_move(*legal_best);

        if (!validate_incremental_state(position, "after self-play make", ply)) {
            std::cerr << "  move: " << move_to_uci(*legal_best) << '\n';
            termination = "incremental-state failure";
            passed = false;
            break;
        }
    }

    print_uci_game(played_moves, termination, result);

    const bool undo_passed = undo_and_validate(
        position, played_moves, previous_positions, "self-play game");
    if (passed && undo_passed) {
        std::cout << "Self-play make/undo regression passed: "
                  << played_moves.size() << " moves made and undone\n";
    }
    return passed && undo_passed;
}
}

int run_make_undo_tests() {
    int failed_tests = 0;
    if (!run_fixed_game_test()) {
        ++failed_tests;
    }
    if (!run_self_play_game_test()) {
        ++failed_tests;
    }
    return failed_tests == 0 ? 0 : 1;
}
