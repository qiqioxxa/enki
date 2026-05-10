#include "movegen.h"
#include "attack_tables.h"
#include "board.h"
#include "types.h"
#include "utils.h"

void MoveGen::generate_moves(const Board& board, MoveList& list) {
    list.clear();
    int king_square = board.king_square(board.white_turn());
    uint64_t occupancy = board.get_occupancy();
    uint64_t our_pieces = board.get_current_player_pieces();
    uint64_t pinned = get_pinned(board, occupancy);
    uint64_t checkers = get_checkers(board, occupancy);
    uint64_t occupancy_without_king = occupancy ^ (1ULL << king_square);

    uint64_t king_moves = AttackTables::king_attacks(king_square) & ~our_pieces;
    while (king_moves) {
        int target = pop_lsb(king_moves);
        Piece moving_piece = board.piece_at(king_square);
        Piece target_piece = board.piece_at(target);
        if (!is_square_attacked(board, target, !board.white_turn(), occupancy_without_king)) {
            list.add(Move{king_square, target, EMPTY, false, false, moving_piece, target_piece});
        }
    }
    if (checkers == 0) {
        Piece moving_piece = board.piece_at(king_square);
        if (can_castle(board, king_square, king_square + 2, occupancy))
            list.add(Move{king_square, king_square + 2, EMPTY, false, true, moving_piece, EMPTY});
        if (can_castle(board, king_square, king_square - 2, occupancy))
            list.add(Move{king_square, king_square - 2, EMPTY, false, true, moving_piece, EMPTY});
    }

    if ((checkers & (checkers - 1)) != 0) return;

    uint64_t evasion_mask = ~0ULL;
    if (checkers != 0) {
        int checker_square = std::countr_zero(checkers);
        evasion_mask = checkers;
        uint64_t enemy_sliders = board.get_bishops(!board.white_turn()) | board.get_rooks(!board.white_turn()) | board.get_queens(!board.white_turn());
        if (checkers & enemy_sliders) {
            evasion_mask |= AttackTables::between_bb(king_square, checker_square);
        }
    }

    uint64_t other_pieces = our_pieces & ~board.get_king(board.white_turn());
    while (other_pieces) {
        int square = pop_lsb(other_pieces);
        Piece moving_piece = board.piece_at(square);
        uint64_t piece_moves = 0;

        switch (moving_piece) {
            case W_PAWN: case B_PAWN: {
                uint64_t enemy_squares = board.white_turn() ? board.get_black_pieces() : board.get_white_pieces();
                uint64_t empty_squares = board.get_empty_squares();
                uint64_t attacks = AttackTables::pawn_attacks(square, board.white_turn());
                uint64_t pushes = AttackTables::pawn_pushes(square, board.white_turn());
                int dir = board.white_turn() ? 8 : -8;

                piece_moves |= attacks & enemy_squares;
                if (board.en_passant_square() != -1 && 
                    abs(square - board.en_passant_square()) == 1 &&
                    abs(square % 8 - board.en_passant_square() % 8) == 1) {
                    piece_moves |= (1ULL << (board.en_passant_square() + dir));
                }

                uint64_t push1 = (1ULL << (square + dir));
                if (pushes & push1 & empty_squares) {
                    piece_moves |= push1;
                    if (square / 8 == (board.white_turn() ? 1 : 6)) {
                        uint64_t push2 = (1ULL << (square + 2*dir));
                        if (pushes & push2 & empty_squares) {
                            piece_moves |= push2;
                        }
                    }
                }
                break;
            }
            case W_KNIGHT: case B_KNIGHT:
                piece_moves = AttackTables::knight_attacks(square) & ~our_pieces;
                break;
            case W_BISHOP: case B_BISHOP:
                piece_moves = AttackTables::bishop_attacks(square, occupancy) & ~our_pieces;
                break;
            case W_ROOK: case B_ROOK:
                piece_moves = AttackTables::rook_attacks(square, occupancy) & ~our_pieces;
                break;
            case W_QUEEN: case B_QUEEN:
                piece_moves = (AttackTables::bishop_attacks(square, occupancy) | AttackTables::rook_attacks(square, occupancy)) & ~our_pieces; 
                break;
            default: std::unreachable();
        }

        if (pinned & (1ULL << square)) {
            piece_moves &= AttackTables::line_bb(square, king_square);
        }

        while (piece_moves) {
            int target = pop_lsb(piece_moves);
            Piece target_piece = board.piece_at(target);

            bool is_promotion = (moving_piece == W_PAWN || moving_piece == B_PAWN) && (target / 8 == 7 || target / 8 == 0);
            bool is_ep = (moving_piece == W_PAWN || moving_piece == B_PAWN) &&
                         board.en_passant_square() != -1 &&
                         target == board.en_passant_square() + (board.white_turn() ? 8 : -8);

            if (is_ep) {
                Move move{square, target, EMPTY, true, false, moving_piece, board.piece_at(board.en_passant_square())};
                uint64_t aftermove_occupancy = (occupancy ^ (1ULL << square) ^ (1ULL << board.en_passant_square())) | (1ULL << target);
                if (!(AttackTables::bishop_attacks(king_square, aftermove_occupancy) & (board.get_bishops(!board.white_turn()) | board.get_queens(!board.white_turn()))) && 
                    !(AttackTables::rook_attacks(king_square, aftermove_occupancy) & (board.get_rooks(!board.white_turn()) | board.get_queens(!board.white_turn())))) {
                    list.add(move);
                }
                continue;
            }

            if (checkers != 0 && !(evasion_mask & (1ULL << target))) continue;

            if (is_promotion) {
                if (board.white_turn()) {
                    list.add(Move{square, target, W_KNIGHT, false, false, moving_piece, target_piece});
                    list.add(Move{square, target, W_BISHOP, false, false, moving_piece, target_piece});
                    list.add(Move{square, target, W_ROOK, false, false, moving_piece, target_piece});
                    list.add(Move{square, target, W_QUEEN, false, false, moving_piece, target_piece});
                } else {
                    list.add(Move{square, target, B_KNIGHT, false, false, moving_piece, target_piece});
                    list.add(Move{square, target, B_BISHOP, false, false, moving_piece, target_piece});
                    list.add(Move{square, target, B_ROOK, false, false, moving_piece, target_piece});
                    list.add(Move{square, target, B_QUEEN, false, false, moving_piece, target_piece});
                }
            } else {
                list.add(Move{square, target, EMPTY, false, false, moving_piece, target_piece});
            }
        }
    }
}

