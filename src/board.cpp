#include "board.h"
#include "attack_tables.h"
#include "zobrist.h"
#include <algorithm>
#include <sstream>

// PUBLIC

Board::Board(bool standard) {
    if (standard) {
        standard_setup();
        white_king_square = 4;
        black_king_square = 60;
        castle_rights |= WK | WQ;
        castle_rights |= BK | BQ;
    }
    key = compute_zobrist();
}

UnmakeInfo Board::make_move(Move move) {
    UnmakeInfo info;
    info.en_passant_square = en_passant_square;
    info.white_king_square = white_king_square;
    info.black_king_square = black_king_square;
    info.castle_rights = castle_rights;
    info.halfmove_clock = halfmove_clock;
    info.fullmove_clock = fullmove_clock;

    update_bitboards(move);
    update_mailbox(move);
    update_state(move);

    key ^= Zobrist::castle[info.castle_rights];
    key ^= Zobrist::castle[castle_rights];
    key ^= Zobrist::en_passant[(info.en_passant_square == -1) ? 16 : (info.en_passant_square - 24)];
    key ^= Zobrist::en_passant[(en_passant_square == -1) ? 16 : (en_passant_square - 24)];
    key ^= Zobrist::turn;

    cached_pinned_valid = false;
    return info;
}
UnmakeInfo Board::make_move(const std::string& str_move) {
    Move move;
    if (!parse_move(str_move, move)) return UnmakeInfo{};

    MoveList list;
    generate_moves(list);
    for (Move valid_move : list) {
        if (move == valid_move) {
            UnmakeInfo info = make_move(move);
            move_stack.push({move, info});
            return info;
        }
    }
    return UnmakeInfo{};
}
void Board::unmake_move(Move move, const UnmakeInfo& info) {
    key ^= Zobrist::castle[castle_rights];
    key ^= Zobrist::castle[info.castle_rights];
    key ^= Zobrist::en_passant[(en_passant_square == -1) ? 16 : (en_passant_square - 24)];
    key ^= Zobrist::en_passant[(info.en_passant_square == -1) ? 16 : (info.en_passant_square - 24)];
    key ^= Zobrist::turn;

    en_passant_square = info.en_passant_square;
    white_king_square = info.white_king_square;
    black_king_square = info.black_king_square;
    castle_rights = info.castle_rights;
    halfmove_clock = info.halfmove_clock;
    fullmove_clock = info.fullmove_clock;
    white_turn_ = !white_turn_;

    update_bitboards(move);
    restore_mailbox(move);
    cached_pinned_valid = false;
}

UnmakeInfo Board::make_null_move() {
    UnmakeInfo info;
    info.en_passant_square = en_passant_square;
    info.halfmove_clock = halfmove_clock;
    info.fullmove_clock = fullmove_clock;

    if (en_passant_square != -1) {
        key ^= Zobrist::en_passant[16];
        key ^= Zobrist::en_passant[en_passant_square - 24];
    }
    en_passant_square = -1;

    halfmove_clock++;
    if (!white_turn_) fullmove_clock++;

    key ^= Zobrist::turn;
    white_turn_ = !white_turn_;

    return info;
}
void Board::unmake_null_move(const UnmakeInfo& info) {
    if (info.en_passant_square != -1) {
        key ^= Zobrist::en_passant[16];
        key ^= Zobrist::en_passant[en_passant_square - 24];
    }
    key ^= Zobrist::turn;

    en_passant_square = info.en_passant_square;
    halfmove_clock = info.halfmove_clock;
    fullmove_clock = info.fullmove_clock;
    white_turn_ = !white_turn_;
}

