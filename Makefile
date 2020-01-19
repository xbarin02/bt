CFLAGS=-std=c89 -pedantic -Wall -Wextra -march=native -Og -g
LDFLAGS=-rdynamic

BIN=bt

.PHONY: all
all: $(BIN)

.PHONY: clean
clean:
	-$(RM) -- $(BIN)
