# Enki chess engine

Enki is a modern chess engine written in C++23. It communicates using the UCI protocol and relies on a handcrafted evaluation function paired with an alpha-beta search.

---

## Strength

* **Estimated rating:** ~2050 Elo
* **Move generation speed:** ~238 Mnodes/sec

---

## Technical details

### Board representation and move generation
* [Bitboards](https://www.chessprogramming.org/Bitboards)
* [Mailbox](https://www.chessprogramming.org/Mailbox)
* [Magic Bitboards](https://www.chessprogramming.org/Magic_Bitboards)

### Search
* [Negamax](https://www.chessprogramming.org/Negamax)
* [Principal Variation Search](https://www.chessprogramming.org/Principal_Variation_Search)
* [Iterative Deepening](https://www.chessprogramming.org/Iterative_Deepening)
* [Aspiration Windows](https://www.chessprogramming.org/Aspiration_Windows)
* [Transposition Table](https://www.chessprogramming.org/Transposition_Table)
* [Zobrist Hashing](https://www.chessprogramming.org/Zobrist_Hashing)
* [Move Ordering](https://www.chessprogramming.org/Move_Ordering)
* Pruning and reductions:
  * [Quiescence Search](https://www.chessprogramming.org/Quiescence_Search) with [Delta Pruning](https://www.chessprogramming.org/Delta_Pruning)
  * [Null-Move Pruning](https://www.chessprogramming.org/Null_Move_Pruning)
  * [Reverse Futility Pruning](https://www.chessprogramming.org/Reverse_Futility_Pruning)
  * [Late Move Reductions](https://www.chessprogramming.org/Late_Move_Reductions)
  * [Mate Distance Pruning](https://www.chessprogramming.org/Mate_Distance_Pruning)

### Evaluation
* Handcrafted Evaluation model
  * [Material](https://www.chessprogramming.org/Material)
  * [Piece-Square Tables](https://www.chessprogramming.org/Piece-Square_Tables)

---

## Building

### Requirements
* C++23 compliant compiler
* Make build automation tool

### Compilation
```bash
git clone https://github.com/qiqioxxa/enki.git
cd enki
make
```

---

## Usage

### Supported UCI Commands
* `uci`
* `setoption` - Hash option only
* `isready`
* `ucinewgame`
* `position [fen <fenstring> | startpos] moves ...`
* `go [wtime <ms>] [btime <ms>] [winc <ms>] [binc <ms>] [movestogo <n>] [depth <n>] [movetime <ms>] [infinite]`
* `stop`
* `go [perft <depth>] [perftallstats <depth>] [tests path <path>]`
* `d` and `dd` - print board to the terminal
* `quit`

---

## Resources and credits

* [Chess Programming Wiki](https://www.chessprogramming.org/Main_Page)
* [Stockfish](https://github.com/official-stockfish/Stockfish)
* [Ethereal](https://github.com/AndyGrant/Ethereal)
* [Cute Chess](https://www.cutechess.com)
* [CCRL](https://www.computerchess.org.uk/4040/index.html)
