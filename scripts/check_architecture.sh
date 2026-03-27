#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

failures=0

print_section() { printf "\n[%s]\n" "$1"; }
pass() { printf "PASS: %s\n" "$1"; }
fail() { printf "FAIL: %s\n" "$1"; failures=$((failures + 1)); }

# ------------------------------------------------------------
# Grep helpers
# ------------------------------------------------------------

GREP_EXCLUDES=(
    --exclude-dir=build
    --exclude-dir=.git
    --exclude-dir=.idea
    --exclude-dir=.vscode
    --exclude='*.o'
    --exclude='*.a'
    --exclude='*.so'
    --exclude='*.dll'
    --exclude='*.exe'
    --exclude='compile_commands.json'
)

existing_paths() {
    local p
    for p in "$@"; do
        [[ -e "$p" ]] && printf '%s\n' "$p"
    done
}

report_matches() {
    local label="$1"
    local pattern="$2"
    shift 2

    mapfile -t paths < <(existing_paths "$@")
    if [[ ${#paths[@]} -eq 0 ]]; then
        pass "$label (skipped)"
        return 0
    fi

    local output
    if output=$(grep -RInE "${GREP_EXCLUDES[@]}" "$pattern" "${paths[@]}" 2>/dev/null); then
        fail "$label"
        printf "%s\n" "$output"
    else
        pass "$label"
    fi
}

report_find_matches() {
    local label="$1"
    local root="$2"
    local name="$3"

    if [[ ! -d "$root" ]]; then
        pass "$label (skipped)"
        return 0
    fi

    local output
    if output=$(find "$root" -type f -name "$name" -print 2>/dev/null) && [[ -n "$output" ]]; then
        fail "$label"
        printf "%s\n" "$output"
    else
        pass "$label"
    fi
}

# ------------------------------------------------------------
# Internal header rule (module-aware, compiler-accurate)
# ------------------------------------------------------------

check_internal_header_scope() {
    local label="internal headers must only be included by .c files in the same module"
    local violations=0

    local matches
    if ! matches=$(grep -RInE "${GREP_EXCLUDES[@]}" \
        '#include[[:space:]]*"[^"]*_internal\.h"' \
        src include 2>/dev/null); then
        pass "$label"
        return 0
    fi

    while IFS= read -r m; do
        local file line content
        file="${m%%:*}"
        line="${m#*:}"; line="${line%%:*}"
        content="${m#*:*:}"

        # Only .c files may include internal headers
        if [[ "$file" != *.c ]]; then
            printf "FAIL: %s\n%s:%s:%s\n  reason: internal headers may only be included by .c files\n" \
                "$label" "$file" "$line" "$content"
            violations=$((violations + 1))
            continue
        fi

        # Extract header name
        local header
        header="$(printf "%s" "$content" | sed -nE 's/.*"([^"]+)".*/\1/p')"

        # Resolve header path (must exist somewhere under src/)
        # Try direct path first (e.g., src/engine/engine_internal.h from #include "engine/engine_internal.h")
        local header_path
        if [[ -f "src/$header" ]]; then
            header_path="src/$header"
        else
            # Fallback: search by basename only
            header_path="$(find src -type f -name "$(basename "$header")" -print | head -n 1)"
        fi

        if [[ -z "$header_path" ]]; then
            printf "FAIL: %s\n%s:%s:%s\n  reason: internal header not found: %s\n" \
                "$label" "$file" "$line" "$content" "$header"
            violations=$((violations + 1))
            continue
        fi

        # Module root = src/<module>
        local file_module header_module
        file_module="$(echo "$file" | cut -d/ -f1-2)"
        header_module="$(echo "$header_path" | cut -d/ -f1-2)"

        if [[ "$file_module" != "$header_module" ]]; then
            printf "FAIL: %s\n%s:%s:%s\n  reason: %s is private to %s\n" \
                "$label" "$file" "$line" "$content" "$header" "$header_module"
            violations=$((violations + 1))
        fi
    done <<< "$matches"

    if [[ $violations -eq 0 ]]; then
        pass "$label"
    else
        failures=$((failures + 1))
    fi
}

# ------------------------------------------------------------
# Paths
# ------------------------------------------------------------

MAIN_PATHS=(src/main.c)

PUBLIC_PATHS=(include)
ENGINE_PUBLIC=(include/engine)
ENGINE_PRIVATE=(src/engine)

GAME_PATHS=(src/game include/game)
ARENA_PATHS=(src/game/arena include/game/arena)
COMBAT_PATHS=(src/game/combat include/game/combat)
AI_PATHS=(src/game/ai include/game/ai)
INPUT_PATHS=(src/input include/input)
RENDER_PATHS=(src/render include/render)
UTILS_PATHS=(src/utils include/utils)
CORE_PATHS=(include/core)

NON_SDL_PATHS=(
    "${GAME_PATHS[@]}"
    "${ARENA_PATHS[@]}"
    "${COMBAT_PATHS[@]}"
    "${AI_PATHS[@]}"
    "${INPUT_PATHS[@]}"
    "${UTILS_PATHS[@]}"
    "${CORE_PATHS[@]}"
)

# ------------------------------------------------------------
# Regex helpers
# ------------------------------------------------------------

SDL_INCLUDE_RE='#include[[:space:]]*[<"]SDL3/SDL[^>"]+[>"]'
SDL_SYMBOL_RE='(^|[^A-Za-z0-9_])SDL_[A-Za-z0-9_]+'
PRIVATE_HEADER_RE='#include[[:space:]]*"[^"]*_internal\.h"'

ENGINE_PUBLIC_RE='#include[[:space:]]*"engine/engine\.h"'
RENDER_PUBLIC_RE='#include[[:space:]]*"render/render\.h"'
GAME_PUBLIC_RE='#include[[:space:]]*"game/game\.h"'
INPUT_PUBLIC_RE='#include[[:space:]]*"input/input\.h"'

ARENA_PUBLIC_RE='#include[[:space:]]*"game/arena/arena\.h"|#include[[:space:]]*"arena/arena\.h"'
COMBAT_PUBLIC_RE='#include[[:space:]]*"game/combat/combat\.h"|#include[[:space:]]*"combat/combat\.h"'
AI_PUBLIC_RE='#include[[:space:]]*"game/ai/ai\.h"|#include[[:space:]]*"ai/ai\.h"'

GAME_PRIVATE_RE='#include[[:space:]]*"([^"]*/)?(arena_internal|combat_internal|ai_internal|game_internal)\.h"'

# ------------------------------------------------------------
# Run checks
# ------------------------------------------------------------

print_section "Architecture Rules"

report_matches \
    "public headers must not include SDL or expose SDL symbols" \
    "$SDL_INCLUDE_RE|$SDL_SYMBOL_RE" \
    "${PUBLIC_PATHS[@]}"

report_matches \
    "SDL is forbidden outside engine/render/main implementation" \
    "$SDL_INCLUDE_RE|$SDL_SYMBOL_RE" \
    "${NON_SDL_PATHS[@]}"

report_find_matches \
    "public include/ must not contain private *_internal.h headers" \
    "include" \
    "*_internal.h"

check_internal_header_scope

report_matches \
    "main must not include arena/combat/ai or private headers" \
    "$ARENA_PUBLIC_RE|$COMBAT_PUBLIC_RE|$AI_PUBLIC_RE|$PRIVATE_HEADER_RE" \
    "${MAIN_PATHS[@]}"

report_matches \
    "main must not call arena/combat/ai APIs directly" \
    '\b(arena_|combat_|ai_)[A-Za-z0-9_]*[[:space:]]*\(' \
    "${MAIN_PATHS[@]}"

report_matches \
    "game must not include engine/render or use SDL" \
    "$ENGINE_PUBLIC_RE|$RENDER_PUBLIC_RE|$SDL_INCLUDE_RE|$SDL_SYMBOL_RE" \
    "${GAME_PATHS[@]}"

report_matches \
    "arena must not include combat/ai/engine/render or use SDL" \
    "$COMBAT_PUBLIC_RE|$AI_PUBLIC_RE|$ENGINE_PUBLIC_RE|$RENDER_PUBLIC_RE|$SDL_INCLUDE_RE|$SDL_SYMBOL_RE" \
    "${ARENA_PATHS[@]}"

report_matches \
    "combat must not include ai/engine/render or use SDL" \
    "$AI_PUBLIC_RE|$ENGINE_PUBLIC_RE|$RENDER_PUBLIC_RE|$SDL_INCLUDE_RE|$SDL_SYMBOL_RE" \
    "${COMBAT_PATHS[@]}"

report_matches \
    "ai must not include engine/render or use SDL" \
    "$ENGINE_PUBLIC_RE|$RENDER_PUBLIC_RE|$SDL_INCLUDE_RE|$SDL_SYMBOL_RE" \
    "${AI_PATHS[@]}"

report_matches \
    "input must not include engine/render or use SDL" \
    "$ENGINE_PUBLIC_RE|$RENDER_PUBLIC_RE|$SDL_INCLUDE_RE|$SDL_SYMBOL_RE" \
    "${INPUT_PATHS[@]}"

report_matches \
    "engine public headers must not include SDL or private headers" \
    "$SDL_INCLUDE_RE|$SDL_SYMBOL_RE|$PRIVATE_HEADER_RE" \
    "${ENGINE_PUBLIC[@]}"

report_matches \
    "engine must not include game/arena/combat/ai/render headers" \
    "$GAME_PUBLIC_RE|$ARENA_PUBLIC_RE|$COMBAT_PUBLIC_RE|$AI_PUBLIC_RE|$RENDER_PUBLIC_RE|$GAME_PRIVATE_RE" \
    "${ENGINE_PRIVATE[@]}"

report_matches \
    "render must not include gameplay private headers" \
    "$GAME_PRIVATE_RE" \
    "${RENDER_PATHS[@]}"

report_matches \
    "utils must not include engine/game/render/input or use SDL" \
    '#include[[:space:]]*"(engine/.+|game/.+|render/.+|input/.+)"|'"$SDL_INCLUDE_RE"'|'"$SDL_SYMBOL_RE" \
    "${UTILS_PATHS[@]}"

report_matches \
    "core must not include engine/game/render/input or use SDL" \
    '#include[[:space:]]*"(engine/.+|game/.+|render/.+|input/.+)"|'"$SDL_INCLUDE_RE"'|'"$SDL_SYMBOL_RE" \
    "${CORE_PATHS[@]}"

if [[ $failures -gt 0 ]]; then
    printf "\nArchitecture checks failed: %d rule(s) violated.\n" "$failures"
    exit 1
fi

printf "\nArchitecture checks passed.\n"
