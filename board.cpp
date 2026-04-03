#include "board.h"

// PUBLIC

ChessBoard::ChessBoard(bool standard) {
    if (!tables_initialized) {
        initialize_attack_tables();
        tables_initialized = true;
    }
    if (standard) {
        standard_setup();
        white_king_square = 4;
        black_king_square = 60;
        white_kingside_castle = white_queenside_castle = true;
        black_kingside_castle = black_queenside_castle = true;
    }
}

UndoInfo ChessBoard::make_move(const Move move) {
    UndoInfo info;
    info.en_passant_square = en_passant_square;
    info.white_king_square = white_king_square;
    info.black_king_square = black_king_square;
    info.castling_rights[0] = white_queenside_castle;
    info.castling_rights[1] = white_kingside_castle;
    info.castling_rights[2] = black_queenside_castle;
    info.castling_rights[3] = black_kingside_castle;
    info.reversible_moves_count = reversible_moves_count;

    info.moving_bb = bb_index_at(move.from());
    info.target_bb = bb_index_at(move.to());
    execute_pseudo_move(move, info.moving_bb, info.target_bb);
    update_flags_and_counters(move, info.moving_bb, info.target_bb);

    return info;
}
bool ChessBoard::make_move(const std::string& str_move) {
    Move move = parse_move(str_move);
    if (!is_pseudo_move(move)) return false;
    if (leaves_king_in_check(move)) return false;
    make_move(move);
    return true;
}
void ChessBoard::unmake_move(const Move move, const UndoInfo& info) {
    en_passant_square = info.en_passant_square;
    white_king_square = info.white_king_square;
    black_king_square = info.black_king_square;
    white_queenside_castle = info.castling_rights[0];
    white_kingside_castle = info.castling_rights[1];
    black_queenside_castle = info.castling_rights[2];
    black_kingside_castle = info.castling_rights[3];
    reversible_moves_count = info.reversible_moves_count;
    total_moves_count--;

    white_turn = !white_turn;

    execute_pseudo_move(move, info.moving_bb, info.target_bb);
}

std::vector<Move> ChessBoard::generate_valid_moves() const {
    std::vector<Move> moves;
    for (Move move : generate_pseudo_moves()) {
        if (!leaves_king_in_check(move)) {
            moves.push_back(move);
        }
    }
    return moves;
}

void ChessBoard::set_turn(bool white) {
    white_turn = white;
}
void ChessBoard::set_piece(const std::string& notation, Piece piece) {
    int square = notation_to_square(notation);
    if (piece == Piece::W_KING) {
        white_king_square = square;
    } else if (piece == Piece::B_KING) {
        black_king_square = square;
    }

    if (piece != Piece::EMPTY) {
        bitboards[piece_to_bb_index(piece)] |= (1ULL << square);
    } else {
        bitboards[bb_index_at(square)] &= ~(1ULL << square);
    }
}

uint64_t ChessBoard::get_pawns(bool white) const {
    return bitboards[white ? 0 : 6];
}
uint64_t ChessBoard::get_knights(bool white) const {
    return bitboards[white ? 1 : 7];
}
uint64_t ChessBoard::get_bishops(bool white) const {
    return bitboards[white ? 2 : 8];
}
uint64_t ChessBoard::get_rooks(bool white) const {
    return bitboards[white ? 3 : 9];
}
uint64_t ChessBoard::get_queens(bool white) const {
    return bitboards[white ? 4 : 10];
}
uint64_t ChessBoard::get_kings(bool white) const {
    return bitboards[white ? 5 : 11];
}
uint64_t ChessBoard::get_white_pieces() const {
    return bitboards[0] | bitboards[1] | bitboards[2] | 
           bitboards[3] | bitboards[4] | bitboards[5];
    }
uint64_t ChessBoard::get_black_pieces() const {
    return bitboards[6] | bitboards[7] | bitboards[8] | 
           bitboards[9] | bitboards[10] | bitboards[11];
}
uint64_t ChessBoard::get_current_player_pieces() const {
    return white_turn ? get_white_pieces() : get_black_pieces();
}
uint64_t ChessBoard::get_occupancy() const {
    return get_white_pieces() | get_black_pieces();
}
uint64_t ChessBoard::get_empty_squares() const {
    return ~get_occupancy();
}
const std::array<uint64_t, 12>& ChessBoard::get_bitboards() const {
        return bitboards;
    }

