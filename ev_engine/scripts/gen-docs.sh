#!/bin/bash
# gen-docs.sh — Generate API documentation from EV engine source
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/src"
OUT="$ROOT/docs/API.md"

mkdir -p "$ROOT/docs"

{
    echo "# Endearing Void Engine — API Reference"
    echo ""
    echo "_Auto-generated on $(date '+%Y-%m-%d %H:%M')_"
    echo ""
    echo "---"
    echo ""

    # Module overview from headers
    echo "## Modules"
    echo ""
    for hdr in "$SRC"/*.h; do
        base=$(basename "$hdr" .h)
        desc=$(head -1 "$hdr" | sed 's|^// *||')
        echo "### \`$base\`"
        echo ""
        echo "$desc"
        echo ""

        # Extract function declarations (lines with a return type, name, and parens)
        echo "**Functions:**"
        echo ""
        echo '```c'
        grep -E '^[A-Za-z].*\(.*\);' "$hdr" || echo "// (no public functions)"
        echo '```'
        echo ""
    done

    echo "---"
    echo ""

    # Function definitions from .c files
    echo "## All Function Definitions"
    echo ""
    for src in "$SRC"/*.c; do
        base=$(basename "$src")
        funcs=$(grep -E '^[a-zA-Z].*\(.*\) \{' "$src" | sed 's/ {$/;/' || true)
        if [ -n "$funcs" ]; then
            echo "### \`$base\`"
            echo ""
            echo '```c'
            echo "$funcs"
            echo '```'
            echo ""
        fi
    done

    echo "---"
    echo ""

    # Scene statistics
    echo "## Scene Statistics"
    echo ""
    echo "| Build Function | Walls | Objects |"
    echo "|----------------|------:|--------:|"
    for src in "$SRC"/*.c; do
        # Find build_ functions and count add_wall / add_object calls within them
        while IFS= read -r func_line; do
            func_name=$(echo "$func_line" | grep -oE 'build_[a-zA-Z_]+')
            [ -z "$func_name" ] && continue

            # Extract the function body (from definition to next function or EOF)
            func_body=$(sed -n "/^.*$func_name.*{/,/^[a-zA-Z]/p" "$src")
            walls=$(echo "$func_body" | grep -c 'add_wall\|add_light_panel' || echo 0)
            objects=$(echo "$func_body" | grep -c 'add_object' || echo 0)
            echo "| \`$func_name\` | $walls | $objects |"
        done < <(grep -E '^void build_' "$src" 2>/dev/null || true)
    done
    echo ""

    echo "---"
    echo ""

    # Build info
    echo "## Build Info"
    echo ""
    echo "- **Compiler:** $(/usr/bin/clang --version | head -1)"
    echo "- **Flags:** \`-Wall -Wextra -std=c99 -O2\`"
    echo "- **Resolution:** 480x300"
    echo "- **Total source lines:** $(wc -l "$SRC"/*.c "$SRC"/*.h | tail -1 | awk '{print $1}')"

} > "$OUT"

echo "Documentation generated: $OUT"
