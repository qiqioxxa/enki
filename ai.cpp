#include "ai.h"

// PUBLIC
// ==================== constructor and initialization ====================

AI::AI() {
    init_passed_masks();
    init_pawn_attacks();
}
void AI::init_passed_masks() {
    for (int cell = 0; cell < 64; cell++) {
        int file = cell & 7;
        int rank = cell >> 3;

        uint64_t white_mask = 0;
        for (int f = std::max(0, file - 1); f <= std::min(7, file + 1); f++) {
            for (int r = rank + 1; r < 8; r++) {
                white_mask |= (1ULL << (r * 8 + f));
            }
        }
        front_spans[0][cell] = white_mask;

        uint64_t black_mask = 0;
        for (int f = std::max(0, file - 1); f <= std::min(7, file + 1); f++) {
            for (int r = 0; r < rank; r++) {
                black_mask |= (1ULL << (r * 8 + f));
            }
        }
        front_spans[1][cell] = black_mask;
    }
}
void AI::init_pawn_attacks() {
    for (int cell = 0; cell < 64; cell++) {
        int file = cell & 7;
        int rank = cell >> 3;

        if (file > 0 && rank < 7) pawn_attacks[0][cell] |= (1ULL << ((rank + 1) * 8 + (file - 1)));
        if (file < 7 && rank < 7) pawn_attacks[0][cell] |= (1ULL << ((rank + 1) * 8 + (file + 1)));

        if (file > 0 && rank > 0) pawn_attacks[1][cell] |= (1ULL << ((rank - 1) * 8 + (file - 1)));
        if (file < 7 && rank > 0) pawn_attacks[1][cell] |= (1ULL << ((rank - 1) * 8 + (file + 1)));
    }
}

// ==================== main function ====================

Move AI::choose_computer_move(const ChessBoard& board, int depth) const {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::vector<Move> moves = board.get_valid_moves();
    sort_moves(moves, board);

    int best_eval = board.is_white_turn() ? INT_MIN : INT_MAX;
    std::vector<Move> best_moves;

    int alpha = INT_MIN;
    int beta = INT_MAX;
    
    for (Move move : moves) {
        ChessBoard test_board = board;
        test_board.make_move(move);

        int eval = minimax(test_board, 0, depth, alpha, beta, !board.is_white_turn());
        std::cout << ChessBoard::index_to_notation(move.from) 
                    << ChessBoard::index_to_notation(move.to) 
                    << " eval: " << eval << '\n';

        if (board.is_white_turn()) {
            if (eval > best_eval) {
                best_moves = {move};
                best_eval = eval;
            } else if (eval == best_eval) {
                best_moves.push_back(move);
            }
            alpha = std::max(alpha, best_eval);
        } else {
            if (eval < best_eval) {
                best_moves = {move};
                best_eval = eval;
            } else if (eval == best_eval) {
                best_moves.push_back(move);
            }
            beta = std::min(beta, best_eval);
        }
    }

    std::uniform_int_distribution<int> dist(0, best_moves.size() - 1);
    return best_moves[dist(gen)];
}

