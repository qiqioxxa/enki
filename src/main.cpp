#include "attack_tables.h"
#include "board.h"
#include "engine.h"
#include "perft.h"
#include "zobrist.h"
#include <iostream>
#include <string>


int main() {
    AttackTables::init();
    Zobrist::init();
    
    Board board;
    Engine engine;

    std::string line;

    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "uci") {
            std::cout << "id name Metis\n";
            std::cout << "id author qiqioxxa\n";
            std::cout << "option name Hash type spin default 16 min 1 max 8192\n";
            std::cout << "uciok\n";
            std::cout.flush();

        } else if (cmd == "setoption") {
            std::string name_kw, option, value_kw, value;
            if (iss >> name_kw >> option >> value_kw >> value) {
                if (name_kw != "name" || value_kw != "value") continue;

                if (option == "Hash") {
                    try {
                        int hash_mb = std::stoi(value);
                        if (hash_mb < 1 || hash_mb > 8192) continue;
                        engine.tt_resize(hash_mb);
                    }
                    catch(...) { continue; }
                }
            }
        
        } else if (cmd == "isready") {
            std::cout << "readyok\n";
            std::cout.flush();

        } else if (cmd == "ucinewgame") {
            board.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            move_stack = {};
            engine.tt_clear();
        
        } else if (cmd == "position") {
            std::string fen, moves_kw;

            std::string fen_part;
            while (iss >> fen_part) {
                if (fen_part == "moves") {
                    moves_kw = fen_part;
                    fen_part = "";
                    break;
                }
                if (fen != "") fen += " ";
                fen += fen_part;
            }
            if (fen == "startpos") fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
            board.set_position(fen);

            if (moves_kw == "moves") {
                std::string move;
                while (iss >> move) {
                    UnmakeInfo info = board.make_move(move);
                    if (info == UnmakeInfo{}) break;
                }
            }

        } else if (cmd == "d") {
            std::cout << board.to_string() << "\n";
            std::cout.flush();
        
        } else if (cmd == "quit") {
            break;
        
        } else if (cmd == "go") {
            Move move = engine.choose_move(board, 6, 10000);
            std::cout << "bestmove " << move.to_string() << "\n";
            std::cout.flush();
        
        } else if (cmd == "move") {
            std::string move;
            if (iss >> move) {
                if (move == "engine") {
                    Move move = engine.choose_move(board, 6, 10000);
                    UnmakeInfo info = board.make_move(move);
                    move_stack.push({move, info});

                } else if (move == "back") {
                    std::string value;
                    if (iss >> value) {
                        int amount;
                        try {
                            amount = std::stoi(value);
                            if (amount < 1 || amount > move_stack.size()) continue;
                        }
                        catch(...) { continue; }
                        for (int i = 0; i < amount; i++) {
                            auto [move, info] = move_stack.top();
                            move_stack.pop();
                            board.unmake_move(move, info);
                        }
                    }

                } else {
                    UnmakeInfo info = board.make_move(move);
                    if (info == UnmakeInfo{}) continue;
                }
                
                std::cout << "Valid moves: " << board.moves_to_string() << "\n";
                std::cout << board.to_string() << "\n";
                std::cout.flush();
            }

        } else std::cout << "Unknown command: \"" << cmd << "\"\n";
    }
}
