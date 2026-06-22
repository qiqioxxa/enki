#include "engine.h"
#include "gamestate.h"
#include "movegen.h"
#include "utils.h"
#include <cmath>
#include <print>


namespace {
    constexpr int MAX_DEPTH = 200;
    constexpr int CHECKMATE = 16384;

    constexpr std::array<int, 13> piece_value {
        100,  320,  330,  500,  900, 0,
        -100, -320, -330, -500, -900, 0,
        0
    };
    constexpr std::array<std::array<uint8_t, 13>, 13> MVV_LVA = {{
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
    constexpr std::array<std::array<int8_t, 64>, 14> positional_bonus = {{
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
    constexpr int WHITE_KING_ENDSPIEL = 12;
    constexpr int BLACK_KING_ENDSPIEL = 13;

    const std::array<std::array<int, 64>, 64> LMR_table = []() {
        std::array<std::array<int, 64>, 64> table;
        for (int depth = 0; depth < 64; depth++) {
            for (int move_index = 0; move_index < 64; move_index++) {
                double r = 0.5 + std::log(depth + 1) * std::log(move_index + 1) / 3;
                table[depth][move_index] = static_cast<int>(r);
            }
        }
        return table;
    }();
}

// PUBLIC

Move Engine::choose_move(Board& board, const SearchParameters& sp) {
    start_ = std::chrono::steady_clock::now();
    allocated_time_ = calculate_time(sp, board.white_turn());
    stop_ = false;
    nodes_ = 0;

    MoveList root_list;
    MoveGen::generate_moves(board, root_list);
    if (root_list.size == 0) return Move{};
    
    Move best_move = root_list[0];
    int best_score = -CHECKMATE * 2;
    int delta = 50;

    int depth = 1;
    int max_depth = sp.depth != 0 ? sp.depth : MAX_DEPTH;

    // iterative deepening
    for (; depth <= max_depth; depth++) {
        if (!sp.infinite && elapsed_ms() > allocated_time_ * 0.6) break;

        // aspiration windows
        int alpha = best_score - delta;
        int beta = best_score + delta;
        if (depth == 1 || std::abs(best_score) >= CHECKMATE - MAX_DEPTH) {
            alpha = -CHECKMATE;
            beta = CHECKMATE;
        }

        int score = search(board, depth, 0, alpha, beta);

        while (!stop_ && (score <= alpha || score >= beta)) {
            if (score <= alpha) {
                alpha -= delta;
            } else {
                beta += delta;
            }
            delta += delta / 2;
            score = search(board, depth, 0, alpha, beta);
        }

        if (stop_) {
            depth--;
            break;
        }

        TTentry* entry = tt_.probe(board.zobrist_key());
        if (entry && entry->key == board.zobrist_key() && !entry->best_move.is_null_move()) {
            best_move = entry->best_move;
        }
        best_score = score;

        delta = 50;

        print_info(depth, best_score, best_move);
    }

    if (stop_) {
        print_info(depth, best_score, best_move);
    }

    return best_move;
}

// PRIVATE

int Engine::search(Board& board, int depth, int ply, int alpha, int beta) {
    if ((nodes_++ & 4095) == 0) {
        if (elapsed_ms() >= allocated_time_) stop_ = true;
    }
    if (stop_) return 0;

    if (board.halfmove_clock() >= 100 || ply > 0 && board.is_repetition()) return 0;

    TTentry* entry = tt_.probe(board.zobrist_key());
    if (entry && entry->key == board.zobrist_key() && entry->depth >= depth) {
        int tt_score = score_from_tt(entry->score, ply);

        if (entry->flag == TTentry::EXACT) return tt_score;
        if (entry->flag == TTentry::LOWERBOUND && tt_score >= beta) return tt_score;
        if (entry->flag == TTentry::UPPERBOUND && tt_score <= alpha) return tt_score;
    }

    int static_eval = evaluate(board);
    bool in_check = king_in_check(board);

    // null-move pruning
    if (depth >= 3 && static_eval >= beta && !in_check) {
        bool turn = board.white_turn();
        if ((board.get_knights(turn) | board.get_bishops(turn) | board.get_rooks(turn) | board.get_queens(turn)) != 0) {
            UnmakeInfo info = board.make_null_move();
            int score = -search(board, depth - 3, ply + 1, -beta, -beta + 1);
            board.unmake_null_move(info);

            if (score >= beta) return score;
        }
    }

    // reverse futility pruning
    if (depth <= 2 && !in_check) {
        int margin = 100 * depth;
        if (static_eval - margin >= beta) {
            return static_eval - margin;
        }
    }

    if (depth == 0) return quiescence(board, alpha, beta);

    MoveList list;
    MoveGen::generate_moves(board, list);

    if (list.size == 0) {
        int score = king_in_check(board) ? -CHECKMATE + ply : 0;
        tt_.record(board.zobrist_key(), Move{}, score_to_tt(score, ply), depth, TTentry::EXACT);
        return score;
    }

    Move tt_move = entry ? entry->best_move : Move{};
    order_moves(list, tt_move);

    int best_score = -CHECKMATE * 2;
    Move best_move = Move{};
    int original_alpha = alpha;

    for (int i = 0; i < list.size; i++) {
        UnmakeInfo info = board.make_move(list[i]);

        bool gives_check = king_in_check(board);
        int score;

        // principal variation search
        if (i == 0) {
            score = -search(board, depth - 1, ply + 1, -beta, -alpha);
            
        } else {
            // late move reduction
            int reduction = (i > 3 && depth >= 3 && !in_check && !gives_check && 
                            list[i].target_piece() == EMPTY && list[i].promotion() == EMPTY
                            ? std::min(LMR_table[std::min(depth, 63)][std::min(i, 63)], depth - 1) : 0);

            score = -search(board, depth - 1 - reduction, ply + 1, -alpha - 1, -alpha);

            if (reduction > 0 && score > alpha) {
                score = -search(board, depth - 1, ply + 1, -alpha - 1, -alpha);
            }

            if (score > alpha && score < beta) {
                score = -search(board, depth - 1, ply + 1, -beta, -alpha);
            }
        }
    
        board.unmake_move(list[i], info);

        if (stop_) return 0;

        if (score > best_score) {
            best_score = score;
            best_move = list[i];
        }
        if (score > alpha) {
            alpha = score;
            if (alpha >= beta) {
                tt_.record(board.zobrist_key(), best_move, score_to_tt(best_score, ply), depth, TTentry::LOWERBOUND);
                return best_score;
            }
        }
    }

    TTentry::TTflag tt_flag = (best_score <= original_alpha) ? TTentry::UPPERBOUND : TTentry::EXACT;
    Move move_to_store = (tt_flag == TTentry::UPPERBOUND) ? Move{} : best_move;
    tt_.record(board.zobrist_key(), move_to_store, score_to_tt(best_score, ply), depth, tt_flag);

    return best_score;
}

int Engine::quiescence(Board& board, int alpha, int beta) {
    int stand_pat = evaluate(board);
    if (stand_pat >= beta) return stand_pat;
    if (stand_pat > alpha) alpha = stand_pat;

    MoveList list;
    MoveGen::generate_moves(board, list);

    if (list.size == 0) return alpha;

    if (!king_in_check(board)) {
        int captures = 0;
        for (int i = 0; i < list.size; ++i) {
            if (list[i].target_piece() != EMPTY) {
                list[captures++] = list[i];
            }
        }
        list.size = captures;
    }

    order_moves(list, Move{});

    for (Move move : list) {
        int victim_val = std::abs(piece_value[move.target_piece()]);
        if (stand_pat + victim_val + 200 < alpha) continue; // delta pruning

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
    score += endspiel ? positional_bonus[WHITE_KING_ENDSPIEL][square] : positional_bonus[W_KING][square];

    square = std::countr_zero(bitboards[B_KING]);
    score += endspiel ? positional_bonus[BLACK_KING_ENDSPIEL][square] : positional_bonus[B_KING][square];

    return board.white_turn() ? score : -score;
}

void Engine::order_moves(MoveList& list, Move tt_move) const {
    int shift = 0;
    if (!tt_move.is_null_move()) {
        for (int i = 0; i < list.size; i++) {
            if (list[i] == tt_move) {
                std::swap(list[0], list[i]);
                shift = 1;
                break;
            }
        }
    }
    std::sort(list.begin() + shift, list.end(), [](Move move1, Move move2) {
        return MVV_LVA[move1.target_piece()][move1.moving_piece()] > MVV_LVA[move2.target_piece()][move2.moving_piece()]; 
    });
}

bool Engine::is_endspiel(const Board& board) const {
    return std::popcount(board.get_occupancy()) < 10;
}

int Engine::calculate_time(const SearchParameters& sp, bool white) const {
    if (sp.movetime != 0) return sp.movetime;
    if (sp.infinite || sp.depth != 0) return INT_MAX;
    int time = white ? sp.wtime : sp.btime;
    int inc = white ? sp.winc : sp.binc;
    int moves_left = sp.movestogo != 0 ? sp.movestogo : 30;
    return time / moves_left + inc * 0.8;
}

int Engine::elapsed_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_).count();
}

void Engine::print_info(int depth, int best_score, Move best_move) const {
    int time = elapsed_ms();
    int nps = time != 0 ? nodes_ / time * 1000 : 0;
    
    std::string score_str;
    if (std::abs(best_score) >= CHECKMATE - MAX_DEPTH) {
        int mate_in = (CHECKMATE - std::abs(best_score) + 1) / 2;
        if (best_score < 0) mate_in = -mate_in;
        score_str = std::format("mate {}", mate_in);
    } else {
        score_str = std::format("cp {}", best_score);
    }

    std::println("info depth {} score {} nodes {} nps {} hashfull {} time {} pv {}",
        depth, score_str, nodes_, nps, tt_.hashfull(), time, best_move.to_string());
    std::fflush(stdout);
}

int Engine::score_to_tt(int score, int ply) const {
    if (score >= CHECKMATE - MAX_DEPTH) return score + ply;
    if (score <= -CHECKMATE + MAX_DEPTH) return score - ply;
    return score;
}
int Engine::score_from_tt(int tt_score, int ply) const {
    if (tt_score >= CHECKMATE - MAX_DEPTH) return tt_score - ply;
    if (tt_score <= -CHECKMATE + MAX_DEPTH) return tt_score + ply;
    return tt_score;
}