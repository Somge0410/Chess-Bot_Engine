#include "TuneEval.h"
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <stdexcept>
#include <system_error>
#include <utility>
#include "evaluation.h"

//static constexpr int flip_square(int sq) {
//    return sq ^ 56;
//}
void TuneEval::normalize_pst(parameters_t& params) {
    // Nur normalisieren, wenn der komplette PST-Bereich (Pawn..King) im Tuning-Fenster liegt.
    // PARAM_END wird im Projekt exklusiv verwendet (< PARAM_END), daher > KING_PST_END.
    if (!(PARAM_START <= PAWN_PST_START && PARAM_END > KING_PST_END)) {
        return;
    }

    struct PstNormEntry {
        int pst_start;
        int value_param;
    };

    const PstNormEntry pst_norm_entries[] = {
        { PAWN_PST_START,   Pawn   },
        { KNIGHT_PST_START, Knight },
        { BISHOP_PST_START, Bishop },
        { ROOK_PST_START,   Rook   },
        { QUEEN_PST_START,  Queen  },
    };

    auto is_tuned = [](int global_index) {
        return global_index >= PARAM_START && global_index < PARAM_END;
    };

    auto local_index_of = [](int global_index) {
        return global_index - PARAM_START;
    };

    auto add_phase_value = [&](int global_index, int phase, double delta) {
        params[local_index_of(global_index)][phase] += delta;
    };

    auto get_phase_value = [&](int global_index, int phase) -> double {
        return params[local_index_of(global_index)][phase];
    };

    for (const auto& entry : pst_norm_entries) {
        int start_sq = 0;
        int end_sq = 64;
        if (entry.pst_start == PAWN_PST_START) {
            start_sq = 8;
            end_sq = 56;
        }

        // Sicherheitscheck: Ohne value_param im Tuning-Fenster keine Normalisierung
        // (sonst könnte die Summe nicht auf piece value verschoben werden).
        if (!is_tuned(entry.value_param)) {
            continue;
        }

        std::vector<int> tuned_pst_indices;
        tuned_pst_indices.reserve(end_sq - start_sq);

        for (int sq = start_sq; sq < end_sq; ++sq) {
            const int global_index = entry.pst_start + sq;
            if (is_tuned(global_index)) {
                tuned_pst_indices.push_back(global_index);
            }
        }

        if (tuned_pst_indices.empty()) {
            continue;
        }

        for (int phase = 0; phase < 2; ++phase) {
            double sum = 0.0;
            for (const int idx : tuned_pst_indices) {
                sum += get_phase_value(idx, phase);
            }

            const double avg = sum / static_cast<double>(tuned_pst_indices.size());

            for (const int idx : tuned_pst_indices) {
                add_phase_value(idx, phase, -avg);
            }

            add_phase_value(entry.value_param, phase, avg);
        }
    }
}
parameters_t TuneEval::get_initial_parameters() {
    parameters_t params(PARAM_LENGTH);
    for(int i=PARAM_START;i<PARAM_END;i++) {
        params[i - PARAM_START] = { static_cast<double>(EvalWeights[i].mg_score), static_cast<double>(EvalWeights[i].eg_score) };
	}
    return params;
}


EvalResult TuneEval::get_external_eval_result(const Position& position) {
    EvalResult result;
    Trace trace;
    evaluate<true>(position, &trace);
    result.game_phase = position.get_game_phase();

    result.coefficients.resize(PARAM_LENGTH);
    for(int i=PARAM_START;i<PARAM_END;i++){
        result.coefficients[i-PARAM_START] = trace.counts[i];
	}

    return result;
}

