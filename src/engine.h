#pragma once

#include "board.h"
#include "ttable.h"
#include "types.h"
#include <chrono>


struct SearchParameters {
    int wtime = 0, btime = 0;
    int winc = 0, binc = 0;
    int movestogo = 0;
    int depth = 0;
    int movetime = 0;
    bool infinite = false;
};

class Engine {
    static constexpr int MAX_DEPTH = 100;
    static constexpr int CHECKMATE = 16384;
    static constexpr std::array<int, 12> piece_value {
        100,  320,  330,  500,  900, 0,
        -100, -320, -330, -500, -900, 0
    };
    static constexpr std::array<std::array<uint8_t, 13>, 13> MVV_LVA = {{
        { 0,  0,  0,  0,  0,  0, 15, 14, 13, 12, 11, 10,  0}, // victim     P, attacker P, N, B, R, Q, K, {p, n, b, r, q, k}, Empty
        { 0,  0,  0,  0,  0,  0, 25, 24, 23, 22, 21, 20,  0}, // victim     N, attacker P, N, B, R, Q, K, {p, n, b, r, q, k}, Empty
        { 0,  0,  0,  0,  0,  0, 35, 34, 33, 32, 31, 30,  0}, // victim     B, attacker P, N, B, R, Q, K, {p, n, b, r, q, k}, Empty
        { 0,  0,  0,  0,  0,  0, 45, 44, 43, 42, 41, 40,  0}, // victim     R, attacker P, N, B, R, Q, K, {p, n, b, r, q, k}, Empty
        { 0,  0,  0,  0,  0,  0, 55, 54, 53, 52, 51, 50,  0}, // victim     Q, attacker P, N, B, R, Q, K, {p, n, b, r, q, k}, Empty
        { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // victim     K, attacker P, N, B, R, Q, K,  p, n, b, r, q, k , Empty
        {15, 14, 13, 12, 11, 10,  0,  0,  0,  0,  0,  0,  0}, // victim     p, attacker {P, N, B, R, Q, K}, p, n, b, r, q, k, Empty
        {25, 24, 23, 22, 21, 20,  0,  0,  0,  0,  0,  0,  0}, // victim     n, attacker {P, N, B, R, Q, K}, p, n, b, r, q, k, Empty
        {35, 34, 33, 32, 31, 30,  0,  0,  0,  0,  0,  0,  0}, // victim     b, attacker {P, N, B, R, Q, K}, p, n, b, r, q, k, Empty
        {45, 44, 43, 42, 41, 40,  0,  0,  0,  0,  0,  0,  0}, // victim     r, attacker {P, N, B, R, Q, K}, p, n, b, r, q, k, Empty
        {55, 54, 53, 52, 51, 50,  0,  0,  0,  0,  0,  0,  0}, // victim     q, attacker {P, N, B, R, Q, K}, p, n, b, r, q, k, Empty
        { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // victim     k, attacker  P, N, B, R, Q, K , p, n, b, r, q, k, Empty
        { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // victim Empty, attacker  P, N, B, R, Q, K , p, n, b, r, q, k, Empty
    }};
    static constexpr std::array<std::array<int8_t, 64>, 14> positional_bonus = {{
        {
             0,   0,   0,   0,   0,   0,   0,   0,
             5,  10,  10, -20, -20,  10,  10,   5,
             5,  -5, -10,   0,   0, -10,  -5,   5,
             0,   0,   0,  20,  20,   0,   0,   0,
             5,   5,  10,  25,  25,  10,   5,   5,
            10,  10,  20,  30,  30,  20,  10,  10,
            50,  50,  50,  50,  50,  50,  50,  50,
             0,   0,   0,   0,   0,   0,   0,   0
        }, // white pawn
        {
            -50, -40, -30, -30, -30, -30, -40, -50,
            -40, -20,   0,   5,   5,   0, -20, -40,
            -30,   5,  10,  15,  15,  10,   5, -30,
            -30,   0,  15,  20,  20,  15,   0, -30,
            -30,   5,  15,  20,  20,  15,   5, -30,
            -30,   0,  10,  15,  15,  10,   0, -30,
            -40, -20,   0,   0,   0,   0, -20, -40,
            -50, -40, -30, -30, -30, -30, -40, -50
        }, // white knight
        {
            -20, -10, -10, -10, -10, -10, -10, -20,
            -10,   5,   0,   0,   0,   0,   5, -10,
            -10,  10,  10,  10,  10,  10,  10, -10,
            -10,   0,  10,  10,  10,  10,   0, -10,
            -10,   5,   5,  10,  10,   5,   5, -10,
            -10,   0,   5,  10,  10,   5,   0, -10,
            -10,   0,   0,   0,   0,   0,   0, -10,
            -20, -10, -10, -10, -10, -10, -10, -20
        }, // white bishop
        {
             0,   0,   0,   5,   5,   0,   0,   0,
            -5,   0,   0,   0,   0,   0,   0,  -5,
            -5,   0,   0,   0,   0,   0,   0,  -5,
            -5,   0,   0,   0,   0,   0,   0,  -5,
            -5,   0,   0,   0,   0,   0,   0,  -5,
            -5,   0,   0,   0,   0,   0,   0,  -5,
             5,  10,  10,  10,  10,  10,  10,   5,
             0,   0,   0,   0,   0,   0,   0,   0
        }, // white rook
        {
            -20, -10, -10,  -5,  -5, -10, -10, -20,
            -10,   0,   5,   0,   0,   0,   0, -10,
            -10,   5,   5,   5,   5,   5,   0, -10,
              0,   0,   5,   5,   5,   5,   0,  -5,
             -5,   0,   5,   5,   5,   5,   0,  -5,
            -10,   0,   5,   5,   5,   5,   0, -10,
            -10,   0,   0,   0,   0,   0,   0, -10,
            -20, -10, -10,  -5,  -5, -10, -10, -20
        }, // white queen
        {
             20,  30,  10,   0,   0,  10,  30,  20,
             20,  20,   0,   0,   0,   0,  20,  20,
            -10, -20, -20, -20, -20, -20, -20, -10,
            -20, -30, -30, -40, -40, -30, -30, -20,
            -30, -40, -40, -50, -50, -40, -40, -30,
            -30, -40, -40, -50, -50, -40, -40, -30,
            -30, -40, -40, -50, -50, -40, -40, -30,
            -30, -40, -40, -50, -50, -40, -40, -30
        }, // white king mittelspiel
        {
              0,   0,   0,   0,   0,   0,   0,   0,
            -50, -50, -50, -50, -50, -50, -50, -50,
            -10, -10, -20, -30, -30, -20, -10, -10,
             -5,  -5, -10, -25, -25, -10,  -5,  -5,
              0,   0,   0, -20, -20,   0,   0,   0,
             -5,   5,  10,   0,   0,  10,   5,  -5,
             -5, -10, -10,  20,  20, -10, -10,  -5,
              0,   0,   0,   0,   0,   0,   0,   0
        }, // black pawn
        {
            50,  40,  30,  30,  30,  30,  40,  50,
            40,  20,   0,   0,   0,   0,  20,  40,
            30,   0, -10, -15, -15, -10,   0,  30,
            30,  -5, -15, -20, -20, -15,  -5,  30,
            30,   0, -15, -20, -20, -15,   0,  30,
            30,  -5, -10, -15, -15, -10,  -5,  30,
            40,  20,   0,  -5,  -5,   0,  20,  40,
            50,  40,  30,  30,  30,  30,  40,  50
        }, // black knight
        {
            20,  10,  10,  10,  10,  10,  10,  20,
            10,   0,   0,   0,   0,   0,   0,  10,
            10,   0,  -5, -10, -10,  -5,   0,  10,
            10,  -5,  -5, -10, -10,  -5,  -5,  10,
            10,   0, -10, -10, -10, -10,   0,  10,
            10, -10, -10, -10, -10, -10, -10,  10,
            10,  -5,   0,   0,   0,   0,  -5,  10,
            20,  10,  10,  10,  10,  10,  10,  20
        }, // black bishop
        {
             0,   0,   0,   0,   0,   0,   0,   0,
            -5, -10, -10, -10, -10, -10, -10,  -5,
             5,   0,   0,   0,   0,   0,   0,   5,
             5,   0,   0,   0,   0,   0,   0,   5,
             5,   0,   0,   0,   0,   0,   0,   5,
             5,   0,   0,   0,   0,   0,   0,   5,
             5,   0,   0,   0,   0,   0,   0,   5,
             0,   0,   0,  -5,  -5,   0,   0,   0
        }, // black rook
        {
            20,  10,  10,   5,   5,  10,  10,  20,
            10,   0,   0,   0,   0,   0,   0,  10,
            10,   0,  -5,  -5,  -5,  -5,   0,  10,
             5,   0,  -5,  -5,  -5,  -5,   0,   5,
             0,   0,  -5,  -5,  -5,  -5,   0,   5,
            10,  -5,  -5,  -5,  -5,  -5,   0,  10,
            10,   0,  -5,   0,   0,   0,   0,  10,
            20,  10,  10,   5,   5,  10,  10,  20
        }, // black queen
        {
             30,  40,  40,  50,  50,  40,  40,  30,
             30,  40,  40,  50,  50,  40,  40,  30,
             30,  40,  40,  50,  50,  40,  40,  30,
             30,  40,  40,  50,  50,  40,  40,  30,
             20,  30,  30,  40,  40,  30,  30,  20,
             10,  20,  20,  20,  20,  20,  20,  10,
            -20, -20,   0,   0,   0,   0, -20, -20,
            -20, -30, -10,   0,   0, -10, -30, -20
        }, // black king mittelspiel
        {
            -50, -30, -30, -30, -30, -30, -30, -50,
            -30, -30,   0,   0,   0,   0, -30, -30,
            -30, -10,  20,  30,  30,  20, -10, -30,
            -30, -10,  30,  40,  40,  30, -10, -30,
            -30, -10,  30,  40,  40,  30, -10, -30,
            -30, -10,  20,  30,  30,  20, -10, -30,
            -30, -20, -10,   0,   0, -10, -20, -30,
            -50, -40, -30, -20, -20, -30, -40, -50
        }, // white king endspiel
        {
            50,  40,  30,  20,  20,  30,  40,  50,
            30,  20,  10,   0,   0,  10,  20,  30,
            30,  10, -20, -30, -30, -20,  10,  30,
            30,  10, -30, -40, -40, -30,  10,  30,
            30,  10, -30, -40, -40, -30,  10,  30,
            30,  10, -20, -30, -30, -20,  10,  30,
            30,  30,   0,   0,   0,   0,  30,  30,
            50,  30,  30,  30,  30,  30,  30,  50
        } // black king endspiel
    }};
    static constexpr int WHITE_KING_ENDSPIEL = 12;
    static constexpr int BLACK_KING_ENDSPIEL = 13;

    mutable TranspositionTable tt_;
    mutable std::chrono::high_resolution_clock::time_point start_;
    mutable int allocated_time_ = 0;
    mutable bool stop_ = false;
    mutable int nodes_ = 0;

public:
    Engine() {};
    Move choose_move(Board& board, const SearchParameters& sp) const;
    void tt_resize(int size_mb) const { tt_.resize(size_mb); }
    void tt_clear() const { tt_.clear(); }
    void stop() const { stop_ = true; }

private:
    int search(Board& board, int depth, int ply, int alpha, int beta) const;
    int quiescence(Board& board, int alpha, int beta) const;
    int evaluate(const Board& board) const;
    void order_moves(MoveList& list, Move tt_move) const;
    bool is_endspiel(const Board& board) const;
    int calculate_time(const SearchParameters& sp, bool white) const;
    int elapsed_ms() const;
    void print_info(int depth, int best_score, Move best_move) const;
};
