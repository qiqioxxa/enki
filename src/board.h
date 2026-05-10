#pragma once

#include "types.h"
#include <array>
#include <bit>
#include <cstdint>
#include <string>


class alignas(64) Board {
    std::array<uint64_t, 12> bitboards_{};
    std::array<Piece, 64> square_to_piece_{};
    bool white_turn_ = true;
    int en_passant_square_ = -1;
    int white_king_square_ = -1;
    int black_king_square_ = -1;
    int castling_rights_ = 0b0000; // KQkq
    int halfmove_clock_ = 0;
    int fullmove_clock_ = 1;
    uint64_t key_ = 0;


public:
    Board(bool standard = true);

    UnmakeInfo make_move(Move move);
    void unmake_move(Move move, const UnmakeInfo& info);

    UnmakeInfo make_null_move();
    void unmake_null_move(const UnmakeInfo& info);

    constexpr uint64_t get_pawns(bool white) const { return bitboards_[white ? W_PAWN : B_PAWN]; }
    constexpr uint64_t get_knights(bool white) const { return bitboards_[white ? W_KNIGHT : B_KNIGHT]; }
    constexpr uint64_t get_bishops(bool white) const { return bitboards_[white ? W_BISHOP : B_BISHOP]; }
    constexpr uint64_t get_rooks(bool white) const { return bitboards_[white ? W_ROOK : B_ROOK]; }
    constexpr uint64_t get_queens(bool white) const { return bitboards_[white ? W_QUEEN : B_QUEEN]; }
    constexpr uint64_t get_king(bool white) const { return bitboards_[white ? W_KING : B_KING]; }
    constexpr uint64_t get_white_pieces() const { return bitboards_[W_PAWN] | bitboards_[W_KNIGHT] | bitboards_[W_BISHOP] | bitboards_[W_ROOK] | bitboards_[W_QUEEN] | bitboards_[W_KING]; }
    constexpr uint64_t get_black_pieces() const { return bitboards_[B_PAWN] | bitboards_[B_KNIGHT] | bitboards_[B_BISHOP] | bitboards_[B_ROOK] | bitboards_[B_QUEEN] | bitboards_[B_KING]; }
    constexpr uint64_t get_current_player_pieces() const { return white_turn_ ? get_white_pieces() : get_black_pieces(); }
    constexpr uint64_t get_occupancy() const { return get_white_pieces() | get_black_pieces(); }
    constexpr uint64_t get_empty_squares() const { return ~get_occupancy(); }
    constexpr const std::array<uint64_t, 12>& get_bitboards() const { return bitboards_; }
    constexpr const std::array<Piece, 64>& get_mailbox() const { return square_to_piece_; }
    constexpr bool white_turn() const { return white_turn_; }
    constexpr int en_passant_square() const { return en_passant_square_; }
    constexpr int king_square(bool white) const { return white ? white_king_square_ : black_king_square_; }
    constexpr int castling_rights() const { return castling_rights_; }
    constexpr int halfmove_clock() const { return halfmove_clock_; }
    constexpr int fullmove_clock() const { return fullmove_clock_; }
    constexpr uint64_t zobrist_key() const { return key_; };
    constexpr Piece piece_at(int square) const { return square_to_piece_[square]; }

    void set_position(const std::string& fen);

    std::string to_string() const;
    std::string to_string_compat() const;
    std::string to_fen() const;

private:
    void standard_setup();

    void update_bitboards(Move move);
    void update_mailbox(Move move);
    void restore_mailbox(Move move);
    void update_state(Move move);
    
    uint64_t compute_zobrist() const;
};