EvalResult TuneEval::get_fen_eval_result(const std::string& fen) {
    Position position(fen);
    return get_external_eval_result(position);
}
namespace {
    std::string get_parameter_name(int index)
    {
        switch (index)
        {
        case Pawn: return "PAWN";
        case Knight: return "KNIGHT";
        case Bishop: return "BISHOP";
        case Rook: return "ROOK";
        case Queen: return "QUEEN";
        case FORWARD_BLOCKED_BACKWARD: return "FORWARD_BLOCKED_BACKWARD";
        case FORWARD_CONTROLLED_BACKWARD: return "FORWARD_CONTROLLED_BACKWARD";
        case FREE_TO_ADV_BACKWARD: return "FREE_TO_ADV_BACKWARD";
        case PAWN_SHIELD_BONUS: return "PAWN_SHIELD_BONUS";
        case DIRECTLY_ON_OPEN_FILE_NEXT_TO_OPEN_PENALTY: return "DIRECTLY_ON_OPEN_FILE_NEXT_TO_OPEN_PENALTY";
        case DIRECTLY_ON_OPEN_FILE_NOT_NEXT_TO_OPEN_PENALTY: return "DIRECTLY_ON_OPEN_FILE_NOT_NEXT_TO_OPEN_PENALTY";
        case NEXT_TO_OPEN_FILE_PENALTY: return "NEXT_TO_OPEN_FILE_PENALTY";
        case DIRECTLY_ON_SEMI_OPEN_FILE_NEXT_TO_OPEN_PENALTY: return "DIRECTLY_ON_SEMI_OPEN_FILE_NEXT_TO_OPEN_PENALTY";
        case DIRECTLY_ON_SEMI_OPEN_FILE_NOT_NEXT_TO_OPEN_PENALTY: return "DIRECTLY_ON_SEMI_OPEN_FILE_NOT_NEXT_TO_OPEN_PENALTY";
        case NEXT_TO_SEMI_OPEN_FILE_PENALTY: return "NEXT_TO_SEMI_OPEN_FILE_PENALTY";
        case ROOK_ON_OPEN_FILE: return "ROOK_ON_OPEN_FILE";
        case ROOK_ON_SEMI_OPEN_FILE: return "ROOK_ON_SEMI_OPEN_FILE";
        case CONNECTED_ROOKS: return "CONNECTED_ROOKS";
        case BISHOP_PAIR: return "BISHOP_PAIR";
        case BAD_BISHOP_BLOCKED: return "BAD_BISHOP_BLOCKED";
        case BAD_BISHOP_UNBLOCKED: return "BAD_BISHOP_UNBLOCKED";
        case TRAPPED_BISHOP: return "TRAPPED_BISHOP";
        case TRAPPED_KNIGHT: return "TRAPPED_KNIGHT";
        case FIANCHETTO_BISHOP: return "FIANCHETTO_BISHOP";
        case BROKEN_FIANCHETTO: return "BROKEN_FIANCHETTO";
        case BISHOP_OUTPOST_NO_OPPOSITE_BISHOP: return "BISHOP_OUTPOST_NO_OPPOSITE_BISHOP";
        case BISHOP_OUTPOST_WITH_OPPOSITE_BISHOP: return "BISHOP_OUTPOST_WITH_OPPOSITE_BISHOP";
        case KNIGHT_OUTPOST_NO_OPPOSITE_BISHOP: return "KNIGHT_OUTPOST_NO_OPPOSITE_BISHOP";
        case KNIGHT_OUTPOST_WITH_OPPOSITE_BISHOP: return "KNIGHT_OUTPOST_WITH_OPPOSITE_BISHOP";
        }

        const std::pair<int, const char*> ranges[] = {
            {PAWN_PST_START, "PAWN_PST"}, {KNIGHT_PST_START, "KNIGHT_PST"},
            {BISHOP_PST_START, "BISHOP_PST"}, {ROOK_PST_START, "ROOK_PST"},
            {QUEEN_PST_START, "QUEEN_PST"}, {KING_PST_START, "KING_PST"},
            {PASSED_PAWNS_START, "PASSED_PAWNS"}, {PROTECTED_PASSED_PAWNS_START, "PROTECTED_PASSED_PAWNS"},
            {BLOCKED_FREE_PAWN_START, "BLOCKED_FREE_PAWN"}, {CANT_REACHED_BY_ENEMY_KING_START, "CANT_REACHED_BY_ENEMY_KING"},
            {OWN_KING_IS_CLOSE_START, "OWN_KING_IS_CLOSE"}, {OWN_KING_IS_FAR_START, "OWN_KING_IS_FAR"},
            {ROOK_BEHIND_FREE_PAWN_START, "ROOK_BEHIND_FREE_PAWN"}, {OP_ROOK_BEHIND_FREE_PAWN_START, "OP_ROOK_BEHIND_FREE_PAWN"},
            {ISOLANI_START, "ISOLANI"}, {BLOCKED_ISOLANI_START, "BLOCKED_ISOLANI"},
            {PROTECTED_ISOLANI_START, "PROTECTED_ISOLANI"}, {DOUBLE_PAWN_FILE_START, "DOUBLE_PAWN_FILE"},
            {NEXT_TO_OPEN_DIAGONAL_PENALTY_START, "NEXT_TO_OPEN_DIAGONAL_PENALTY"}, {MOBILITY_START, "MOBILITY"}
        };
        for (const auto& [start, name] : ranges)
        {
            if (index >= start && index <= start + (start == PAWN_PST_START || start == KNIGHT_PST_START ||
                start == BISHOP_PST_START || start == ROOK_PST_START || start == QUEEN_PST_START ||
                start == KING_PST_START ? 63 : start == DOUBLE_PAWN_FILE_START ? 7 :
                start == NEXT_TO_OPEN_DIAGONAL_PENALTY_START ? 6 : start == MOBILITY_START ? 3 : 15))
            {
                return std::string(name) + "+" + std::to_string(index - start);
            }
        }
        return "PARAM_" + std::to_string(index);
    }

