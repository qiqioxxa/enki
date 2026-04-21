#include "board.h"
#include "perft.h"
#include <iostream>

int main() {
    /*run_tests("tests/perft_positions.txt");
    return 0;
    
    Board board;
    board.set_position("8/P6k/8/8/8/8/8/K7");
    std::cout << "Valid moves: " << board.moves_to_string() << "\n";
    std::cout << board.to_string(true) << "\n";
    while (board.get_game_state() == GameState::ONGOING) {
        std::string human_move;
        bool executed = false;
        while (!executed) {
            std::cin >> human_move;
            executed = board.make_move(human_move);
        }
        std::cout << "Valid moves: " << board.moves_to_string() << "\n";
        std::cout << board.to_string(true) << "\n";
    }*/

    Board board;
    board.set_position("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    std::cout << "Valid moves: " << board.moves_to_string() << "\n";
    std::cout << board.to_string(true) << "\n";
    int depth = 6;
    perft_divide(board, depth);
}
