#include "board.h"

int main() {
    /*ChessBoard board1;
    //board1.set_piece("e5", Piece::W_BISHOP);
    //board1.set_piece("b5", Piece::W_KNIGHT);
    board1.print_moves();
    board1.print_board();
    while (!board1.is_game_over()) {
        std::string human_move;
        bool is_executed = false;
        while (!is_executed) {
            std::cin >> human_move;
            if (human_move.size() != 4 && human_move.size() != 5) continue;
            is_executed = board1.make_move(human_move);
        }
        board1.print_moves();
        board1.print_board();
    }
    for (int i = 1; i <= 6; i++) {
        ChessBoard board;

        auto start = std::chrono::high_resolution_clock::now();
        std::array<uint64_t, 7> stats = board.perft(i);
        std::array<uint64_t, 7> prev_stats = board.perft(i-1);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        for (int j = 1; j < 7; j++) {
            stats[j] -= prev_stats[j];
        }

        std::cout << i << " ";
        for (int j = 0; j < 7; j++) {
            std::cout << stats[j] << " ";
        }
        std::cout << duration << std::endl;
        if (i == 6) {
            board.perft_divide(i);
        }
    }
    return 0;*/
    ChessBoard board;
    board.perft_divide(6);
}
