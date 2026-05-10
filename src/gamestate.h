#pragma once

#include "board.h"
#include "movegen.h"
#include "types.h"
#include <bit>
#include <cstdint>


enum class GameState : uint8_t {
    ONGOING,
    WHITE_WINS,
    BLACK_WINS,
    DRAW_STALEMATE,
    DRAW_50MOVE_RULE,
    DRAW_INSUFFICIENT_MATERIAL,
    DRAW_REPETITION
};

inline bool draw_by_insufficient_material(const Board& board) {
    bool qrp = board.get_queens(true) | board.get_rooks(true) | board.get_pawns(true) | 
               board.get_queens(false) | board.get_rooks(false) | board.get_pawns(false);
    if (qrp) return false;

    int w_bishops = std::popcount(board.get_bishops(true));
    int w_knights = std::popcount(board.get_knights(true));
    int b_bishops = std::popcount(board.get_bishops(false));
    int b_knights = std::popcount(board.get_knights(false));

    int w_minors = w_bishops + w_knights;
    int b_minors = b_bishops + b_knights;

    if (w_minors == 0 && b_minors <= 1) return true;
    if (b_minors == 0 && w_minors <= 1) return true;

    if (w_bishops == 1 && w_knights == 0 && b_bishops == 1 && b_knights == 0) {
        int w_square = std::countr_zero(board.get_bishops(true));
        int b_square = std::countr_zero(board.get_bishops(false));
        bool same_color = ((w_square / 8 + w_square % 8) % 2) == ((b_square / 8 + b_square % 8) % 2);
        return same_color;
    }

    return false;
}

inline bool king_in_check(const Board& board) {
    return MoveGen::get_checkers(board, board.get_occupancy()) != 0;
}

inline GameState get_game_state(const Board& board) {
    MoveList list;
    MoveGen::generate_moves(board, list);
    bool in_check = king_in_check(board);

    if (list.size == 0) {
        if (in_check) {
            return board.white_turn() ? GameState::BLACK_WINS : GameState::WHITE_WINS;
        }
        return GameState::DRAW_STALEMATE;
    }

    if (board.halfmove_clock() >= 100) return GameState::DRAW_50MOVE_RULE;
    if (draw_by_insufficient_material(board)) return GameState::DRAW_INSUFFICIENT_MATERIAL;
    if (false) return GameState::DRAW_REPETITION;

    return GameState::ONGOING;
}