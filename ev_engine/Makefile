CC = /usr/bin/clang
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS = -lraylib -lm -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo
INCLUDES = -I/opt/homebrew/include
LIBDIRS = -L/opt/homebrew/lib

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
BIN = endearing_void

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LIBDIRS) $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all run clean