bool ChessBoard::is_check() const {
    int king_square = white_turn ? white_king_square : black_king_square;
    if (king_square == -1) return false;

    return is_square_attacked(king_square, !white_turn);
}
bool ChessBoard::is_checkmate() const {
    return generate_valid_moves().empty() && is_check();
}
bool ChessBoard::is_stalemate() const {
    return generate_valid_moves().empty() && !is_check();
}
bool ChessBoard::is_fifty_moves_rule() const {
    return reversible_moves_count >= 100;
}
bool ChessBoard::is_game_over() const {
    if (is_checkmate()) {
        if (white_turn) {
            std::cout << "Checkmate! Black wins in " << (total_moves_count + 1) / 2 << " moves!\n";
        } else {
            std::cout << "Checkmate! White wins in " << (total_moves_count + 1) / 2 << " moves!\n";
        }
        return true;
    } else if (is_stalemate()) {
        std::cout << "Stalemate in " << (total_moves_count + 1) / 2 << " moves!\n";
        return true;
    } else if (is_fifty_moves_rule()) {
        std::cout << "Draw! 50 moves rule\n";
        return true;
    }
    return false;
}

bool ChessBoard::is_pawn(Piece piece) {
    return piece == Piece::W_PAWN || piece == Piece::B_PAWN;
}
bool ChessBoard::is_king(Piece piece) {
    return piece == Piece::W_KING || piece == Piece::B_KING;
}
bool ChessBoard::is_rook(Piece piece) {
    return piece == Piece::W_ROOK || piece == Piece::B_ROOK;
}

void ChessBoard::print_board(bool flipping) const {
    std::map<Piece, std::string> symbols = {
        {Piece::W_KING,   "♚"}, {Piece::B_KING,   "♚"},
        {Piece::W_QUEEN,  "♛"}, {Piece::B_QUEEN,  "♛"},
        {Piece::W_ROOK,   "♜"}, {Piece::B_ROOK,   "♜"},
        {Piece::W_BISHOP, "♝"}, {Piece::B_BISHOP, "♝"},
        {Piece::W_KNIGHT, "♞"}, {Piece::B_KNIGHT, "♞"},
        {Piece::W_PAWN,   "♟"}, {Piece::B_PAWN,   "♟"},
        {Piece::EMPTY,    " "}
    };

    if (white_turn || !flipping) {
        std::cout << "   a  b  c  d  e  f  g  h\n";
        for (int i = 0; i < 8; i++) {
            std::cout << 8 - i << " ";
            for (int j = 0; j < 8; j++) {
                int square = (7 - i) * 8 + j;
                Piece piece = piece_at(square);
                int piece_value = static_cast<int>(piece);
                
                if ((i + j) % 2 == 0) {
                    std::cout << "\033[48;5;215m";
                } else {
                    std::cout << "\033[48;5;94m";
                }

                if (piece_value > 0) {
                    std::cout << "\033[97m";
                } else if (piece_value < 0) {
                    std::cout << "\033[30m";
                }
                
                if (piece != Piece::EMPTY) {
                    std::cout << " " << symbols[piece] << " ";
                } else {
                    std::cout << "   ";
                }
                
                std::cout << "\033[0m";
            }
            std::cout << " " << 8 - i << "\n";
        }
        std::cout << "   a  b  c  d  e  f  g  h\n";
    } else {
        std::cout << "   h  g  f  e  d  c  b  a\n";
        for (int i = 0; i < 8; i++) {
            std::cout << i + 1 << " ";
            for (int j = 0; j < 8; j++) {
                int square = i * 8 + (7 - j);
                Piece piece = piece_at(square);
                int piece_value = static_cast<int>(piece);
                
                if ((i + j) % 2 == 0) {
                    std::cout << "\033[48;5;215m";
                } else {
                    std::cout << "\033[48;5;94m";
                }

                if (piece_value > 0) {
                    std::cout << "\033[97m";
                } else if (piece_value < 0) {
                    std::cout << "\033[30m";
                }
                
                if (piece != Piece::EMPTY) {
                    std::cout << " " << symbols[piece] << " ";
                } else {
                    std::cout << "   ";
                }
                
                std::cout << "\033[0m";
            }
            std::cout << " " << i + 1 << "\n";
        }
        std::cout << "   h  g  f  e  d  c  b  a\n";
    }
}
void ChessBoard::print_moves() const {
    std::vector<Move> moves = generate_valid_moves();
    for (const auto& move : moves) {
        std::cout << move.to_string() << " ";
    }
    std::cout << "\n";
}
void ChessBoard::perft_divide(int depth) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<Move> moves = generate_valid_moves();
    uint64_t total_nodes = 0;

    std::cout << "=== perft divide (depth " << depth << ") ===\n";
    
    for (const auto& move : moves) {
        UndoInfo info = make_move(move);
        uint64_t nodes = perft(depth - 1)[0];
        total_nodes += nodes;
        std::cout << move.to_string() << ": " << nodes << "\n";
        unmake_move(move, info);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << "total: " << total_nodes << " (" << duration << " ms)\n";
}
std::array<uint64_t, 7> ChessBoard::perft(int depth) {
    if (depth == 0) return {1, 0, 0, 0, 0, 0, 0};

    // nodes, captures, en_passants, castles, promotions, checks, checkmates
    std::array<uint64_t, 7> stats = {0, 0, 0, 0, 0, 0, 0};
    std::vector<Move> moves = generate_valid_moves();

    for (const auto& move : moves) {
        UndoInfo info = make_move(move);
        
        stats[1] += info.target_bb != -1 || move.is_en_passant() ? 1 : 0;
        stats[2] += move.is_en_passant() ? 1 : 0;
        stats[3] += move.is_castling() ? 1 : 0;
        stats[4] += move.promotion() ? 1 : 0;
        stats[5] += is_check() ? 1 : 0;
        stats[6] += is_checkmate() ? 1 : 0;

        auto move_stats = perft(depth-1);
        for (int i = 0; i < 7; i++) {
            stats[i] += move_stats[i];
        }

        unmake_move(move, info);
    }

    return stats;
}

