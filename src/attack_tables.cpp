#include "attack_tables.h"

// PUBLIC

void AttackTables::init() {
    for (int square = 0; square < 64; square++) {
        pawn_pushes_[0][square] = compute_pawn_pushes_from(square, true);
        pawn_pushes_[1][square] = compute_pawn_pushes_from(square, false);
        pawn_attacks_[0][square] = compute_pawn_attacks_from(square, true);
        pawn_attacks_[1][square] = compute_pawn_attacks_from(square, false);
        knight_attacks_[square] = compute_knight_attacks_from(square);
        king_attacks_[square] = compute_king_attacks_from(square);
    }

    const std::array<std::pair<int, int>, 4> bishop_dirs = {{ {-1, -1}, {-1, 1}, {1, -1}, {1, 1} }};
    const std::array<std::pair<int, int>, 4> rook_dirs = {{ {-1, 0}, {1, 0}, {0, -1}, {0, 1} }};
    
    for (int square = 0; square < 64; ++square) {
        uint64_t mask = bishop_masks[square];
        uint64_t cur_blockers = 0;
        
        do {
            uint64_t index = (cur_blockers * bishop_magics[square]) >> (64 - bishop_shifts[square]);
            bishop_attacks_[square][index] = compute_sliding_attacks_from(square, bishop_dirs, cur_blockers);
            cur_blockers = (cur_blockers - mask) & mask;
        } while (cur_blockers != 0);
        
        mask = rook_masks[square];
        cur_blockers = 0;
        
        do {
            uint64_t index = (cur_blockers * rook_magics[square]) >> (64 - rook_shifts[square]);
            rook_attacks_[square][index] = compute_sliding_attacks_from(square, rook_dirs, cur_blockers);
            cur_blockers = (cur_blockers - mask) & mask;
        } while (cur_blockers != 0);
    }
}

// PRIVATE

uint64_t AttackTables::compute_pawn_pushes_from(int square, bool white) {
    uint64_t pushes = 0;
    int rank = square / 8;
    int file = square % 8;
    for (int target = 0; target < 64; target++) {
        if (target == square) continue;
        int rank_diff = target / 8 - rank;
        int file_diff = target % 8 - file;
        if (white) {
            if (rank_diff == 1 && file_diff == 0) {
                pushes |= (1ULL << target);
            }
            if (rank == 1 && rank_diff == 2 && file_diff == 0) {
                pushes |= (1ULL << target);
            }
        } else {
            if (rank_diff == -1 && file_diff == 0) {
                pushes |= (1ULL << target);
            }
            if (rank == 6 && rank_diff == -2 && file_diff == 0) {
                pushes |= (1ULL << target);
            }
        }
    }
    return pushes;
}
uint64_t AttackTables::compute_pawn_attacks_from(int square, bool white) {
    uint64_t attacks = 0;
    int rank = square / 8;
    int file = square % 8;
    for (int target = 0; target < 64; target++) {
        if (target == square) continue;
        int rank_diff = target / 8 - rank;
        int file_diff = target % 8 - file;
        if (white) {
            if ((rank_diff == 1 && file_diff == 1) || (rank_diff == 1 && file_diff == -1)) {
                attacks |= (1ULL << target);
            }
        } else {
            if ((rank_diff == -1 && file_diff == 1) || (rank_diff == -1 && file_diff == -1)) {
                attacks |= (1ULL << target);
            }
        }
    }
    return attacks;
}
uint64_t AttackTables::compute_knight_attacks_from(int square) {
    uint64_t attacks = 0;
    int rank = square / 8;
    int file = square % 8;
    for (int target = 0; target < 64; target++) {
        if (target == square) continue;
        int rank_diff = std::abs(target / 8 - rank);
        int file_diff = std::abs(target % 8 - file);
        if ((rank_diff == 1 && file_diff == 2) || (rank_diff == 2 && file_diff == 1)) {
            attacks |= (1ULL << target);
        }
    }
    return attacks;
}
uint64_t AttackTables::compute_king_attacks_from(int square) {
    uint64_t attacks = 0;
    int rank = square / 8;
    int file = square % 8;
    for (int target = 0; target < 64; target++) {
        if (target == square) continue;
        int rank_diff = std::abs(target / 8 - rank);
        int file_diff = std::abs(target % 8 - file);
        if (rank_diff <= 1 && file_diff <= 1) {
            attacks |= (1ULL << target);
        }
    }
    return attacks;
}
uint64_t AttackTables::compute_sliding_attacks_from(int square, const std::array<std::pair<int, int>, 4>& directions, uint64_t occupancy) {
    uint64_t attacks = 0;
    int rank = square / 8;
    int file = square % 8;
    for (const auto& dir : directions) {
        int r = rank;
        int f = file;
        while (true) {
            r += dir.first;
            f += dir.second;
            if (r < 0 || r > 7 || f < 0 || f > 7) break;
            attacks |= (1ULL << (r * 8 + f));
            if (occupancy & (1ULL << (r * 8 + f))) break;
        }
    }
    return attacks;
}
