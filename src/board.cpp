#include "board.h"
#include "utils.h"
#include "zobrist.h"
#include <algorithm>
#include <sstream>

// PUBLIC

Board::Board(bool standard) {
    if (standard) {
        standard_setup();
        white_king_square_ = 4;
        black_king_square_ = 60;
        castling_rights_ |= WK | WQ;
        castling_rights_ |= BK | BQ;
    }
    key_ = compute_zobrist();
}

UnmakeInfo Board::make_move(Move move) {
    UnmakeInfo info;
    info.en_passant_square = en_passant_square_;
    info.white_king_square = white_king_square_;
    info.black_king_square = black_king_square_;
    info.castling_rights = castling_rights_;
    info.halfmove_clock = halfmove_clock_;

    positions_[game_ply_] = key_;

    update_bitboards(move);
    update_mailbox(move);
    update_state(move);

    key_ ^= Zobrist::castling[info.castling_rights];
    key_ ^= Zobrist::castling[castling_rights_];
    key_ ^= Zobrist::en_passant[(info.en_passant_square == -1) ? 16 : (info.en_passant_square - 24)];
    key_ ^= Zobrist::en_passant[(en_passant_square_ == -1) ? 16 : (en_passant_square_ - 24)];
    key_ ^= Zobrist::turn;

    return info;
}
void Board::unmake_move(Move move, const UnmakeInfo& info) {
    key_ ^= Zobrist::castling[castling_rights_];
    key_ ^= Zobrist::castling[info.castling_rights];
    key_ ^= Zobrist::en_passant[(en_passant_square_ == -1) ? 16 : (en_passant_square_ - 24)];
    key_ ^= Zobrist::en_passant[(info.en_passant_square == -1) ? 16 : (info.en_passant_square - 24)];
    key_ ^= Zobrist::turn;

    en_passant_square_ = info.en_passant_square;
    white_king_square_ = info.white_king_square;
    black_king_square_ = info.black_king_square;
    castling_rights_ = info.castling_rights;
    halfmove_clock_ = info.halfmove_clock;
    game_ply_--;
    white_turn_ = !white_turn_;

    update_bitboards(move);
    restore_mailbox(move);
}

UnmakeInfo Board::make_null_move() {
    UnmakeInfo info;
    info.en_passant_square = en_passant_square_;
    info.halfmove_clock = halfmove_clock_;

    if (en_passant_square_ != -1) {
        key_ ^= Zobrist::en_passant[16];
        key_ ^= Zobrist::en_passant[en_passant_square_ - 24];
    }
    en_passant_square_ = -1;

    halfmove_clock_++;
    game_ply_++;

    key_ ^= Zobrist::turn;
    white_turn_ = !white_turn_;

    return info;
}
void Board::unmake_null_move(const UnmakeInfo& info) {
    if (info.en_passant_square != -1) {
        key_ ^= Zobrist::en_passant[16];
        key_ ^= Zobrist::en_passant[en_passant_square_ - 24];
    }
    key_ ^= Zobrist::turn;

    en_passant_square_ = info.en_passant_square;
    halfmove_clock_ = info.halfmove_clock;
    game_ply_--;
    white_turn_ = !white_turn_;
}

void Board::set_position(const std::string& fen) {
    std::istringstream iss(fen);
    std::string placement, active_color, castling, en_passant, halfmove, fullmove;
    castling = en_passant = "-";
    halfmove = "0"; fullmove = "1";
    
    iss >> placement >> active_color >> castling >> en_passant >> halfmove >> fullmove;

    if (std::ranges::count(placement, '/') != 7 || active_color != "w" && active_color != "b") return;

    for (int i = 0; i < 12; i++) {
        bitboards_[i] = 0;
    }
    for (int i = 0; i < 64; i++) {
        square_to_piece_[i] = EMPTY;
    }
    white_king_square_ = black_king_square_ = -1;
    en_passant_square_ = -1;
    castling_rights_ = 0;
    halfmove_clock_ = 0;
    game_ply_ = 0;
    key_ = 0;
    
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
                bitboards_[piece] |= (1ULL << square);
                square_to_piece_[square] = piece;
                
                if (piece == W_KING) {
                    white_king_square_ = square;
                } else if (piece == B_KING) {
                    black_king_square_ = square;
                }
            }
            file++;
        }
    }
    
    white_turn_ = (active_color == "w");
    
    if (castling != "-") {
        for (char c : castling) {
            switch (c) {
                case 'K': castling_rights_ |= WK; break;
                case 'Q': castling_rights_ |= WQ; break;
                case 'k': castling_rights_ |= BK; break;
                case 'q': castling_rights_ |= BQ; break;
            }
        }
    }
    
    if (en_passant != "-") {
        en_passant_square_ = notation_to_square(en_passant) + (white_turn_ ? -8 : 8);
    }
    
    halfmove_clock_ = std::stoi(halfmove);
    
    game_ply_ = std::stoi(fullmove) * 2 + !white_turn_;

    key_ = compute_zobrist();
}

