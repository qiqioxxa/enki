#include "engine.h"
#include "gamestate.h"
#include "movegen.h"
#include "utils.h"
#include <chrono>
#include <print>

// PUBLIC

Move Engine::choose_move(Board& board, const SearchParameters& sp) const {
    start_ = std::chrono::high_resolution_clock::now();
    allocated_time_ = calculate_time(sp, board.white_turn());
    stop_ = false;
    nodes_ = 0;

    MoveList root_list;
    MoveGen::generate_moves(board, root_list);
    if (root_list.size == 0) return Move{};
    
    Move best_move = root_list[0];
    int best_score = -CHECKMATE * 2;
    int delta = 50;
    int max_depth = sp.depth != 0 ? sp.depth : MAX_DEPTH;

    for (int depth = 1; depth <= max_depth; depth++) {
        if (!sp.infinite && elapsed_ms() > allocated_time_ * 0.6) break;

        int alpha = best_score - delta;
        int beta = best_score + delta;
        if (depth == 1 || std::abs(best_score) > CHECKMATE - MAX_DEPTH) {
            alpha = -CHECKMATE;
            beta = CHECKMATE;
        }

        int score = search(board, depth, alpha, beta);

        while (!stop_ && (score <= alpha || score >= beta)) {
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

        if (stop_) break;

        TTentry* entry = tt.probe(board.zobrist_key());
        if (entry && entry->key == board.zobrist_key() && !entry->best_move.is_null_move()) {
            best_move = entry->best_move;
            best_score = score;
        }
        delta = 50;

        int time = elapsed_ms();
        int nps = time != 0 ? nodes_ / time * 1000 : 0;
        
        std::string score_str;
        if (std::abs(best_score) > CHECKMATE - MAX_DEPTH) {
            int mate_in = (CHECKMATE - std::abs(best_score) + 1) / 2; // doesn't work
            score_str = std::format("mate {}", mate_in);
        } else {
            score_str = std::format("cp {}", best_score);
        }

        std::println("info depth {} score {} nodes {} nps {} hashfull {} time {}",
            depth, score_str, nodes_.load(), nps, tt.hashfull(), time);
        std::fflush(stdout);
    }

    return best_move;
}

// PRIVATE

int Engine::search(Board& board, int depth, int alpha, int beta) const {
    if ((nodes_.fetch_add(1) & 4095) == 0) {
        if (elapsed_ms() >= allocated_time_) stop_ = true;
    }
    if (stop_) return 0;

    TTentry* entry = tt.probe(board.zobrist_key());
    if (entry && entry->depth >= depth) {
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

    int best_score = -CHECKMATE * 2;
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
    int stand_pat = evaluate(board);
    if (stand_pat >= beta) return stand_pat;
    if (stand_pat > alpha) alpha = stand_pat;

    MoveList list;
    MoveGen::generate_moves(board, list);

    if (!king_in_check(board)) {
        int captures = 0;
        for (int i = 0; i < list.size; ++i) {
            if (list[i].target_piece() != EMPTY) {
                list[captures++] = list[i];
            }
        }
        list.size = captures;
    }

    if (list.size == 0) return alpha;

    order_moves(list, Move{});

    for (Move move : list) {
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
    std::sort(list.begin() + shift, list.end(), [this](Move move1, Move move2) {
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
