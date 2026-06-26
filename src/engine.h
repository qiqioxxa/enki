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
    TranspositionTable tt_;
    std::chrono::steady_clock::time_point start_;
    int allocated_time_ = 0;
    bool stop_ = false;
    int nodes_ = 0;
    uint8_t generation_ = 0;

public:
    Engine() {};
    Move choose_move(Board& board, const SearchParameters& sp);
    void tt_resize(int size_mb) { tt_.resize(size_mb); }
    void tt_clear() { tt_.clear(); }
    void stop() { stop_ = true; }

private:
    int search(Board& board, int depth, int ply, int alpha, int beta);
    int quiescence(Board& board, int alpha, int beta);
    int evaluate(const Board& board) const;
    void order_moves(MoveList& list, Move tt_move) const;
    bool is_endspiel(const Board& board) const;
    int calculate_time(const SearchParameters& sp, bool white) const;
    int elapsed_ms() const;

    void print_info(int depth, int best_score, Board& board) const;
    std::string pv_line_from_tt(Board& board, int depth) const;

    int score_to_tt(int score, int ply) const;
    int score_from_tt(int tt_score, int ply) const;
};
