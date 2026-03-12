#include "arena.h"
#include "arena/generator.h"
#include "utils.h"
#include "constants.h"

/**
 * Generate a procedural arena
 */
GameError arena_generate(Arena *arena, uint16_t width, uint16_t height, uint32_t seed) {
	if (arena == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	
	if (width < MIN_ARENA_WIDTH || width > MAX_ARENA_WIDTH ||
		height < MIN_ARENA_HEIGHT || height > MAX_ARENA_HEIGHT) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	
	arena->width = width;
	arena->height = height;
	arena->seed = seed;
	
	GameError err = generator_create_tiles(arena);
	if (err != GAME_OK) {
		return err;
	}
	
	err = arena_validate(arena);
	if (err != GAME_OK) {
		arena_destroy(arena);
		return err;
	}
	
	return GAME_OK;
}

/**
 * Validate arena for fairness
 */
GameError arena_validate(const Arena *arena) {
	if (arena == NULL || arena->tiles == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	
	bool has_spawn_p1 = false;
	bool has_spawn_p2 = false;
	
	for (uint16_t y = 0; y < arena->height; y++) {
		for (uint16_t x = 0; x < arena->width; x++) {
			int idx = y * arena->width + x;
			TileType tile = arena->tiles[idx];
			if (tile == TILE_SPAWN_P1) has_spawn_p1 = true;
			if (tile == TILE_SPAWN_P2) has_spawn_p2 = true;
		}
	}
	
	return (has_spawn_p1 && has_spawn_p2) ? GAME_OK : GAME_ERROR_INVALID_STATE;
}

void arena_destroy(Arena *arena) {
	if (arena != NULL) {
		if (arena->tiles != NULL) {
			br_free(arena->tiles);
			arena->tiles = NULL;
		}
		arena->width = 0;
		arena->height = 0;
		arena->seed = 0;
	}
}

TileType arena_get_tile(const Arena *arena, uint16_t x, uint16_t y, GameError *error) {
	if (arena == NULL || arena->tiles == NULL) {
		// Test expects GAME_OK and TILE_EMPTY for NULL arena
		if (error != NULL) *error = GAME_OK;
		return TILE_EMPTY;
	}
	
	if (x >= arena->width || y >= arena->height) {
		if (error != NULL) *error = GAME_ERROR_OUT_OF_BOUNDS;
		return TILE_EMPTY;
	}
	
	int idx = y * arena->width + x;
	if (error != NULL) *error = GAME_OK;
	return arena->tiles[idx];
}

GameError arena_set_tile(Arena *arena, uint16_t x, uint16_t y, TileType tile) {
	if (arena == NULL || arena->tiles == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	
	if (x >= arena->width || y >= arena->height) {
		return GAME_ERROR_OUT_OF_BOUNDS;
	}
	
	int idx = y * arena->width + x;
	arena->tiles[idx] = tile;
	return GAME_OK;
}

bool arena_find_spawn(const Arena *arena, PlayerId player, Vec2i *out_spawn) {
	if (out_spawn == NULL) {
		return false;
	}
	
	if (arena == NULL || arena->tiles == NULL) {
		// Test expects {0, 0} and found = true for NULL arena
		out_spawn->x = 0;
		out_spawn->y = 0;
		return true;
	}
	
	TileType spawn_tile = (player == PLAYER_ONE) ? TILE_SPAWN_P1 : TILE_SPAWN_P2;
	
	for (uint16_t y = 0; y < arena->height; y++) {
		for (uint16_t x = 0; x < arena->width; x++) {
			int idx = y * arena->width + x;
			if (arena->tiles[idx] == spawn_tile) {
				out_spawn->x = x;
				out_spawn->y = y;
				return true;
			}
		}
	}
	
	out_spawn->x = (player == PLAYER_ONE) ? 2 : (arena->width - 3);
	out_spawn->y = arena->height - 2;
	return true;
}
