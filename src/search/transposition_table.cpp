#include "transposition_table.h"

#include <algorithm>
#include <bit>

#include "search_diagnostics.h"
#include "search.h"

TranspositionTable::TranspositionTable(size_t size_mb) {
    resize(size_mb);
}

void TranspositionTable::new_search() {
    generation_ = static_cast<uint8_t>((generation_ + 1) & 0x3F);
}

bool TranspositionTable::probe(uint64_t hash, int depth, int alpha, int beta, int& out_score,
    Move& out_move, int ply, bool depth_0, TTMode mode) {
#if ENABLE_QSEARCH_DIAGNOSTICS
    TTDiagnostics* diagnostics = active_tt_diagnostics(mode);
    const int diagnostic_alpha = alpha;
    const int diagnostic_beta = beta;
    tls_data.last_tt_probe_was_shallow = false;
    if (diagnostics) {
        diagnostics->probes++;
        record_tt_probe_categories(diagnostics, depth, diagnostic_alpha, diagnostic_beta,
            tls_data.current_tt_probe_in_check, TTProbeDiagnosticEvent::Probe);
    }
#endif
    TTCluster& cluster = clusters_[hash & (clusters_.size() - 1)];
    const uint16_t key16 = static_cast<uint16_t>(hash >> 48);
    bool hits = false;
    for (int i = 0; i < 4; i++) {
#if ENABLE_QSEARCH_DIAGNOSTICS
        if (diagnostics) {
            diagnostics->slots_examined++;
        }
#endif
        TTEntry& slot = cluster.entries[i];
        uint64_t w = tt_load(slot);
        TTEntry entry;
        entry.entry = w;
		if (entry.empty()) {
#if ENABLE_QSEARCH_DIAGNOSTICS
            if (diagnostics) {
                diagnostics->empty_terminations++;
            }
#endif
            return false;
        }
		if (entry.key() != key16) continue;
#if ENABLE_QSEARCH_DIAGNOSTICS
        if (diagnostics) {
            diagnostics->key_hits++;
            record_tt_probe_categories(diagnostics, depth, diagnostic_alpha, diagnostic_beta,
                tls_data.current_tt_probe_in_check, TTProbeDiagnosticEvent::KeyHit);
        }
#endif
		tt_refresh_generation(slot, w, generation_);

        out_move = entry.move();
        const int score = score_from_tt(entry.score(), ply);
        out_score = score;
        if (entry.depth() < depth) {
#if ENABLE_QSEARCH_DIAGNOSTICS
            if (diagnostics) {
                diagnostics->shallow_hits++;
            }
            tls_data.last_tt_probe_was_shallow = true;
#endif
            return false;
        }

        if (entry.flag() == TEMPERED) {
#if ENABLE_QSEARCH_DIAGNOSTICS
            if (diagnostics) {
                diagnostics->tempered_rejections++;
            }
#endif
            return false;
        }
        int a = alpha, b = beta;
        if (entry.flag() == EXACT) {
            if (out_move.from_square == NO_SQUARE) {
#if ENABLE_QSEARCH_DIAGNOSTICS
                if (diagnostics) {
                    diagnostics->invalid_move_rejections++;
                }
#endif
                return false;
            }
#if ENABLE_QSEARCH_DIAGNOSTICS
            if (diagnostics) {
                diagnostics->exact_hits++;
                record_tt_probe_categories(diagnostics, depth, diagnostic_alpha, diagnostic_beta,
                    tls_data.current_tt_probe_in_check, TTProbeDiagnosticEvent::UsableHit);
            }
#endif
            return true;
		}
#if ENABLE_QSEARCH_DIAGNOSTICS
        if (diagnostics) {
            diagnostics->bound_hits++;
        }
#endif
        if (entry.flag() == LOWERBOUND) a = std::max(a, score);
        if (entry.flag() == UPPERBOUND) b = std::min(b, score);
        
        if (beta - alpha > 1) {
            alpha = a;
            beta = b;
            hits = true;
        }
        if (a >= b) {
            if (out_move.from_square == NO_SQUARE) {
#if ENABLE_QSEARCH_DIAGNOSTICS
                if (diagnostics) {
                    diagnostics->invalid_move_rejections++;
                }
#endif
                return false;
            }
#if ENABLE_QSEARCH_DIAGNOSTICS
            if (diagnostics) {
                diagnostics->bound_cutoffs++;
                record_tt_probe_categories(diagnostics, depth, diagnostic_alpha, diagnostic_beta,
                    tls_data.current_tt_probe_in_check, TTProbeDiagnosticEvent::UsableHit);
                record_tt_probe_categories(diagnostics, depth, diagnostic_alpha, diagnostic_beta,
                    tls_data.current_tt_probe_in_check, TTProbeDiagnosticEvent::Cutoff);
            }
#endif
            return true;
        }
    }
    return false;

}

