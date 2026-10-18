#include "pawn_table.h"

#include <algorithm>
#include <limits>

#include "search_parameters.h"

namespace {
    std::size_t entry_count(int size_log2) {
        static_assert(PAWN_TABLE_MIN_SIZE_LOG2 >= 0
            && PAWN_TABLE_MIN_SIZE_LOG2 <= PAWN_TABLE_MAX_SIZE_LOG2
            && PAWN_TABLE_MAX_SIZE_LOG2 < std::numeric_limits<std::size_t>::digits);
        const int exponent = std::clamp(size_log2,
            PAWN_TABLE_MIN_SIZE_LOG2, PAWN_TABLE_MAX_SIZE_LOG2);
        return std::size_t{1} << exponent;
    }
}

PawnTable::PawnTable() : PawnTable(PAWN_TABLE_SIZE_LOG2) {}

PawnTable::PawnTable(int size_log2)
    : table_(entry_count(size_log2)), index_mask_(table_.size() - 1) {}

void PawnTable::resize(int size_log2) {
    const std::size_t new_size = entry_count(size_log2);
    if (new_size == table_.size()) return;

    // Replacing the vector also releases excess capacity when shrinking.
    std::vector<PawnEntry>(new_size).swap(table_);
    index_mask_ = new_size - 1;
#if ENABLE_QSEARCH_DIAGNOSTICS
    diagnostics_ = {};
    occupied_entries_ = 0;
#endif
}
void PawnTable::clear() {
	for (auto& entry : table_) {
		entry.valid = false;
	}
#if ENABLE_QSEARCH_DIAGNOSTICS
	diagnostics_ = {};
	occupied_entries_ = 0;
#endif
}
const PawnEntry* PawnTable::probe(uint64_t key) const {
    const std::size_t index = key & index_mask_;
    const PawnEntry& entry = table_[index];

#if ENABLE_QSEARCH_DIAGNOSTICS
    ++diagnostics_.probes;
#endif
    if (entry.valid && entry.key == key) {
#if ENABLE_QSEARCH_DIAGNOSTICS
        ++diagnostics_.hits;
#endif
        return &entry;
    }

#if ENABLE_QSEARCH_DIAGNOSTICS
    ++diagnostics_.misses;
#endif
    return nullptr;
}
void PawnTable::store(uint64_t key, const PawnData& data) {
    const std::size_t index = key & index_mask_;
    PawnEntry& entry = table_[index];
#if ENABLE_QSEARCH_DIAGNOSTICS
    ++diagnostics_.stores;
    if (!entry.valid) {
        ++diagnostics_.empty_inserts;
        ++occupied_entries_;
    }
    else if (entry.key == key) {
        ++diagnostics_.same_key_updates;
    }
    else {
        ++diagnostics_.replacements;
    }
#endif
    entry.key = key;
    entry.data = data;
    entry.valid = true;
}

#if ENABLE_QSEARCH_DIAGNOSTICS
void PawnTableDiagnostics::add(const PawnTableDiagnostics& other, bool include_occupancy) {
    probes += other.probes;
    hits += other.hits;
    misses += other.misses;
    stores += other.stores;
    empty_inserts += other.empty_inserts;
    same_key_updates += other.same_key_updates;
    replacements += other.replacements;
    if (include_occupancy) {
        occupied_entries += other.occupied_entries;
        capacity_entries += other.capacity_entries;
    }
}

void PawnTable::reset_diagnostics() {
    diagnostics_ = {};
}

PawnTableDiagnostics PawnTable::diagnostics() const {
    PawnTableDiagnostics result = diagnostics_;
    result.occupied_entries = occupied_entries_;
    result.capacity_entries = table_.size();
    return result;
}
#endif
