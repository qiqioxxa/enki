#include "uci.h"
#include "movegen.h"
#include "perft.h"
#include "utils.h"
#include <iostream>
#include <optional>
#include <iostream>


void UCI::run() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "uci") handle_uci();
        else if (cmd == "setoption") handle_setoption(iss);
        else if (cmd == "isready") handle_isready();
        else if (cmd == "ucinewgame") handle_ucinewgame();
        else if (cmd == "position") handle_position(iss);
        else if (cmd == "go") handle_go(iss);
        else if (cmd == "d") handle_d();
        else if (cmd == "dd") handle_dd();
        else if (cmd == "quit") return;
        else if (!uci_mode) std::cout << "Unknown command: \"" << line << "\"" << std::endl;
    }
}

void UCI::handle_uci() {
    uci_mode = true;
    std::cout << "id name Metis\n";
    std::cout << "id author qiqioxxa\n";
    std::cout << "option name Hash type spin default 16 min 1 max 8192\n";
    std::cout << "uciok\n";
    std::cout.flush();
}
void UCI::handle_setoption(std::istringstream& iss) {
    std::string name_kw, option, value_kw, value;
    if (iss >> name_kw >> option >> value_kw >> value) {
        if (name_kw != "name" || value_kw != "value") return;

        if (option == "Hash") {
            try {
                int hash_mb = std::stoi(value);
                if (hash_mb < 1 || hash_mb > 8192) return;
                engine.tt_resize(hash_mb);
            }
            catch(...) { return; }
        }
    }
}
void UCI::handle_isready() {
    std::cout << "readyok\n";
    std::cout.flush();
}
void UCI::handle_ucinewgame() {
    board.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    engine.tt_clear();
}
void UCI::handle_position(std::istringstream& iss) {
    std::string fen_kw, fen, moves_kw;

    if (iss >> fen_kw) {
        if (fen_kw != "startpos" && fen_kw != "fen") return;

        if (fen_kw == "startpos") {
            fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
            iss >> moves_kw;
        } else {
            std::string fen_part;
            while (iss >> fen_part) {
                if (fen_part == "moves") {
                    moves_kw = "moves";
                    fen_part = "";
                    break;
                }
                if (fen != "") fen += " ";
                fen += fen_part;
            }
        }
        board.set_position(fen);

        if (moves_kw == "moves") {
            std::string move_str;
            Move move;
            while (iss >> move_str) {
                if (uci_mode) {
                    move = parse_move_uci(board, move_str);
                    board.make_move(move);
                } else {
                    std::optional<Move> opt_move = parse_move_input(board, move_str);
                    if (opt_move) {
                        board.make_move(*opt_move);
                    }
                }
            }
        }
    }
}
void UCI::handle_go(std::istringstream& iss) {
    std::string option;
    if (iss >> option) {
        if (option == "perft") {
            std::string value;
            if (iss >> value) {
                int depth;
                try {
                    depth = std::stoi(value);
                    if (depth < 0) return;
                }
                catch(...) { return; }
                perft_divide(board, depth);
                std::cout.flush();
            }

        } else if (option == "perftallstats") {
            std::string value;
            if (iss >> value) {
                int depth;
                try {
                    depth = std::stoi(value);
                    if (depth < 0) return;
                }
                catch(...) { return; }

                std::array<long long, 7> stats = perft(board, depth, true);
                std::array<long long, 7> correction = perft(board, depth - 1, true);
                std::cout << "Nodes:      " << stats[0] << "\n";
                std::cout << "Captures:   " << stats[1] - correction[1] << "\n";
                std::cout << "EPs:        " << stats[2] - correction[2] << "\n";
                std::cout << "Castles:    " << stats[3] - correction[3] << "\n";
                std::cout << "Promotions: " << stats[4] - correction[4] << "\n";
                std::cout << "Checks:     " << stats[5] - correction[5] << "\n";
                std::cout << "Mates:      " << stats[6] << "\n";
                std::cout.flush();
            }

        } else if (option == "tests") {
            run_tests("tests/perft_positions.txt");
            std::cout.flush();
        }
    }
    Move move = engine.choose_move(board, MAX_DEPTH, MAX_TIME_MS);
    std::cout << "bestmove " << move.to_string() << "\n";
    std::cout.flush();
}
void UCI::handle_d() {
    std::cout << board.to_string_compat() << "\n";
    std::cout << "Fen: " << board.to_fen() << "\n";
    std::cout.flush();
}
void UCI::handle_dd() {
    std::cout << board.to_string() << "\n";
    std::cout << "Fen: " << board.to_fen() << "\n";
    std::cout.flush();
}

Move UCI::parse_move_uci(const Board& board, const std::string& move_str) {
    int from = notation_to_square(move_str.substr(0, 2));
    int to = notation_to_square(move_str.substr(2, 2));

    Piece promotion = EMPTY;
    if (move_str.size() == 5) {
        switch (move_str[4]) {
            case 'n': case 'N': promotion = board.white_turn() ? W_KNIGHT : B_KNIGHT; break;
            case 'b': case 'B': promotion = board.white_turn() ? W_BISHOP : B_BISHOP; break;
            case 'r': case 'R': promotion = board.white_turn() ? W_ROOK : B_ROOK; break;
            case 'q': case 'Q': promotion = board.white_turn() ? W_QUEEN : B_QUEEN; break;
            default: break;
        }
    }

    Piece moving_piece = board.piece_at(from);

    bool ep = (moving_piece == W_PAWN || moving_piece == B_PAWN) && 
                board.en_passant_square() != -1 && 
                to == board.en_passant_square() + (board.white_turn() ? 8 : -8);

    bool castling = (moving_piece == W_KING || moving_piece == B_KING) && abs(from - to) == 2;
    
    Piece target_piece = board.piece_at(ep ? board.en_passant_square() : to);
    
    return Move{from, to, promotion, ep, castling, moving_piece, target_piece};
}
std::optional<Move> UCI::parse_move_input(const Board& board, const std::string& move_str) {
    if (move_str.length() != 4 && move_str.length() != 5) return std::nullopt;
    if (!std::ranges::contains("abcdefghABCDEFGH", move_str[0]) || !std::ranges::contains("12345678", move_str[1])) return std::nullopt;
    if (!std::ranges::contains("abcdefghABCDEFGH", move_str[2]) || !std::ranges::contains("12345678", move_str[3])) return std::nullopt;

    int from = notation_to_square(move_str.substr(0, 2));
    int to = notation_to_square(move_str.substr(2, 2));

    Piece promotion = EMPTY;
    if (move_str.size() == 5) {
        switch (move_str[4]) {
            case 'n': case 'N': promotion = board.white_turn() ? W_KNIGHT : B_KNIGHT; break;
            case 'b': case 'B': promotion = board.white_turn() ? W_BISHOP : B_BISHOP; break;
            case 'r': case 'R': promotion = board.white_turn() ? W_ROOK : B_ROOK; break;
            case 'q': case 'Q': promotion = board.white_turn() ? W_QUEEN : B_QUEEN; break;
            default: return std::nullopt;
        }
    }

    MoveList list;
    MoveGen::generate_moves(board, list);
    for (Move valid_move : list) {
        if (valid_move.from() == from && valid_move.to() == to && valid_move.promotion() == promotion) {
            return valid_move;
        }
    }

    return std::nullopt;
}
