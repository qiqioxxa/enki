#pragma once

#include "board.h"
#include "engine.h"
#include "types.h"
#include <sstream>
#include <string>

class UCI {
    Board board{};
    Engine engine{};
    bool uci_mode = false;
    const int MAX_DEPTH = 7;
    const int MAX_TIME_MS = 10000;

public:
    void run();

private:
    void handle_uci();
    void handle_setoption(std::istringstream& iss);
    void handle_isready();
    void handle_ucinewgame();
    void handle_position(std::istringstream& iss);
    void handle_go(std::istringstream& iss);
    void handle_d();
    void handle_dd();

    Move parse_move_uci(const Board& board, const std::string& move_str);
    std::optional<Move> parse_move_input(const Board& board, const std::string& move_str);
};
