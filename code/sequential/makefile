CC = g++
CCOPT = -std=c++11
CFLAGS = -Ofast 

BINDIR = $(CURDIR)/bin
INCDIR = $(CURDIR)/include
SRCDIR = $(CURDIR)/src

# Source files.
SRC = $(wildcard $(SRCDIR)/*.cpp)

BIN = gmpr

# Builds everything.
all:
	mkdir -p $(BINDIR)
	$(CC) $(CCOPT) $(CFLAGS) $(SRC) -o $(BINDIR)/$(BIN)

# Cleans compilation files.
clean:
	@rm -f $(BINDIR)/$(BIN)
