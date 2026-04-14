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
    }*/

    ChessBoard board;
    //board.make_move("f2f4");
    //board.make_move("e7e5");
    //board.make_move("e1f2"); 
    //board.make_move("d8f6");
    board.set_position("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    board.print_moves();
    board.print_board();
    /*board.print_moves();
    board.print_board();
    board.make_move("g8h8");
    board.print_moves();
    board.print_board();
    board.make_move("e1c1");
    board.print_moves();
    board.print_board();*/  
    int depth = 6;
    /*for (int i = 1; i <= depth; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        std::array<long long, 7> stats = board.perft(i, true);
        std::array<long long, 7> prev_stats = board.perft(i-1, true);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        for (int j = 1; j < 6; j++) {
            stats[j] -= prev_stats[j];
        }

        std::cout << i << " ";
        for (int j = 0; j < 7; j++) {
            std::cout << stats[j] << " ";
        }
        std::cout << duration << std::endl;
    }*/

   board.perft_divide(depth);
}