bool Board::is_repetition() const {
    int start = std::max(0, game_ply_ - halfmove_clock_);
    for (int i = game_ply_ - 2; i >= start; i -= 2) {
        if (positions_[i] == key_) {
            return true;
        }
    }
    return false;
}

std::string Board::to_string() const {
    static const std::string symbols[13] = {
        "♟", "♞", "♝", "♜", "♛", "♚",
        "♟", "♞", "♝", "♜", "♛", "♚",
        " "
    };

    std::string out;
    out.reserve(1024);

    out += "   a  b  c  d  e  f  g  h\n";
    for (int i = 0; i < 8; i++) {
        int rank = 7 - i;
        out += std::to_string(rank + 1) + " ";
        
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            Piece piece = piece_at(square);
            bool light = ((square / 8 + square % 8) % 2 == 1);

            out += light ? "\033[48;5;215m" : "\033[48;5;94m";
            out += (piece <= 5) ? "\033[97m" : "\033[30m";
            out += " " + std::string(symbols[piece]) + " ";
            out += "\033[0m";
        }
        out += " " + std::to_string(rank + 1) + "\n";
    }
    out += "   a  b  c  d  e  f  g  h\n";

    return out;
}
std::string Board::to_string_compat() const {
    static const char symbols[13] = {
        'P', 'N', 'B', 'R', 'Q', 'K',
        'p', 'n', 'b', 'r', 'q', 'k',
        ' '
    };

    std::string out;
    out.reserve(1024);

    out += "    a   b   c   d   e   f   g   h\n";
    out += "  +---+---+---+---+---+---+---+---+\n";
    for (int i = 0; i < 8; i++) {
        int rank = 7 - i;
        out += std::to_string(rank + 1) + " | ";
        
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            Piece piece = piece_at(square);

            out += symbols[piece];
            out += " | ";
        }
        out += std::to_string(rank + 1) + "\n";
        out += "  +---+---+---+---+---+---+---+---+\n";
    }
    out += "    a   b   c   d   e   f   g   h\n";

    return out;
}
std::string Board::to_fen() const {
    static const char symbols[13] = {
        'P', 'N', 'B', 'R', 'Q', 'K',
        'p', 'n', 'b', 'r', 'q', 'k',
        ' '
    };

    std::string fen;
    fen.reserve(128);

    for (int rank = 7; rank >= 0; rank--) {
        int empties = 0;
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            Piece piece = piece_at(square);
            if (piece == EMPTY) {
                empties++;
                continue;
            } else if (empties != 0) {
                fen += std::to_string(empties);
                empties = 0;
            }
            fen += symbols[piece];
        }
        if (empties != 0) {
            fen += std::to_string(empties);
            empties = 0;
        }
        if (rank > 0) fen += '/';
    }

    fen += (white_turn_ ? " w " : " b ");
    
    std::string castling;
    if (castling_rights_ & WK) castling += "K";
    if (castling_rights_ & WQ) castling += "Q";
    if (castling_rights_ & BK) castling += "k";
    if (castling_rights_ & BQ) castling += "q";
    fen += (castling.size() == 0 ? "-" : castling);

    if (en_passant_square_ != -1) {
        int eps_normalized = en_passant_square_ + (white_turn_ ? 8 : -8);
        std::string eps_str = std::string(1, 'a' + (eps_normalized % 8)) + std::to_string(eps_normalized / 8 + 1);
        fen += " " + eps_str + " ";
    } else {
        fen += " - ";
    }

    fen += std::to_string(halfmove_clock_);
    fen += " ";
    fen += std::to_string(game_ply_ / 2);

    return fen;
}

