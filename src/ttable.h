#pragma once

#include "types.h"
#include <vector>
#include <algorithm>


struct TTentry {
    enum TTflag : uint8_t { EXACT, LOWERBOUND, UPPERBOUND };
    uint64_t key;
    Move best_move;
    int16_t score;
    uint8_t depth;
    TTflag flag;
};

class TranspositionTable {
    std::vector<TTentry> table;
    uint32_t mask;

public:
    TranspositionTable(int size_mb = 16) {
        resize(size_mb);
    }

    int hashfull() const {
        int count = 0;
        for (int i = 0; i < std::min(10000, (int)table.size()); i++) {
            if (table[i].key != 0) count++;
        }
        return count / 10;
    }

    void clear() { std::fill(table.begin(), table.end(), TTentry{}); }

    void resize(int size_mb) {
        int raw_entries = (size_mb * 1024 * 1024) / sizeof(TTentry);
        int entries = 1;
        while ((entries << 1) <= raw_entries) entries <<= 1;

        table.resize(entries);
        table.shrink_to_fit();
        mask = entries - 1;
        clear();
    }

    TTentry* probe(uint64_t key) {
        TTentry* entry = &table[key & mask];
        return (entry->key == key) ? entry : nullptr;
    }

    void record(uint64_t key, Move move, uint16_t score, uint8_t depth, TTentry::TTflag flag) {
        TTentry* entry = &table[key & mask];
        if (depth >= entry->depth) {
            entry->key = key;
            entry->best_move = move;
            entry->score = score;
            entry->depth = depth;
            entry->flag = flag;
        }
    }
};
