#pragma once

#include <cstdint>
#include <string>


enum CastleRights : uint8_t {
    WK = 0b1000, 
    WQ = 0b0100, 
    BK = 0b0010, 
    BQ = 0b0001
};

enum Piece : int8_t {
    W_PAWN = 0, W_KNIGHT = 1, W_BISHOP = 2, W_ROOK = 3, W_QUEEN = 4, W_KING = 5,
    B_PAWN = 6, B_KNIGHT = 7, B_BISHOP = 8, B_ROOK = 9, B_QUEEN = 10, B_KING = 11,
    EMPTY = 12,
};

struct Move {
    uint32_t data; // ttttmmmmceppppttttttffffff

    Move() { data = 0; }

    Move(int from, int to, Piece promotion, bool ep, bool castling, Piece moving_piece, Piece target_piece) {
        data = (from & 0x3F) | 
               ((to & 0x3F) << 6) | 
               ((promotion & 0xF) << 12) | 
               ((ep ? 1 : 0) << 16) | 
               ((castling ? 1 : 0) << 17) | 
               ((moving_piece & 0xF) << 18) | 
               ((target_piece & 0xF) << 22);
    }

    int from() const { return data & 0x3F; }
    int to() const { return (data >> 6) & 0x3F; }
    Piece promotion() const { return static_cast<Piece>((data >> 12) & 0xF); }
    bool is_en_passant() const { return (data >> 16) & 0x1; }
    bool is_castling() const { return (data >> 17) & 0x1; }
    Piece moving_piece() const { return static_cast<Piece>((data >> 18) & 0xF); }
    Piece target_piece() const { return static_cast<Piece>((data >> 22) & 0xF); }

    bool is_null_move() const { return data == 0; }

    std::string to_string() const {
        std::string from_str = std::string(1, 'a' + (from() % 8)) + std::to_string(from() / 8 + 1);
        std::string to_str = std::string(1, 'a' + (to() % 8)) + std::to_string(to() / 8 + 1);
        std::string promotion_str = promotion() != EMPTY ? std::string(1, "nbrq"[static_cast<int>(promotion()) - static_cast<int>(moving_piece()) - 1]) : "";
        return from_str + to_str + promotion_str;
    }

    bool operator==(Move other) const {
        return data == other.data;
    }
};

struct MoveList {
    Move moves[218];
    int size = 0;

    void add(Move move) { moves[size++] = move; }
    void clear() { size = 0; }

    Move& operator[](int i) { return moves[i]; }
    const Move& operator[](int i) const { return moves[i]; }

    Move* begin() { return moves; }
    Move* end() { return moves + size; }
    const Move* begin() const { return moves; }
    const Move* end() const { return moves + size; }
};

struct UnmakeInfo {
    int8_t en_passant_square;
    uint8_t white_king_square;
    uint8_t black_king_square;
    uint8_t castling_rights;
    uint8_t halfmove_clock;
};
