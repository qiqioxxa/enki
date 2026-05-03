#pragma once

#include <cstdint>
#include <random>


namespace Zobrist {
    inline uint64_t pieces[12][64];
    inline uint64_t castle[16];
    inline uint64_t en_passant[17];
    inline uint64_t turn;

    inline void init() {
        std::mt19937_64 gen(0x123456789ABCDEF);
        std::uniform_int_distribution<uint64_t> uid;

        for (int i = 0; i < 12; i++) {
            for (int j = 0; j < 64; j++) {
                pieces[i][j] = uid(gen);
            }
        }
        for (int i = 0; i < 16; i++) {
            castle[i] = uid(gen);
        }
        for (int i = 0; i < 17; i++) {
            en_passant[i] = uid(gen);
        }
        turn = uid(gen);
    }
}
