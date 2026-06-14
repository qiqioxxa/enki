COMPILER  = clang++
FLAGS     = -std=c++23 -O3
PROFFLAGS = -std=c++23 -O3 -g -L/opt/homebrew/lib -lprofiler

SRCDIR   = src
BUILDDIR = build
SOURCES  = $(wildcard $(SRCDIR)/*.cpp)
HEADERS  = $(wildcard $(SRCDIR)/*.h)

TARGET ?= enki
OUT     = $(BUILDDIR)/$(TARGET)

$(OUT): $(SOURCES) $(HEADERS) | $(BUILDDIR)
	$(COMPILER) $(FLAGS) $(SOURCES) -o $@

profiling: $(SOURCES) $(HEADERS) | $(BUILDDIR)
	$(COMPILER) $(PROFFLAGS) $(SOURCES) -o $(BUILDDIR)/profiling

$(BUILDDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILDDIR)

.PHONY: clean profiling
