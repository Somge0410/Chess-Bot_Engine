#pragma once
#include "types.h"
#include <cstddef>
#include <vector>

#ifndef ENABLE_QSEARCH_DIAGNOSTICS
#define ENABLE_QSEARCH_DIAGNOSTICS 0
#endif

#if ENABLE_QSEARCH_DIAGNOSTICS
struct PawnTableDiagnostics {
	uint64_t probes{};
	uint64_t hits{};
	uint64_t misses{};
	uint64_t stores{};
	uint64_t empty_inserts{};
	uint64_t same_key_updates{};
	uint64_t replacements{};
	uint64_t occupied_entries{};
	uint64_t capacity_entries{};

	void add(const PawnTableDiagnostics& other, bool include_occupancy = true);
};
#endif

struct PawnData {
	std::array<Bitboard, 2> passed_pawns{};
	std::array<Bitboard, 2> isolated{};
	EvaluationResult structure_score{};
	std::array<uint8_t, 2> pawnless_files{};
};

struct PawnEntry {
	uint64_t key{};
	PawnData data{};
	bool valid{};
};

class PawnTable {
public:
	PawnTable();
	explicit PawnTable(int size_log2);
	// Call only between searches. An unchanged size preserves cached entries.
	void resize(int size_log2);
	const PawnEntry* probe(uint64_t key) const;
	void store(uint64_t key, const PawnData& data);
	void clear();
	std::size_t capacity() const noexcept { return table_.size(); }
#if ENABLE_QSEARCH_DIAGNOSTICS
	// Reset search events while keeping the cached positions and occupancy.
	void reset_diagnostics();
	PawnTableDiagnostics diagnostics() const;
#endif
private:
	std::vector<PawnEntry> table_;
	std::size_t index_mask_;
#if ENABLE_QSEARCH_DIAGNOSTICS
	mutable PawnTableDiagnostics diagnostics_{};
	uint64_t occupied_entries_{};
#endif
};
