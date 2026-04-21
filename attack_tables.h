#pragma once

#include <array>
#include <cmath>
#include <cstdint>


class AttackTables {
public:
    static void init();

    static inline uint64_t bishop_attacks(int square, uint64_t occupancy) {
        uint64_t mask = bishop_masks[square];
        uint64_t blockers = occupancy & mask;
        uint64_t index = (blockers * bishop_magics[square]) >> (64 - bishop_shifts[square]);
        return bishop_attacks_[square][index];
    }
    static inline uint64_t rook_attacks(int square, uint64_t occupancy) {
        uint64_t mask = rook_masks[square];
        uint64_t blockers = occupancy & mask;
        uint64_t index = (blockers * rook_magics[square]) >> (64 - rook_shifts[square]);
        return rook_attacks_[square][index];
    }
    static inline uint64_t pawn_pushes(int square, bool white) {
        return pawn_pushes_[!white][square];
    }
    static inline uint64_t pawn_attacks(int square, bool white) {
        return pawn_attacks_[!white][square];
    }
    static inline uint64_t knight_attacks(int square) {
        return knight_attacks_[square];
    }
    static inline uint64_t king_attacks(int square) {
        return king_attacks_[square];
    }


    static inline bool initialized = false;

    static constexpr inline std::array<std::array<uint64_t, 64>, 64> line_bb = []() {
        std::array<std::array<uint64_t, 64>, 64> lines{};
        for (int s1 = 0; s1 < 64; s1++) {
            for (int s2 = s1 + 1; s2 < 64; s2++) {
                int r1 = s1 / 8, f1 = s1 % 8;
                int r2 = s2 / 8, f2 = s2 % 8;
                int dr = r2 - r1, df = f2 - f1;

                if (dr != 0 && df != 0 && dr != df && dr != -df) continue;

                int r_step = (dr == 0) ? 0 : (dr > 0 ? 1 : -1);
                int f_step = (df == 0) ? 0 : (df > 0 ? 1 : -1);

                int r_start = r1, f_start = f1;
                while (r_start - r_step >= 0 && r_start - r_step <= 7 && 
                       f_start - f_step >= 0 && f_start - f_step <= 7) {
                    r_start -= r_step;
                    f_start -= f_step;
                }
                
                uint64_t line = 0;
                while (r_start >= 0 && r_start <= 7 && f_start >= 0 && f_start <= 7) {
                    line |= (1ULL << (r_start * 8 + f_start));
                    r_start += r_step;
                    f_start += f_step;
                }

                lines[s1][s2] = lines[s2][s1] = line;
            }
        }
        return lines;
    }();
    static constexpr inline std::array<std::array<uint64_t, 64>, 64> between_bb = []() {
        std::array<std::array<uint64_t, 64>, 64> betweens{};
        for (int s1 = 0; s1 < 64; s1++) {
            for (int s2 = s1 + 1; s2 < 64; s2++) {
                int r1 = s1 / 8, f1 = s1 % 8;
                int r2 = s2 / 8, f2 = s2 % 8;
                int dr = r2 - r1, df = f2 - f1;

                if (dr != 0 && df != 0 && dr != df && dr != -df) continue;

                int r_step = (dr == 0) ? 0 : (dr > 0 ? 1 : -1);
                int f_step = (df == 0) ? 0 : (df > 0 ? 1 : -1);

                int r_start = r1 + r_step, f_start = f1 + f_step;
                
                uint64_t between = 0;
                while (r_start != r2 || f_start != f2) {
                    between |= (1ULL << (r_start * 8 + f_start));
                    r_start += r_step;
                    f_start += f_step;
                }

                betweens[s1][s2] = betweens[s2][s1] = between;
            }
        }
        return betweens;
    }();

private:
    static constexpr inline std::array<uint64_t, 64> bishop_magics = {
        0xFFEDF9FD7CFCFFFFull, 0xFC0962854A77F576ull, 0x5822022042000000ull, 0x2CA804A100200020ull,
        0x0204042200000900ull, 0x2002121024000002ull, 0xFC0A66C64A7EF576ull, 0x7FFDFDFCBD79FFFFull,
        0xFC0846A64A34FFF6ull, 0xFC087A874A3CF7F6ull, 0x1001080204002100ull, 0x1810080489021800ull,
        0x0062040420010A00ull, 0x5028043004300020ull, 0xFC0864AE59B4FF76ull, 0x3C0860AF4B35FF76ull,
        0x73C01AF56CF4CFFBull, 0x41A01CFAD64AAFFCull, 0x040C0422080A0598ull, 0x4228020082004050ull,
        0x0200800400E00100ull, 0x020B001230021040ull, 0x7C0C028F5B34FF76ull, 0xFC0A028E5AB4DF76ull,
        0x0020208050A42180ull, 0x001004804B280200ull, 0x2048020024040010ull, 0x0102C04004010200ull,
        0x020408204C002010ull, 0x02411100020080C1ull, 0x102A008084042100ull, 0x0941030000A09846ull,
        0x0244100800400200ull, 0x4000901010080696ull, 0x0000280404180020ull, 0x0800042008240100ull,
        0x0220008400088020ull, 0x04020182000904C9ull, 0x0023010400020600ull, 0x0041040020110302ull,
        0xDCEFD9B54BFCC09Full, 0xF95FFA765AFD602Bull, 0x1401210240484800ull, 0x0022244208010080ull,
        0x1105040104000210ull, 0x2040088800C40081ull, 0x43FF9A5CF4CA0C01ull, 0x4BFFCD8E7C587601ull,
        0xFC0FF2865334F576ull, 0xFC0BF6CE5924F576ull, 0x80000B0401040402ull, 0x0020004821880A00ull,
        0x8200002022440100ull, 0x0009431801010068ull, 0xC3FFB7DC36CA8C89ull, 0xC3FF8A54F4CA2C89ull,
        0xFFFFFCFCFD79EDFFull, 0xFC0863FCCB147576ull, 0x040C000022013020ull, 0x2000104000420600ull,
        0x0400000260142410ull, 0x0800633408100500ull, 0xFC087E8E4BB2F736ull, 0x43FF9E4EF4CA2C89ull,
    };
    static constexpr inline std::array<uint64_t, 64> rook_magics = {
        0xA180022080400230ull, 0x0040100040022000ull, 0x0080088020001002ull, 0x0080080280841000ull,
        0x4200042010460008ull, 0x04800A0003040080ull, 0x0400110082041008ull, 0x008000A041000880ull,
        0x10138001A080C010ull, 0x0000804008200480ull, 0x00010011012000C0ull, 0x0022004128102200ull,
        0x000200081201200Cull, 0x202A001048460004ull, 0x0081000100420004ull, 0x4000800380004500ull,
        0x0000208002904001ull, 0x0090004040026008ull, 0x0208808010002001ull, 0x2002020020704940ull,
        0x8048010008110005ull, 0x6820808004002200ull, 0x0A80040008023011ull, 0x00B1460000811044ull,
        0x4204400080008EA0ull, 0xB002400180200184ull, 0x2020200080100380ull, 0x0010080080100080ull,
        0x2204080080800400ull, 0x0000A40080360080ull, 0x02040604002810B1ull, 0x008C218600004104ull,
        0x8180004000402000ull, 0x488C402000401001ull, 0x4018A00080801004ull, 0x1230002105001008ull,
        0x8904800800800400ull, 0x0042000C42003810ull, 0x008408110400B012ull, 0x0018086182000401ull,
        0x2240088020C28000ull, 0x001001201040C004ull, 0x0A02008010420020ull, 0x0010003009010060ull,
        0x0004008008008014ull, 0x0080020004008080ull, 0x0282020001008080ull, 0x50000181204A0004ull,
        0x48FFFE99FECFAA00ull, 0x48FFFE99FECFAA00ull, 0x497FFFADFF9C2E00ull, 0x613FFFDDFFCE9200ull,
        0xFFFFFFE9FFE7CE00ull, 0xFFFFFFF5FFF3E600ull, 0x0010301802830400ull, 0x510FFFF5F63C96A0ull,
        0xEBFFFFB9FF9FC526ull, 0x61FFFEDDFEEDAEAEull, 0x53BFFFEDFFDEB1A2ull, 0x127FFFB9FFDFB5F6ull,
        0x411FFFDDFFDBF4D6ull, 0x0801000804000603ull, 0x0003FFEF27EEBE74ull, 0x7645FFFECBFEA79Eull,
    };
    static constexpr inline std::array<int, 64> bishop_shifts = {
        6, 5, 5, 5, 5, 5, 5, 6,
        5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 7, 7, 7, 7, 5, 5,
        5, 5, 7, 9, 9, 7, 5, 5,
        5, 5, 7, 9, 9, 7, 5, 5,
        5, 5, 7, 7, 7, 7, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5,
        6, 5, 5, 5, 5, 5, 5, 6
    };
    static constexpr inline std::array<int, 64> rook_shifts = {
        12, 11, 11, 11, 11, 11, 11, 12,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        12, 11, 11, 11, 11, 11, 11, 12
    };
    static constexpr inline std::array<uint64_t, 64> bishop_masks = []() {
        std::array<uint64_t, 64> masks{};
        for (int square = 0; square < 64; square++) {
            int rank = square / 8, file = square % 8;
            uint64_t mask = 0;
            for (int r = rank + 1, f = file + 1; r < 7 && f < 7; r++, f++) mask |= (1ULL << (r * 8 + f));
            for (int r = rank - 1, f = file - 1; r > 0 && f > 0; r--, f--) mask |= (1ULL << (r * 8 + f));
            for (int r = rank + 1, f = file - 1; r < 7 && f > 0; r++, f--) mask |= (1ULL << (r * 8 + f));
            for (int r = rank - 1, f = file + 1; r > 0 && f < 7; r--, f++) mask |= (1ULL << (r * 8 + f));
            masks[square] = mask;
        }
        return masks;
    }();
    static constexpr inline std::array<uint64_t, 64> rook_masks = []() {
        std::array<uint64_t, 64> masks{};
        for (int square = 0; square < 64; square++) {
            int rank = square / 8, file = square % 8;
            uint64_t mask = 0;            
            for (int r = rank + 1; r < 7; r++) mask |= (1ULL << (r * 8 + file));
            for (int r = rank - 1; r > 0; r--) mask |= (1ULL << (r * 8 + file));
            for (int f = file + 1; f < 7; f++) mask |= (1ULL << (rank * 8 + f));
            for (int f = file - 1; f > 0; f--) mask |= (1ULL << (rank * 8 + f));
            masks[square] = mask;
        }
        return masks;
    }();
    
    static inline std::array<std::array<uint64_t, 64>, 2> pawn_pushes_;
    static inline std::array<std::array<uint64_t, 64>, 2> pawn_attacks_;
    static inline std::array<uint64_t, 64> knight_attacks_;
    static inline std::array<uint64_t, 64> king_attacks_;
    static inline std::array<std::array<uint64_t, 512>, 64> bishop_attacks_;
    static inline std::array<std::array<uint64_t, 4096>, 64> rook_attacks_;

    static uint64_t compute_pawn_pushes_from(int square, bool white);
    static uint64_t compute_pawn_attacks_from(int square, bool white);
    static uint64_t compute_knight_attacks_from(int square);
    static uint64_t compute_king_attacks_from(int square);
    static uint64_t compute_sliding_attacks_from(int square, const std::array<std::pair<int, int>, 4>& directions, uint64_t occupancy);
};
