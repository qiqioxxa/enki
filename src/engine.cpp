#include "engine.h"
#include "gamestate.h"
#include "movegen.h"
#include "utils.h"
#include <iostream>

// PUBLIC
static uint64_t total = 0;
static uint64_t hits = 0;

Move Engine::choose_move(Board& board, int max_depth, int time_limit_ms) const {
    auto start = std::chrono::high_resolution_clock::now();

    Move best_move;
    int best_score = -CHECKMATE;
    int delta = 50;

    MoveList root_list;
    MoveGen::generate_moves(board, root_list);
    if (root_list.size == 0) return Move{};
    best_move = root_list[0];

    for (int depth = 1; depth <= max_depth; depth++) {
        uint64_t iter_total = total;
        uint64_t iter_hits = hits;
        auto iter_start = std::chrono::high_resolution_clock::now();

        if (time_limit_ms > 0) {
            auto now = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > time_limit_ms * 0.25) break;
        }

        int alpha = best_score - delta;
        int beta = best_score + delta;
        if (depth == 1 || std::abs(best_score) > CHECKMATE - 1000) {
            alpha = -CHECKMATE;
            beta = CHECKMATE;
        }

        int score = search(board, depth, alpha, beta);

        while (score <= alpha || score >= beta) {
            if (score <= alpha) {
                beta = (alpha + beta) / 2;
                alpha -= delta;
            } else {
                alpha = (alpha + beta) / 2;
                beta += delta;
            }
            delta += delta / 2;
            score = search(board, depth, alpha, beta);
        }

        TTentry* entry = tt.probe(board.zobrist_key());
        if (entry && entry->key == board.zobrist_key() && !entry->best_move.is_null_move()) {
            best_move = entry->best_move;
            best_score = score;
        }

        auto iter_end = std::chrono::high_resolution_clock::now();
        double iter_ms = std::chrono::duration_cast<std::chrono::milliseconds>(iter_end - iter_start).count();
        if (iter_ms < 1) iter_ms = 1;

        uint64_t d_nodes = total - iter_total;
        uint64_t d_hits  = hits - iter_hits;
        double d_rate    = d_nodes > 0 ? (100.0 * d_hits / d_nodes) : 0.0;
        double d_nps     = (d_nodes * 1000.0) / iter_ms;
        /*std::println("depth: {} score: {} best: {} time: {}s Mnodes: {} Mnps: {} hits: {}\% hashfull: {}", 
                     depth, score, best_move.to_string(), iter_ms / 1000, (double)d_nodes / 1'000'000, d_nps / 1'000'000, d_rate, tt.hashfull());*/
    }

    return best_move;
}

// PRIVATE

int Engine::search(Board& board, int depth, int alpha, int beta) const {
    total++;
    TTentry* entry = tt.probe(board.zobrist_key());
    if (entry && entry->depth >= depth) {
        hits++;
        if (entry->flag == TTentry::EXACT) return entry->score;
        if (entry->flag == TTentry::LOWERBOUND && entry->score >= beta) return entry->score;
        if (entry->flag == TTentry::UPPERBOUND && entry->score <= alpha) return entry->score;
    }

    MoveList list;
    MoveGen::generate_moves(board, list);

    if (list.size == 0) {
        int score = king_in_check(board) ? -CHECKMATE - depth : 0;
        tt.record(board.zobrist_key(), Move{}, score, depth, TTentry::EXACT);
        return score;
    }
    if (depth == 0) return quiescence(board, alpha, beta);

    Move tt_move = entry ? entry->best_move : Move{};
    order_moves(list, tt_move);

    int best_score = -CHECKMATE*2;
    Move best_move = Move{};
    int original_alpha = alpha;

    for (Move move : list) {
        UnmakeInfo info = board.make_move(move);
        int score = -search(board, depth - 1, -beta, -alpha);
        board.unmake_move(move, info);

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
        if (score > alpha) {
            alpha = score;
            if (alpha >= beta) {
                tt.record(board.zobrist_key(), best_move, best_score, depth, TTentry::LOWERBOUND);
                return best_score;
            }
        }
    }

    TTentry::TTflag tt_flag = (best_score <= original_alpha) ? TTentry::UPPERBOUND : TTentry::EXACT;

    tt.record(board.zobrist_key(), best_move, best_score, depth, tt_flag);
    return best_score;
}
int Engine::quiescence(Board& board, int alpha, int beta) const {
    // 1. Stand-pat: оценка текущей позиции без ходов
    int stand_pat = evaluate(board);
    if (stand_pat >= beta) return stand_pat; // fail-soft отсечка
    if (stand_pat > alpha) alpha = stand_pat;

    // 2. Генерируем ходы и оставляем только взятия (включая en-passant)
    MoveList list;
    MoveGen::generate_moves(board, list);

    int captures = 0;

    for (int i = 0; i < list.size; ++i) {
        if (list[i].target_piece() != EMPTY) {
            list[captures++] = list[i]; // компактируем массив на месте
        }
    }
    list.size = captures;

    if (list.size == 0) return alpha; // Нет взятий → позиция "тихая"

    // 3. Сортировка взятий по MVV-LVA (ваша таблица)
    std::sort(list.begin(), list.end(), [this](Move move1, Move move2) {
        return MVV_LVA[move1.target_piece()][move1.moving_piece()] > MVV_LVA[move2.target_piece()][move2.moving_piece()]; 
    });

    // 4. Перебор взятий
    for (Move move : list) {
        // Delta pruning: пропускаем взятия, которые заведомо не поднимут alpha
        int victim_val = piece_value[move.target_piece()];
        if (stand_pat + victim_val + 200 < alpha) continue;

        UnmakeInfo info = board.make_move(move);
        int score = -quiescence(board, -beta, -alpha);
        board.unmake_move(move, info);

        if (score >= beta) return score;
        if (score > alpha) alpha = score;
    }

    return alpha;
}

int Engine::evaluate(const Board& board) const {
    int score = 0;
    const std::array<uint64_t, 12>& bitboards = board.get_bitboards();
    bool endspiel = is_endspiel(board);

    for (int p = 0; p < 12; p++) {
        if (p == W_KING || p == B_KING) continue;

        uint64_t pieces = bitboards[p];
        while (pieces) {
            int square = pop_lsb(pieces);
            score += piece_value[p];
            score += positional_bonus[p][square];
        }
    }

    int square = std::countr_zero(bitboards[W_KING]);
    score += endspiel ? positional_bonus[W_KING + 1][square] : positional_bonus[W_KING][square];

    square = std::countr_zero(bitboards[B_KING]);
    score += endspiel ? positional_bonus[B_KING + 1][square] : positional_bonus[B_KING][square];

    return board.white_turn() ? score : -score;
}

void Engine::order_moves(MoveList& list, Move tt_move) const {
    if (!tt_move.is_null_move()) {
        for (int i = 0; i < list.size; i++) {
            if (list[i] == tt_move) {
                std::swap(list[0], list[i]);
                break;
            }
        }
    }
    std::sort(list.begin() + 1, list.end(), [this](Move move1, Move move2) {
        return MVV_LVA[move1.target_piece()][move1.moving_piece()] > MVV_LVA[move2.target_piece()][move2.moving_piece()]; 
    });
}

bool Engine::is_endspiel(const Board& board) const {
    return std::popcount(board.get_occupancy()) < 10;
}
