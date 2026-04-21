COMPILER = clang++
FLAGS = -std=c++23 -O3
PROFFLAGS = -std=c++23 -O3 -g -L/opt/homebrew/lib -lprofiler
SOURCES = main.cpp board.cpp attack_tables.cpp
HEADERS = $(wildcard *.h)

chess: $(SOURCES)
	$(COMPILER) $(FLAGS) $(SOURCES) -o chess

chess-profiling: $(SOURCES)
	$(COMPILER) $(PROFFLAGS) $(SOURCES) -o chess-profiling

clean:
	rm -rf chess chess-profiling chess-profiling.dSYM .prof
