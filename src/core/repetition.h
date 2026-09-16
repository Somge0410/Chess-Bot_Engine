#pragma once
#include <array>
#include <stdexcept>

struct RepetitionTracker {
    static constexpr int CAP = 600;
    std::array<uint64_t, CAP> keys{};
    std::array<uint8_t, CAP> vals{};
    uint16_t current_start = 0;
    uint16_t current_end = 0;
    uint8_t current_twofold_positions = 0;

    void clear() {
        current_start = 0;
        current_end = 0;
        current_twofold_positions = 0;
    }
    void reset_boundaries(int new_start, int new_current_twofold) {
        current_end = current_start;
        current_start = new_start;
        current_twofold_positions = new_current_twofold;
    }
    uint8_t count(uint64_t h) const {
        for (int i = current_start; i != current_end; i++) {
            if (keys[i] == h) {
                return vals[i];
            }
        }
        return 0;
    }
    bool has_any_twofold() const {
        return current_twofold_positions > 0;
    }
    bool has_any_threefold() const {
        for (int i = current_start; i != current_end; i++) {
            if (vals[i] >= 3) return true;
        }
        return false;
    }
    void push(uint64_t h) {
        // find existing
        for (int i = current_start; i != current_end; ++i) {
            if (keys[i] == h) {
                if (vals[i] == 1) current_twofold_positions++;
                vals[i]++;
                return;
            }
        }
        keys[current_end] = h;
        vals[current_end] = 1;
        current_end++;
    }
    void pop(uint64_t h) {
        for (int i = current_start; i != current_end; i++) {
            if (keys[i] == h) {
                if (vals[i] == 2) current_twofold_positions--;
                if (--vals[i] == 0) {
                    // Remove entry by swapping with last
                    keys[i] = keys[current_end - 1];
                    vals[i] = vals[current_end - 1];
                    current_end--;
                }
                return;
            }
        }
        throw std::runtime_error("Hash not found in repetition tracker during pop");
    }
    void recover_from_old(uint64_t h, int old_start, int old_twofold) {
        pop(h);
        current_start = old_start;
        current_twofold_positions = old_twofold;

    }
    uint16_t get_start() const {
        return current_start;
    }
    uint16_t get_end() const {
        return current_end;
    }
    uint8_t get_twofold() const {
        return current_twofold_positions;
    }
    void reset(uint64_t h) {
        current_start = current_end;
        current_twofold_positions = 0;
        push(h);
    }
};
