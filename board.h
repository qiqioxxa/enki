#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <sstream>
#include <string>


enum Piece : int8_t {
    EMPTY = -1,
    W_PAWN = 0, W_KNIGHT = 1, W_BISHOP = 2, W_ROOK = 3, W_QUEEN = 4, W_KING = 5,
    B_PAWN = 6, B_KNIGHT = 7, B_BISHOP = 8, B_ROOK = 9, B_QUEEN = 10, B_KING = 11
};
struct Move {
    uint32_t data; // cepppttttttffffff

    Move() : data(0) {}

    Move(int from, int to, Piece promotion = EMPTY, bool ep = false, bool castle = false) {
        data = (from & 0x3F) | 
               ((to & 0x3F) << 6) | 
               ((promotion & 0x7) << 12) | 
               ((ep ? 1 : 0) << 15) | 
               ((castle ? 1 : 0) << 16);
    }

    int from() const { return data & 0x3F; }
    int to() const { return (data >> 6) & 0x3F; }
    int promotion(bool is_white = true) const {
        int promo_code = (data >> 12) & 0x7;
        if (promo_code == 0x7) return EMPTY;
        return is_white ? promo_code : promo_code + 6;
    }
    bool is_en_passant() const { return (data >> 15) & 0x1; }
    bool is_castling() const { return (data >> 16) & 0x1; }

    std::string to_string() const {
        std::string from_str = std::string(1, 'a' + (from() % 8)) + std::to_string(from() / 8 + 1);
        std::string to_str = std::string(1, 'a' + (to() % 8)) + std::to_string(to() / 8 + 1);
        std::string promotion_str = promotion() != EMPTY ? std::string(1, "nbrq"[promotion()-1]) : "";
        return from_str + to_str + promotion_str;
    }

    bool operator==(const Move& other) const {
        return data == other.data;
    }
};
struct UnmakeInfo {
    uint8_t en_passant_square;
    uint8_t white_king_square;
    uint8_t black_king_square;
    bool castling_rights[4];
    uint8_t halfmove_clock;
    Piece moving_piece;
    Piece target_piece;
};
struct MoveList {
    Move moves[256];
    int size = 0;

    inline void add(Move move) { moves[size++] = move; }
    inline void clear() { size = 0; }
    inline Move& operator[](int i) { return moves[i]; }
    inline const Move& operator[](int i) const { return moves[i]; }

    inline Move* begin() { return moves; }
    inline Move* end() { return moves + size; }
    inline const Move* begin() const { return moves; }
    inline const Move* end() const { return moves + size; }
};
enum class GameState : uint8_t {
    ONGOING,
    WHITE_WINS,
    BLACK_WINS,
    DRAW_STALEMATE,
    DRAW_50MOVE_RULE,
    DRAW_INSUFFICIENT_MATERIAL,
    DRAW_REPETITION
};

class Board {
public:
    Board(bool standard = true);

    UnmakeInfo make_move(const Move& move);
    bool make_move(const std::string& str_move);
    void unmake_move(const Move& move, const UnmakeInfo& info);

    void generate_moves(MoveList& list) const;

    bool king_in_check() const;
    GameState get_game_state() const;

    inline uint64_t get_pawns(bool white) const {
        return bitboards[white ? W_PAWN : B_PAWN];
    }
    inline uint64_t get_knights(bool white) const {
        return bitboards[white ? W_KNIGHT : B_KNIGHT];
    }
    inline uint64_t get_bishops(bool white) const {
        return bitboards[white ? W_BISHOP : B_BISHOP];
    }
    inline uint64_t get_rooks(bool white) const {
        return bitboards[white ? W_ROOK : B_ROOK];
    }
    inline uint64_t get_queens(bool white) const {
        return bitboards[white ? W_QUEEN : B_QUEEN];
    }
    inline uint64_t get_kings(bool white) const {
        return bitboards[white ? W_KING : B_KING];
    }
    inline uint64_t get_white_pieces() const {
        return bitboards[W_PAWN] | bitboards[W_KNIGHT] | bitboards[W_BISHOP] | 
               bitboards[W_ROOK] | bitboards[W_QUEEN] | bitboards[W_KING];
        }
    inline uint64_t get_black_pieces() const {
        return bitboards[B_PAWN] | bitboards[B_KNIGHT] | bitboards[B_BISHOP] | 
               bitboards[B_ROOK] | bitboards[B_QUEEN] | bitboards[B_KING];
    }
    inline uint64_t get_current_player_pieces() const {
        return white_turn ? get_white_pieces() : get_black_pieces();
    }
    inline uint64_t get_occupancy() const {
        return get_white_pieces() | get_black_pieces();
    }
    inline uint64_t get_empty_squares() const {
        return ~get_occupancy();
    }
    inline const std::array<uint64_t, 12>& get_bitboards() const {
        return bitboards;
    }

    void set_position(const std::string& fen);
    void set_piece(const std::string& notation, Piece piece);

    std::string to_string(bool flipping = false) const;
    std::string moves_to_string() const;

    static int notation_to_square(const std::string& notation) {
        return ((notation[1] - '0') - 1) * 8 + std::tolower(notation[0]) - 'a';
    }
    static int pop_lsb(uint64_t& bb) {
        int index = std::countr_zero(bb);
        bb &= bb - 1;
        return index;
    }

    Piece piece_at(int square) const;

private:
    void standard_setup();

    void update_position(const Move& move, Piece moving_piece, Piece target_piece);
    void update_state(const Move& move, Piece moving_piece, Piece target_piece);

    bool parse_move(const std::string& move_str, Move& move) const;
    
    bool draw_by_insufficient_material() const;

    bool can_castle(int king_square, int target, uint64_t occupancy) const;
    bool is_square_attacked(int square, bool by_white, uint64_t occupancy) const;
    uint64_t get_checkers(uint64_t occupancy) const;
    uint64_t get_pinned(bool white, uint64_t occupancy) const;


    static inline bool tables_initialized = false;

    std::array<uint64_t, 12> bitboards{};
    bool white_turn = true;
    int en_passant_square = -1;
    int white_king_square = -1;
    int black_king_square = -1;
    bool white_kingside_castle = false;
    bool white_queenside_castle = false;
    bool black_kingside_castle = false;
    bool black_queenside_castle = false;
    int halfmove_clock = 0;
    int fullmove_clock = 1;
    mutable uint64_t cached_pinned = 0;
    mutable bool cached_pinned_valid = false;
};
