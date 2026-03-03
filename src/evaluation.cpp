#include <iostream>
#include <mutex>
#include "evaluation.h"
#include "constants.h"
#include "pst.h"
#include "utils.h"
#include "bitboard_masks.h"
#include "Evaluation_data.h"
#include "attack_rays.h"
#include "see.h"
#include "adjustable_parameters.h"
namespace {
    struct KsStats {
        int min = 1000000;
        int max = -1000000;
        long long sum = 0;
        long long count = 0;
    };

    static KsStats g_ks_stats;
    static std::mutex g_ks_mtx;

    static thread_local KsStats tls_ks_stats;
    static thread_local long long tls_samples = 0;

    static void merge_ks_stats() {
        std::lock_guard<std::mutex> lock(g_ks_mtx);
        if (tls_ks_stats.min < g_ks_stats.min) g_ks_stats.min = tls_ks_stats.min;
        if (tls_ks_stats.max > g_ks_stats.max) g_ks_stats.max = tls_ks_stats.max;
        g_ks_stats.sum += tls_ks_stats.sum;
        g_ks_stats.count += tls_ks_stats.count;

        tls_ks_stats = KsStats{};
    }

    static void log_king_safety_sample(int value) {
        if (value < tls_ks_stats.min) tls_ks_stats.min = value;
        if (value > tls_ks_stats.max) tls_ks_stats.max = value;
        tls_ks_stats.sum += value;
        tls_ks_stats.count++;
        tls_samples++;

        if ((tls_samples % 4096) == 0) {
            merge_ks_stats();
        }
        if ((tls_samples % 65536) == 0) {
            std::lock_guard<std::mutex> lock(g_ks_mtx);
            double avg = g_ks_stats.count == 0 ? 0.0 : (double)g_ks_stats.sum / g_ks_stats.count;
            std::cerr << "[KS] min=" << g_ks_stats.min
                << " max=" << g_ks_stats.max
                << " avg=" << avg
                << " samples=" << g_ks_stats.count << "\n";
        }
    }
}

static thread_local PawnEvalEntry pawn_evaluation_table[PAWN_HASH_SIZE] = {};

EvalContext::EvalContext(const Board& b)
    : board(b),
    pieces{ b.get_pieces_table() },
    color_pieces{ b.get_color_pieces(Color::WHITE),b.get_color_pieces(Color::BLACK) },
    all(b.get_all_pieces()),
    castle_rights(b.get_castle_rights()),
    king_sq{ b.get_king_square(Color::WHITE),b.get_king_square(Color::BLACK) },
    game_phase(b.get_game_phase()),
    backward_pawns{ 0,0 },
    isolated_pawns{ 0,0 },
    passed_pawns{ 0,0 },
    no_pawn_file_count{ 0,0 },    // ADD THIS
    no_color_pawn_files{ 0,0 },   // ADD THIS
    open_files(0)
{
    for (size_t file = 0; file < 8; ++file) {
        uint64_t file_mask = FILE_MASK[file];
        bool white_has_pawns = (pieces[0][to_int(PieceType::PAWN)] & file_mask) != 0;
        bool black_has_pawns = (pieces[1][to_int(PieceType::PAWN)] & file_mask) != 0;
        if (!white_has_pawns)
            no_pawn_files[0][no_pawn_file_count[0]++] = static_cast<uint8_t>(file);
        if (!black_has_pawns)
            no_pawn_files[1][no_pawn_file_count[1]++] = static_cast<uint8_t>(file);

        if (!white_has_pawns && !black_has_pawns) {
            no_color_pawn_files[0] |= file_mask;
            no_color_pawn_files[1] |= file_mask;
            open_files |= file_mask;
        }
        else if (!white_has_pawns && black_has_pawns) {
            no_color_pawn_files[0] |= file_mask;
        }
        else if (white_has_pawns && !black_has_pawns) {
            no_color_pawn_files[1] |= file_mask;
        }
    }
}



// --- Forward Declarations for static helper functions ---
static EvaluationResult evaluate_material(const EvalContext& ctx);
static EvaluationResult evaluate_positional(const Board& board);

static PawnEvalEntry compute_pawn_eval_entry(EvalContext& ctx);
static EvaluationResult eval_backward_pawns(EvalContext& ctx);

static EvaluationResult eval_iso_passed_pawns(EvalContext& ctx);
static EvaluationResult eval_king_safety_score(const EvalContext& ctx);
static EvaluationResult evaluate_material(const EvalContext& ctx) {
    return ctx.board.get_material_score();
}
static EvaluationResult evaluate_positional(const Board& board) {
    return board.get_positional_score();
}

