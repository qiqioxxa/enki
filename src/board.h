#pragma once

#include "types.h"
#include <array>
#include <bit>
#include <cstdint>
#include <string>
#include <stack>


inline std::stack<std::pair<Move, UnmakeInfo>> move_stack;

constexpr int notation_to_square(const std::string& notation) {
    return ((notation[1] - '0') - 1) * 8 + std::tolower(notation[0]) - 'a';
}
constexpr int pop_lsb(uint64_t& bb) {
    int index = std::countr_zero(bb);
    bb &= bb - 1;
    return index;
}

class alignas(64) Board {
    std::array<uint64_t, 12> bitboards{};
    std::array<Piece, 64> square_to_piece{};
    bool white_turn_ = true;
    int en_passant_square = -1;
    int white_king_square = -1;
    int black_king_square = -1;
    uint8_t castle_rights = 0b0000; // KQkq
    int halfmove_clock = 0;
    int fullmove_clock = 1;
    mutable uint64_t cached_pinned = 0;
    mutable bool cached_pinned_valid = false;
    uint64_t key = 0;


public:
    Board(bool standard = true);

    UnmakeInfo make_move(Move move);
    UnmakeInfo make_move(const std::string& str_move);
    void unmake_move(Move move, const UnmakeInfo& info);

    UnmakeInfo make_null_move();
    void unmake_null_move(const UnmakeInfo& info);

    void generate_moves(MoveList& list) const;

    bool king_in_check() const;
    GameState get_game_state() const;

    constexpr uint64_t get_pawns(bool white) const { return bitboards[white ? W_PAWN : B_PAWN]; }
    constexpr uint64_t get_knights(bool white) const { return bitboards[white ? W_KNIGHT : B_KNIGHT]; }
    constexpr uint64_t get_bishops(bool white) const { return bitboards[white ? W_BISHOP : B_BISHOP]; }
    constexpr uint64_t get_rooks(bool white) const { return bitboards[white ? W_ROOK : B_ROOK]; }
    constexpr uint64_t get_queens(bool white) const { return bitboards[white ? W_QUEEN : B_QUEEN]; }
    constexpr uint64_t get_kings(bool white) const { return bitboards[white ? W_KING : B_KING]; }
    constexpr uint64_t get_white_pieces() const { return bitboards[W_PAWN] | bitboards[W_KNIGHT] | bitboards[W_BISHOP] | bitboards[W_ROOK] | bitboards[W_QUEEN] | bitboards[W_KING]; }
    constexpr uint64_t get_black_pieces() const { return bitboards[B_PAWN] | bitboards[B_KNIGHT] | bitboards[B_BISHOP] | bitboards[B_ROOK] | bitboards[B_QUEEN] | bitboards[B_KING]; }
    constexpr uint64_t get_current_player_pieces() const { return white_turn_ ? get_white_pieces() : get_black_pieces(); }
    constexpr uint64_t get_occupancy() const { return get_white_pieces() | get_black_pieces(); }
    constexpr uint64_t get_empty_squares() const { return ~get_occupancy(); }
    constexpr const std::array<uint64_t, 12>& get_bitboards() const { return bitboards; }
    constexpr bool white_turn() const { return white_turn_; }
    constexpr Piece piece_at(int square) const { return square_to_piece[square]; }
    constexpr uint64_t zobrist_key() const { return key; };

    void set_position(const std::string& fen);
    void set_piece(const std::string& notation, Piece piece);

    std::string to_string(bool flipping = false) const;
    std::string moves_to_string() const;

private:
    void standard_setup();

    void update_bitboards(Move move);
    void update_mailbox(Move move);
    void restore_mailbox(Move move);
    void update_state(Move move);

    bool parse_move(const std::string& move_str, Move& move) const;
    
    bool draw_by_insufficient_material() const;

    bool can_castle(int king_square, int target, uint64_t occupancy) const;
    bool is_square_attacked(int square, bool by_white, uint64_t occupancy) const;
    uint64_t get_checkers(uint64_t occupancy) const;
    uint64_t get_pinned(bool white, uint64_t occupancy) const;

    uint64_t compute_zobrist() const;
};
