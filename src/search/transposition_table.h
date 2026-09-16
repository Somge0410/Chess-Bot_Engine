#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "Move.h"
#include "search_parameters.h"

#ifndef DEFAULT_TT_MB
#define DEFAULT_TT_MB 128
#endif

inline constexpr size_t MAX_MEMORY_TT_MB = DEFAULT_TT_MB;

enum TTFlag { EXACT, LOWERBOUND, UPPERBOUND, TEMPERED };

enum class TTMode { Negamax, Quiescence, PrincipalVariation };

struct TTEntry {
    alignas(8) uint64_t entry;
    static constexpr uint64_t GENERATION_MASK = 0x3Full << 26;

    TTEntry() : entry(uint64_t(0xFF) << 16) {}
    explicit TTEntry(uint64_t raw_entry) : entry(raw_entry) {}
    TTEntry(int16_t score, uint8_t depth, TTFlag bound, uint8_t generation,
        const Move& move, uint16_t key)
        : entry((uint64_t(score) & 0xFFFFull)
            | ((uint64_t(depth) & 0xFFull) << 16)
            | ((uint64_t(bound) & 0x3ull) << 24)
            | ((uint64_t(generation) & 0x3Full) << 26)
            | ((uint64_t(move.get_int()) & 0xFFFFull) << 32)
            | ((uint64_t(key) & 0xFFFFull) << 48)) {}

    bool empty() const { return ((entry >> 16) & 0xFFull) == 0xFFull; }
    int16_t score() const { return static_cast<int16_t>(entry & 0xFFFFull); }
    uint16_t depth() const { return static_cast<uint8_t>((entry >> 16) & 0xFFull); }
    TTFlag flag() const { return static_cast<TTFlag>((entry >> 24) & 0x3ull); }
    uint8_t generation() const { return static_cast<uint8_t>((entry >> 26) & 0x3Full); }
    uint64_t with_generation(uint8_t new_generation) const {
        return (entry & ~GENERATION_MASK) | ((uint64_t(new_generation) & 0x3Full) << 26);
    }
    uint16_t move_packed() const { return static_cast<uint16_t>((entry >> 32) & 0xFFFFull); }
    Move move() const {
        const uint16_t packed = move_packed();
        Move result;
        result.from_square = static_cast<Square>(packed & 0x3F);
        result.to_square = static_cast<Square>((packed >> 6) & 0x3F);
        result.promotion_piece = static_cast<PieceType>((packed >> 12) & 0x0F);
        return result;
    }
    uint16_t key() const { return static_cast<uint16_t>((entry >> 48) & 0xFFFFull); }
};

struct alignas(32) TTCluster { TTEntry entries[4]; };
static_assert(sizeof(TTCluster) == 32);
static_assert(alignof(TTCluster) == 32);

constexpr int score_to_tt(int score, int ply) {
    if (score >= MATE_THRESHOLD) return score + ply;
    if (score <= -MATE_THRESHOLD) return score - ply;
    return score;
}
constexpr int score_from_tt(int score, int ply) {
    if (score >= MATE_THRESHOLD) return score - ply;
    if (score <= -MATE_THRESHOLD) return score + ply;
    return score;
}
constexpr uint8_t generation_age(uint8_t entry_generation, uint8_t current_generation) {
    return static_cast<uint8_t>((current_generation - entry_generation) & 63);
}

static_assert(score_to_tt(MATE_SCORE - 7, 7) == MATE_SCORE);
static_assert(score_from_tt(MATE_SCORE, 3) == MATE_SCORE - 3);
static_assert(score_to_tt(-MATE_SCORE + 7, 7) == -MATE_SCORE);
static_assert(score_from_tt(-MATE_SCORE, 3) == -MATE_SCORE + 3);
static_assert(generation_age(63, 0) == 1);
static_assert(generation_age(0, 1) == 1);
static_assert(generation_age(32, 1) == 33);
static_assert(generation_age(0, 63) == 63);

inline uint64_t tt_load(TTEntry& entry) {
    return std::atomic_ref<uint64_t>(entry.entry).load(std::memory_order_relaxed);
}
inline void tt_store(TTEntry& entry, uint64_t value) {
    std::atomic_ref<uint64_t>(entry.entry).store(value, std::memory_order_relaxed);
}
inline void tt_refresh_generation(TTEntry& entry, uint64_t observed, uint8_t generation) {
    TTEntry current(observed);
    const uint64_t refreshed = current.with_generation(generation);
    if (refreshed == observed) return;
    std::atomic_ref<uint64_t>(entry.entry).compare_exchange_strong(
        observed, refreshed, std::memory_order_relaxed, std::memory_order_relaxed);
}

struct TTOccupancy {
    uint64_t capacity_entries = 0;
    uint64_t occupied_entries = 0;
    uint64_t current_generation_entries = 0;
    std::array<uint64_t, 5> cluster_occupancy{};
};

class TranspositionTable {
public:
    explicit TranspositionTable(size_t size_mb = MAX_MEMORY_TT_MB);
    void resize(size_t size_mb);
    void new_search();
    uint8_t generation() const { return generation_; }
    bool probe(uint64_t hash, int depth, int alpha, int beta, int& out_score,
        Move& out_move, int ply, bool is_depth_0 = false, TTMode mode = TTMode::Negamax);
    bool store(uint64_t hash, int depth, int original_alpha, int beta, int best_score,
        Move& best_move, int ply, bool is_best_tempered, bool is_any_tempered = false,
        TTMode mode = TTMode::Negamax);
    TTOccupancy occupancy() const;

private:
    std::vector<TTCluster> clusters_;
    uint8_t generation_ = 0;
};
