#include "engine.h"

#include <algorithm>

#include "MoveGenerator.h"
#include "evaluation.h"
#include "move_ordering.h"
#include "search_parameters.h"
#include "search_diagnostics.h"
#include "see.h"

int Engine::quiescence_search(Position& pos, int alpha, int beta, int search_ply, int qply,
    ThreadLocalData* tls, uint64_t checkers, bool after_check_invasions) {
    if (stop_search.load(std::memory_order_relaxed)) {
        return 0;
    }
    if (tls) {
        tls->qnodes++;
#if ENABLE_QSEARCH_DIAGNOSTICS
        tls->qply_sum += static_cast<uint64_t>(qply);
        tls->max_qply = std::max(tls->max_qply, static_cast<uint32_t>(qply));
        increment_diagnostic(tls, qply_bucket(qply));
#endif
        tls->flush_counters(this);
        if (tls->should_check_time() && time_manager.is_time_up()) {
            stop_search.store(true, std::memory_order_relaxed);
            return 0;
        }
    }
#if ENABLE_QSEARCH_DIAGNOSTICS
    if (pos.is_fifty_move_rule_draw()) {
        increment_diagnostic(tls, SearchDiagCounter::QFiftyMoveDraws);
        return 0;
    }
    if (pos.is_repetition_draw(3)) {
        increment_diagnostic(tls, SearchDiagCounter::QRepetitionDraws);
        return 0;
    }
#else
    if (pos.is_fifty_move_rule_draw() || pos.is_repetition_draw(3)) {
        return 0;
    }
#endif
    if (checkers == CHECKERS_UNKNOWN) {
        checkers = pos.get_checkers();
    }
    bool in_check = checkers != 0;
#if ENABLE_QSEARCH_DIAGNOSTICS
    if (tls && in_check) {
        tls->qnodes_in_check++;
    }
#endif
    constexpr int max_qply_index = ThreadLocalData::QSEARCH_PLY_CAPACITY - 1;
    const int qmove_list_index = std::min(qply, max_qply_index);
    MoveList& moves = tls->qmove_lists[qmove_list_index];
    moves.clear();
	const uint64_t hash = pos.get_hash();

    for(int previous =qply-2;previous >=0; previous -= 2) {
        if (tls->qsearch_hashes[previous]== hash) {
#if ENABLE_QSEARCH_DIAGNOSTICS
            tls->cycle_cutoffs++;
#endif
            return 0;
        }
	}
	tls->qsearch_hashes[qply] = hash;

    if(qply >= MAX_QUIET_PLY && !in_check) {
#if ENABLE_QSEARCH_DIAGNOSTICS
        increment_diagnostic(tls, SearchDiagCounter::QSoftCapStaticReturns);
#endif
        return pos.is_white_to_move() ? evaluate(pos) : -evaluate(pos);
	}
    if (qply >=max_qply_index) {
#if ENABLE_QSEARCH_DIAGNOSTICS
        if (tls) {
            tls->hard_cap_hits++;
        }
#endif
        if(!in_check) {
#if ENABLE_QSEARCH_DIAGNOSTICS
            increment_diagnostic(tls, SearchDiagCounter::QHardCapStaticReturns);
#endif
            return pos.is_white_to_move() ? evaluate(pos) : -evaluate(pos);
        }
        else {
#if ENABLE_QSEARCH_DIAGNOSTICS
            increment_diagnostic(tls, SearchDiagCounter::QHardCapDrawReturns);
#endif
            return 0; // emergency heuristic, after so many checks its likely a repetitive check.
        }
    }


    if (in_check) {
        MoveGenerator::generate_moves(pos, moves, checkers);
        if (moves.empty()) {
#if ENABLE_QSEARCH_DIAGNOSTICS
            increment_diagnostic(tls, SearchDiagCounter::QCheckmates);
#endif
            return -MATE_SCORE + search_ply;
        }
    }
    else {
        int stand_pat = pos.is_white_to_move() ? evaluate(pos) : -evaluate(pos);
        if (stand_pat >= beta) {
#if ENABLE_QSEARCH_DIAGNOSTICS
            increment_diagnostic(tls, SearchDiagCounter::QStandPatCutoffs);
#endif
            return stand_pat;
        }
        if (stand_pat > alpha) alpha = stand_pat;
		const bool include_quiet_checks = qply == 0 || after_check_invasions;
        if(include_quiet_checks)
            MoveGenerator::generate_captures_with_checks(pos, moves, checkers);
        else
			MoveGenerator::generate_captures(pos, moves, checkers);
    }
#if ENABLE_QSEARCH_DIAGNOSTICS
    increment_diagnostic(tls, SearchDiagCounter::QMovesGenerated, moves.size());
#endif

    int best_score = in_check ? -MATE_SCORE : alpha;
    int scores[256];
    score_qsearch_moves(moves, scores);
#if ENABLE_QSEARCH_DIAGNOSTICS
    uint64_t qmoves_searched = 0;
#endif

    for (int i = 0; i < (int)moves.size();) {
        if (stop_search.load(std::memory_order_relaxed)) {
            break;
        }
        pick_best(moves, scores, i);
        Move move = moves[i];

        if (!in_check) {
            if (scores[i] == std::numeric_limits<int>::min()) break;
            const bool is_capture = move.piece_captured != PieceType::None;
            if (is_capture && see_move(pos, move) < 0) {
#if ENABLE_QSEARCH_DIAGNOSTICS
                increment_diagnostic(tls, SearchDiagCounter::QSeePrunes);
#endif
                scores[i] = std::numeric_limits<int>::min();
                continue;
            }
        }

        pos.make_move(move);
        const uint64_t child_checkers = pos.get_checkers();
#if ENABLE_QSEARCH_DIAGNOSTICS
        if (tls && !in_check &&
            move.piece_captured == PieceType::None &&
            move.promotion_piece == PieceType::None && child_checkers != 0) {
            tls->quiet_checks_searched++;
        }
#endif
        int score = -quiescence_search(pos, -beta, -alpha, search_ply + 1, qply + 1, tls, child_checkers,in_check);
        pos.undo_move();
		++i;
#if ENABLE_QSEARCH_DIAGNOSTICS
        qmoves_searched++;
        increment_diagnostic(tls, SearchDiagCounter::QMovesSearched);
#endif

        if (score > best_score) best_score = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) {
#if ENABLE_QSEARCH_DIAGNOSTICS
            if (in_check) {
                increment_diagnostic(tls, SearchDiagCounter::QEvasionBetaCutoffs);
            }
            else if (move.piece_captured != PieceType::None) {
                increment_diagnostic(tls, SearchDiagCounter::QCaptureBetaCutoffs);
            }
            else if (move.promotion_piece != PieceType::None) {
                increment_diagnostic(tls, SearchDiagCounter::QPromotionBetaCutoffs);
            }
            else if (child_checkers != 0) {
                increment_diagnostic(tls, SearchDiagCounter::QQuietCheckBetaCutoffs);
            }
#endif
            break;
        }
    }

#if ENABLE_QSEARCH_DIAGNOSTICS
    if (!stop_search.load(std::memory_order_relaxed) && !in_check && qmoves_searched == 0) {
        increment_diagnostic(tls, SearchDiagCounter::QNoTacticalMoves);
    }
#endif

    return best_score;
}
