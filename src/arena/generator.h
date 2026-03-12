#ifndef ARENA_GENERATOR_H
#define ARENA_GENERATOR_H

#include "types.h"

/**
 * Procedural arena generation
 * 
 * Pattern: Generate-Validate-Retry (from architecture)
 * 
 * Steps:
 * 1. Seed RNG with arena seed
 * 2. Generate random platform layout
 * 3. Validate fairness (both spawns reachable, roughly symmetric)
 * 4. If invalid, regenerate
 */

GameError generator_create_tiles(Arena *arena);

#endif