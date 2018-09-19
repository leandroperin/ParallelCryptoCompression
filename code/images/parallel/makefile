CC = g++
CCOPT = -std=c++11
CFLAGS = -Ofast -mfpmath=sse -funroll-loops --param max-unroll-times=8 -w -fopenmp

BINDIR = $(CURDIR)/bin
INCDIR = $(CURDIR)/include
SRCDIR = $(CURDIR)/src

# Source files.
SRC = $(wildcard $(SRCDIR)/*.cpp)

BIN = gmpr

# Builds everything.
all:
	mkdir -p $(BINDIR)
	$(CC) $(CCOPT) $(CFLAGS) $(SRC) -o $(BINDIR)/$(BIN) `pkg-config opencv --cflags --libs`

# Cleans compilation files.
clean:
	@rm -f $(BINDIR)/$(BIN)