bool MoveGen::can_castle(const Board& board, int king_square, int target, uint64_t occupancy) {
    if (king_square != 4 && king_square != 60) return false;

    if (target > king_square) {
        Piece rook = board.piece_at(king_square + 3);
        if (rook != W_ROOK && board.white_turn() || rook != B_ROOK && !board.white_turn()) return false;

        if (target == 6 && !(board.castling_rights() & WK)) return false;
        if (target == 62 && !(board.castling_rights() & BK)) return false;

        uint64_t castling_path = (1ULL << (king_square + 1)) | (1ULL << (king_square + 2));
        if (occupancy & castling_path) return false;

        if (is_square_attacked(board, king_square + 1, !board.white_turn(), occupancy) || is_square_attacked(board, king_square + 2, !board.white_turn(), occupancy)) return false;
    } else {
        Piece rook = board.piece_at(king_square - 4);
        if (rook != W_ROOK && board.white_turn() || rook != B_ROOK && !board.white_turn()) return false;

        if (target == 2 && !(board.castling_rights() & WQ)) return false;
        if (target == 58 && !(board.castling_rights() & BQ)) return false;
        
        uint64_t castling_path = (1ULL << (king_square - 1)) | (1ULL << (king_square - 2)) | (1ULL << (king_square - 3));
        if (occupancy & castling_path) return false;
        
        if (is_square_attacked(board, king_square - 1, !board.white_turn(), occupancy) || is_square_attacked(board, king_square - 2, !board.white_turn(), occupancy)) return false;
    }

    return true;
}
bool MoveGen::is_square_attacked(const Board& board, int square, bool by_white, uint64_t occupancy) {
    if (AttackTables::bishop_attacks(square, occupancy) & (board.get_bishops(by_white) | board.get_queens(by_white))) return true;
    if (AttackTables::rook_attacks(square, occupancy) & (board.get_rooks(by_white) | board.get_queens(by_white))) return true;

    if (AttackTables::pawn_attacks(square, !by_white) & board.get_pawns(by_white)) return true;
    if (AttackTables::knight_attacks(square) & board.get_knights(by_white)) return true;
    if (AttackTables::king_attacks(square) & board.get_king(by_white)) return true;

    return false;
}
uint64_t MoveGen::get_checkers(const Board& board, uint64_t occupancy) {
    int king_square = board.king_square(board.white_turn());
    uint64_t checkers = 0;

    checkers |= AttackTables::bishop_attacks(king_square, occupancy) & (board.get_bishops(!board.white_turn()) | board.get_queens(!board.white_turn()));
    checkers |= AttackTables::rook_attacks(king_square, occupancy) & (board.get_rooks(!board.white_turn()) | board.get_queens(!board.white_turn()));
    checkers |= AttackTables::pawn_attacks(king_square, board.white_turn()) & board.get_pawns(!board.white_turn());
    checkers |= AttackTables::knight_attacks(king_square) & board.get_knights(!board.white_turn());

    return checkers;
}
uint64_t MoveGen::get_pinned(const Board& board, uint64_t occupancy) {
    int king_square = board.king_square(board.white_turn());
    uint64_t our_pieces = board.get_current_player_pieces();

    uint64_t bishop_attacks = AttackTables::bishop_attacks(king_square, occupancy);
    uint64_t bishop_blockers_on_ray = bishop_attacks & our_pieces;
    uint64_t bishop_attacks_without_blockers = AttackTables::bishop_attacks(king_square, occupancy ^ bishop_blockers_on_ray);

    uint64_t rook_attacks = AttackTables::rook_attacks(king_square, occupancy);
    uint64_t rook_blockers_on_ray = rook_attacks & our_pieces;
    uint64_t rook_attacks_without_blockers = AttackTables::rook_attacks(king_square, occupancy ^ rook_blockers_on_ray);

    uint64_t pinners = (bishop_attacks ^ bishop_attacks_without_blockers) & (board.get_bishops(!board.white_turn()) | board.get_queens(!board.white_turn())) |
                        (rook_attacks ^ rook_attacks_without_blockers) & (board.get_rooks(!board.white_turn()) | board.get_queens(!board.white_turn()));

    uint64_t pinned = 0;
    while (pinners) {
        int pinner_square = pop_lsb(pinners);
        pinned |= AttackTables::between_bb(king_square, pinner_square) & our_pieces;
    }

    return pinned;
}