void Board::generate_moves(MoveList& list) const {
    list.clear();
    int king_square = white_turn_ ? white_king_square : black_king_square;
    uint64_t occupancy = get_occupancy();
    uint64_t our_pieces = get_current_player_pieces();
    uint64_t pinned = get_pinned(white_turn_, occupancy);
    uint64_t checkers = get_checkers(occupancy);
    uint64_t occupancy_without_king = occupancy ^ (1ULL << king_square);

    uint64_t king_moves = AttackTables::king_attacks(king_square) & ~our_pieces;
    while (king_moves) {
        int target = pop_lsb(king_moves);
        Piece moving_piece = piece_at(king_square);
        Piece target_piece = piece_at(target);
        if (!is_square_attacked(target, !white_turn_, occupancy_without_king)) {
            list.add(Move{king_square, target, EMPTY, false, false, moving_piece, target_piece});
        }
    }
    if (checkers == 0) {
        Piece moving_piece = piece_at(king_square);
        if (can_castle(king_square, king_square + 2, occupancy))
            list.add(Move{king_square, king_square + 2, EMPTY, false, true, moving_piece, EMPTY});
        if (can_castle(king_square, king_square - 2, occupancy))
            list.add(Move{king_square, king_square - 2, EMPTY, false, true, moving_piece, EMPTY});
    }

    if ((checkers & (checkers - 1)) != 0) return;

    uint64_t evasion_mask = ~0ULL;
    if (checkers != 0) {
        int checker_square = std::countr_zero(checkers);
        evasion_mask = checkers;
        uint64_t enemy_sliders = get_bishops(!white_turn_) | get_rooks(!white_turn_) | get_queens(!white_turn_);
        if (checkers & enemy_sliders) {
            evasion_mask |= AttackTables::between_bb(king_square, checker_square);
        }
    }

    uint64_t other_pieces = our_pieces & ~get_kings(white_turn_);
    while (other_pieces) {
        int square = pop_lsb(other_pieces);
        Piece moving_piece = piece_at(square);
        uint64_t piece_moves = 0;

        switch (moving_piece) {
            case W_PAWN: case B_PAWN: {
                uint64_t enemy_squares = white_turn_ ? get_black_pieces() : get_white_pieces();
                uint64_t empty_squares = get_empty_squares();
                uint64_t attacks = AttackTables::pawn_attacks(square, white_turn_);
                uint64_t pushes = AttackTables::pawn_pushes(square, white_turn_);
                int dir = white_turn_ ? 8 : -8;

                piece_moves |= attacks & enemy_squares;
                if (en_passant_square != -1 && 
                    abs(square - en_passant_square) == 1 &&
                    abs(square % 8 - en_passant_square % 8) == 1) {
                    piece_moves |= (1ULL << (en_passant_square + dir));
                }

                uint64_t push1 = (1ULL << (square + dir));
                if (pushes & push1 & empty_squares) {
                    piece_moves |= push1;
                    if (square / 8 == (white_turn_ ? 1 : 6)) {
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
            Piece target_piece = piece_at(target);

            bool is_promotion = (moving_piece == W_PAWN || moving_piece == B_PAWN) && (target / 8 == 7 || target / 8 == 0);
            bool is_ep = (moving_piece == W_PAWN || moving_piece == B_PAWN) &&
                         en_passant_square != -1 &&
                         target == en_passant_square + (white_turn_ ? 8 : -8);

            if (is_ep) {
                Move move{square, target, EMPTY, true, false, moving_piece, piece_at(en_passant_square)};
                uint64_t aftermove_occupancy = (occupancy ^ (1ULL << square) ^ (1ULL << en_passant_square)) | (1ULL << target);
                if (!(AttackTables::bishop_attacks(king_square, aftermove_occupancy) & (get_bishops(!white_turn_) | get_queens(!white_turn_))) && 
                    !(AttackTables::rook_attacks(king_square, aftermove_occupancy) & (get_rooks(!white_turn_) | get_queens(!white_turn_)))) {
                    list.add(move);
                }
                continue;
            }

            if (checkers != 0 && !(evasion_mask & (1ULL << target))) continue;

            if (is_promotion) {
                if (white_turn_) {
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

bool Board::king_in_check() const {
    return get_checkers(get_occupancy()) != 0;
}
GameState Board::get_game_state() const {
    MoveList list;
    generate_moves(list);
    bool in_check = king_in_check();

    if (list.size == 0) {
        if (in_check) {
            return white_turn_ ? GameState::BLACK_WINS : GameState::WHITE_WINS;
        }
        return GameState::DRAW_STALEMATE;
    }

    if (halfmove_clock >= 100) return GameState::DRAW_50MOVE_RULE;
    if (draw_by_insufficient_material()) return GameState::DRAW_INSUFFICIENT_MATERIAL;
    if (false) return GameState::DRAW_REPETITION;

    return GameState::ONGOING;
}

void Board::set_position(const std::string& fen) {
    std::istringstream iss(fen);
    std::string placement, active_color, castle, en_passant, halfmove, fullmove;
    castle = en_passant = "-";
    halfmove = "0"; fullmove = "1";
    
    iss >> placement >> active_color >> castle >> en_passant >> halfmove >> fullmove;

    if (std::ranges::count(placement, '/') != 7 || active_color != "w" && active_color != "b") return;

    for (int i = 0; i < 12; i++) {
        bitboards[i] = 0;
    }
    for (int i = 0; i < 64; i++) {
        square_to_piece[i] = EMPTY;
    }
    white_king_square = black_king_square = -1;
    en_passant_square = -1;
    castle_rights = 0;
    halfmove_clock = 0;
    fullmove_clock = 1;
    
    int rank = 7;
    int file = 0;
    
    for (char c : placement) {
        if (c == '/') {
            rank--;
            file = 0;
            continue;
        }
        
        if (std::isdigit(c)) {
            file += (c - '0');
        } else {
            int square = rank * 8 + file;
            Piece piece = EMPTY;
            
            switch (c) {
                case 'P': piece = W_PAWN; break;
                case 'N': piece = W_KNIGHT; break;
                case 'B': piece = W_BISHOP; break;
                case 'R': piece = W_ROOK; break;
                case 'Q': piece = W_QUEEN; break;
                case 'K': piece = W_KING; break;
                case 'p': piece = B_PAWN; break;
                case 'n': piece = B_KNIGHT; break;
                case 'b': piece = B_BISHOP; break;
                case 'r': piece = B_ROOK; break;
                case 'q': piece = B_QUEEN; break;
                case 'k': piece = B_KING; break;
                default: break;
            }
            
            if (piece != EMPTY) {
                bitboards[piece] |= (1ULL << square);
                square_to_piece[square] = piece;
                
                if (piece == W_KING) {
                    white_king_square = square;
                } else if (piece == B_KING) {
                    black_king_square = square;
                }
            }
            file++;
        }
    }
    
    white_turn_ = (active_color == "w");
    
    if (castle != "-") {
        for (char c : castle) {
            switch (c) {
                case 'K': castle_rights |= WK; break;
                case 'Q': castle_rights |= WQ; break;
                case 'k': castle_rights |= BK; break;
                case 'q': castle_rights |= BQ; break;
            }
        }
    }
    
    if (en_passant == "-") {
        en_passant_square = -1;
    } else {
        en_passant_square = notation_to_square(en_passant) + (white_turn_ ? -8 : 8);
    }
    
    halfmove_clock = std::stoi(halfmove);
    
    int fullmove_num = std::stoi(fullmove);
    fullmove_clock = fullmove_num;
    if (!white_turn_) {
        fullmove_clock++;
    }

    key = compute_zobrist();
}
void Board::set_piece(const std::string& notation, Piece piece) {
    int square = notation_to_square(notation);
    if (piece == W_KING) {
        white_king_square = square;
    } else if (piece == B_KING) {
        black_king_square = square;
    }

    if (piece != EMPTY) {
        bitboards[piece] |= (1ULL << square);
        square_to_piece[square] = piece;
        key ^= Zobrist::pieces[piece][square];
    } else {
        Piece deleted_piece = piece_at(square);
        bitboards[deleted_piece] &= ~(1ULL << square);
        square_to_piece[square] = piece;
        key ^= Zobrist::pieces[deleted_piece][square];
    }
}

std::string Board::to_string(bool flipping) const {
    static const char* symbols[] = {
        "♟", "♞", "♝", "♜", "♛", "♚",
        "♟", "♞", "♝", "♜", "♛", "♚"
    };

    flipping = flipping && !white_turn_;

    std::string out;
    out.reserve(1024);

    if (!flipping) {
        out += "   a  b  c  d  e  f  g  h\n";
        for (int i = 0; i < 8; i++) {
            int rank = 7 - i;
            out += std::to_string(rank + 1) + " ";
            
            for (int file = 0; file < 8; file++) {
                int square = rank * 8 + file;
                Piece piece = piece_at(square);
                bool light = ((square / 8 + square % 8) % 2 == 1);

                out += light ? "\033[48;5;215m" : "\033[48;5;94m";
                out += (piece >= 0 && piece <= 5) ? "\033[97m" : "\033[30m";
                out += (piece != EMPTY ? " " + std::string(symbols[piece]) + " " : "   ");
                out += "\033[0m";
            }
            out += " " + std::to_string(rank + 1) + "\n";
        }
        out += "   a  b  c  d  e  f  g  h";

    } else {
        out += "   h  g  f  e  d  c  b  a\n";
        for (int i = 0; i < 8; i++) {
            int rank = i;
            out += std::to_string(rank + 1) + " ";
            
            for (int file = 7; file >= 0; file--) {
                int square = rank * 8 + file;
                Piece piece = piece_at(square);
                bool light = ((square / 8 + square % 8) % 2 == 1);

                out += light ? "\033[48;5;215m" : "\033[48;5;94m";
                out += (piece >= 0 && piece <= 5) ? "\033[97m" : "\033[30m";
                out += (piece != EMPTY ? " " + std::string(symbols[piece]) + " " : "   ");
                out += "\033[0m";
            }
            out += " " + std::to_string(rank + 1) + "\n";
        }
        out += "   h  g  f  e  d  c  b  a";
    }

    return out;
}
std::string Board::moves_to_string() const {
    MoveList list;
    generate_moves(list);
    if (list.size == 0) return "(нет ходов)";
    
    std::string out;
    out.reserve(list.size * 6);
    for (int i = 0; i < list.size; i++) {
        out += list[i].to_string() + (i + 1 < list.size ? " " : "");
    }
    return out;
}

// PRIVATE

void Board::standard_setup() {
    for (int i = 8; i < 16; i++) bitboards[W_PAWN] |= (1ULL << i);
    bitboards[W_KNIGHT] |= (1ULL << 1); bitboards[W_KNIGHT] |= (1ULL << 6);
    bitboards[W_BISHOP] |= (1ULL << 2); bitboards[W_BISHOP] |= (1ULL << 5);
    bitboards[W_ROOK] |= (1ULL << 0); bitboards[W_ROOK] |= (1ULL << 7);
    bitboards[W_QUEEN] |= (1ULL << 3);
    bitboards[W_KING] |= (1ULL << 4);
    for (int i = 48; i < 56; i++) bitboards[B_PAWN] |= (1ULL << i);
    bitboards[B_KNIGHT] |= (1ULL << 57); bitboards[B_KNIGHT] |= (1ULL << 62);
    bitboards[B_BISHOP] |= (1ULL << 58); bitboards[B_BISHOP] |= (1ULL << 61);
    bitboards[B_ROOK] |= (1ULL << 56); bitboards[B_ROOK] |= (1ULL << 63);
    bitboards[B_QUEEN] |= (1ULL << 59);
    bitboards[B_KING] |= (1ULL << 60);

    for (int i = 0; i < 16; i++) square_to_piece[i] = W_PAWN;
    square_to_piece[1] = W_KNIGHT; square_to_piece[6] = W_KNIGHT;
    square_to_piece[2] = W_BISHOP; square_to_piece[5] = W_BISHOP;
    square_to_piece[0] = W_ROOK; square_to_piece[7] = W_ROOK;
    square_to_piece[3] = W_QUEEN;
    square_to_piece[4] = W_KING;
    for (int i = 16; i < 48; i++) square_to_piece[i] = EMPTY;
    for (int i = 48; i < 56; i++) square_to_piece[i] = B_PAWN;
    square_to_piece[57] = B_KNIGHT; square_to_piece[62] = B_KNIGHT;
    square_to_piece[58] = B_BISHOP; square_to_piece[61] = B_BISHOP;
    square_to_piece[56] = B_ROOK; square_to_piece[63] = B_ROOK;
    square_to_piece[59] = B_QUEEN;
    square_to_piece[60] = B_KING;
}

void Board::update_bitboards(Move move) {
    uint8_t from = move.from(), to = move.to();
    Piece moving_piece = move.moving_piece();
    Piece target_piece = move.target_piece();
    if (move.is_en_passant()) {
        Piece captured_pawn = white_turn_ ? B_PAWN : W_PAWN;
        bitboards[moving_piece] ^= (1ULL << from) | (1ULL << to);
        bitboards[captured_pawn] ^= (1ULL << en_passant_square);
        key ^= Zobrist::pieces[moving_piece][from] ^ Zobrist::pieces[moving_piece][to];
        key ^= Zobrist::pieces[captured_pawn][en_passant_square];

    } else if (move.is_castling()) {
        int rook_from, rook_to;
        if (from < to) {
            rook_from = from + 3;
            rook_to = from + 1;
        } else {
            rook_from = from - 4;
            rook_to = from - 1;
        }
        Piece rook = white_turn_ ? W_ROOK : B_ROOK;
        bitboards[moving_piece] ^= (1ULL << from) | (1ULL << to);
        bitboards[rook] ^= (1ULL << rook_from) | (1ULL << rook_to);
        key ^= Zobrist::pieces[moving_piece][from] ^ Zobrist::pieces[moving_piece][to];
        key ^= Zobrist::pieces[rook][rook_from] ^ Zobrist::pieces[rook][rook_to];

    } else if (Piece promotion = move.promotion(); promotion != EMPTY) {
        bitboards[moving_piece] ^= (1ULL << from);
        bitboards[promotion] ^= (1ULL << to);
        key ^= Zobrist::pieces[moving_piece][from];
        key ^= Zobrist::pieces[promotion][to];
        if (target_piece != EMPTY) {
            bitboards[target_piece] ^= (1ULL << to);
            key ^= Zobrist::pieces[target_piece][to];
        }

    } else {
        bitboards[moving_piece] ^= (1ULL << from) | (1ULL << to);
        key ^= Zobrist::pieces[moving_piece][from] ^ Zobrist::pieces[moving_piece][to];
        if (target_piece != EMPTY) {
            bitboards[target_piece] ^= (1ULL << to);
            key ^= Zobrist::pieces[target_piece][to];
        }
    }
}
void Board::update_mailbox(Move move) {
    uint8_t from = move.from(), to = move.to();
    Piece moving_piece = move.moving_piece();
    if (move.is_en_passant()) {
        Piece captured_pawn = white_turn_ ? B_PAWN : W_PAWN;
        square_to_piece[from] = EMPTY; square_to_piece[to] = moving_piece;
        square_to_piece[en_passant_square] = EMPTY;

    } else if (move.is_castling()) {
        int rook_from, rook_to;
        if (from < to) {
            rook_from = from + 3;
            rook_to = from + 1;
        } else {
            rook_from = from - 4;
            rook_to = from - 1;
        }
        Piece rook = white_turn_ ? W_ROOK : B_ROOK;
        square_to_piece[from] = EMPTY; square_to_piece[to] = moving_piece;
        square_to_piece[rook_from] = EMPTY; square_to_piece[rook_to] = rook; 

    } else if (Piece promotion = move.promotion(); promotion != EMPTY) {
        square_to_piece[from] = EMPTY;
        square_to_piece[to] = promotion;

    } else {
        square_to_piece[from] = EMPTY; square_to_piece[to] = moving_piece;
    }
}
void Board::restore_mailbox(Move move) {
    uint8_t from = move.from(), to = move.to();
    Piece moving_piece = move.moving_piece();
    Piece target_piece = move.target_piece();
    if (move.is_en_passant()) {
        Piece captured_pawn = white_turn_ ? B_PAWN : W_PAWN;
        square_to_piece[from] = moving_piece; square_to_piece[to] = EMPTY;
        square_to_piece[en_passant_square] = captured_pawn;

    } else if (move.is_castling()) {
        int rook_from, rook_to;
        if (from < to) {
            rook_from = from + 3;
            rook_to = from + 1;
        } else {
            rook_from = from - 4;
            rook_to = from - 1;
        }
        Piece rook = white_turn_ ? W_ROOK : B_ROOK;
        square_to_piece[from] = moving_piece; square_to_piece[to] = EMPTY;
        square_to_piece[rook_from] = rook; square_to_piece[rook_to] = EMPTY; 

    } else if (Piece promotion = move.promotion(); promotion != EMPTY) {
        square_to_piece[from] = moving_piece;
        square_to_piece[to] = target_piece;

    } else {
        square_to_piece[from] = moving_piece; square_to_piece[to] = target_piece;
    }
}
void Board::update_state(Move move) {
    uint8_t from = move.from(), to = move.to();
    Piece moving_piece = move.moving_piece();
    Piece target_piece = move.target_piece();
    if (moving_piece == W_KING) {
        white_king_square = to;
    } else if (moving_piece == B_KING) {
        black_king_square = to;
    }

    if ((moving_piece == W_PAWN || moving_piece == B_PAWN) && abs(from - to) == 16) {
        en_passant_square = to;
    } else {
        en_passant_square = -1;
    }

    if (moving_piece == W_KING) {
        castle_rights &= ~(WK | WQ);
    } else if (moving_piece == B_KING) {
        castle_rights &= ~(BK | BQ);
    }
    
    if (from == 7 || to == 7) castle_rights &= ~WK;
    if (from == 0 || to == 0) castle_rights &= ~WQ;
    if (from == 63 || to == 63) castle_rights &= ~BK;
    if (from == 56 || to == 56) castle_rights &= ~BQ;    

    if (target_piece != EMPTY || moving_piece == W_PAWN || moving_piece == B_PAWN) {
        halfmove_clock = 0;
    } else {
        halfmove_clock++;
    }
    if (!white_turn_) fullmove_clock++;

    white_turn_ = !white_turn_;
}

bool Board::parse_move(const std::string& move_str, Move& move) const {
    if (move_str.length() != 4 && move_str.length() != 5) return false;
    if (!std::ranges::contains("abcdefghABCDEFGH", move_str[0]) || !std::ranges::contains("12345678", move_str[1])) return false;
    if (!std::ranges::contains("abcdefghABCDEFGH", move_str[2]) || !std::ranges::contains("12345678", move_str[3])) return false;

    int from = notation_to_square(move_str.substr(0, 2));
    int to = notation_to_square(move_str.substr(2, 2));
    if (from < 0 || from > 63 || to < 0 || to > 63) return false;

    Piece moving_piece = piece_at(from);
    if (moving_piece == EMPTY) return false;
    
    Piece target_piece = piece_at(to);
    Piece promotion = EMPTY;
    std::string str_promotion = move_str.substr(4, 1);
    switch (str_promotion[0]) {
        case 'n': case 'N': promotion = W_KNIGHT; break;
        case 'b': case 'B': promotion = W_BISHOP; break;
        case 'r': case 'R': promotion = W_ROOK; break;
        case 'q': case 'Q': promotion = W_QUEEN; break;
        default: if (!str_promotion.empty()) return false;
    }

    bool is_ep = (moving_piece == W_PAWN || moving_piece == B_PAWN) && 
                en_passant_square != -1 && 
                to == en_passant_square + (white_turn_ ? 8 : -8) &&
                abs(from % 8 - en_passant_square % 8) == 1;

    bool is_castling = (moving_piece == W_KING || moving_piece == B_KING) && abs(from - to) == 2 && can_castle(from, to, get_occupancy());
    
    move = Move{from, to, promotion, is_ep, is_castling, moving_piece, target_piece};
    return true;
}

bool Board::draw_by_insufficient_material() const {
    bool qrp = bitboards[W_QUEEN] | bitboards[W_ROOK] | bitboards[W_PAWN] | 
               bitboards[B_QUEEN] | bitboards[B_ROOK] | bitboards[B_PAWN];
    if (qrp) return false;

    int w_bishops = std::popcount(bitboards[W_BISHOP]);
    int w_knights = std::popcount(bitboards[W_KNIGHT]);
    int b_bishops = std::popcount(bitboards[B_BISHOP]);
    int b_knights = std::popcount(bitboards[B_KNIGHT]);

    int w_minors = w_bishops + w_knights;
    int b_minors = b_bishops + b_knights;

    if (w_minors == 0 && b_minors <= 1) return true;
    if (b_minors == 0 && w_minors <= 1) return true;

    if (w_bishops == 1 && w_knights == 0 && b_bishops == 1 && b_knights == 0) {
        int w_square = std::countr_zero(bitboards[W_BISHOP]);
        int b_square = std::countr_zero(bitboards[B_BISHOP]);
        bool same_color = ((w_square / 8 + w_square % 8) % 2) == ((b_square / 8 + b_square % 8) % 2);
        return same_color;
    }

    return false;
}

bool Board::can_castle(int king_square, int target, uint64_t occupancy) const {
    if (king_square != 4 && king_square != 60) return false;

    if (target > king_square) {
        Piece rook = piece_at(king_square + 3);
        if (rook != W_ROOK && white_turn_ || rook != B_ROOK && !white_turn_) return false;

        if (target == 6 && !(castle_rights & WK)) return false;
        if (target == 62 && !(castle_rights & BK)) return false;

        uint64_t castling_path = (1ULL << (king_square + 1)) | (1ULL << (king_square + 2));
        if (occupancy & castling_path) return false;

        if (is_square_attacked(king_square + 1, !white_turn_, occupancy) || is_square_attacked(king_square + 2, !white_turn_, occupancy)) return false;
    } else {
        Piece rook = piece_at(king_square - 4);
        if (rook != W_ROOK && white_turn_ || rook != B_ROOK && !white_turn_) return false;

        if (target == 2 && !(castle_rights & WQ)) return false;
        if (target == 58 && !(castle_rights & BQ)) return false;
        
        uint64_t castling_path = (1ULL << (king_square - 1)) | (1ULL << (king_square - 2)) | (1ULL << (king_square - 3));
        if (occupancy & castling_path) return false;
        
        if (is_square_attacked(king_square - 1, !white_turn_, occupancy) || is_square_attacked(king_square - 2, !white_turn_, occupancy)) return false;
    }

    return true;
}
bool Board::is_square_attacked(int square, bool by_white, uint64_t occupancy) const {
    if (AttackTables::bishop_attacks(square, occupancy) & (get_bishops(by_white) | get_queens(by_white))) return true;
    if (AttackTables::rook_attacks(square, occupancy) & (get_rooks(by_white) | get_queens(by_white))) return true;

    if (AttackTables::pawn_attacks(square, !by_white) & get_pawns(by_white)) return true;
    if (AttackTables::knight_attacks(square) & get_knights(by_white)) return true;
    if (AttackTables::king_attacks(square) & get_kings(by_white)) return true;

    return false;
}
uint64_t Board::get_checkers(uint64_t occupancy) const {
    int king_square = white_turn_ ? white_king_square : black_king_square;
    uint64_t checkers = 0;

    checkers |= AttackTables::bishop_attacks(king_square, occupancy) & (get_bishops(!white_turn_) | get_queens(!white_turn_));
    checkers |= AttackTables::rook_attacks(king_square, occupancy) & (get_rooks(!white_turn_) | get_queens(!white_turn_));
    checkers |= AttackTables::pawn_attacks(king_square, white_turn_) & get_pawns(!white_turn_);
    checkers |= AttackTables::knight_attacks(king_square) & get_knights(!white_turn_);

    return checkers;
}
uint64_t Board::get_pinned(bool white, uint64_t occupancy) const {
    if (!cached_pinned_valid) {
        int king_square = white ? white_king_square : black_king_square;
        uint64_t our_pieces = get_current_player_pieces();

        uint64_t bishop_attacks = AttackTables::bishop_attacks(king_square, occupancy);
        uint64_t bishop_blockers_on_ray = bishop_attacks & our_pieces;
        uint64_t bishop_attacks_without_blockers = AttackTables::bishop_attacks(king_square, occupancy ^ bishop_blockers_on_ray);

        uint64_t rook_attacks = AttackTables::rook_attacks(king_square, occupancy);
        uint64_t rook_blockers_on_ray = rook_attacks & our_pieces;
        uint64_t rook_attacks_without_blockers = AttackTables::rook_attacks(king_square, occupancy ^ rook_blockers_on_ray);

        uint64_t pinners = (bishop_attacks ^ bishop_attacks_without_blockers) & (get_bishops(!white) | get_queens(!white)) |
                           (rook_attacks ^ rook_attacks_without_blockers) & (get_rooks(!white) | get_queens(!white));

        uint64_t pinned = 0;
        while (pinners) {
            int pinner_square = pop_lsb(pinners);
            pinned |= AttackTables::between_bb(king_square, pinner_square) & our_pieces;
        }

        cached_pinned = pinned;
        cached_pinned_valid = true;
    }
    return cached_pinned;
}

uint64_t Board::compute_zobrist() const {
    uint64_t key = 0;

    for (int piece = 0; piece < 12; piece++) {
        uint64_t bb = bitboards[piece];
        while (bb) {
            int square = pop_lsb(bb);
            key ^= Zobrist::pieces[piece][square];
        }
    }
    key ^= Zobrist::castle[castle_rights];
    key ^= Zobrist::en_passant[(en_passant_square == -1) ? 16 : (en_passant_square - 24)];
    key ^= (white_turn_ ? Zobrist::turn : 0);

    return key;
}
