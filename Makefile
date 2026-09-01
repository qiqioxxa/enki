ifeq ($(OS), Windows_NT)
	SHELL   := cmd.exe
    EXT     ?= .exe
	LDFLAGS ?= -static -s
	LDLIBS   ?= -lstdc++exp
	MKDIR   := mkdir
    RM      := if exist build rmdir /s /q
else
	MKDIR := mkdir -p
    RM    := rm -rf

	ifeq ($(shell uname -s), Darwin)
		CXX       ?= clang++
		IS_DARWIN := 1
	endif
endif

CXX      ?= g++
CXXFLAGS ?= -std=c++23 -O3

SRCDIR   := src
BUILDDIR := build
TARGET   ?= enki

SOURCES := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS := $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
OUT     := $(BUILDDIR)/$(TARGET)$(EXT)

.PHONY: all clean profiling

all: $(OUT)

$(OUT): $(OBJECTS) | $(BUILDDIR)
		$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
		$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

ifeq ($(IS_DARWIN), 1)
profiling: $(SOURCES) | $(BUILDDIR)
		$(CXX) $(CXXFLAGS) -g $(SOURCES) -o $(BUILDDIR)/profiling -L/opt/homebrew/lib -lprofiler
endif

$(BUILDDIR):
		$(MKDIR) $@

clean:
		$(RM) $(BUILDDIR)

-include $(DEPENDS)