// PRIVATE

void Board::standard_setup() {
    for (int i = 8; i < 16; i++) bitboards_[W_PAWN] |= (1ULL << i);
    bitboards_[W_KNIGHT] |= (1ULL << 1); bitboards_[W_KNIGHT] |= (1ULL << 6);
    bitboards_[W_BISHOP] |= (1ULL << 2); bitboards_[W_BISHOP] |= (1ULL << 5);
    bitboards_[W_ROOK] |= (1ULL << 0); bitboards_[W_ROOK] |= (1ULL << 7);
    bitboards_[W_QUEEN] |= (1ULL << 3);
    bitboards_[W_KING] |= (1ULL << 4);
    for (int i = 48; i < 56; i++) bitboards_[B_PAWN] |= (1ULL << i);
    bitboards_[B_KNIGHT] |= (1ULL << 57); bitboards_[B_KNIGHT] |= (1ULL << 62);
    bitboards_[B_BISHOP] |= (1ULL << 58); bitboards_[B_BISHOP] |= (1ULL << 61);
    bitboards_[B_ROOK] |= (1ULL << 56); bitboards_[B_ROOK] |= (1ULL << 63);
    bitboards_[B_QUEEN] |= (1ULL << 59);
    bitboards_[B_KING] |= (1ULL << 60);

    for (int i = 0; i < 16; i++) square_to_piece_[i] = W_PAWN;
    square_to_piece_[1] = W_KNIGHT; square_to_piece_[6] = W_KNIGHT;
    square_to_piece_[2] = W_BISHOP; square_to_piece_[5] = W_BISHOP;
    square_to_piece_[0] = W_ROOK; square_to_piece_[7] = W_ROOK;
    square_to_piece_[3] = W_QUEEN;
    square_to_piece_[4] = W_KING;
    for (int i = 16; i < 48; i++) square_to_piece_[i] = EMPTY;
    for (int i = 48; i < 56; i++) square_to_piece_[i] = B_PAWN;
    square_to_piece_[57] = B_KNIGHT; square_to_piece_[62] = B_KNIGHT;
    square_to_piece_[58] = B_BISHOP; square_to_piece_[61] = B_BISHOP;
    square_to_piece_[56] = B_ROOK; square_to_piece_[63] = B_ROOK;
    square_to_piece_[59] = B_QUEEN;
    square_to_piece_[60] = B_KING;
}