int ChessBoard::notation_to_square(const std::string& notation) {
    return ((notation[1] - '0') - 1) * 8 + std::tolower(notation[0]) - 'a';
}
std::string ChessBoard::square_to_notation(int square) {
    std::string letter(1, 'a' + square % 8);
    std::string digit = std::to_string(square / 8 + 1);
    return letter + digit;
}
int ChessBoard::pop_lsb(uint64_t& bb) {
    int index = std::countr_zero(bb);
    bb &= bb - 1;
    return index;
}

// PRIVATE

const std::array<uint64_t, 64> ChessBoard::bishop_magics = {
    0xFFEDF9FD7CFCFFFFull, 0xFC0962854A77F576ull, 0x5822022042000000ull, 0x2CA804A100200020ull,
    0x0204042200000900ull, 0x2002121024000002ull, 0xFC0A66C64A7EF576ull, 0x7FFDFDFCBD79FFFFull,
    0xFC0846A64A34FFF6ull, 0xFC087A874A3CF7F6ull, 0x1001080204002100ull, 0x1810080489021800ull,
    0x0062040420010A00ull, 0x5028043004300020ull, 0xFC0864AE59B4FF76ull, 0x3C0860AF4B35FF76ull,
    0x73C01AF56CF4CFFBull, 0x41A01CFAD64AAFFCull, 0x040C0422080A0598ull, 0x4228020082004050ull,
    0x0200800400E00100ull, 0x020B001230021040ull, 0x7C0C028F5B34FF76ull, 0xFC0A028E5AB4DF76ull,
    0x0020208050A42180ull, 0x001004804B280200ull, 0x2048020024040010ull, 0x0102C04004010200ull,
    0x020408204C002010ull, 0x02411100020080C1ull, 0x102A008084042100ull, 0x0941030000A09846ull,
    0x0244100800400200ull, 0x4000901010080696ull, 0x0000280404180020ull, 0x0800042008240100ull,
    0x0220008400088020ull, 0x04020182000904C9ull, 0x0023010400020600ull, 0x0041040020110302ull,
    0xDCEFD9B54BFCC09Full, 0xF95FFA765AFD602Bull, 0x1401210240484800ull, 0x0022244208010080ull,
    0x1105040104000210ull, 0x2040088800C40081ull, 0x43FF9A5CF4CA0C01ull, 0x4BFFCD8E7C587601ull,
    0xFC0FF2865334F576ull, 0xFC0BF6CE5924F576ull, 0x80000B0401040402ull, 0x0020004821880A00ull,
    0x8200002022440100ull, 0x0009431801010068ull, 0xC3FFB7DC36CA8C89ull, 0xC3FF8A54F4CA2C89ull,
    0xFFFFFCFCFD79EDFFull, 0xFC0863FCCB147576ull, 0x040C000022013020ull, 0x2000104000420600ull,
    0x0400000260142410ull, 0x0800633408100500ull, 0xFC087E8E4BB2F736ull, 0x43FF9E4EF4CA2C89ull,
};
const std::array<uint64_t, 64> ChessBoard::rook_magics = {
    0xA180022080400230ull, 0x0040100040022000ull, 0x0080088020001002ull, 0x0080080280841000ull,
    0x4200042010460008ull, 0x04800A0003040080ull, 0x0400110082041008ull, 0x008000A041000880ull,
    0x10138001A080C010ull, 0x0000804008200480ull, 0x00010011012000C0ull, 0x0022004128102200ull,
    0x000200081201200Cull, 0x202A001048460004ull, 0x0081000100420004ull, 0x4000800380004500ull,
    0x0000208002904001ull, 0x0090004040026008ull, 0x0208808010002001ull, 0x2002020020704940ull,
    0x8048010008110005ull, 0x6820808004002200ull, 0x0A80040008023011ull, 0x00B1460000811044ull,
    0x4204400080008EA0ull, 0xB002400180200184ull, 0x2020200080100380ull, 0x0010080080100080ull,
    0x2204080080800400ull, 0x0000A40080360080ull, 0x02040604002810B1ull, 0x008C218600004104ull,
    0x8180004000402000ull, 0x488C402000401001ull, 0x4018A00080801004ull, 0x1230002105001008ull,
    0x8904800800800400ull, 0x0042000C42003810ull, 0x008408110400B012ull, 0x0018086182000401ull,
    0x2240088020C28000ull, 0x001001201040C004ull, 0x0A02008010420020ull, 0x0010003009010060ull,
    0x0004008008008014ull, 0x0080020004008080ull, 0x0282020001008080ull, 0x50000181204A0004ull,
    0x48FFFE99FECFAA00ull, 0x48FFFE99FECFAA00ull, 0x497FFFADFF9C2E00ull, 0x613FFFDDFFCE9200ull,
    0xFFFFFFE9FFE7CE00ull, 0xFFFFFFF5FFF3E600ull, 0x0010301802830400ull, 0x510FFFF5F63C96A0ull,
    0xEBFFFFB9FF9FC526ull, 0x61FFFEDDFEEDAEAEull, 0x53BFFFEDFFDEB1A2ull, 0x127FFFB9FFDFB5F6ull,
    0x411FFFDDFFDBF4D6ull, 0x0801000804000603ull, 0x0003FFEF27EEBE74ull, 0x7645FFFECBFEA79Eull,
};
const std::array<int, 64> ChessBoard::bishop_shifts = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};
const std::array<int, 64> ChessBoard::rook_shifts = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};
std::array<uint64_t, 64> ChessBoard::bishop_masks{};
std::array<uint64_t, 64> ChessBoard::rook_masks{};
std::array<std::array<uint64_t, 64>, 2> ChessBoard::pawn_pushes;
std::array<std::array<uint64_t, 64>, 2> ChessBoard::pawn_attacks;
std::array<uint64_t, 64> ChessBoard::knight_attacks;
std::array<uint64_t, 64> ChessBoard::king_attacks;
std::array<std::array<uint64_t, 512>, 64> ChessBoard::bishop_attacks{};
std::array<std::array<uint64_t, 4096>, 64> ChessBoard::rook_attacks{};