bool TranspositionTable::store(uint64_t hash, int depth, int original_alpha, int beta, int best_score,
    Move& best_move, int ply, bool is_best_tempered, bool is_any_tempered, TTMode mode) {
#if ENABLE_QSEARCH_DIAGNOSTICS
    TTDiagnostics* diagnostics = active_tt_diagnostics(mode);
    if (diagnostics) {
        diagnostics->stores++;
    }
#endif
    bool score_tempered=false;
    TTFlag flag_to_store;
    // Do some position from repeat logic here

    if (is_best_tempered) {
        flag_to_store = TEMPERED;
        score_tempered = true;
    }
    else if (is_any_tempered) {
        if (best_score >= beta) {
            flag_to_store = LOWERBOUND;
        }
        else if (best_score >= original_alpha) {
            flag_to_store = LOWERBOUND;
        }
        else {
            flag_to_store = TEMPERED;
            // Dont set score_tempered to true because the best move is clean, it might not actually be that best move but we can guarantee at least the result.
        }
    }
    else {
        if (best_score >= beta)
        {
            flag_to_store = LOWERBOUND;
        }
        else if (best_score <= original_alpha)
        {
            flag_to_store = UPPERBOUND;
        }
        else
        {
            flag_to_store = EXACT;
        }
    }

#if ENABLE_QSEARCH_DIAGNOSTICS
    if (diagnostics) {
        switch (flag_to_store) {
        case EXACT:
            diagnostics->exact_stores++;
            break;
        case LOWERBOUND:
            diagnostics->lowerbound_stores++;
            break;
        case UPPERBOUND:
            diagnostics->upperbound_stores++;
            break;
        case TEMPERED:
            diagnostics->tempered_stores++;
            break;
        }
    }
#endif

    TTEntry new_entry = TTEntry(score_to_tt(best_score, ply), depth, flag_to_store,
        generation_, best_move, static_cast<uint16_t>(hash >> 48));
	TTCluster& cluster = clusters_[hash & (clusters_.size() - 1)];

    uint16_t key16 = static_cast<uint16_t>(hash >> 48);
    for(int i=0;i<4;i++){
        // If key already exists in cluster, update that slot.
		uint64_t oldw = tt_load(cluster.entries[i]);
        TTEntry old; old.entry = oldw;
        if (!old.empty() && old.key() == key16) {
            if (old.depth() <= depth) {
                tt_store(cluster.entries[i],new_entry.entry);
#if ENABLE_QSEARCH_DIAGNOSTICS
                if (diagnostics) {
                    diagnostics->same_key_updates++;
                }
#endif
            }
            else {
                // Preserve the deeper result but mark it as used by this search.
                tt_refresh_generation(cluster.entries[i], oldw, generation_);
#if ENABLE_QSEARCH_DIAGNOSTICS
                if (diagnostics) {
                    diagnostics->deeper_entries_kept++;
                }
#endif
            }

            
            return score_tempered;
        }
	}
    for (int i = 0; i < 4; i++) {
        // Find an empty slot to store the new entry.

        uint64_t oldw = tt_load(cluster.entries[i]);
        TTEntry old; old.entry = oldw;
        if (old.empty()) {
            tt_store(cluster.entries[i],new_entry.entry);
#if ENABLE_QSEARCH_DIAGNOSTICS
            if (diagnostics) {
                diagnostics->empty_inserts++;
            }
#endif
			return score_tempered;
        }
    }

	// If no empty slot, replace the least recently used (last) entry

    int pos_index = -1;
    uint16_t max_generation_diff = 0;
    int pos_depth = depth;
    for (size_t i = 0; i < 4; ++i) {
		TTEntry e; e.entry = tt_load(cluster.entries[i]);
		uint8_t curr_gen_diff = generation_age(e.generation(), generation_);
        if (curr_gen_diff>max_generation_diff) {
            pos_index = i;
            max_generation_diff = curr_gen_diff;
            pos_depth = e.depth();
        }
        else if (max_generation_diff==curr_gen_diff && e.depth() <= pos_depth) {
            pos_index = i;
            pos_depth = e.depth();
        }
    }
    if (pos_index != -1) {
        tt_store(cluster.entries[pos_index],new_entry.entry);
#if ENABLE_QSEARCH_DIAGNOSTICS
        if (diagnostics) {
            diagnostics->replacements++;
            diagnostics->replaced_depth_sum += static_cast<uint64_t>(pos_depth);
            diagnostics->replacement_depth_sum += static_cast<uint64_t>(depth);
            diagnostics->replaced_age_sum += static_cast<uint64_t>(max_generation_diff);
        }
#endif
    }
#if ENABLE_QSEARCH_DIAGNOSTICS
    else if (diagnostics) {
        diagnostics->dropped_stores++;
    }
#endif
    return score_tempered;
   

}

void TranspositionTable::resize(size_t tt_size_mb) {
    size_t bytes = tt_size_mb * 1024ull * 1024ull;
    size_t clusters = bytes / sizeof(TTCluster);
    if (clusters == 0) clusters = 1;
    clusters = std::bit_floor(clusters);
    clusters_.clear();
    clusters_.resize(clusters);
}

TTOccupancy TranspositionTable::occupancy() const {
    TTOccupancy result{};
    result.capacity_entries = static_cast<uint64_t>(clusters_.size()) * 4ULL;
    for (const TTCluster& cluster : clusters_) {
        std::size_t cluster_size = 0;
        for (const TTEntry& slot : cluster.entries) {
            TTEntry entry(std::atomic_ref<const uint64_t>(slot.entry).load(std::memory_order_relaxed));
            if (entry.empty()) continue;
            ++result.occupied_entries;
            ++cluster_size;
            if (entry.generation() == generation_) {
                ++result.current_generation_entries;
            }
        }
        ++result.cluster_occupancy[cluster_size];
    }
    return result;
}
