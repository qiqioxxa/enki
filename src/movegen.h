#pragma once

#include "board.h"


namespace MoveGen {
    void generate_moves(const Board& board, MoveList& list);

    bool can_castle(const Board& board, int king_square, int target, uint64_t occupancy);
    bool is_square_attacked(const Board& board, int square, bool by_white, uint64_t occupancy);
    uint64_t get_checkers(const Board& board, uint64_t occupancy);
    uint64_t get_pinned(const Board& board, uint64_t occupancy);
}