void ChessBoard::standard_setup() {
    for (int i = 8; i < 16; i++) bitboards[0] ^= (1ULL << i);
    bitboards[1] ^= (1ULL << 1); bitboards[1] ^= (1ULL << 6);
    bitboards[2] ^= (1ULL << 2); bitboards[2] ^= (1ULL << 5);
    bitboards[3] ^= (1ULL << 0); bitboards[3] ^= (1ULL << 7);
    bitboards[4] ^= (1ULL << 3);
    bitboards[5] ^= (1ULL << 4);
    for (int i = 48; i < 56; i++) bitboards[6] ^= (1ULL << i);
    bitboards[7] ^= (1ULL << 57); bitboards[7] ^= (1ULL << 62);
    bitboards[8] ^= (1ULL << 58); bitboards[8] ^= (1ULL << 61);
    bitboards[9] ^= (1ULL << 56); bitboards[9] ^= (1ULL << 63);
    bitboards[10] ^= (1ULL << 59);
    bitboards[11] ^= (1ULL << 60);
}
void ChessBoard::initialize_masks() {
    for (int square = 0; square < 64; square++) {
        uint64_t bishop_mask = 0, rook_mask = 0;
        int rank = square / 8, file = square % 8;
        
        for (int r = rank + 1, f = file + 1; r < 7 && f < 7; r++, f++) bishop_mask |= (1ULL << (r * 8 + f));
        for (int r = rank - 1, f = file - 1; r > 0 && f > 0; r--, f--) bishop_mask |= (1ULL << (r * 8 + f));
        for (int r = rank + 1, f = file - 1; r < 7 && f > 0; r++, f--) bishop_mask |= (1ULL << (r * 8 + f));
        for (int r = rank - 1, f = file + 1; r > 0 && f < 7; r--, f++) bishop_mask |= (1ULL << (r * 8 + f));
        bishop_masks[square] = bishop_mask;
        
        for (int r = rank + 1; r < 7; r++) rook_mask |= (1ULL << (r * 8 + file));
        for (int r = rank - 1; r > 0; r--) rook_mask |= (1ULL << (r * 8 + file));
        for (int f = file + 1; f < 7; f++) rook_mask |= (1ULL << (rank * 8 + f));
        for (int f = file - 1; f > 0; f--) rook_mask |= (1ULL << (rank * 8 + f));
        rook_masks[square] = rook_mask;
    }
}
void ChessBoard::initialize_attack_tables() {
    for (int square = 0; square < 64; square++) {
        pawn_pushes[0][square] = compute_pawn_pushes_from(square, true);
        pawn_pushes[1][square] = compute_pawn_pushes_from(square, false);
        pawn_attacks[0][square] = compute_pawn_attacks_from(square, true);
        pawn_attacks[1][square] = compute_pawn_attacks_from(square, false);
        knight_attacks[square] = compute_knight_attacks_from(square);
        king_attacks[square] = compute_king_attacks_from(square);
    }

    initialize_masks();

    const std::array<std::pair<int, int>, 4> bishop_dirs = {{ {-1, -1}, {-1, 1}, {1, -1}, {1, 1} }};
    const std::array<std::pair<int, int>, 4> rook_dirs = {{ {-1, 0}, {1, 0}, {0, -1}, {0, 1} }};
    
    for (int square = 0; square < 64; ++square) {
        uint64_t mask = bishop_masks[square];
        uint64_t cur_blockers = 0;
        
        do {
            uint64_t index = (cur_blockers * bishop_magics[square]) >> (64 - bishop_shifts[square]);
            bishop_attacks[square][index] = compute_sliding_attacks_from(square, bishop_dirs, cur_blockers);
            cur_blockers = (cur_blockers - mask) & mask;
        } while (cur_blockers != 0);
        
        mask = rook_masks[square];
        cur_blockers = 0;
        
        do {
            uint64_t index = (cur_blockers * rook_magics[square]) >> (64 - rook_shifts[square]);
            rook_attacks[square][index] = compute_sliding_attacks_from(square, rook_dirs, cur_blockers);
            cur_blockers = (cur_blockers - mask) & mask;
        } while (cur_blockers != 0);
    }
}
uint64_t ChessBoard::compute_pawn_pushes_from(int square, bool white) {
    uint64_t pushes = 0;
    int rank = square / 8;
    int file = square % 8;
    for (int target = 0; target < 64; target++) {
        if (target == square) continue;
        int rank_diff = target / 8 - rank;
        int file_diff = target % 8 - file;
        if (white) {
            if (rank_diff == 1 && file_diff == 0) {
                pushes |= (1ULL << target);
            }
            if (rank == 1 && rank_diff == 2 && file_diff == 0) {
                pushes |= (1ULL << target);
            }
        } else {
            if (rank_diff == -1 && file_diff == 0) {
                pushes |= (1ULL << target);
            }
            if (rank == 6 && rank_diff == -2 && file_diff == 0) {
                pushes |= (1ULL << target);
            }
        }
    }
    return pushes;
}
uint64_t ChessBoard::compute_pawn_attacks_from(int square, bool white) {
    uint64_t attacks = 0;
    int rank = square / 8;
    int file = square % 8;
    for (int target = 0; target < 64; target++) {
        if (target == square) continue;
        int rank_diff = target / 8 - rank;
        int file_diff = target % 8 - file;
        if (white) {
            if ((rank_diff == 1 && file_diff == 1) || (rank_diff == 1 && file_diff == -1)) {
                attacks |= (1ULL << target);
            }
        } else {
            if ((rank_diff == -1 && file_diff == 1) || (rank_diff == -1 && file_diff == -1)) {
                attacks |= (1ULL << target);
            }
        }
    }
    return attacks;
}
uint64_t ChessBoard::compute_knight_attacks_from(int square) {
    uint64_t attacks = 0;
    int rank = square / 8;
    int file = square % 8;
    for (int target = 0; target < 64; target++) {
        if (target == square) continue;
        int rank_diff = abs(target / 8 - rank);
        int file_diff = abs(target % 8 - file);
        if ((rank_diff == 1 && file_diff == 2) || (rank_diff == 2 && file_diff == 1)) {
            attacks |= (1ULL << target);
        }
    }
    return attacks;
}
uint64_t ChessBoard::compute_king_attacks_from(int square) {
    uint64_t attacks = 0;
    int rank = square / 8;
    int file = square % 8;
    for (int target = 0; target < 64; target++) {
        if (target == square) continue;
        int rank_diff = abs(target / 8 - rank);
        int file_diff = abs(target % 8 - file);
        if (rank_diff <= 1 && file_diff <= 1) {
            attacks |= (1ULL << target);
        }
    }
    return attacks;
}
uint64_t ChessBoard::compute_sliding_attacks_from(int square, const std::array<std::pair<int, int>, 4>& directions, uint64_t blockers) {
    uint64_t attacks = 0;
    int rank = square / 8;
    int file = square % 8;
    for (const auto& dir : directions) {
        int r = rank;
        int f = file;
        while (true) {
            r += dir.first;
            f += dir.second;
            if (r < 0 || r > 7 || f < 0 || f > 7) break;
            attacks |= (1ULL << (r * 8 + f));
            if (blockers & (1ULL << (r * 8 + f))) break;
        }
    }
    return attacks;
}