static EvaluationResult eval_iso_passed_pawns(EvalContext& ctx) {
    EvaluationResult score = { 0,0 };
    int iso_count = 0;
    int blocked_iso_count = 0;
    for (size_t color = 0; color < 2; color++) {
        int ecolor = color == 0 ? 1 : 0;
        uint64_t pawns = ctx.pieces[color][to_int(PieceType::PAWN)];
        while (pawns)
        {
            int pawn_square = get_lsb(pawns);
            int file_index = pawn_square % 8;
            int rank_index = color == 0 ? pawn_square / 8 : 7 - pawn_square / 8;
            if ((ctx.pieces[ecolor][to_int(PieceType::PAWN)] & PASSED_PAWN_MASK[color][pawn_square]) == 0)
            {
                ctx.passed_pawns[color] |= (1ULL << pawn_square);
                if (color == 0) {

                    score.mg_score += passed_pawns_MG[pawn_square];
                    score.eg_score += passed_pawns_MG[pawn_square];
                }
                else {
                    score.mg_score -= passed_pawns_MG[flip_square(pawn_square)];
                    score.eg_score -= passed_pawns_EG[flip_square(pawn_square)];

                }
                pawns &= pawns - 1;
                continue;
            }
            if ((ctx.pieces[color][to_int(PieceType::PAWN)] & ADJACENT_FILE_MASK[file_index]) == 0)
            {
                ctx.isolated_pawns[color] |= (1ULL << pawn_square);
                int forward_square = color == 0 ? pawn_square + 8 : pawn_square - 8;
                if ((ctx.board.get_color_pieces(Color(ecolor)) & bit64(forward_square)) != 0)
                    blocked_iso_count += color == 0 ? 1 : -1;
                else
                    iso_count += color == 0 ? 1 : -1;

            }
            pawns &= pawns - 1;
        }
    }
    score.mg_score += iso_count * ISOLATED_PAWN_PENALTY_MG;
    score.eg_score += iso_count * ISOLATED_PAWN_PENALTY_EG;
    score.mg_score += blocked_iso_count * BLOCKED_ISO_PENALTY_MG;
    score.eg_score += blocked_iso_count * BLOCKED_ISO_PENALTY_EG;
    return score;
}
static EvaluationResult eval_pawn_islands(const EvalContext& ctx) {
    EvaluationResult score = { 0,0 };
    int pawn_island_count = 0;
    bool white_active_island = false;
    bool black_active_island = false;
    for (size_t file = 0; file < 8; ++file) {
        uint64_t file_mask = FILE_MASK[file];
        if (white_active_island && (ctx.pieces[0][to_int(PieceType::PAWN)] & file_mask) == 0) {
            pawn_island_count++;
            white_active_island = false;
        }
        else if ((ctx.pieces[0][to_int(PieceType::PAWN)] & file_mask) != 0) {
            white_active_island = true;
            if (file == 7) {

                pawn_island_count++;
            }
        }
        if (black_active_island && (ctx.pieces[1][to_int(PieceType::PAWN)] & file_mask) == 0) {

            pawn_island_count--;
            black_active_island = false;
        }
        else if ((ctx.pieces[1][to_int(PieceType::PAWN)] & file_mask) != 0) {
            black_active_island = true;
            if (file == 7) {

                pawn_island_count--;
            }
        }
    }
    score.mg_score += pawn_island_count * PAWN_ISLAND_PENALTY_MG;
    score.eg_score += pawn_island_count * PAWN_ISLAND_PENALTY_EG;
    return score;
}
static EvaluationResult eval_candidate_passed_pawn(const EvalContext& ctx) {
    EvaluationResult score = { 0,0 };
    uint64_t all_pawns = ctx.pieces[0][to_int(PieceType::PAWN)] | ctx.pieces[1][to_int(PieceType::PAWN)];
    for (size_t color = 0; color < 2; color++) {
        int ecolor = color == 0 ? 1 : 0;
        uint64_t potential_pawns = ctx.pieces[color][to_int(PieceType::PAWN)] & ~ctx.passed_pawns[color];
        for (size_t file = 0; file < 8; file++) {
            uint64_t pawns_on_file = potential_pawns & FILE_MASK[file];
            while (pawns_on_file) {
                int pawn_square = get_lsb(pawns_on_file);
                uint64_t forwads_path = FORWARD_WAY_MASK[color][pawn_square];
                if ((all_pawns & forwads_path) != 0) {
                    pawns_on_file &= pawns_on_file - 1;
                    continue;
                }
                bool is_contender = true;
                while (forwads_path) {
                    int sq = get_lsb(forwads_path);
                    uint64_t attackers = ctx.pieces[ecolor][to_int(PieceType::PAWN)] & PAWN_ATTACKS[color][sq];
                    uint64_t defenders = ctx.pieces[color][to_int(PieceType::PAWN)] & PAWN_ATTACKS[ecolor][sq];
                    if (popcount(attackers) > popcount(defenders)) {
                        is_contender = false;
                        break;
                    }
                    forwads_path &= forwads_path - 1;
                }
                if (is_contender) {
                    if (color == 0) {

                        //score.mg_score += candidate_passed_pawns_MG[pawn_square];
                         //score.eg_score += candidate_passed_pawns_EG[pawn_square];
                    }
                    else {
                        //score.mg_score -= candidate_passed_pawns_MG[flip_square(pawn_square)];
                        //score.eg_score -= candidate_passed_pawns_EG[flip_square(pawn_square)];

                    }
                }
                pawns_on_file &= pawns_on_file - 1;
            }
        }
    }
    return score;
}
static EvaluationResult eval_pawn_majorities(const EvalContext& ctx) {
    EvaluationResult score = { 0,0 };
    bool island_active = false;
    int number_of_majorities = 0;
    bool muted = false;
    for (size_t file = 0; file < 8; file++) {
        uint64_t file_mask = FILE_MASK[file];
        bool white_has_pawns = (ctx.pieces[0][to_int(PieceType::PAWN)] & file_mask) != 0;
        bool black_has_pawns = (ctx.pieces[1][to_int(PieceType::PAWN)] & file_mask) != 0;
        if (island_active) {
            if (white_has_pawns && !black_has_pawns) {
                number_of_majorities++;
                island_active = false;
            }
            else if (!white_has_pawns && black_has_pawns) {
                number_of_majorities--;
                island_active = false;
            }
            else if (!white_has_pawns && !black_has_pawns) {
                island_active = false;
            }
        }
        else if (white_has_pawns && !black_has_pawns) {
            number_of_majorities++;
            muted = true;

        }
        else if (black_has_pawns && !white_has_pawns) {
            number_of_majorities--;
        }
        else if (white_has_pawns && black_has_pawns) {
            island_active = true;
        }
    }
    score.mg_score += number_of_majorities * PAWN_MAJORITY_BONUS_MG;
    score.eg_score += number_of_majorities * PAWN_MAJORITY_BONUS_EG;
    return score;
}
static EvaluationResult eval_double_pawns(const EvalContext& ctx) {
    EvaluationResult score = { 0,0 };
    int doubled_count[8] = { 0,0,0,0,0,0,0,0 };
    for (size_t file = 0; file < 8; ++file)
    {
        uint64_t file_mask = FILE_MASK[file];
        int white_doubled = popcount(ctx.pieces[0][to_int(PieceType::PAWN)] & file_mask);
        if (white_doubled > 1)
        {
            doubled_count[file] += white_doubled - 1;


        }
        int black_doubled = popcount(ctx.pieces[1][to_int(PieceType::PAWN)] & file_mask);
        if (black_doubled > 1)
        {
            doubled_count[file] -= black_doubled - 1;
        }

        score.mg_score += doubled_count[file] * DOUBLED_PAWN_PENALTY_MG[file];
        score.eg_score += doubled_count[file] * DOUBLED_PAWN_PENALTY_EG[file];
    }
    return score;
}

