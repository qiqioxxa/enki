#pragma once

#include "board.h"
#include "engine.h"
#include "types.h"
#include <optional>
#include <sstream>
#include <string>
#include <thread>


class UCI {
    Board board;
    Engine engine;
    std::thread search_thread;
    bool uci_mode = false;

public:
    UCI() {};
    ~UCI() { if (search_thread.joinable()) search_thread.join(); }
    void run();

private:
    void handle_uci();
    void handle_setoption(std::istringstream& iss);
    void handle_isready();
    void handle_ucinewgame();
    void handle_position(std::istringstream& iss);
    void handle_go(std::istringstream& iss);
    void handle_stop();
    void handle_d();
    void handle_dd();

    Move parse_move_uci(const Board& board, const std::string& move_str);
    std::optional<Move> parse_move_input(const Board& board, const std::string& move_str);
};
