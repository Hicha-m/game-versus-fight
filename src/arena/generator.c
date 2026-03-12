#include "arena/generator.h"
#include "utils.h"
#include "constants.h"

/**
 * Simple procedural arena generation
 * 
 * Algorithm:
 * 1. Create bottom platform (solid floor)
 * 2. Create floating platforms scattered randomly
 * 3. Create spawn zones for both players
 * 4. Ensure basic fairness (roughly symmetric)
 */
GameError generator_create_tiles(Arena *arena) {
	if (arena == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	
	// Allocate tile array
	size_t tile_count = (size_t)arena->width * arena->height;
	arena->tiles = (TileType *)br_calloc(tile_count, sizeof(TileType));
	
	if (arena->tiles == NULL) {
		return GAME_ERROR_OUT_OF_MEMORY;
	}
	
	// Initialize RNG with arena seed for reproducibility
	math_seed_rng(arena->seed);
	
	// Create bottom platform (solid floor for movement)
	for (uint16_t x = 0; x < arena->width; x++) {
		arena->tiles[(arena->height - 1) * arena->width + x] = TILE_SOLID;
	}
	
	// Create spawn zones
	if (arena->width > 4) {
		// Player 1 spawn: bottom-left
		int spawn_idx = (arena->height - 2) * arena->width + 1;
		if (spawn_idx >= 0 && spawn_idx < (int)tile_count) {
			arena->tiles[spawn_idx] = TILE_SPAWN_P1;
		}
		
		// Player 2 spawn: bottom-right
		spawn_idx = (arena->height - 2) * arena->width + (arena->width - 2);
		if (spawn_idx >= 0 && spawn_idx < (int)tile_count) {
			arena->tiles[spawn_idx] = TILE_SPAWN_P2;
		}
	}
	
	// Create floating platforms (roughly symmetric for fairness)
	int mid_x = arena->width / 2;
	for (uint16_t y = arena->height - 4; y > 2; y -= 2) {
		// Left side platforms
		if (y > 2 && mid_x > 2) {
			int platform_x = 2 + (math_random_u32() % (mid_x - 2));
			int idx = y * arena->width + platform_x;
			if (idx >= 0 && idx < (int)tile_count) {
				arena->tiles[idx] = TILE_PLATFORM;
				if (platform_x + 1 < arena->width && idx + 1 < (int)tile_count) {
					arena->tiles[idx + 1] = TILE_PLATFORM;
				}
			}
		}
		
		// Right side platforms (mirror left for fairness)
		if (y > 2 && mid_x < arena->width - 2) {
			int platform_x = mid_x + 2 + (math_random_u32() % (arena->width - mid_x - 2));
			int idx = y * arena->width + platform_x;
			if (idx >= 0 && idx < (int)tile_count) {
				arena->tiles[idx] = TILE_PLATFORM;
				if (platform_x + 1 < arena->width && idx + 1 < (int)tile_count) {
					arena->tiles[idx + 1] = TILE_PLATFORM;
				}
			}
		}
	}
	
	return GAME_OK;
}