// PRIVATE
// ==================== recursive search ====================
int AI::minimax(const ChessBoard& board, int depth, int max_depth, int alpha, int beta, bool maximizing) const {
    if (board.is_checkmate()) {
        std::cout << "mate in minimax! depth=" << depth << " maximizing=" << maximizing << std::endl;
        return maximizing ? (-100000 + depth) : (100000 - depth);
    }
    if (board.is_stalemate() || board.is_fifty_moves_rule()) {
        return maximizing ? -depth : depth;
    }
    
    if (depth == max_depth) {
        return quiescence(board, 0, alpha, beta, maximizing);
    }

    std::vector<Move> moves = board.get_valid_moves();
    sort_moves(moves, board);

    if (maximizing) {
        int max_eval = INT_MIN;
        for (Move move : moves) {
            ChessBoard test_board = board;
            test_board.make_move(move);

            int eval = minimax(test_board, depth + 1, max_depth, alpha, beta, false);
            max_eval = std::max(max_eval, eval);

            alpha = std::max(alpha, eval);
            if (beta <= alpha) {
                break;
            }
        }
        return max_eval;
    } else {
        int min_eval = INT_MAX;
        for (Move move : moves) {
            ChessBoard test_board = board;
            test_board.make_move(move);

            int eval = minimax(test_board, depth + 1, max_depth, alpha, beta, true);
            min_eval = std::min(min_eval, eval);

            beta = std::min(beta, eval);
            if (beta <= alpha) {
                break;
            }
        }
        return min_eval;
    }
}
int AI::quiescence(const ChessBoard& board, int depth, int alpha, int beta, bool maximizing) const {
    if (board.is_checkmate()) {
        std::cout << "mate in quiescence! depth=" << depth << " maximizing=" << maximizing << std::endl;
        return maximizing ? (-100000 + depth) : (100000 - depth);
    }
    if (board.is_stalemate() || board.is_fifty_moves_rule()) {
        return maximizing ? -depth : depth;
    }
    if (depth > 10) {
        std::cout << "depth limit exceeded!" << " maximizing=" << maximizing << std::endl;
        return evaluate(board);
    }

    int best_score = evaluate(board);

    if (maximizing) {
        if (best_score >= beta) return best_score;
        alpha = std::max(alpha, best_score);
    } else {
        if (best_score <= alpha) return best_score;
        beta = std::min(beta, best_score);
    }

    std::vector<Move> captures = get_capture_moves(board);
    sort_moves(captures, board);

    for (const Move capture : captures) {
        ChessBoard test_board = board;
        test_board.make_move(capture);

        int score = quiescence(test_board, depth + 1, alpha, beta, !maximizing);

        if (maximizing) {
            if (score >= beta) return score;
            if (score > best_score) best_score = score;
            alpha = std::max(alpha, score);
        } else {
            if (score <= alpha) return score;
            if (score < best_score) best_score = score;
            beta = std::min(beta, score);
        }
    }

    return best_score;
}

// ==================== evaluation function ====================
int AI::evaluate(const ChessBoard& board) const {
    int score = 0;
    bool endspiel = is_endspiel(board);

    std::array<uint64_t, 12> bbs = board.get_bitboards();
    for (int piece_bb = 0; piece_bb < 12; piece_bb++) {
        uint64_t bb = bbs[piece_bb];
        if (bb == 0) continue;

        Piece piece = bb_to_piece[piece_bb];
        while (bb) {
            // 1. material
            score += static_cast<int>(piece);
            // 2. piece-square tables
            int cell = pop_lsb(bb);
            score += evaluate_positional_bonus(piece_bb, cell, endspiel);
        }
    }

    // 3. pawn structure
    uint64_t white_pawns = board.get_pawns(true);
    uint64_t black_pawns = board.get_pawns(false);
    
    score += evaluate_isolated_pawns(white_pawns);
    score -= evaluate_isolated_pawns(black_pawns);

    score += evaluate_doubled_pawns(white_pawns);
    score -= evaluate_doubled_pawns(black_pawns);

    score += evaluate_passed_pawns(white_pawns, black_pawns, board.get_all_pieces(), true);
    score -= evaluate_passed_pawns(black_pawns, white_pawns, board.get_all_pieces(), false);
    
    score += evaluate_pawn_islands(white_pawns);
    score -= evaluate_pawn_islands(black_pawns);
    score += evaluate_backward_pawns(white_pawns);
    score -= evaluate_backward_pawns(black_pawns);
    score += evaluate_connected_pawns(white_pawns);
    score -= evaluate_connected_pawns(black_pawns);
    score += evaluate_pawn_chains(white_pawns);
    score -= evaluate_pawn_chains(black_pawns);
    score += evaluate_open_files(white_pawns);
    score -= evaluate_open_files(black_pawns);

    return score;
}
int AI::evaluate_positional_bonus(int bb_index, int real_index, bool endspiel) const {
    int table_index = bb_index < 6 ? real_index ^ 56 : (63 - real_index) ^ 56;
    bool is_white = bb_index < 6;
        switch (bb_index) {
            case 0: case 6: return pawn_table[table_index] * (is_white ? 1 : -1);
            case 1: case 7: return knight_table[table_index] * (is_white ? 1 : -1);
            case 2: case 8: return bishop_table[table_index] * (is_white ? 1 : -1);
            case 3: case 9: return rook_table[table_index] * (is_white ? 1 : -1);
            case 4: case 10: return queen_table[table_index] * (is_white ? 1 : -1);
            case 5: case 11: return (endspiel ? king_eg_table[table_index] : king_mg_table[table_index]) * (is_white ? 1 : -1);
            default: return 0;
        }
}

