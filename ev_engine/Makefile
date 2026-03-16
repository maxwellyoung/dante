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
	rm -f $(OBJ) $(BIN) endearing_void_release tests/test_main

# ── Quality & Analysis ──────────────────────────────────────────────

CHECK_FLAGS = -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wdouble-promotion \
              -Wformat=2 -Wundef -Wno-unused-parameter -Wno-sign-conversion \
              -std=c99 -fsyntax-only

check:
	@echo "=== Static analysis build ==="
	@fail=0; \
	for f in src/*.c; do \
		printf "  %-30s" "$$f"; \
		if $(CC) $(CHECK_FLAGS) $(INCLUDES) $$f 2>/dev/null; then \
			echo "OK"; \
		else \
			echo "WARN"; \
			$(CC) $(CHECK_FLAGS) $(INCLUDES) $$f; \
			fail=1; \
		fi; \
	done; \
	echo "=== Done ==="; \
	if [ $$fail -eq 0 ]; then echo "All files clean."; else echo "Warnings found (see above)."; fi

count:
	@echo "=== Line counts ==="
	@wc -l src/*.c src/*.h | sort -n
	@echo ""

todo:
	@echo "=== TODOs, FIXMEs, HACKs, XXXs ==="
	@grep -rn 'TODO\|FIXME\|HACK\|XXX' src/ || echo "  (none found)"
	@echo ""

release: clean
	@echo "=== Release build ==="
	$(CC) -std=c99 -O3 -DNDEBUG $(INCLUDES) src/*.c -o endearing_void_release $(LIBDIRS) $(LDFLAGS)
	strip endearing_void_release
	@ls -lh endearing_void_release
	@echo "=== Release binary ready ==="

fmt:
	@echo "=== Formatting source ==="
	@clang-format -i src/*.c src/*.h
	@echo "Done."

test: tests/test_main
	@echo "=== Running headless tests ==="
	@./tests/test_main
	@echo ""

tests/test_main: tests/test_main.c src/ev_types.h src/scene.h src/player.h src/audio.h src/render.h src/lighting.h
	$(CC) -std=c99 -Wall -Wextra $(INCLUDES) -DHEADLESS_TEST tests/test_main.c -o tests/test_main

.PHONY: all run clean check count todo release fmt test
