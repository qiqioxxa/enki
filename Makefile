COMPILER = clang++
FLAGS = -std=c++23 -O2
PROFFLAGS = -std=c++23 -O2 -g -L/opt/homebrew/lib -lprofiler
SOURCES = main.cpp board.cpp attack_tables.cpp

chess: $(SOURCES)
	$(COMPILER) $(FLAGS) $(SOURCES) -o chess

chess-profiling: $(SOURCES)
	$(COMPILER) $(PROFFLAGS) $(SOURCES) -o chess-profiling

clean:
	rm -rf chess chess-profiling chess-profiling.dSYM .prof
