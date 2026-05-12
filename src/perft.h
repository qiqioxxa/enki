#pragma once

#include "board.h"
#include "gamestate.h"
#include "movegen.h"
#include <fstream>
#include <print>
#include <sstream>


std::array<long long, 7> perft(Board& board, int depth, bool all_stats = false) {
    if (depth == 0) return {1, 0, 0, 0, 0, 0, 0};

    MoveList list;
    MoveGen::generate_moves(board, list);

    if (depth == 1) {
        if (!all_stats) return {list.size, 0, 0, 0, 0, 0, 0};

        std::array<long long, 7> stats = {list.size, 0, 0, 0, 0, 0, 0};
        for (Move move : list) {
            stats[1] += (move.target_piece() != EMPTY) ? 1 : 0;
            stats[2] += move.is_en_passant() ? 1 : 0;
            stats[3] += move.is_castling() ? 1 : 0;
            stats[4] += move.promotion() != EMPTY ? 1 : 0;

            UnmakeInfo info = board.make_move(move);
            if (king_in_check(board)) {
                stats[5]++;
                MoveList opp_moves;
                MoveGen::generate_moves(board, opp_moves);
                if (opp_moves.size == 0) stats[6]++;
            }
            board.unmake_move(move, info);
        }
        return stats;
    }

    std::array<long long, 7> stats = {0, 0, 0, 0, 0, 0, 0};
    for (Move move : list) {
        UnmakeInfo info = board.make_move(move);
        
        if (all_stats) {
            stats[1] += (move.target_piece() != EMPTY) ? 1 : 0;
            stats[2] += move.is_en_passant() ? 1 : 0;
            stats[3] += move.is_castling() ? 1 : 0;
            stats[4] += move.promotion() != EMPTY ? 1 : 0;
            stats[5] += king_in_check(board) ? 1 : 0;
        }
        
        std::array<long long, 7> sub = perft(board, depth - 1, all_stats);
        for (int i = 0; i < 7; i++) stats[i] += sub[i];

        board.unmake_move(move, info);
    }

    return stats;
}
void perft_divide(Board& board, int depth) {
    if (depth < 1) return;
    auto start = std::chrono::high_resolution_clock::now();
    MoveList list;
    MoveGen::generate_moves(board, list);
    uint64_t total_nodes = 0;

    std::println("=== Perft divide (depth {}) ===", depth);
    
    for (Move move : list) {
        UnmakeInfo info = board.make_move(move);
        uint64_t nodes = perft(board, depth - 1, false)[0];
        total_nodes += nodes;
        std::println("{}: {}", move.to_string(), nodes);
        board.unmake_move(move, info);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::println("Total: {} ({} ms)", total_nodes, duration);
}

void run_tests(const std::string& file_path) {
    std::ifstream file(file_path);
    std::string line;
    int i = 1;
    auto total_time = 0;
    long long total_nodes = 0;
    long long expected_nodes = 0;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string fen, depth, result;

        std::getline(iss, fen, '|');
        std::getline(iss, depth, '|');
        std::getline(iss, result);

        expected_nodes += std::stoi(result);

        std::println("Testing position {}: {}|{}|{}", i, fen, depth, result);

        Board board;
        board.set_position(fen);

        auto start = std::chrono::high_resolution_clock::now();
        long long nodes = perft(board, std::stoi(depth), false)[0];
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        total_time += duration;
        total_nodes += nodes;

        if (std::to_string(nodes) != result ) {
            std::println("❗️ Test failed! Expected {} but got {}", result, nodes);
        }

        i++;
    }

    std::println("Total nodes: {}, total time: {} ms, avg speed: {} Mnodes/s", total_nodes, total_time, (total_nodes / total_time / 1000.0));
    std::println("Expected:    {}", expected_nodes);
}
