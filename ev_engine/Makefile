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

# Dev mode — skip title, boot straight into a scene
# Usage: make dev SCENE=STATE_SPACE_LOBBY
#   or:  make dev  (defaults to STATE_SPACE_LOBBY)
SCENE ?= STATE_SPACE_LOBBY
DEV_OBJ = $(SRC:.c=.dev.o)

dev: $(DEV_OBJ)
	$(CC) $(DEV_OBJ) -o $(BIN) $(LIBDIRS) $(LDFLAGS)
	./$(BIN)

src/%.dev.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -DDEV_START=$(SCENE) -c $< -o $@

# Dev watch — auto-rebuild + restart on source changes, preserves scene
dev-watch:
	@./dev-watch.sh $(SCENE)

# Dev diff — quick QA rebuild + diff against baseline (shows regressions instantly)
dev-diff:
	@echo "[dev-diff] Building QA + diffing against baseline..."
	@make -j4 -s qa 2>&1 | grep -E '^\[QA\]' | tail -5
	@./qa/run.sh --diff

clean:
	rm -f $(OBJ) $(BIN) endearing_void_release tests/test_main src/*.qa.o src/*.e2e.o qa/audit_report.json a.out

# ── QA Mode — automated screenshot + pixel analysis ───────────────
QA_OBJ = $(SRC:.c=.qa.o)

qa: validate-model-registry $(QA_OBJ)
	$(CC) $(QA_OBJ) -o $(BIN) $(LIBDIRS) $(LDFLAGS)
	@mkdir -p qa/screenshots
	./$(BIN)
	@rm -f src/*.qa.o

src/%.qa.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -DQA_MODE -c $< -o $@

# Full QA pipeline: build + run + diff + archive
qa-full:
	@./qa/run.sh

# Save current screenshots as the baseline to diff against
qa-baseline:
	@mkdir -p qa/baseline
	@cp qa/screenshots/*_hero.png qa/baseline/ 2>/dev/null && echo "[QA] Baseline saved ($(ls qa/baseline/*.png 2>/dev/null | wc -l | tr -d ' ') screenshots)" || echo "[QA] No screenshots to save — run 'make qa' first"

# Watch for source changes and re-run QA automatically
qa-watch:
	@./qa/run.sh --watch

# Show diff against baseline without re-running
qa-diff:
	@./qa/run.sh --diff

# Show QA run history
qa-history:
	@./qa/run.sh --history

# ── E2E QA — Full multi-angle, multi-profile pipeline ─────────────
E2E_OBJ = $(SRC:.c=.e2e.o)

qa-e2e: validate-model-registry $(E2E_OBJ)
	$(CC) $(E2E_OBJ) -o $(BIN) $(LIBDIRS) $(LDFLAGS)
	@mkdir -p qa/screenshots qa/diffs
	./$(BIN)
	@rm -f src/*.e2e.o

src/%.e2e.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -DQA_MODE -DQA_E2E -c $< -o $@

# Full E2E pipeline: multi-angle + 6-profile analysis + flow testing
qa-e2e-full:
	@./qa/run_e2e.sh

# E2E baseline
qa-e2e-baseline:
	@./qa/run_e2e.sh --baseline

# E2E watch mode
qa-e2e-watch:
	@./qa/run_e2e.sh --watch

# E2E grade only (re-run analyzer, no rebuild)
qa-e2e-grade:
	@./qa/run_e2e.sh --grade

# ── Engine Audit — hardcore technical probing ──────────────────
AUDIT_OBJ = $(SRC:.c=.audit.o)

qa-audit: validate-model-registry $(AUDIT_OBJ)
	$(CC) $(AUDIT_OBJ) -o $(BIN) $(LIBDIRS) $(LDFLAGS)
	@mkdir -p qa/screenshots
	./$(BIN)
	@rm -f src/*.audit.o

src/%.audit.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -DQA_MODE -DQA_AUDIT -c $< -o $@

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
	@python3 scripts/validate_model_registry.py

validate-model-registry:
	@python3 scripts/validate_model_registry.py

quality:
	@echo "=== Quality gate ==="
	@$(MAKE) clean
	@$(MAKE) -j4
	@$(MAKE) check
	@$(MAKE) test
	@$(MAKE) validate-model-registry
	@./qa/run.sh
	@./qa/run_e2e.sh --grade
	@echo "=== Quality gate passed ==="

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

devclean:
	rm -f src/*.dev.o

# Playtest build — clean binary for external testers, no debug keys
playtest: clean
	@echo "=== Playtest build ==="
	$(CC) -std=c99 -O2 -DNDEBUG -DPLAYTEST $(INCLUDES) src/*.c -o endearing_void_playtest $(LIBDIRS) $(LDFLAGS)
	@mkdir -p playtest
	@cp endearing_void_playtest playtest/endearing_void
	@cp -r assets playtest/assets 2>/dev/null || true
	@echo "Three hours to kill." > playtest/README.txt
	@echo "" >> playtest/README.txt
	@echo "Arrow keys / WASD to move. Mouse to look." >> playtest/README.txt
	@echo "E to interact. ESC to quit." >> playtest/README.txt
	@echo "" >> playtest/README.txt
	@echo "After playing, please tell me:" >> playtest/README.txt
	@echo "  1. How long did it take? (check terminal for runtime)" >> playtest/README.txt
	@echo "  2. Where did you pause and look around?" >> playtest/README.txt
	@echo "  3. Where did you rush?" >> playtest/README.txt
	@echo "  4. What confused you?" >> playtest/README.txt
	@echo "  5. How did you feel at the end?" >> playtest/README.txt
	@ls -lh playtest/endearing_void
	@echo "=== Playtest ready: ./playtest/ ==="

.PHONY: all run clean devclean check count todo release fmt test dev dev-watch dev-diff playtest qa qa-full qa-baseline qa-watch qa-diff qa-history qa-e2e qa-e2e-full qa-e2e-baseline qa-e2e-watch qa-e2e-grade qa-audit validate-model-registry quality