int AI::evaluate_isolated_pawns(uint64_t pawns) const {
    int penalty = 0;

    for (int file = 0; file < 8; file++) {
        uint64_t pawns_on_file = pawns & files[file];
        if (pawns_on_file == 0) continue;

        bool has_left = (file > 0 && (files[file - 1] & pawns) != 0);
        bool has_right = (file < 7 && (files[file + 1] & pawns) != 0);
        if (!has_left && !has_right) {
            penalty += std::popcount(pawns_on_file) * 20;
        } else if (has_left != has_right) {
            penalty += std::popcount(pawns_on_file) * 10;
        }
    }
    return -penalty;
}
int AI::evaluate_doubled_pawns(uint64_t pawns) const {
    int penalty = 0;

    for (int file = 0; file < 8; file++) {
        uint64_t pawns_on_file = pawns & files[file];
        if (pawns_on_file == 0) continue;

        int pawns_count = std::popcount(pawns_on_file);
        if (pawns_count > 1) {
            penalty += (pawns_count - 1) * 25;
        }
    }
    return -penalty;
}
int AI::evaluate_passed_pawns(uint64_t our_pawns, uint64_t their_pawns, uint64_t all_pieces, bool is_white) const {
    int bonus = 0;
    const int rank_bonus[8] = { 0, 0, 10, 20, 40, 80, 120, 0};
    int color = is_white ? 0 : 1;

    while (our_pawns) {
        int cell = pop_lsb(our_pawns);
        uint64_t path_mask = front_spans[color][cell];

        if ((path_mask & their_pawns) == 0) {
            int rank = cell >> 3;
            int base_bonus = 30;
            int advancement_bonus = is_white ? rank_bonus[rank] : rank_bonus[7 - rank];
            int protection_bonus = 0;

            uint64_t protector_cells = pawn_attacks[!color][cell];
            if (protector_cells & our_pawns) {
                protection_bonus = 20;
            }

            bonus += base_bonus + advancement_bonus + protection_bonus;
        }
    }

    return bonus;
}
int AI::evaluate_pawn_islands(uint64_t pawns) const {
    return 0;
}
int AI::evaluate_backward_pawns(uint64_t pawns) const {
    return 0;
}
int AI::evaluate_connected_pawns(uint64_t pawns) const {
    return 0;
}
int AI::evaluate_pawn_chains(uint64_t pawns) const {
    return 0;
}
int AI::evaluate_open_files(uint64_t pawns) const {
    return 0;
}

// ==================== utilities ====================
void AI::sort_moves(std::vector<Move>& moves, const ChessBoard& board) const {        
    std::vector<std::pair<Move, int>> scored_moves;

    const auto& board_array = board.get_board();
    for (Move move : moves) {
        int score = 0;

        if (board_array[move.to] != Piece::EMPTY) {
            int victim_value = abs(static_cast<int>(board_array[move.to]));
            int aggressor_value = abs(static_cast<int>(board_array[move.from]));
            score = 10000 + victim_value * 10 - aggressor_value;
        } else if (move.is_en_passant) {
            score = 10900;
        }
        if (is_check_move(board, move)) {
            score += 5000;
        }
        if (move.promotion != Piece::EMPTY) {
            score += 4000;
        }

        scored_moves.push_back({ move, score });
    }
    std::sort(scored_moves.begin(), scored_moves.end(), 
                [](auto& a, auto& b) { return a.second > b.second; });
    
    moves.clear();
    for (auto& scored : scored_moves) {
        moves.push_back(scored.first);
    }
}
bool AI::is_check_move(const ChessBoard& board, Move move) const {
    ChessBoard temp_board = board;
    temp_board.make_move(move);
    return temp_board.is_check();
}
std::vector<Move> AI::get_capture_moves(const ChessBoard& board) const {
    std::vector<Move> captures;

    const auto& board_array = board.get_board();
    for (Move move : board.get_valid_moves()) {
        if (board_array[move.to] != Piece::EMPTY || move.is_en_passant) {
            captures.push_back(move);
        }
    }
    return captures;
}
bool AI::is_endspiel(const ChessBoard& board) const {
    int piece_count = 0;
    int material = 0;

    for (Piece piece : board.get_board()) {
        if (piece != Piece::EMPTY && !ChessBoard::is_king(piece)) {
            piece_count++;
            material += abs(static_cast<int>(piece));
        }
    }
    return piece_count < 10 || material < 1600;
}
int AI::pop_lsb(uint64_t& bb) const {
    int lsb_index = std::countr_zero(bb);
    bb &= bb - 1;
    return lsb_index;
}