void ChessBoard::execute_pseudo_move(const Move move, int moving_bb, int target_bb) const {
    uint8_t from = move.from(), to = move.to();
    if (move.is_en_passant()) {
        int captured_pawn_bb = white_turn ? 6 : 0;
        bitboards[moving_bb] ^= (1ULL << from) | (1ULL << to);
        bitboards[captured_pawn_bb] ^= (1ULL << en_passant_square);

    } else if (move.is_castling()) {
        int rook_from, rook_to;
        if (from < to) {
            rook_from = from + 3;
            rook_to = from + 1;
        } else {
            rook_from = from - 4;
            rook_to = to - 1;
        }
        int rook_bb = white_turn ? 3 : 9;
        bitboards[moving_bb] ^= (1ULL << from) | (1ULL << to);
        bitboards[rook_bb] ^= (1ULL << rook_from) | (1ULL << rook_to);

    } else if (move.promotion() != 0) {
        int promo_bb = piece_to_bb_index(move.promotion(white_turn));
        bitboards[moving_bb] ^= (1ULL << from);
        bitboards[promo_bb] ^= (1ULL << to);
        if (target_bb != -1) {
            bitboards[target_bb] ^= (1ULL << to);
        }

    } else {
        bitboards[moving_bb] ^= (1ULL << from) | (1ULL << to);
        if (target_bb != -1) {
            bitboards[target_bb] ^= (1ULL << to);
        }
    }
}
void ChessBoard::update_flags_and_counters(const Move move, int moving_bb, int target_bb) {
    uint8_t from = move.from(), to = move.to();
    if (moving_bb == 5) {
        white_king_square = to;
    } else if (moving_bb == 11) {
        black_king_square = to;
    }

    if ((moving_bb == 0 || moving_bb == 6) && abs(from - to) == 16) {
        en_passant_square = to;
    } else {
        en_passant_square = -1;
    }

    if (moving_bb == 5) {
        white_queenside_castle = false;
        white_kingside_castle = false;
    } else if (moving_bb == 11) {
        black_queenside_castle = false;
        black_kingside_castle = false;
    }
    
    if (from == 0 || to == 0) white_queenside_castle = false;
    if (from == 7 || to == 7) white_kingside_castle = false;
    if (from == 56 || to == 56) black_queenside_castle = false;
    if (from == 63 || to == 63) black_kingside_castle = false;

    if (target_bb != -1 || moving_bb == 0 || moving_bb == 6) {
        reversible_moves_count = 0;
    } else {
        reversible_moves_count++;
    }
    total_moves_count++;

    white_turn = !white_turn;
}