    double get_phase_value(const parameters_t& parameters, int global_index, int phase)
    {
        const int local_index = global_index - PARAM_START;
        if (local_index >= 0 && local_index < static_cast<int>(parameters.size()))
        {
            return parameters[local_index][phase];
        }

        return (phase == static_cast<int>(PhaseStages::Midgame))
            ? static_cast<double>(EvalWeights[global_index].mg_score)
            : static_cast<double>(EvalWeights[global_index].eg_score);
    }

    int scaled_weight(const parameters_t& parameters, int index, int phase, double scale)
    {
        const auto value = std::round(get_phase_value(parameters, index, phase) * scale);
        return static_cast<int>(std::clamp(value,
            static_cast<double>(std::numeric_limits<int16_t>::min()),
            static_cast<double>(std::numeric_limits<int16_t>::max())));
    }

    void write_weight_initializer(std::ostream& output, const parameters_t& parameters)
    {
        const int mg = static_cast<int>(PhaseStages::Midgame);
        const int eg = static_cast<int>(PhaseStages::Endgame);
        const double pawn_mg = get_phase_value(parameters, Pawn, mg);
        const double scale = pawn_mg != 0.0 ? 100.0 / pawn_mg : 0.0;

        output << "#include \"evaluation.h\"\nEvaluationResult EvalWeights[PARAM_COUNT] = {\n";
        for (int index = 0; index < PARAM_COUNT; ++index)
        {
            output << "    {" << scaled_weight(parameters, index, mg, scale) << ", "
                << scaled_weight(parameters, index, eg, scale) << "}, // "
                << get_parameter_name(index) << '\n';
        }
        output << "};\n";
    }
}

void TuneEval::print_parameters(parameters_t& parameters) {
    write_weight_initializer(std::cout, parameters);
}

void TuneEval::write_parameters(const parameters_t& parameters, const std::string& path)
{
    const std::filesystem::path weights_path(path);
    auto temporary_path = weights_path;
    temporary_path += ".spsa.tmp";
    auto backup_path = weights_path;
    backup_path += ".spsa.bak";

    {
        std::ofstream output(temporary_path, std::ios::trunc);
        if (!output)
        {
            throw std::runtime_error("Unable to write temporary weights file " + temporary_path.string());
        }
        write_weight_initializer(output, parameters);
        output.close();
        if (!output)
        {
            std::filesystem::remove(temporary_path);
            throw std::runtime_error("Failed while writing temporary weights file " + temporary_path.string());
        }
    }

    std::error_code error;
    std::filesystem::remove(backup_path, error);
    std::filesystem::rename(weights_path, backup_path);
    try
    {
        std::filesystem::rename(temporary_path, weights_path);
    }
    catch (...)
    {
        std::filesystem::rename(backup_path, weights_path);
        throw;
    }
    std::filesystem::remove(backup_path, error);
}