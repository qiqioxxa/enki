#pragma once

#include <cstdint>
#include <array>
#include <map>
#include <vector>
#include <string>
#include <iostream>

enum class Piece {
    EMPTY = 0,
    W_PAWN = 100, W_KNIGHT = 320, W_BISHOP = 330, W_ROOK = 500, W_QUEEN = 900, W_KING = 20000,
    B_PAWN = -100, B_KNIGHT = -320, B_BISHOP = -330, B_ROOK = -500, B_QUEEN = -900, B_KING = -20000
};

struct Move {
    uint16_t data; // ceppttttttffffff

    Move(int from, int to, int promotion = 0, bool ep = false, bool castle = false) {
        data = (from & 0x3F) | 
               ((to & 0x3F) << 6) | 
               ((promotion & 0x3) << 12) | 
               ((ep ? 1 : 0) << 14) | 
               ((castle ? 1 : 0) << 15);
    }

    int from() const { return data & 0x3F; }
    int to() const { return (data >> 6) & 0x3F; }
    int promotion() const { return (data >> 12) & 0x3; }
    Piece promotion(bool is_white) const {
        int promo_code = promotion();
        switch (promo_code) {
            case 1: return is_white ? Piece::W_KNIGHT : Piece::B_KNIGHT;
            case 2: return is_white ? Piece::W_QUEEN : Piece::B_QUEEN;
            default: return Piece::EMPTY;
        }
    }
    bool is_en_passant() const { return (data >> 14) & 0x1; }
    bool is_castling() const { return (data >> 15) & 0x1; }

    std::string to_string() const {
        char letter_from = 'a' + (from() % 8);
        char letter_to = 'a' + (to() % 8);
        int digit_from = from() / 8 + 1;
        int digit_to = to() / 8 + 1;

        return std::string(1, letter_from) + std::to_string(digit_from) + 
               std::string(1, letter_to) + std::to_string(digit_to) + 
               (promotion() ? std::string(1, "KQ"[promotion() - 1]) : "");
    }

    bool operator==(const Move other) const {
        return data == other.data;
    }
};

class ChessBoard {
public:
    ChessBoard(bool standard = true);

    bool make_move(const Move move);
    bool make_move(const std::string& str_move);

    std::vector<Move> generate_valid_moves() const;

    void set_turn(bool white);
    void set_piece(const std::string& notation, Piece piece);

    uint64_t get_pawns(bool white) const;
    uint64_t get_knights(bool white) const;
    uint64_t get_bishops(bool white) const;
    uint64_t get_rooks(bool white) const;
    uint64_t get_queens(bool white) const;
    uint64_t get_kings(bool white) const;
    uint64_t get_white_pieces() const;
    uint64_t get_black_pieces() const;
    uint64_t get_current_player_pieces() const;
    uint64_t get_occupancy() const;
    uint64_t get_empty_squares() const;
    const std::array<uint64_t, 12>& get_bitboards() const;

    bool is_check() const;
    bool is_checkmate() const;
    bool is_stalemate() const;
    bool is_fifty_moves_rule() const;
    bool is_game_over() const;

    bool is_pawn(Piece piece) const;
    bool is_king(Piece piece) const;
    bool is_rook(Piece piece) const;

    void print_board(bool flipping = false) const;
    void print_moves() const;
    void perft_divide(int depth) const;
    std::array<uint64_t, 7> perft(int depth) const;

    static int notation_to_square(const std::string& notation);
    static std::string square_to_notation(int square);
    static int pop_lsb(uint64_t& bb);
    
private:
    std::array<uint64_t, 12> bitboards{};
    bool white_turn = true;
    int en_passant_square = -1;
    int white_king_square = -1;
    int black_king_square = -1;
    bool white_queenside_castle = false;
    bool white_kingside_castle = false;
    bool black_queenside_castle = false;
    bool black_kingside_castle = false;
    int reversible_moves_count = 0;
    int total_moves_count = 0;

    static const std::array<uint64_t, 64> bishop_magics;
    static const std::array<uint64_t, 64> rook_magics;
    static const std::array<int, 64> bishop_shifts;
    static const std::array<int, 64> rook_shifts;

    static std::array<uint64_t, 64> bishop_masks;
    static std::array<uint64_t, 64> rook_masks;
    static std::array<std::array<uint64_t, 64>, 2> pawn_pushes;
    static std::array<std::array<uint64_t, 64>, 2> pawn_attacks;
    static std::array<uint64_t, 64> knight_attacks;
    static std::array<uint64_t, 64> king_attacks;
    static std::array<std::array<uint64_t, 512>, 64> bishop_attacks;
    static std::array<std::array<uint64_t, 4096>, 64> rook_attacks;
    static inline bool tables_initialized = false;


    void initialize_bitboards();
    static void initialize_masks();
    static void initialize_attack_tables();
    static uint64_t compute_pawn_pushes_from(int square, bool white);
    static uint64_t compute_pawn_attacks_from(int square, bool white);
    static uint64_t compute_knight_attacks_from(int square);
    static uint64_t compute_king_attacks_from(int square);
    static uint64_t compute_sliding_attacks_from(int square, const std::array<std::pair<int, int>, 4>& directions, uint64_t blockers);

    void execute_pseudo_move(const Move move);
    void update_flags_and_counters(const Move move, Piece moving_piece, Piece target_piece);

    std::vector<Move> generate_pseudo_moves() const;
    uint64_t generate_pawn_moves_from(int square, bool white) const;
    uint64_t generate_knight_moves_from(int square, bool white) const;
    uint64_t generate_bishop_moves_from(int square, bool white) const;
    uint64_t generate_rook_moves_from(int square, bool white) const;
    uint64_t generate_queen_moves_from(int square, bool white) const;
    uint64_t generate_king_moves_from(int square, bool white) const;

    Move parse_move(const std::string& str_move) const;
    bool is_pseudo_move(const Move move) const;
    bool leaves_king_in_check(const Move move) const;

    int bb_index_at(int square) const;
    Piece piece_at(int square) const;
    int piece_to_bb_index(Piece piece) const;
    Piece bb_index_to_piece(int bb_index) const;

    bool is_current_player_piece(Piece piece) const;
    bool is_square_attacked(int square, bool by_white) const;
    bool can_castle(int king_square, int target) const;
};