std::vector<Move> ChessBoard::generate_pseudo_moves() const {
    std::vector<Move> moves;

    uint64_t our_pieces = get_current_player_pieces();
    while(our_pieces) {
        int square = pop_lsb(our_pieces);
        Piece piece = piece_at(square);
        bool is_white = static_cast<int>(piece) > 0;
        uint64_t piece_moves = 0;

        switch (abs(static_cast<int>(piece))) {
            case static_cast<int>(Piece::W_PAWN):
                piece_moves = generate_pawn_moves_from(square, white_turn);
                break;
            case static_cast<int>(Piece::W_KNIGHT):
                piece_moves = generate_knight_moves_from(square, white_turn);
                break;
            case static_cast<int>(Piece::W_BISHOP):
                piece_moves = generate_bishop_moves_from(square, white_turn);
                break;
            case static_cast<int>(Piece::W_ROOK):
                piece_moves = generate_rook_moves_from(square, white_turn);
                break;
            case static_cast<int>(Piece::W_QUEEN):
                piece_moves = generate_queen_moves_from(square, white_turn);
                break;
            case static_cast<int>(Piece::W_KING):
                piece_moves = generate_king_moves_from(square, white_turn);
                break;
            default:
                break;
        }

        while (piece_moves) {
            int target = pop_lsb(piece_moves);
            if (is_king(piece_at(target))) continue;

            bool is_promotion = is_pawn(piece) && (target / 8 == 7 || target / 8 == 0);

            bool is_ep = is_pawn(piece) && 
                        en_passant_square != -1 && 
                        target == en_passant_square + (white_turn ? 8 : -8) &&
                        abs(square % 8 - en_passant_square % 8) == 1;

            bool is_castling = is_king(piece) && abs(square - target) == 2;

            if (is_promotion) {
                moves.emplace_back(square, target, 1);
                moves.emplace_back(square, target, 2);
            } else {
                moves.emplace_back(square, target, 0, is_ep, is_castling);
            }
        }
    }
    return moves;
}
uint64_t ChessBoard::generate_pawn_moves_from(int square, bool white) const {
    uint64_t valid_targets = 0;
    uint64_t empty_squares = get_empty_squares();
    
    if (white) {
        uint64_t enemy_squares = get_black_pieces();
        uint64_t attacks = pawn_attacks[0][square];
        uint64_t pushes = pawn_pushes[0][square];

        while (attacks) {
            int target = pop_lsb(attacks);
            valid_targets |= enemy_squares & (1ULL << target);
        }
        if (en_passant_square != -1 && 
            abs(square - en_passant_square) == 1 &&
            abs(square % 8 - en_passant_square % 8) == 1) {
            valid_targets |= (1ULL << (en_passant_square + 8));
        }

        while (pushes) {
            int target = pop_lsb(pushes);
            if (target - square == 8 && (empty_squares & (1ULL << target))) {
                valid_targets |= (1ULL << target);
            } else if (target - square == 16 && (empty_squares & (1ULL << (square + 8))) && (empty_squares & (1ULL << target))) {
                valid_targets |= (1ULL << target);
            }
        }

    } else {
        uint64_t enemy_squares = get_white_pieces();
        uint64_t attacks = pawn_attacks[1][square];
        uint64_t pushes = pawn_pushes[1][square];

        while (attacks) {
            int target = pop_lsb(attacks);
            valid_targets |= enemy_squares & (1ULL << target);
        }
        if (en_passant_square != -1 && 
            abs(square - en_passant_square) == 1 &&
            abs(square % 8 - en_passant_square % 8) == 1) {
            valid_targets |= (1ULL << (en_passant_square - 8));
        }

        while (pushes) {
            int target = pop_lsb(pushes);
            if (target - square == -8 && (empty_squares & (1ULL << target))) {
                valid_targets |= (1ULL << target);
            } else if (target - square == -16 && (empty_squares & (1ULL << (square - 8))) && (empty_squares & (1ULL << target))) {
                valid_targets |= (1ULL << target);
            }
        }
    }

    return valid_targets;
}
uint64_t ChessBoard::generate_knight_moves_from(int square, bool white) const {
    uint64_t valid_targets = knight_attacks[square];
    uint64_t our_pieces = white ? get_white_pieces() : get_black_pieces();
    valid_targets &= ~our_pieces;
    return valid_targets;
}
uint64_t ChessBoard::generate_king_moves_from(int square, bool white) const {
    uint64_t valid_targets = king_attacks[square];
    uint64_t our_pieces = white_turn ? get_white_pieces() : get_black_pieces();
    valid_targets &= ~our_pieces;

    if (white_turn && square == 4) {
        if (white_kingside_castle && can_castle(4, 6)) {
            valid_targets |= (1ULL << 6);
        }
        if (white_queenside_castle && can_castle(4, 2)) {
            valid_targets |= (1ULL << 2);
        }
    } else if (!white_turn && square == 60) {
        if (black_kingside_castle && can_castle(60, 62)) {
            valid_targets |= (1ULL << 62);
        }
        if (black_queenside_castle && can_castle(60, 58)) {
            valid_targets |= (1ULL << 58);
        }
    }

    return valid_targets;
}
uint64_t ChessBoard::generate_bishop_moves_from(int square, bool white) const {
    uint64_t mask = bishop_masks[square];
    uint64_t blockers = get_occupancy() & mask;
    uint64_t index = (blockers * bishop_magics[square]) >> (64 - bishop_shifts[square]);

    uint64_t valid_targets = bishop_attacks[square][index];
    uint64_t our_pieces = white ? get_white_pieces() : get_black_pieces();
    valid_targets &= ~our_pieces;
    return valid_targets;
}
uint64_t ChessBoard::generate_rook_moves_from(int square, bool white) const {
    uint64_t mask = rook_masks[square];
    uint64_t blockers = get_occupancy() & mask;
    uint64_t index = (blockers * rook_magics[square]) >> (64 - rook_shifts[square]);

    uint64_t valid_targets = rook_attacks[square][index];
    uint64_t our_pieces = white ? get_white_pieces() : get_black_pieces();
    valid_targets &= ~our_pieces;
    return valid_targets;
}
uint64_t ChessBoard::generate_queen_moves_from(int square, bool white) const {
    return generate_bishop_moves_from(square, white) | generate_rook_moves_from(square, white);
}

