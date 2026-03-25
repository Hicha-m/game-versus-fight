#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

failures=0

print_section() {
    printf "\n[%s]\n" "$1"
}

report_matches() {
    local label="$1"
    local pattern="$2"
    shift 2
    local paths=("$@")

    local existing=()
    local p
    for p in "${paths[@]}"; do
        if [[ -e "$p" ]]; then
            existing+=("$p")
        fi
    done

    if [[ ${#existing[@]} -eq 0 ]]; then
        return 0
    fi

    local output
    if output=$(grep -RInE "$pattern" "${existing[@]}" 2>/dev/null); then
        printf "FAIL: %s\n" "$label"
        printf "%s\n" "$output"
        failures=$((failures + 1))
    else
        printf "PASS: %s\n" "$label"
    fi
}

print_section "Architecture Rules"

MAIN_PATHS=(src/main.c)
ENGINE_PATHS=(src/engine include/engine)
GAME_PATHS=(src/game include/game)
ARENA_PATHS=(src/game/arena include/game/arena)
COMBAT_PATHS=(src/game/combat include/game/combat)
AI_PATHS=(src/game/ai include/game/ai)
RENDER_PATHS=(src/render include/render)
UTILS_PATHS=(src/utils include/utils)
CORE_PATHS=(include/core)

NON_SDL_PATHS=(
    "${GAME_PATHS[@]}"
    "${ARENA_PATHS[@]}"
    "${COMBAT_PATHS[@]}"
    "${AI_PATHS[@]}"
    "${UTILS_PATHS[@]}"
    "${CORE_PATHS[@]}"
)

report_matches \
    "SDL is forbidden outside engine/render/main" \
    '#include[[:space:]]*<SDL3/SDL.h>|\bSDL_[A-Za-z0-9_]+' \
    "${NON_SDL_PATHS[@]}"

report_matches \
    "main must not include arena/combat/ai" \
    '#include[[:space:]]+"((game/)?arena(/arena)?\.h|(game/)?combat(/combat)?\.h|(game/)?ai(/ai)?\.h)"' \
    "${MAIN_PATHS[@]}"

report_matches \
    "main must not call arena/combat/ai APIs directly" \
    '\b(arena_|combat_|ai_)[A-Za-z0-9_]*[[:space:]]*\(' \
    "${MAIN_PATHS[@]}"

report_matches \
    "game must not include engine/render or use SDL" \
    '#include[[:space:]]+"(engine/engine.h|render/render.h)"|#include[[:space:]]*<SDL3/SDL.h>|\bSDL_[A-Za-z0-9_]+' \
    "${GAME_PATHS[@]}"

report_matches \
    "engine must not include game/arena/combat/ai/render" \
    '#include[[:space:]]+"(game/game\.h|game/arena/arena\.h|game/combat/combat\.h|game/ai/ai\.h|arena/arena\.h|combat/combat\.h|ai/ai\.h|render/render\.h)"' \
    "${ENGINE_PATHS[@]}"

report_matches \
    "arena must not include game/combat/ai/engine/render or use SDL" \
    '#include[[:space:]]+"(game/game\.h|game/combat/combat\.h|game/ai/ai\.h|engine/engine\.h|render/render\.h|combat/combat\.h|ai/ai\.h)"|#include[[:space:]]*<SDL3/SDL.h>|\bSDL_[A-Za-z0-9_]+' \
    "${ARENA_PATHS[@]}"

report_matches \
    "combat must not include game/ai/engine/render or use SDL" \
    '#include[[:space:]]+"(game/game\.h|game/ai/ai\.h|engine/engine\.h|render/render\.h|ai/ai\.h)"|#include[[:space:]]*<SDL3/SDL.h>|\bSDL_[A-Za-z0-9_]+' \
    "${COMBAT_PATHS[@]}"

report_matches \
    "ai must not include game/engine/render or use SDL" \
    '#include[[:space:]]+"(game/game\.h|engine/engine\.h|render/render\.h)"|#include[[:space:]]*<SDL3/SDL.h>|\bSDL_[A-Za-z0-9_]+' \
    "${AI_PATHS[@]}"

report_matches \
    "render must not include gameplay private headers" \
    '#include[[:space:]]+"(arena_internal\.h|combat_internal\.h|ai_internal\.h|game_internal\.h)"|#include[[:space:]]+"src/game/.+_internal\.h"' \
    "${RENDER_PATHS[@]}"

report_matches \
    "utils must not include engine/game/render and must not use SDL" \
    '#include[[:space:]]+"(engine/.+|game/.+|render/.+)"|#include[[:space:]]*<SDL3/SDL.h>|\bSDL_[A-Za-z0-9_]+' \
    "${UTILS_PATHS[@]}"

if [[ $failures -gt 0 ]]; then
    printf "\nArchitecture checks failed: %d rule(s) violated.\n" "$failures"
    exit 1
fi

printf "\nArchitecture checks passed.\n"