void Board::update_bitboards(Move move) {
    uint8_t from = move.from(), to = move.to();
    Piece moving_piece = move.moving_piece();
    Piece target_piece = move.target_piece();
    if (move.is_en_passant()) {
        Piece captured_pawn = white_turn_ ? B_PAWN : W_PAWN;
        bitboards_[moving_piece] ^= (1ULL << from) | (1ULL << to);
        bitboards_[captured_pawn] ^= (1ULL << en_passant_square_);
        key_ ^= Zobrist::pieces[moving_piece][from] ^ Zobrist::pieces[moving_piece][to];
        key_ ^= Zobrist::pieces[captured_pawn][en_passant_square_];

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
        bitboards_[moving_piece] ^= (1ULL << from) | (1ULL << to);
        bitboards_[rook] ^= (1ULL << rook_from) | (1ULL << rook_to);
        key_ ^= Zobrist::pieces[moving_piece][from] ^ Zobrist::pieces[moving_piece][to];
        key_ ^= Zobrist::pieces[rook][rook_from] ^ Zobrist::pieces[rook][rook_to];

    } else if (Piece promotion = move.promotion(); promotion != EMPTY) {
        bitboards_[moving_piece] ^= (1ULL << from);
        bitboards_[promotion] ^= (1ULL << to);
        key_ ^= Zobrist::pieces[moving_piece][from];
        key_ ^= Zobrist::pieces[promotion][to];
        if (target_piece != EMPTY) {
            bitboards_[target_piece] ^= (1ULL << to);
            key_ ^= Zobrist::pieces[target_piece][to];
        }

    } else {
        bitboards_[moving_piece] ^= (1ULL << from) | (1ULL << to);
        key_ ^= Zobrist::pieces[moving_piece][from] ^ Zobrist::pieces[moving_piece][to];
        if (target_piece != EMPTY) {
            bitboards_[target_piece] ^= (1ULL << to);
            key_ ^= Zobrist::pieces[target_piece][to];
        }
    }
}
void Board::update_mailbox(Move move) {
    uint8_t from = move.from(), to = move.to();
    Piece moving_piece = move.moving_piece();
    if (move.is_en_passant()) {
        Piece captured_pawn = white_turn_ ? B_PAWN : W_PAWN;
        square_to_piece_[from] = EMPTY; square_to_piece_[to] = moving_piece;
        square_to_piece_[en_passant_square_] = EMPTY;

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
        square_to_piece_[from] = EMPTY; square_to_piece_[to] = moving_piece;
        square_to_piece_[rook_from] = EMPTY; square_to_piece_[rook_to] = rook; 

    } else if (Piece promotion = move.promotion(); promotion != EMPTY) {
        square_to_piece_[from] = EMPTY;
        square_to_piece_[to] = promotion;

    } else {
        square_to_piece_[from] = EMPTY; square_to_piece_[to] = moving_piece;
    }
}
void Board::restore_mailbox(Move move) {
    uint8_t from = move.from(), to = move.to();
    Piece moving_piece = move.moving_piece();
    Piece target_piece = move.target_piece();
    if (move.is_en_passant()) {
        Piece captured_pawn = white_turn_ ? B_PAWN : W_PAWN;
        square_to_piece_[from] = moving_piece; square_to_piece_[to] = EMPTY;
        square_to_piece_[en_passant_square_] = captured_pawn;

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
        square_to_piece_[from] = moving_piece; square_to_piece_[to] = EMPTY;
        square_to_piece_[rook_from] = rook; square_to_piece_[rook_to] = EMPTY; 

    } else if (Piece promotion = move.promotion(); promotion != EMPTY) {
        square_to_piece_[from] = moving_piece;
        square_to_piece_[to] = target_piece;

    } else {
        square_to_piece_[from] = moving_piece; square_to_piece_[to] = target_piece;
    }
}
void Board::update_state(Move move) {
    uint8_t from = move.from(), to = move.to();
    Piece moving_piece = move.moving_piece();
    Piece target_piece = move.target_piece();
    if (moving_piece == W_KING) {
        white_king_square_ = to;
    } else if (moving_piece == B_KING) {
        black_king_square_ = to;
    }

    if ((moving_piece == W_PAWN || moving_piece == B_PAWN) && abs(from - to) == 16) {
        en_passant_square_ = to;
    } else {
        en_passant_square_ = -1;
    }

    if (moving_piece == W_KING) {
        castling_rights_ &= ~(WK | WQ);
    } else if (moving_piece == B_KING) {
        castling_rights_ &= ~(BK | BQ);
    }
    
    if (from == 7 || to == 7) castling_rights_ &= ~WK;
    if (from == 0 || to == 0) castling_rights_ &= ~WQ;
    if (from == 63 || to == 63) castling_rights_ &= ~BK;
    if (from == 56 || to == 56) castling_rights_ &= ~BQ;    

    if (target_piece != EMPTY || moving_piece == W_PAWN || moving_piece == B_PAWN) {
        halfmove_clock_ = 0;
    } else {
        halfmove_clock_++;
    }
    game_ply_++;

    white_turn_ = !white_turn_;
}

uint64_t Board::compute_zobrist() const {
    uint64_t key = 0;

    for (int piece = 0; piece < 12; piece++) {
        uint64_t bb = bitboards_[piece];
        while (bb) {
            int square = pop_lsb(bb);
            key ^= Zobrist::pieces[piece][square];
        }
    }
    key ^= Zobrist::castling[castling_rights_];
    key ^= Zobrist::en_passant[(en_passant_square_ == -1) ? 16 : (en_passant_square_ - 24)];
    key ^= (white_turn_ ? Zobrist::turn : 0);

    return key;
}