Move ChessBoard::parse_move(const std::string& str_move) const {
    int from = notation_to_square(str_move.substr(0, 2));
    int to = notation_to_square(str_move.substr(2, 2));
    int promotion = 0;

    std::string str_promotion = str_move.substr(4, 1);
    if (std::tolower(str_promotion[0]) == 'q') {
        promotion = 2;
    } else if (std::tolower(str_promotion[0]) == 'k') {
        promotion = 1;
    }

    bool is_ep = is_pawn(piece_at(from)) && 
                en_passant_square != -1 && 
                to == en_passant_square + (white_turn ? 8 : -8) &&
                abs(from % 8 - en_passant_square % 8) == 1;

    bool is_castling = is_king(piece_at(from)) && abs(from - to) == 2 && can_castle(from, to);
    
    return Move(from, to, promotion, is_ep, is_castling);
}
bool ChessBoard::is_pseudo_move(const Move move) const {
    std::vector<Move> moves = generate_pseudo_moves();
    for (const auto& pm : moves) {
        if (pm == move) {
            return true;
        }
    }
    return false;
}
bool ChessBoard::leaves_king_in_check(const Move move) const {
    int moving_bb = bb_index_at(move.from());
    int target_bb = bb_index_at(move.to());
    ChessBoard test_board = *this;
    test_board.execute_pseudo_move(move, moving_bb, target_bb);
    if (moving_bb == 5) {
        test_board.white_king_square = move.to();
    } else if (moving_bb == 11) {
        test_board.black_king_square = move.to();
    }
    test_board.set_turn(white_turn);
    return test_board.is_check();
}

