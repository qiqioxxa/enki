#pragma once

#include "types.h"
#include <algorithm>
#include <bit>
#include <climits>
#include <vector>


struct TTentry {
    enum TTflag : uint8_t { UPPERBOUND, LOWERBOUND, EXACT };
    uint64_t key;
    Move best_move;
    int16_t score;
    uint8_t depth;
    TTflag flag : 2;
    uint8_t generation : 6;
};

struct alignas(64) Bucket {
    std::array<TTentry, 4> entries;
};

class TranspositionTable {
    std::vector<Bucket> table;
    uint32_t mask;

public:
    TranspositionTable(int size_mb = 16) {
        resize(size_mb);
    }

    int hashfull(uint8_t generation) const {
        int curr_gen_entries = 0;
        int total = 0;
        for (int i = 0; i < std::min(10000, static_cast<int>(table.size())); i++) {
            for (const TTentry& entry : table[i].entries) {
                if (entry.generation == generation) curr_gen_entries++;
                total++;
            }
        }
        return curr_gen_entries * 1000 / total;
    }

    void clear() { std::fill(table.begin(), table.end(), Bucket{}); }

    void resize(int size_mb) {
        uint32_t target_buckets = (static_cast<uint64_t>(size_mb) * 1024 * 1024) / sizeof(Bucket);

        table.resize(std::bit_floor(target_buckets));
        table.shrink_to_fit();
        mask = table.size() - 1;
        clear();
    }

    const TTentry* probe(uint64_t key) const {
        const Bucket& bucket = table[key & mask];
        for (const TTentry& entry : bucket.entries) {
            if (entry.key == key) return &entry;
        }
        return nullptr;
    }

    void record(uint64_t key, Move move, int16_t score, uint8_t depth, TTentry::TTflag flag, uint8_t generation) {
        Bucket& bucket = table[key & mask];

        // cache hit: overwriting
        for (TTentry& entry : bucket.entries) {
            if (key == entry.key) {
                entry.best_move = move;
                entry.score = score;
                entry.depth = depth;
                entry.flag = flag;
                entry.generation = generation;
                return;
            }
        }

        // cache miss: selecting least valuable entry to overwrite
        TTentry* candidate = nullptr;
        int max_age = 1;
        for (TTentry& entry : bucket.entries) {
            if (entry.key == 0) {
                candidate = &entry;
                break;
            } else {
                int age = generation - entry.generation;
                if (age > max_age) {
                    max_age = age;
                    candidate = &entry;
                }
            }
        }

        if (candidate == nullptr) {
            int min_depth = INT_MAX;
            for (TTentry& entry : bucket.entries) {
                int effective_depth = entry.depth + 5 * (entry.flag == TTentry::EXACT) + 2 * (entry.flag == TTentry::LOWERBOUND);
                if (effective_depth < min_depth) {
                    min_depth = effective_depth;
                    candidate = &entry;
                }
            }
        }

        candidate->key = key;
        candidate->best_move = move;
        candidate->score = score;
        candidate->depth = depth;
        candidate->flag = flag;
        candidate->generation = generation;
    }
};
