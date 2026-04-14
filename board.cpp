#include "board.h"
#include "attack_tables.h"

// PUBLIC

ChessBoard::ChessBoard(bool standard) {
    if (!tables_initialized) {
        AttackTables::init();
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

UnmakeInfo ChessBoard::make_move(const Move& move) {
    UnmakeInfo info;
    info.en_passant_square = en_passant_square;
    info.white_king_square = white_king_square;
    info.black_king_square = black_king_square;
    info.castling_rights[0] = white_kingside_castle;
    info.castling_rights[1] = white_queenside_castle;
    info.castling_rights[2] = black_kingside_castle;
    info.castling_rights[3] = black_queenside_castle;
    info.reversible_moves_count = reversible_moves_count;

    info.from_bb = bb_index_at(move.from());
    info.to_bb = bb_index_at(move.to());
    execute_pseudo_move(move, info.from_bb, info.to_bb);
    update_flags_and_counters(move, info.from_bb, info.to_bb);

    return info;
}
bool ChessBoard::make_move(const std::string& str_move) {
    Move move = parse_move(str_move);
    if (!is_pseudo_move(move)) return false;
    if (leaves_king_in_check(move)) return false;
    make_move(move);
    return true;
}
void ChessBoard::unmake_move(const Move& move, const UnmakeInfo& info) {
    en_passant_square = info.en_passant_square;
    white_king_square = info.white_king_square;
    black_king_square = info.black_king_square;
    white_kingside_castle = info.castling_rights[0];
    white_queenside_castle = info.castling_rights[1];
    black_kingside_castle = info.castling_rights[2];
    black_queenside_castle = info.castling_rights[3];
    reversible_moves_count = info.reversible_moves_count;
    if (white_turn) {
        total_moves_count--;
    }
    white_turn = !white_turn;

    execute_pseudo_move(move, info.from_bb, info.to_bb);
}

void ChessBoard::generate_valid_moves(MoveList& list) const {
    generate_pseudo_moves(list);
    int valid_count = 0;

    uint64_t checkers = get_checkers();
    if (checkers != 0) {
        for (const Move& move : list) {
            if (!leaves_king_in_check(move)) {
                list[valid_count++] = move;
            }
        }
    } else {
        for (const Move& move : list) {
            if (is_legal_move(move)) {
                list[valid_count++] = move;
            }
        }
    }
    list.size = valid_count;
}

bool ChessBoard::is_check() const {
    int king_square = white_turn ? white_king_square : black_king_square;
    if (king_square == -1) return false;

    return is_square_attacked(king_square, !white_turn, get_occupancy());
}
bool ChessBoard::is_checkmate() const {
    MoveList list;
    generate_valid_moves(list);
    return list.size == 0 && is_check();
}
bool ChessBoard::is_stalemate() const {
    MoveList list;
    generate_valid_moves(list);
    return list.size == 0 && !is_check();
}
bool ChessBoard::is_fifty_moves_rule() const {
    return reversible_moves_count >= 100;
}
bool ChessBoard::is_game_over() const {
    if (is_checkmate()) {
        if (white_turn) {
            std::cout << "Checkmate! Black wins in " << total_moves_count << " moves!\n";
        } else {
            std::cout << "Checkmate! White wins in " << total_moves_count << " moves!\n";
        }
        return true;
    } else if (is_stalemate()) {
        std::cout << "Stalemate in " << total_moves_count << " moves!\n";
        return true;
    } else if (is_fifty_moves_rule()) {
        std::cout << "Draw! 50 moves rule\n";
        return true;
    }
    return false;
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

void ChessBoard::set_position(const std::string& fen) {
    for (int i = 0; i < 12; i++) {
        bitboards[i] = 0;
    }
    white_king_square = black_king_square = -1;
    en_passant_square = -1;
    white_kingside_castle = white_queenside_castle = false;
    black_kingside_castle = black_queenside_castle = false;
    reversible_moves_count = 0;
    total_moves_count = 0;
    
    std::istringstream iss(fen);
    std::string placement, active_color, castling, en_passant, halfmove, fullmove;
    castling = en_passant = "-";
    halfmove = "0"; fullmove = "1";
    
    iss >> placement >> active_color >> castling >> en_passant >> halfmove >> fullmove;
    
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
            Piece piece = Piece::EMPTY;
            
            switch (c) {
                case 'P': piece = Piece::W_PAWN; break;
                case 'N': piece = Piece::W_KNIGHT; break;
                case 'B': piece = Piece::W_BISHOP; break;
                case 'R': piece = Piece::W_ROOK; break;
                case 'Q': piece = Piece::W_QUEEN; break;
                case 'K': piece = Piece::W_KING; break;
                case 'p': piece = Piece::B_PAWN; break;
                case 'n': piece = Piece::B_KNIGHT; break;
                case 'b': piece = Piece::B_BISHOP; break;
                case 'r': piece = Piece::B_ROOK; break;
                case 'q': piece = Piece::B_QUEEN; break;
                case 'k': piece = Piece::B_KING; break;
                default: break;
            }
            
            if (piece != Piece::EMPTY) {
                int bb_index = piece_to_bb_index(piece);
                bitboards[bb_index] |= (1ULL << square);
                
                if (piece == Piece::W_KING) {
                    white_king_square = square;
                } else if (piece == Piece::B_KING) {
                    black_king_square = square;
                }
            }
            file++;
        }
    }
    
    white_turn = (active_color == "w");
    
    if (castling != "-") {
        for (char c : castling) {
            switch (c) {
                case 'K': white_kingside_castle = true; break;
                case 'Q': white_queenside_castle = true; break;
                case 'k': black_kingside_castle = true; break;
                case 'q': black_queenside_castle = true; break;
            }
        }
    }
    
    if (en_passant == "-") {
        en_passant_square = -1;
    } else {
        en_passant_square = notation_to_square(en_passant);
    }
    
    reversible_moves_count = std::stoi(halfmove);
    
    int fullmove_num = std::stoi(fullmove);
    total_moves_count = fullmove_num;
    if (!white_turn) {
        total_moves_count++;
    }
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
void ChessBoard::set_turn(bool white) {
    white_turn = white;
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
    MoveList list;
    generate_valid_moves(list);
    for (const Move& move : list) {
        std::cout << move.to_string() << " ";
    }
    std::cout << "\n";
}

void ChessBoard::perft_divide(int depth) {
    auto start = std::chrono::high_resolution_clock::now();
    MoveList list;
    generate_valid_moves(list);
    uint64_t total_nodes = 0;

    std::cout << "=== perft divide (depth " << depth << ") ===\n";
    
    for (const Move& move : list) {
        UnmakeInfo info = make_move(move);
        uint64_t nodes = perft(depth - 1, false)[0];
        total_nodes += nodes;
        std::cout << move.to_string() << ": " << nodes << "\n";
        unmake_move(move, info);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << "total: " << total_nodes << " (" << duration << " ms)\n";
}
std::array<long long, 7> ChessBoard::perft(int depth, bool all_stats) {
    if (depth == 0) return {1, 0, 0, 0, 0, 0, 0};

    MoveList list;
    generate_valid_moves(list);

    if (depth == 1) {
        if (!all_stats) return {list.size, 0, 0, 0, 0, 0, 0};

        std::array<long long, 7> stats = {list.size, 0, 0, 0, 0, 0, 0};
        for (const Move& move : list) {
            stats[1] += (bb_index_at(move.to()) != -1 || move.is_en_passant()) ? 1 : 0;
            stats[2] += move.is_en_passant() ? 1 : 0;
            stats[3] += move.is_castling() ? 1 : 0;
            stats[4] += move.promotion() ? 1 : 0;

            UnmakeInfo info = make_move(move);
            if (is_check()) {
                stats[5]++;
                MoveList opp_moves;
                generate_valid_moves(opp_moves);
                if (opp_moves.size == 0) stats[6]++;
            }
            unmake_move(move, info);
        }
        return stats;
    }

    std::array<long long, 7> stats = {0, 0, 0, 0, 0, 0, 0};
    for (const Move& move : list) {
        UnmakeInfo info = make_move(move);
        
        if (all_stats) {
            stats[1] += (info.to_bb != -1 || move.is_en_passant()) ? 1 : 0;
            stats[2] += move.is_en_passant() ? 1 : 0;
            stats[3] += move.is_castling() ? 1 : 0;
            stats[4] += move.promotion() ? 1 : 0;
            stats[5] += is_check() ? 1 : 0;
        }
        
        std::array<long long, 7> sub = perft(depth - 1, all_stats);
        for (int i = 0; i < 7; i++) stats[i] += sub[i];

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

void ChessBoard::execute_pseudo_move(const Move& move, int from_bb, int to_bb) {
    uint8_t from = move.from(), to = move.to();
    if (move.is_en_passant()) {
        int captured_pawn_bb = white_turn ? 6 : 0;
        bitboards[from_bb] ^= (1ULL << from) | (1ULL << to);
        bitboards[captured_pawn_bb] ^= (1ULL << en_passant_square);

    } else if (move.is_castling()) {
        int rook_from, rook_to;
        if (from < to) {
            rook_from = from + 3;
            rook_to = from + 1;
        } else {
            rook_from = from - 4;
            rook_to = from - 1;
        }
        int rook_bb = white_turn ? 3 : 9;
        bitboards[from_bb] ^= (1ULL << from) | (1ULL << to);
        bitboards[rook_bb] ^= (1ULL << rook_from) | (1ULL << rook_to);

    } else if (move.promotion() != 0) {
        int promo_bb = piece_to_bb_index(move.promotion(white_turn));
        bitboards[from_bb] ^= (1ULL << from);
        bitboards[promo_bb] ^= (1ULL << to);
        if (to_bb != -1) {
            bitboards[to_bb] ^= (1ULL << to);
        }

    } else {
        bitboards[from_bb] ^= (1ULL << from) | (1ULL << to);
        if (to_bb != -1) {
            bitboards[to_bb] ^= (1ULL << to);
        }
    }
}
void ChessBoard::update_flags_and_counters(const Move& move, int from_bb, int to_bb) {
    uint8_t from = move.from(), to = move.to();
    if (from_bb == 5) {
        white_king_square = to;
    } else if (from_bb == 11) {
        black_king_square = to;
    }

    if ((from_bb == 0 || from_bb == 6) && abs(from - to) == 16) {
        en_passant_square = to;
    } else {
        en_passant_square = -1;
    }

    if (from_bb == 5) {
        white_kingside_castle = false;
        white_queenside_castle = false;
    } else if (from_bb == 11) {
        black_kingside_castle = false;
        black_queenside_castle = false;
    }
    
    if (from == 7 || to == 7) white_kingside_castle = false;
    if (from == 0 || to == 0) white_queenside_castle = false;
    if (from == 63 || to == 63) black_kingside_castle = false;
    if (from == 56 || to == 56) black_queenside_castle = false;

    if (to_bb != -1 || from_bb == 0 || from_bb == 6) {
        reversible_moves_count = 0;
    } else {
        reversible_moves_count++;
    }
    if (!white_turn) {
        total_moves_count++;
    }

    white_turn = !white_turn;
}

void ChessBoard::generate_pseudo_moves(MoveList& list) const {
    list.clear();

    uint64_t occupancy = get_occupancy();
    uint64_t our_pieces = get_current_player_pieces();
    while(our_pieces) {
        int square = pop_lsb(our_pieces);
        int square_bb = bb_index_at(square);
        uint64_t piece_moves = 0;

        switch (square_bb) {
            case 0: case 6:
                piece_moves = generate_pawn_moves_from(square, white_turn);
                break;
            case 1: case 7:
                piece_moves = generate_knight_moves_from(square, white_turn);
                break;
            case 2: case 8:
                piece_moves = generate_bishop_moves_from(square, white_turn, occupancy);
                break;
            case 3: case 9:
                piece_moves = generate_rook_moves_from(square, white_turn, occupancy);
                break;
            case 4: case 10:
                piece_moves = generate_queen_moves_from(square, white_turn, occupancy);
                break;
            case 5: case 11:
                piece_moves = generate_king_moves_from(square, white_turn);
                break;
            default:
                break;
        }

        while (piece_moves) {
            int target = pop_lsb(piece_moves);
            int target_bb = bb_index_at(target);
            if (target_bb == 5 || target_bb == 11) continue;

            bool is_promotion = (square_bb == 0 || square_bb == 6) && (target / 8 == 7 || target / 8 == 0);

            bool is_ep = (square_bb == 0 || square_bb == 6) && 
                        en_passant_square != -1 && 
                        target == en_passant_square + (white_turn ? 8 : -8) &&
                        abs(square % 8 - en_passant_square % 8) == 1;

            bool is_castling = (square_bb == 5 || square_bb == 11) && abs(square - target) == 2;

            if (is_promotion) {
                list.add(Move{square, target, 1});
                list.add(Move{square, target, 2});
                list.add(Move{square, target, 3});
                list.add(Move{square, target, 4});
            } else {
                list.add(Move{square, target, 0, is_ep, is_castling});
            }
        }
    }
}
uint64_t ChessBoard::generate_pawn_moves_from(int square, bool white) const {
    uint64_t valid_targets = 0;
    uint64_t empty_squares = get_empty_squares();
    
    if (white) {
        uint64_t enemy_squares = get_black_pieces();
        uint64_t attacks = AttackTables::pawn_attacks[0][square];
        uint64_t pushes = AttackTables::pawn_pushes[0][square];

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
        uint64_t attacks = AttackTables::pawn_attacks[1][square];
        uint64_t pushes = AttackTables::pawn_pushes[1][square];

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
    return AttackTables::knight_attacks[square] & ~get_current_player_pieces();
}
uint64_t ChessBoard::generate_king_moves_from(int square, bool white) const {
    uint64_t valid_targets = AttackTables::king_attacks[square];
    uint64_t our_pieces = get_current_player_pieces();
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
uint64_t ChessBoard::generate_bishop_moves_from(int square, bool white, uint64_t occupancy) const {
    return AttackTables::bishop_attacks(square, occupancy) & ~get_current_player_pieces();
}
uint64_t ChessBoard::generate_rook_moves_from(int square, bool white, uint64_t occupancy) const {
    return AttackTables::rook_attacks(square, occupancy) & ~get_current_player_pieces();

}
uint64_t ChessBoard::generate_queen_moves_from(int square, bool white, uint64_t occupancy) const {
    return generate_bishop_moves_from(square, white, occupancy) | generate_rook_moves_from(square, white, occupancy);
}

Move ChessBoard::parse_move(const std::string& str_move) const {
    int from = notation_to_square(str_move.substr(0, 2));
    int from_bb = bb_index_at(from);
    int to = notation_to_square(str_move.substr(2, 2));
    int promotion = 0;

    std::string str_promotion = str_move.substr(4, 1);
    if (std::tolower(str_promotion[0]) == 'q') {
        promotion = 2;
    } else if (std::tolower(str_promotion[0]) == 'k') {
        promotion = 1;
    }

    bool is_ep = (from_bb == 0 || from_bb == 6) && 
                en_passant_square != -1 && 
                to == en_passant_square + (white_turn ? 8 : -8) &&
                abs(from % 8 - en_passant_square % 8) == 1;

    bool is_castling = (from_bb == 5 || from_bb == 11) && abs(from - to) == 2 && can_castle(from, to);
    
    return Move(from, to, promotion, is_ep, is_castling);
}
bool ChessBoard::is_pseudo_move(const Move& move) const {
    MoveList list;
    generate_pseudo_moves(list);
    for (const Move& pm : list) {
        if (pm == move) {
            return true;
        }
    }
    return false;
}
bool ChessBoard::leaves_king_in_check(const Move& move) const {
    int from_bb = bb_index_at(move.from());
    int to_bb = bb_index_at(move.to());
    ChessBoard test_board = *this;
    test_board.execute_pseudo_move(move, from_bb, to_bb);
    if (from_bb == 5) {
        test_board.white_king_square = move.to();
    } else if (from_bb == 11) {
        test_board.black_king_square = move.to();
    }
    return test_board.is_check();
}
bool ChessBoard::is_legal_move(const Move& move) const {
    uint8_t from = move.from(), to = move.to();

    if (move.is_en_passant()) {
        int king_square = white_turn ? white_king_square : black_king_square;
        uint64_t occupancy = (get_occupancy() ^ (1ULL << from) ^ (1ULL << en_passant_square)) | (1ULL << to);
        return !(AttackTables::bishop_attacks(king_square, occupancy) & (get_bishops(!white_turn) | get_queens(!white_turn))) &&
               !(AttackTables::rook_attacks(king_square, occupancy) & (get_rooks(!white_turn) | get_queens(!white_turn)));
    }

    if (move.is_castling()) return true;

    int from_bb = bb_index_at(from);
    if (from_bb == 5 || from_bb == 11) return !is_square_attacked(to, !white_turn, get_occupancy() ^ (1ULL << from));

    return !(get_pinned_pieces(white_turn) & (1ULL << from)) || AttackTables::line_bb[from][to] & get_kings(white_turn);
}

bool ChessBoard::can_castle(int king_square, int target) const {
    if (is_check()) return false;
    if (king_square != 4 && king_square != 60) return false;

    if (target > king_square) {
        int rook_square = king_square + 3;
        int rook_bb = bb_index_at(rook_square);
        if (rook_bb != 3 && white_turn || rook_bb != 9 && !white_turn) return false;

        if (target == 6 && !white_kingside_castle) return false;
        if (target == 62 && !black_kingside_castle) return false;

        uint64_t castling_path = (1ULL << (king_square + 1)) | (1ULL << (king_square + 2));
        if (get_occupancy() & castling_path) return false;

        if (is_square_attacked(king_square + 1, !white_turn, get_occupancy()) || is_square_attacked(king_square + 2, !white_turn, get_occupancy())) return false;
    } else {
        int rook_square = king_square - 4;
        int rook_bb = bb_index_at(rook_square);
        if (rook_bb != 3 && white_turn || rook_bb != 9 && !white_turn) return false;

        if (target == 2 && !white_queenside_castle) return false;
        if (target == 58 && !black_queenside_castle) return false;
        
        uint64_t castling_path = (1ULL << (king_square - 1)) | (1ULL << (king_square - 2)) | (1ULL << (king_square - 3));
        if (get_occupancy() & castling_path) return false;
        
        if (is_square_attacked(king_square - 1, !white_turn, get_occupancy()) || is_square_attacked(king_square - 2, !white_turn, get_occupancy())) return false;
    }

    return true;
}
bool ChessBoard::is_square_attacked(int square, bool by_white, uint64_t occupancy) const {
    if (square < 0 || square > 63) return false;

    uint64_t bishop_attacks = generate_bishop_moves_from(square, !by_white, occupancy);
    if (bishop_attacks & (get_bishops(by_white) | get_queens(by_white))) return true;

    uint64_t rook_attacks = generate_rook_moves_from(square, !by_white, occupancy);
    if (rook_attacks & (get_rooks(by_white) | get_queens(by_white))) return true;
 
    if (AttackTables::pawn_attacks[by_white ? 1 : 0][square] & get_pawns(by_white)) return true;
    if (AttackTables::knight_attacks[square] & get_knights(by_white)) return true;
    if (AttackTables::king_attacks[square] & get_kings(by_white)) return true;
    
    return false;
}
uint64_t ChessBoard::get_checkers() const {
    int king_square = white_turn ? white_king_square : black_king_square;
    uint64_t occupancy = get_occupancy();
    uint64_t checkers = 0;

    uint64_t bishop_attacks_ = AttackTables::bishop_attacks(king_square, occupancy);
    checkers |= bishop_attacks_ & (get_bishops(!white_turn) | get_queens(!white_turn));

    uint64_t rook_attacks_ = AttackTables::rook_attacks(king_square, occupancy);
    checkers |= rook_attacks_ & (get_rooks(!white_turn) | get_queens(!white_turn));

    checkers |= AttackTables::pawn_attacks[!white_turn ? 1 : 0][king_square] & get_pawns(!white_turn);
    checkers |= AttackTables::knight_attacks[king_square] & get_knights(!white_turn);

    return checkers;
}
uint64_t ChessBoard::get_pinned_pieces(bool white) const {
    int king_square = white ? white_king_square : black_king_square;
    uint64_t occupancy = get_occupancy();
    uint64_t our_pieces = get_current_player_pieces();
    uint64_t pinned = 0;

    uint64_t attacks = AttackTables::bishop_attacks(king_square, occupancy);
    uint64_t blockers_on_ray = attacks & our_pieces;
    uint64_t attacks_without_blockers = AttackTables::bishop_attacks(king_square, occupancy ^ blockers_on_ray);
    uint64_t pinners = (attacks ^ attacks_without_blockers) & (get_bishops(!white) | get_queens(!white));

    attacks = AttackTables::rook_attacks(king_square, occupancy);
    blockers_on_ray = attacks & our_pieces;
    attacks_without_blockers = AttackTables::rook_attacks(king_square, occupancy ^ blockers_on_ray);
    pinners |= (attacks ^ attacks_without_blockers) & (get_rooks(!white) | get_queens(!white));

    while (pinners) {
        int pinner_square = pop_lsb(pinners);
        pinned |= AttackTables::between_bb[king_square][pinner_square] & our_pieces;
    }

    return pinned;
}

int ChessBoard::bb_index_at(int square) const {
    uint64_t mask = (1ULL << square);
    for (int i = 0; i < 12; i++) {
        if (bitboards[i] & mask) return i;
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