int ChessBoard::bb_index_at(int square) const {
    for (int bb = 0; bb < 12; bb++) {
        if (bitboards[bb] & (1ULL << square)) {
            return bb;
        }
    }
    return -1;
}
Piece ChessBoard::piece_at(int square) const {
    return bb_index_to_piece(bb_index_at(square));
}
int ChessBoard::piece_to_bb_index(Piece piece) const {
    switch (piece) {
        case Piece::W_PAWN: return 0;
        case Piece::W_KNIGHT: return 1;
        case Piece::W_BISHOP: return 2;
        case Piece::W_ROOK: return 3;
        case Piece::W_QUEEN: return 4;
        case Piece::W_KING: return 5;
        case Piece::B_PAWN: return 6;
        case Piece::B_KNIGHT: return 7;
        case Piece::B_BISHOP: return 8;
        case Piece::B_ROOK: return 9;
        case Piece::B_QUEEN: return 10;
        case Piece::B_KING: return 11;
        default: return -1;
    }
}
Piece ChessBoard::bb_index_to_piece(int bb_index) const {
    switch (bb_index) {
        case 0: return Piece::W_PAWN;
        case 1: return Piece::W_KNIGHT;
        case 2: return Piece::W_BISHOP;
        case 3: return Piece::W_ROOK;
        case 4: return Piece::W_QUEEN;
        case 5: return Piece::W_KING;
        case 6: return Piece::B_PAWN;
        case 7: return Piece::B_KNIGHT;
        case 8: return Piece::B_BISHOP;
        case 9: return Piece::B_ROOK;
        case 10: return Piece::B_QUEEN;
        case 11: return Piece::B_KING;
        default: return Piece::EMPTY;
    }
}

bool ChessBoard::is_current_player_piece(Piece piece) const {
    if (white_turn) {
        return static_cast<int>(piece) > 0;
    } else {
        return static_cast<int>(piece) < 0;
    }
}
bool ChessBoard::is_square_attacked(int square, bool by_white) const {
    uint64_t occupied = get_occupancy();

    uint64_t bishop_mask = bishop_masks[square];
    uint64_t bishop_blockers = occupied & bishop_mask;
    uint64_t bishop_index = (bishop_blockers * bishop_magics[square]) >> (64 - bishop_shifts[square]);
    uint64_t bishop_valid_targets = bishop_attacks[square][bishop_index];

    if (bishop_valid_targets & (get_bishops(by_white) | get_queens(by_white))) return true;

    uint64_t rook_mask = rook_masks[square];
    uint64_t rook_blockers = occupied & rook_mask;
    uint64_t rook_index = (rook_blockers * rook_magics[square]) >> (64 - rook_shifts[square]);
    uint64_t rook_valid_targets = rook_attacks[square][rook_index];
    
    if (rook_valid_targets & (get_rooks(by_white) | get_queens(by_white))) return true;
 
    if (pawn_attacks[by_white ? 1 : 0][square] & get_pawns(by_white)) return true;
    if (knight_attacks[square] & get_knights(by_white)) return true;
    if (king_attacks[square] & get_kings(by_white)) return true;
    
    return false;
}
bool ChessBoard::can_castle(int king_square, int target) const {
    if (is_check()) return false;
    if (king_square != 4 && king_square != 60) return false;

    if (target > king_square) {
        int rook_square = king_square + 3;
        if (!is_rook(piece_at(rook_square)) || !is_current_player_piece(piece_at(rook_square))) return false;

        if (target == 6 && !white_kingside_castle) return false;
        if (target == 62 && !black_kingside_castle) return false;

        uint64_t castling_path = (1ULL << (king_square + 1)) | (1ULL << (king_square + 2));
        if (get_occupancy() & castling_path) return false;

        if (is_square_attacked(king_square + 1, !white_turn) || is_square_attacked(king_square + 2, !white_turn)) return false;
    } else {
        int rook_square = king_square - 4;
        if (!is_rook(piece_at(rook_square)) || !is_current_player_piece(piece_at(rook_square))) return false;

        if (target == 2 && !white_queenside_castle) return false;
        if (target == 58 && !black_queenside_castle) return false;
        
        uint64_t castling_path = (1ULL << (king_square - 1)) | (1ULL << (king_square - 2)) | (1ULL << (king_square - 3));
        if (get_occupancy() & castling_path) return false;
        
        if (is_square_attacked(king_square - 1, !white_turn) || is_square_attacked(king_square - 2, !white_turn)) return false;
    }

    return true;
}