static EvaluationResult eval_king_safety_score(const EvalContext& ctx) {
    EvaluationResult score = { 0,0 };
    int pawn_shield_count = 0;
    int next_to_open_count = 0;
    int next_to_semi_open_count = 0;
    int next_to_open_diagonal_count[7] = { 0,0,0,0,0,0,0 };
    for (size_t color = 0; color < 2; color++) {
        if (ctx.king_sq[color] == -1) return { 0,0 };
        int ecolor = color == 0 ? 1 : 0;
        uint64_t king_square_colors = ((bit64(ctx.king_sq[color]) & LIGHT_SQUARES) != 0) ? LIGHT_SQUARES : DARK_SQUARES;
        uint64_t shield_mask = KING_SHIELD[color][ctx.king_sq[color]];
        int king_file_index = ctx.king_sq[color] % 8;

        //1. Pawn Shield Bonus
        if (!is_on_center_files(ctx.king_sq[color])) {
            int shield_pawns_count = popcount(ctx.pieces[color][to_int(PieceType::PAWN)] & shield_mask);
            pawn_shield_count += color == 0 ? shield_pawns_count : -shield_pawns_count;
        }
        // 2 Open File Penalty
        bool king_next_to_open_file = ctx.open_files & THREE_SURR_FILE_MASK[king_file_index];
        if (king_next_to_open_file) {
            next_to_open_count += color == 0 ? 1 : -1;
        }
        // 3. Semi Open File Penalty
        bool semi_open_in_shield = ctx.no_color_pawn_files[color] & THREE_SURR_FILE_MASK[king_file_index];
        if (semi_open_in_shield) {
            next_to_semi_open_count += color == 0 ? 1 : -1;
        }

        //5. Open Diagonal Penalty
        uint64_t bishop_attack_mask = get_bishop_attacks(ctx.king_sq[color], 0) & ~FILE_MASK[0] & ~FILE_MASK[1];
        uint64_t op_bishop_queen_on_mask = bishop_attack_mask & (ctx.pieces[ecolor][2] | ctx.pieces[ecolor][4]);
        while (op_bishop_queen_on_mask) {
            int sq = get_lsb(op_bishop_queen_on_mask);
            uint64_t line_between = LINE_BETWEEN[sq][ctx.king_sq[color]];
            int count = popcount(line_between & ctx.pieces[0][color]);
            if (count > 6) count = 6;
            next_to_open_diagonal_count[count] += color == 0 ? 1 : -1;

            op_bishop_queen_on_mask &= op_bishop_queen_on_mask - 1;
        }
    }
    //TODO: Scale safety score with enemy material.
    score.mg_score += pawn_shield_count * PAWN_SHIELD_BONUS_MG;
    score.eg_score += pawn_shield_count * PAWN_SHIELD_BONUS_EG;

    score.mg_score += next_to_open_count * NEXT_TO_OPEN_FILE_PENALTY_MG;
    score.eg_score += next_to_open_count * NEXT_TO_OPEN_FILE_PENALTY_EG;

    score.mg_score += next_to_semi_open_count * NEXT_TO_SEMI_OPEN_FILE_PENALTY_MG;
    score.eg_score += next_to_semi_open_count * NEXT_TO_SEMI_OPEN_FILE_PENALTY_EG;
    for (size_t count = 0; count < 7; count++) {

        score.mg_score += next_to_open_diagonal_count[count] * NEXT_TO_OPEN_DIAGONAL_PENALTY_MG[count];
        score.eg_score += next_to_open_diagonal_count[count] * NEXT_TO_OPEN_DIAGONAL_PENALTY_EG[count];

    }
    return score;

}
static EvaluationResult eval_attack_score(const EvalContext& ctx) {
    EvaluationResult score = { 0,0 };
    int pawn_attacks_in_king_zone_count = 0;
    int pawn_attacks_in_small_king_zone_count = 0;
    int king_tropism_queen_distance_count[8] = { 0, 0,0,0,0,0,0,0 };
    int king_tropism_other_distance_count[8] = { 0, 0,0,0,0,0,0,0 };
    int king_attackers_count[8] = { 0,0,0,0,0,0,0 };
    int rooks_on_open_file_next_to_king_count = 0;
    int queens_on_open_file_next_to_king_count = 0;
    int rooks_on_semi_open_file_next_to_king_count = 0;
    int queens_on_semi_open_file_next_to_king_count = 0;
    int weak_piece_attack_count[8] = { 0,0,0,0,0,0,0,0 };
    for (size_t color = 0; color < 2; color++) {
        int ecolor = color == 0 ? 1 : 0;

        int op_king_file_index = ctx.king_sq[ecolor] % 8;
        uint64_t op_king_zone = KING_ZONE[ctx.king_sq[ecolor]];
        uint64_t op_small_king_zone = SMALL_KING_ZONE[ctx.king_sq[ecolor]];

        //1. Pawn Attacks in King Zone (cheap to compute)
        uint64_t pawn_attacks = get_pawn_attacks(ctx.pieces[color][to_int(PieceType::PAWN)], static_cast<Color>(color));
        pawn_attacks_in_king_zone_count += color == 0 ? popcount(pawn_attacks & op_king_zone) : -popcount(pawn_attacks & op_king_zone);
        pawn_attacks_in_small_king_zone_count += color == 0 ? popcount(pawn_attacks & op_small_king_zone) : -popcount(pawn_attacks & op_small_king_zone);

        //2. King Tropism (distance-based)
        auto king_tropism = [&](uint64_t attackers, PieceType pt) {
            while (attackers) {
                int sq = get_lsb(attackers);
                if (pt == PieceType::QUEEN) {

                    king_tropism_queen_distance_count[king_distance(sq, ctx.king_sq[ecolor])] += color == 0 ? 1 : -1;
                }
                else {
                    king_tropism_other_distance_count[king_distance(sq, ctx.king_sq[ecolor])] += color == 0 ? 1 : -1;
                }
                attackers &= attackers - 1;
            }
            };

        king_tropism(ctx.pieces[color][to_int(PieceType::ROOK)], PieceType::ROOK);
        king_tropism(ctx.pieces[color][to_int(PieceType::BISHOP)], PieceType::BISHOP);
        king_tropism(ctx.pieces[color][to_int(PieceType::QUEEN)], PieceType::QUEEN);
        king_tropism(ctx.pieces[color][to_int(PieceType::KNIGHT)], PieceType::KNIGHT);

        //3. King Attackers (detailed attack evaluation)
        int attackers_count = 0;
        int value_count = 0;
        auto king_attackers = [&](uint64_t attackers, PieceType pt, uint64_t& all) {
            if (pt == PieceType::KNIGHT) {
                while (attackers) {
                    int sq = get_lsb(attackers);
                    int number_of_squares_attacked = popcount(KNIGHT_ATTACKS[sq] & op_small_king_zone);
                    if (number_of_squares_attacked > 0) {
                        attackers_count += 1;
                    }
                    value_count += 20 * number_of_squares_attacked;
                    attackers &= attackers - 1;

                }
            }
            else if (pt == PieceType::BISHOP) {
                while (attackers) {
                    int sq = get_lsb(attackers);
                    int number_of_squares_attacked = popcount(get_bishop_attacks(sq, all) & op_small_king_zone);
                    if (number_of_squares_attacked > 0) {
                        attackers_count += 1;
                        all &= ~(1ULL << sq);
                    }
                    value_count += 20 * number_of_squares_attacked;
                    attackers &= attackers - 1;
                }
            }
            else if (pt == PieceType::ROOK) {
                while (attackers) {
                    int sq = get_lsb(attackers);
                    int number_of_squares_attacked = popcount(get_rook_attacks(sq, all) & op_small_king_zone);
                    if (number_of_squares_attacked > 0) {
                        attackers_count += 1;
                        all &= ~(1ULL << sq);
                    }
                    value_count += 40 * number_of_squares_attacked;
                    attackers &= attackers - 1;
                }

            }
            else if (pt == PieceType::QUEEN) {
                while (attackers) {
                    int sq = get_lsb(attackers);
                    int number_of_squares_attacked = popcount(get_queen_attacks(sq, all) & op_small_king_zone);
                    if (number_of_squares_attacked > 0) {
                        attackers_count += 1;
                        all &= ~(1ULL << sq);
                    }
                    value_count += 80 * number_of_squares_attacked;
                    attackers &= attackers - 1;

                }
            }
            };
        uint64_t occ = ctx.all;

        king_attackers(ctx.pieces[color][to_int(PieceType::KNIGHT)], PieceType::KNIGHT, occ);//Order important for occ update
        king_attackers(ctx.pieces[color][to_int(PieceType::BISHOP)], PieceType::BISHOP, occ);
        king_attackers(ctx.pieces[color][to_int(PieceType::ROOK)], PieceType::ROOK, occ);
        king_attackers(ctx.pieces[color][to_int(PieceType::QUEEN)], PieceType::QUEEN, occ);
        //scale by attacker count (non-linear bonus)
        if (attackers_count > 7) attackers_count = 7;
        king_attackers_count[attackers_count] += color == 0 ? value_count : -value_count;
        //score[color] += value_count * KING_ATTACKER_VALUE_SCALE[attackers_count];

        //5. Bonus if attacker has rooks/queens on open/semi-open files near king

        int r = popcount((ctx.pieces[color][to_int(PieceType::ROOK)]) & THREE_SURR_FILE_MASK[op_king_file_index] & ctx.open_files);
        rooks_on_open_file_next_to_king_count += color == 0 ? r : -r;
        int q = popcount((ctx.pieces[color][to_int(PieceType::QUEEN)]) & THREE_SURR_FILE_MASK[op_king_file_index] & ctx.open_files);
        queens_on_open_file_next_to_king_count += color == 0 ? q : -q;

        r = popcount((ctx.pieces[color][to_int(PieceType::ROOK)]) & THREE_SURR_FILE_MASK[op_king_file_index] & ctx.no_color_pawn_files[ecolor] & ~ctx.open_files);
        rooks_on_semi_open_file_next_to_king_count += color == 0 ? r : -r;
        q = popcount((ctx.pieces[color][to_int(PieceType::QUEEN)]) & THREE_SURR_FILE_MASK[op_king_file_index] & ctx.no_color_pawn_files[ecolor] & ctx.open_files);
        queens_on_semi_open_file_next_to_king_count += color == 0 ? q : -q;

        // --- A. Identify weak/undefended pieces ---
        uint64_t defended_by_op_pawns = get_pawn_attacks(ctx.pieces[ecolor][to_int(PieceType::PAWN)], static_cast<Color>(ecolor));

        //1. Undefended pieces (not protected by pawns)
        uint64_t weak_pieces = 0;
        weak_pieces |= ctx.pieces[ecolor][to_int(PieceType::KNIGHT)];
        weak_pieces |= ctx.pieces[ecolor][to_int(PieceType::BISHOP)];
        weak_pieces |= ctx.pieces[ecolor][to_int(PieceType::ROOK)];
        weak_pieces |= ctx.pieces[ecolor][to_int(PieceType::QUEEN)];
        weak_pieces &= ~defended_by_op_pawns;
        weak_pieces |= ctx.backward_pawns[ecolor];
        weak_pieces |= ctx.isolated_pawns[ecolor];
        weak_pieces |= ctx.passed_pawns[ecolor];
        // --- B. Calculate attacks on weak pieces ---
        while (weak_pieces) {

            int value_count = 0;
            int attack_count = 0;
            uint64_t occ = ctx.all;
            int sq = get_lsb(weak_pieces);
            int new_attackers = popcount(ctx.pieces[color][to_int(PieceType::KNIGHT)] & KNIGHT_ATTACKS[sq]);
            value_count += KNIGHT_ATTACK_VALUE * new_attackers;
            attack_count += new_attackers;

            uint64_t bishop_attackers = ctx.pieces[color][to_int(PieceType::BISHOP)] & get_bishop_attacks(sq, occ);
            new_attackers = popcount(bishop_attackers);
            value_count += BISHOP_ATTACK_VALUE * new_attackers;
            attack_count += new_attackers;
            occ &= ~bishop_attackers;

            uint64_t rook_attackers = ctx.pieces[color][to_int(PieceType::ROOK)] & get_rook_attacks(sq, occ);
            new_attackers = popcount(rook_attackers);
            value_count += ROOK_ATTACK_VALUE * new_attackers;
            attack_count += new_attackers;
            occ &= ~rook_attackers;

            new_attackers = popcount(ctx.pieces[color][to_int(PieceType::QUEEN)] & get_queen_attacks(sq, occ));
            value_count += QUEEN_ATTACK_VALUE * new_attackers;
            attack_count += new_attackers;

            if (attack_count > 7) attack_count = 7;
            weak_piece_attack_count[attack_count] += color == 0 ? value_count : -value_count;
            weak_pieces &= weak_pieces - 1;
        }





    }
    score.mg_score += pawn_attacks_in_king_zone_count * PAWN_ATTACKS_IN_KING_ZONE_BONUS_MG;
    score.eg_score += pawn_attacks_in_king_zone_count * PAWN_ATTACKS_IN_KING_ZONE_BONUS_EG;

    score.mg_score += pawn_attacks_in_small_king_zone_count * PAWN_ATTACKS_IN_SMALL_KING_ZONE_BONUS_MG;
    score.eg_score += pawn_attacks_in_small_king_zone_count * PAWN_ATTACKS_IN_SMALL_KING_ZONE_BONUS_EG;
    for (int i = 0; i < 8; i++) {
        score.mg_score += king_tropism_queen_distance_count[i] * KING_TROPISM_QUEEN_BONUS_MG[i];
        score.eg_score += king_tropism_queen_distance_count[i] * KING_TROPISM_QUEEN_BONUS_EG[i];
        score.mg_score += king_tropism_other_distance_count[i] * KING_TROPISM_OTHER_BONUS_MG[i];
        score.eg_score += king_tropism_other_distance_count[i] * KING_TROPISM_OTHER_BONUS_EG[i];
    }
    for (int i = 0; i < 8; i++) {
        score.mg_score += king_attackers_count[i] * KING_ATTACKER_VALUE_SCALE_MG[i];
        score.eg_score += king_attackers_count[i] * KING_ATTACKER_VALUE_SCALE_EG[i];
        score.mg_score += weak_piece_attack_count[i] * WEAK_PIECE_ATTACK_BONUS_MG[i];
        score.eg_score += weak_piece_attack_count[i] * WEAK_PIECE_ATTACK_BONUS_EG[i];
    }
    score.mg_score += rooks_on_open_file_next_to_king_count * ROOK_ON_OPEN_FILE_NEXT_TO_KING_BONUS_MG;
    score.eg_score += rooks_on_open_file_next_to_king_count * ROOK_ON_OPEN_FILE_NEXT_TO_KING_BONUS_EG;
    score.mg_score += queens_on_open_file_next_to_king_count * QUEEN_ON_OPEN_FILE_NEXT_TO_KING_BONUS_MG;
    score.eg_score += queens_on_open_file_next_to_king_count * QUEEN_ON_OPEN_FILE_NEXT_TO_KING_BONUS_EG;
    score.mg_score += rooks_on_semi_open_file_next_to_king_count * ROOK_ON_SEMI_OPEN_FILE_NEXT_TO_KING_BONUS_MG;
    score.eg_score += rooks_on_semi_open_file_next_to_king_count * ROOK_ON_SEMI_OPEN_FILE_NEXT_TO_KING_BONUS_EG;
    score.mg_score += queens_on_semi_open_file_next_to_king_count * QUEEN_ON_SEMI_OPEN_FILE_NEXT_TO_KING_BONUS_MG;
    score.eg_score += queens_on_semi_open_file_next_to_king_count * QUEEN_ON_SEMI_OPEN_FILE_NEXT_TO_KING_BONUS_EG;

    return score;
}
// dont use this? 
static  EvaluationResult eval_defence_score(const EvalContext& ctx) {
	EvaluationResult score = { 0,0 };
	uint64_t all_attacks[2] = { 0,0 };
	int undefended_pieces_count[5] = { 0,0,0,0,0 };
	int hanging_oieces_count[5] = { 0,0,0,0,0 };    
    for (size_t color = 0; color < 2; color++) {
		int ecolor = color == 0 ? 1 : 0;
		all_attacks[color] |= get_pawn_attacks(ctx.pieces[color][to_int(PieceType::PAWN)], static_cast<Color>(color));
        all_attacks[color] |= ctx.board.get_knight_attacks_for_color(static_cast<Color>(color));
        all_attacks[color] |= ctx.board.get_bishop_attacks_for_color(static_cast<Color>(color));
        all_attacks[color] |= ctx.board.get_rook_attacks_for_color(static_cast<Color>(color));
        all_attacks[color] |= ctx.board.get_queen_attacks_for_color(static_cast<Color>(color));
        all_attacks[color] |= ctx.board.get_king_attacks_for_color(static_cast<Color>(color));
    }
    for (size_t color = 0; color < 2; color++) {
        int ecolor = color == 0 ? 1 : 0;
        uint64_t undefended_pieces = ctx.color_pieces[color] & ~all_attacks[color];
        uint64_t hanging_pieces = undefended_pieces & all_attacks[ecolor];
        for (PieceType pt : {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN }) {
            int undefended_count = popcount(undefended_pieces & ctx.pieces[color][to_int(pt)]&~hanging_pieces);
            int hanging_count = popcount(hanging_pieces & ctx.pieces[color][to_int(pt)]);
            undefended_pieces_count[to_int(pt)] += color == 0 ? undefended_count : -undefended_count;
            hanging_oieces_count[to_int(pt)] += color == 0 ? hanging_count : -hanging_count;
        }
    }
    for (int i = 0; i < 5; i++) {
        score.mg_score += undefended_pieces_count[i] * UNDEFENDED_PIECE_PENALTY_MG[i];
        score.eg_score += undefended_pieces_count[i] * UNDEFENDED_PIECE_PENALTY_EG[i];
        score.mg_score += hanging_oieces_count[i] * HANGING_PIECE_PENALTY_MG[i];
        score.eg_score += hanging_oieces_count[i] * HANGING_PIECE_PENALTY_EG[i];
	}
	return score;
}
static EvaluationResult eval_backward_pawns(EvalContext& ctx) {
    EvaluationResult score = { 0,0 };
    int blocked_backward_count = 0;
    int forwad_controlled_backward_count = 0;
    int free_to_advance_backward_count = 0;
    for (size_t color = 0; color < 2; color++) {
        int ecolor = color == 0 ? 1 : 0;
        uint64_t pawns = ctx.pieces[color][to_int(PieceType::PAWN)] & ~ctx.passed_pawns[color] & ~ctx.isolated_pawns[color];

        while (pawns)
        {
            int pawn_square = get_lsb(pawns);
            int file_index = pawn_square % 8;
            int forward_square = color == to_int(Color::WHITE) ? pawn_square + 8 : pawn_square - 8;

            if (forward_square >= 0 && forward_square < 64) {
                uint64_t forward_mask = bit64(forward_square);
                uint64_t adjacent_backwards = PAWN_ATTACKS[ecolor][pawn_square];
                bool has_adjacent_support = (ctx.pieces[color][to_int(PieceType::PAWN)] & adjacent_backwards) != 0;
                bool forward_blocked = (ctx.all & forward_mask) != 0;
                if (!has_adjacent_support) {
                    if (forward_blocked) {
                        ctx.backward_pawns[color] |= (1ULL << pawn_square);
                        blocked_backward_count += color == 0 ? 1 : -1;
                    }
					else if ((PAWN_ATTACKS[color][forward_square] & ctx.pieces[ecolor][to_int(PieceType::PAWN)])!=0)
                    {
                        ctx.backward_pawns[color] |= (1ULL << pawn_square);
                        forwad_controlled_backward_count += color == 0 ? 1 : -1;

                    }
                    else {
                        free_to_advance_backward_count += color == 0 ? 1 : -1;
                    }
                }

            }
            pawns &= pawns - 1;
        }

    }
    score.mg_score += blocked_backward_count * FORWARD_BLOCKED_BACKWARD_MG
        + forwad_controlled_backward_count * FORWARD_CONTROLLED_BACKWARD_PENALTY_MG
        + free_to_advance_backward_count * FREE_TO_ADVANCE_BACKWARD_MG;

    score.eg_score += blocked_backward_count * FORWARD_BLOCKED_BACKWARD_EG
        + forwad_controlled_backward_count * FORWARD_CONTROLLED_BACKWARD_PENALTY_EG
        + free_to_advance_backward_count * FREE_TO_ADVANCE_BACKWARD_EG;
    return score;
}
static EvaluationResult evaluate_rook_activity(const EvalContext& ctx) {
    EvaluationResult score = { 0,0 };
        int open_count = 0;
        int semi_open_count = 0;
        int rook_behind_free_pawn = 0;
        int connected_rooks = 0;
        int rooks_attacking = 0;
        open_count += popcount(ctx.pieces[0][to_int(PieceType::ROOK)] & ctx.open_files);
        open_count -= popcount(ctx.pieces[1][to_int(PieceType::ROOK)] & ctx.open_files);
        semi_open_count += popcount(ctx.pieces[0][to_int(PieceType::ROOK)] & ctx.no_color_pawn_files[0] & ~ctx.open_files);
        semi_open_count -= popcount(ctx.pieces[1][to_int(PieceType::ROOK)] & ctx.no_color_pawn_files[1] & ~ctx.open_files);
        for (size_t file = 0; file < 8; file++) {
            uint64_t file_mask = FILE_MASK[file];
            int white_passed = get_lsb(ctx.passed_pawns[0] & file_mask);
            int black_passed = get_lsb(ctx.passed_pawns[1] & file_mask);
            if (white_passed >= 0) {
                rook_behind_free_pawn += popcount(ctx.pieces[0][to_int(PieceType::ROOK)] & FORWARD_WAY_MASK[1][white_passed]);
            }
            if (black_passed >= 0) {
                rook_behind_free_pawn -= popcount(ctx.pieces[1][to_int(PieceType::ROOK)] & FORWARD_WAY_MASK[0][black_passed]);
            }
        }
        int white_rook_square = get_lsb(ctx.pieces[0][to_int(PieceType::ROOK)]);
        int black_rook_square = get_lsb(ctx.pieces[1][to_int(PieceType::ROOK)]);
        int white_connected = get_rook_attacks(white_rook_square, ctx.all) & ctx.pieces[0][to_int(PieceType::ROOK)];
        int black_connected = get_rook_attacks(black_rook_square, ctx.all) & ctx.pieces[1][to_int(PieceType::ROOK)];
        connected_rooks += popcount(white_connected) - popcount(black_connected);
        score.mg_score = open_count*ROOK_OPEN_FILE_BONUS_MG;
		score.eg_score = open_count * ROOK_OPEN_FILE_BONUS_EG;
		score.mg_score += semi_open_count * ROOK_SEMI_OPEN_FILE_BONUS_MG;
		score.eg_score += semi_open_count * ROOK_SEMI_OPEN_FILE_BONUS_EG;
		score.mg_score += rook_behind_free_pawn * ROOK_BEHIND_FREE_PAWN_BONUS_MG;
		score.eg_score += rook_behind_free_pawn * ROOK_BEHIND_FREE_PAWN_BONUS_EG;
		score.mg_score += connected_rooks * CONNECTED_ROOKS_BONUS_MG;
		score.eg_score += connected_rooks * CONNECTED_ROOKS_BONUS_EG;
        return score;
}
static EvaluationResult evaluate_bishop_pair(const EvalContext& ctx) {
    EvaluationResult score = { 0,0 };
    int count = 0;
    count += popcount(ctx.pieces[0][to_int(PieceType::BISHOP)]) >= 2 ? 1 : 0;
    count -= popcount(ctx.pieces[1][to_int(PieceType::BISHOP)]) >= 2 ? 1 : 0;
    score.mg_score += count * BISHOP_PAIR_BONUS_MG;
    score.eg_score += count * BISHOP_PAIR_BONUS_EG;
    return score;
}
static EvaluationResult evaluate_mobility(const EvalContext& ctx) {
    EvaluationResult mobility = { 0,0 };
    for (PieceType pt : {PieceType::KNIGHT, PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN}) {
        int mob_count = 0;
        for (int color = 0; color < 2; color++) {
            uint64_t pieces = ctx.pieces[color][to_int(pt)];
            while (pieces) {
                int sq = get_lsb(pieces);
                if (color == 0)
                    mob_count += popcount(get_piece_attacks(pt, sq, ctx.all) & ~ctx.color_pieces[color]);
                else
                    mob_count -= popcount(get_piece_attacks(pt, sq, ctx.all) & ~ctx.color_pieces[color]);
                pieces &= pieces - 1;
            }
        }

        mobility.mg_score += mob_count * MOBILITY_BONUS_MG[to_int(pt)];
        mobility.eg_score += mob_count * MOBILITY_BONUS_EG[to_int(pt)];
    }
    return mobility;
}
static EvaluationResult evaluate_bad_bishop(const EvalContext& ctx) {
    EvaluationResult score = { 0,0 };
    int blocked_penalty_count = 0;
    int unblocked_penalty_count = 0;
    for (int color = 0; color < 2; color++) {
        uint64_t bishops = ctx.pieces[color][to_int(PieceType::BISHOP)];

        while (bishops) {
            int sq = get_lsb(bishops);
            uint64_t bishop_color_mask = (bit64(sq) & LIGHT_SQUARES) != 0 ? LIGHT_SQUARES : DARK_SQUARES;


            // Zähle blockierte eigene Bauern auf der gleichen Farbe
            int blocked_pawns = 0;
            uint64_t pawns_to_check = ctx.pieces[color][to_int(PieceType::PAWN)] & bishop_color_mask;

            while (pawns_to_check) {
                int pawn_sq = get_lsb(pawns_to_check);
                int forward_sq = (color == 0) ? pawn_sq + 8 : pawn_sq - 8;

                // Prüfe ob Bauer blockiert ist
                if (forward_sq >= 0 && forward_sq < 64 && (ctx.all & (1ULL << forward_sq))) {
                    blocked_penalty_count += color == 0 ? 1 : -1;
                }
                else {
                    unblocked_penalty_count += color == 0 ? 1 : -1;
                }

                pawns_to_check &= pawns_to_check - 1;
            }

            bishops &= bishops - 1;
        }
    }

    score.mg_score += blocked_penalty_count * BAD_BISHOP_BLCOKED_MG;
    score.eg_score += blocked_penalty_count * BAD_BISHOP_BLOCKED_EG;
    score.mg_score += unblocked_penalty_count * BAD_BISHOP_UNBLOCKED_MG;
    score.eg_score += unblocked_penalty_count * BAD_BISHOP_UNBLOCKED_EG;

    return score;
}
static EvaluationResult evaluate_fianchetto_bishop(const EvalContext& ctx) {
    EvaluationResult score = { 0,0 };
    int intact_count = 0;
    int broken_count = 0;

    // Fianchetto-Positionen: b2, g2 (Weiß), b7, g7 (Schwarz)
    const uint64_t WHITE_FIANCHETTO = (1ULL << 9) | (1ULL << 14);  // b2, g2
    const uint64_t BLACK_FIANCHETTO = (1ULL << 49) | (1ULL << 54); // b7, g7

    for (int color = 0; color < 2; color++) {
        uint64_t fianchetto_mask = (color == 0) ? WHITE_FIANCHETTO : BLACK_FIANCHETTO;
        uint64_t bishops_fianchetto = ctx.pieces[color][to_int(PieceType::BISHOP)] & fianchetto_mask;

        while (bishops_fianchetto) {
            int sq = get_lsb(bishops_fianchetto);

            // Prüfe ob Bauernstruktur intakt ist (Bauer auf b3/g3 oder b6/g6)
            int pawn_sq = (color == 0) ? sq + 8 : sq - 8;
            bool pawn_structure_intact = (ctx.pieces[color][to_int(PieceType::PAWN)] & (1ULL << pawn_sq)) != 0;

            if (pawn_structure_intact) {
                intact_count += (color == 0) ? 1 : -1;
            }
            else {
                // Strafe wenn Fianchetto-Bauer fehlt (schwacher König)
                broken_count += (color == 0) ? 1 : -1;
            }

            bishops_fianchetto &= bishops_fianchetto - 1;
        }
    }
    score.mg_score += intact_count * FIANCHETTO_BISHOP_BONUS_MG;
    score.eg_score += intact_count * FIANCHETTO_BISHOP_BONUS_EG;
    score.mg_score += broken_count * BROKEN_FIANCHETTO_PENALTY_MG;
    score.eg_score += broken_count * BROKEN_FIANCHETTO_PENALTY_EG;

    return score;
}
static EvaluationResult evaluate_trapped_minor_pieces(const EvalContext& ctx) {
    EvaluationResult score = { 0,0 };
    int trapped_bishop_count = 0;
    int trapped_knight_count = 0;

    // Typische Fallen:
    // - Läufer auf a7/h7 (Weiß) oder a2/h2 (Schwarz) eingekesselt von Bauern
    // - Springer in der Ecke ohne Fluchtfelder

    for (int color = 0; color < 2; color++) {
        int ecolor = 1 - color;

        // Läufer-Fallen
        uint64_t bishops = ctx.pieces[color][to_int(PieceType::BISHOP)];
        while (bishops) {
            int sq = get_lsb(bishops);

            // Läufer gefangen am Brettrand
            bool is_trapped = false;

            if (color == 0) {
                // a7-Falle: Läufer auf a7, Bauer auf b6
                if (sq == 48 && (ctx.pieces[ecolor][to_int(PieceType::PAWN)] & (1ULL << 41))) is_trapped = true;
                // h7-Falle: Läufer auf h7, Bauer auf g6
                if (sq == 55 && (ctx.pieces[ecolor][to_int(PieceType::PAWN)] & (1ULL << 46))) is_trapped = true;
            }
            else {
                // a2-Falle: Läufer auf a2, Bauer auf b3
                if (sq == 8 && (ctx.pieces[ecolor][to_int(PieceType::PAWN)] & (1ULL << 17))) is_trapped = true;
                // h2-Falle: Läufer auf h2, Bauer auf g3
                if (sq == 15 && (ctx.pieces[ecolor][to_int(PieceType::PAWN)] & (1ULL << 22))) is_trapped = true;
            }

            if (is_trapped) {
                trapped_bishop_count += color == 0 ? 1 : -1;
            }

            bishops &= bishops - 1;
        }

        // Springer-Fallen (Ecken mit blockierten Fluchtfeldern)
        uint64_t knights = ctx.pieces[color][to_int(PieceType::KNIGHT)];
        while (knights) {
            int sq = get_lsb(knights);
            int file = sq % 8;
            int rank = sq / 8;

            // In Ecke und alle Fluchtfelder blockiert
            if ((file == 0 || file == 7) && (rank == 0 || rank == 7)) {
                uint64_t escape_squares = KNIGHT_ATTACKS[sq];
                int blocked_escapes = popcount(escape_squares & ctx.color_pieces[color]);

                if (blocked_escapes >= 2) {  // Meiste Fluchtfelder blockiert
                    trapped_knight_count += color == 0 ? 1 : -1;
                }
                else {
                    bool is_trapped = true;
                    uint64_t unoccupied_escapes = escape_squares & ~ctx.color_pieces[color];
                    while (unoccupied_escapes) {
                        int escape_sq = get_lsb(unoccupied_escapes);
                        PieceType pt = ctx.board.get_piece_on_square(escape_sq);
                        if (see_capture(ctx.board, sq, escape_sq, static_cast<Color>(color), PieceType::KNIGHT, pt) >= 0) {
                            is_trapped = false;
                            break;
                        }
                        unoccupied_escapes &= unoccupied_escapes - 1;
                    }
                    if (is_trapped) {
                        trapped_knight_count += color == 0 ? 1 : -1;
                    }
                }
            }

            knights &= knights - 1;
        }
    }
    score.mg_score += trapped_bishop_count * TRAPPED_BISHOP_PENALTY_MG;
    score.eg_score += trapped_bishop_count * TRAPPED_BISHOP_PENALTY_EG;
    score.mg_score += trapped_knight_count * TRAPPED_KNIGHT_PENALTY_MG;
    score.eg_score += trapped_knight_count * TRAPPED_KNIGHT_PENALTY_EG;

    return score;
}
static EvaluationResult evaluate_outpost(const EvalContext& ctx) {
    EvaluationResult score = { 0,0 };
    int bishop_outpost_count_no_op_bishop = 0;
    int bishop_outpost_count_with_op_bishop = 0;
    int knight_outpost_count_no_op_bishop = 0;
    int knight_outpost_count_with_op_bishop = 0;
    for (int color = 0; color < 2; color++) {
        int ecolor = color == 0 ? 1 : 0;
        uint64_t defended_by_own_pawn = get_pawn_attacks(ctx.pieces[color][to_int(PieceType::PAWN)], static_cast<Color>(color));
        uint64_t out_post_mask = color == 0 ? WHITE_OUTPOST_MASK : BLACK_OUTPOST_MASK;
        uint64_t possible_bishop_outposts[2];
        uint64_t possible_knight_outposts[2];
        possible_bishop_outposts[0] = defended_by_own_pawn & out_post_mask & LIGHT_SQUARES & (ctx.pieces[color][to_int(PieceType::BISHOP)]);
        possible_bishop_outposts[1] = defended_by_own_pawn & out_post_mask & DARK_SQUARES & (ctx.pieces[color][to_int(PieceType::BISHOP)]);
        possible_knight_outposts[0] = defended_by_own_pawn & out_post_mask & LIGHT_SQUARES & (ctx.pieces[color][to_int(PieceType::KNIGHT)]);
        possible_knight_outposts[1] = defended_by_own_pawn & out_post_mask & DARK_SQUARES & (ctx.pieces[color][to_int(PieceType::KNIGHT)]);
        bool light_square_bishop_exists = (ctx.pieces[ecolor][to_int(PieceType::BISHOP)] & LIGHT_SQUARES) != 0;
        bool dark_square_bishop_exists = (ctx.pieces[ecolor][to_int(PieceType::BISHOP)] & DARK_SQUARES) != 0;
        auto count_outposts = [&](uint64_t outposts, bool enemy_bishop_exists, int& counter_no_op_bishop, int& counter_with_op_bishop) {
            while (outposts) {
                int square = get_lsb(outposts);
                if ((ctx.pieces[ecolor][to_int(PieceType::PAWN)] & OUTPOST_MASK[color][square]) == 0) {
                    if (enemy_bishop_exists) {
                        counter_with_op_bishop += color == 0 ? 1 : -1;
                    }
                    else {
                        counter_no_op_bishop += color == 0 ? 1 : -1;
                    }
                }
                outposts &= outposts - 1;
            }
            };
        count_outposts(possible_bishop_outposts[0], light_square_bishop_exists, bishop_outpost_count_no_op_bishop, bishop_outpost_count_with_op_bishop);
        count_outposts(possible_bishop_outposts[1], dark_square_bishop_exists, bishop_outpost_count_no_op_bishop, bishop_outpost_count_with_op_bishop);
        count_outposts(possible_knight_outposts[0], light_square_bishop_exists, knight_outpost_count_no_op_bishop, knight_outpost_count_with_op_bishop);
        count_outposts(possible_knight_outposts[1], dark_square_bishop_exists, knight_outpost_count_no_op_bishop, knight_outpost_count_with_op_bishop);

    }
    score.mg_score += bishop_outpost_count_no_op_bishop * BISHOP_OUTPOST_BONUS_NO_OPPOSITE_BISHOP_MG;
    score.eg_score += bishop_outpost_count_no_op_bishop * BISHOP_OUTPOST_BONUS_NO_OPPOSITE_BISHOP_EG;
    score.mg_score += bishop_outpost_count_with_op_bishop * BISHOP_OUTPOST_BONUS_WITH_OPPOSITE_BISHOP_MG;
    score.eg_score += bishop_outpost_count_with_op_bishop * BISHOP_OUTPOST_BONUS_WITH_OPPOSITE_BISHOP_MG;
    score.mg_score += knight_outpost_count_no_op_bishop * KNIGHT_OUTPOST_BONUS_NO_OPPOSITE_BISHOP_MG;
    score.eg_score += knight_outpost_count_no_op_bishop * KNIGHT_OUTPOST_BONUS_NO_OPPOSITE_BISHOP_EG;
    score.mg_score += knight_outpost_count_with_op_bishop * KNIGHT_OUTPOST_BONUS_WITH_OPPOSITE_BISHOP_MG;
    score.eg_score += knight_outpost_count_with_op_bishop * KNIGHT_OUTPOST_BONUS_WITH_OPPOSITE_BISHOP_EG;

    return score;
}
static EvaluationResult evaluate_pawns(EvalContext& ctx) {
    EvaluationResult score = { 0,0 };
    uint64_t pawn_key = ctx.board.get_pawn_key();
    int idx = pawn_key & (PAWN_HASH_SIZE - 1);
    PawnEvalEntry& entry = pawn_evaluation_table[idx];
    if (!(entry.valid && entry.key == pawn_key)) {
        entry = compute_pawn_eval_entry(ctx);
    }
    else {
        ctx.isolated_pawns[0] = entry.isolated_pawns[0];
        ctx.isolated_pawns[1] = entry.isolated_pawns[1];
        ctx.passed_pawns[0] = entry.passed_pawns[0];
        ctx.passed_pawns[1] = entry.passed_pawns[1];
		ctx.backward_pawns[0] = entry.backward_pawns[0];
		ctx.backward_pawns[1] = entry.backward_pawns[1];
    }

	score += entry.isolated_passed_score + entry.backward_score + entry.doubled_score;
	return score;
}
static int tapered(EvaluationResult score, int game_phase) {
    return (score.mg_score * game_phase + score.eg_score * (24 - game_phase)) / 24;
}
int evaluate(const Board& board, uint8_t terms_mask) {
    EvaluationResult score = { 0,0 };
    struct EvalContext ctx(board);

	score += evaluate_material(ctx);
    score += evaluate_positional(board);
    score += evaluate_pawns(ctx);
    if (terms_mask != EvalAll) return tapered(score,ctx.game_phase);
	score += eval_king_safety_score(ctx);
	score += evaluate_mobility(ctx);
	score += evaluate_rook_activity(ctx);
	score+=evaluate_bishop_pair(ctx);
	score+=evaluate_bad_bishop(ctx);
	score+=evaluate_fianchetto_bishop(ctx);
	score+=evaluate_trapped_minor_pieces(ctx);
	score+=evaluate_outpost(ctx);

	return tapered(score, ctx.game_phase);
}

static PawnEvalEntry compute_pawn_eval_entry(EvalContext& ctx) {
    PawnEvalEntry entry{};
    entry.key = ctx.board.get_pawn_key();
    entry.valid = true;

    entry.isolated_passed_score = eval_iso_passed_pawns(ctx);
    entry.doubled_score = eval_double_pawns(ctx);
	entry.backward_score = eval_backward_pawns(ctx);
        
    entry.isolated_pawns[0] = ctx.isolated_pawns[0];
    entry.isolated_pawns[1] = ctx.isolated_pawns[1];
    entry.passed_pawns[0] = ctx.passed_pawns[0];
    entry.passed_pawns[1] = ctx.passed_pawns[1];
	entry.backward_pawns[0] = ctx.backward_pawns[0];
	entry.backward_pawns[1] = ctx.backward_pawns[1];

    return entry;
}
