#include "arena.h"
#include <stdlib.h>
#include <stddef.h>


GameError arena_generate(Arena *arena, uint16_t width, uint16_t height, uint32_t seed) {
	if (arena == NULL) return GAME_ERROR_INVALID_ARGUMENT;

	/* Free any previous allocation */
	free(arena->tiles);
	arena->tiles = NULL;
	arena->width  = width;
	arena->height = height;
	arena->seed   = seed;

	if (width == 0 || height == 0) return GAME_OK;

	size_t count = (size_t)width * height;
	arena->tiles = calloc(count, sizeof(TileType));
	if (arena->tiles == NULL) return GAME_ERROR_OUT_OF_MEMORY;

	/* Floor: bottom row = TILE_SOLID */
	for (uint16_t x = 0; x < width; x++)
		arena->tiles[(size_t)(height - 1) * width + x] = TILE_SOLID;

	/* Center platform: row height-5, middle third of map */
	if (height >= 5) {
		uint16_t plat_row   = height - 5;
		uint16_t plat_start = width / 3;
		uint16_t plat_end   = (uint16_t)(2 * width / 3);
		for (uint16_t x = plat_start; x <= plat_end; x++)
			arena->tiles[(size_t)plat_row * width + x] = TILE_PLATFORM;
	}

	/* Spawn markers: row height-2 */
	if (height >= 2 && width >= 6) {
		uint16_t spawn_row = height - 2;
		arena->tiles[(size_t)spawn_row * width + 2]           = TILE_SPAWN_P1;
		arena->tiles[(size_t)spawn_row * width + (width - 3)] = TILE_SPAWN_P2;
	}

	return GAME_OK;
}

GameError arena_validate(const Arena *arena) {
	if (arena == NULL || arena->tiles == NULL) return GAME_ERROR_INVALID_STATE;
	return GAME_OK;
}

void arena_destroy(Arena *arena) {
	if (arena != NULL) {
		free(arena->tiles);
		arena->tiles  = NULL;
		arena->width  = 0;
		arena->height = 0;
		arena->seed   = 0;
	}
}

TileType arena_get_tile(const Arena *arena, uint16_t x, uint16_t y, GameError *error) {
	if (arena == NULL || arena->tiles == NULL || x >= arena->width || y >= arena->height) {
		if (error) *error = GAME_ERROR_OUT_OF_BOUNDS;
		return TILE_EMPTY;
	}
	if (error) *error = GAME_OK;
	return arena->tiles[(size_t)y * arena->width + x];
}

GameError arena_set_tile(Arena *arena, uint16_t x, uint16_t y, TileType tile) {
	if (arena == NULL || arena->tiles == NULL || x >= arena->width || y >= arena->height)
		return GAME_ERROR_OUT_OF_BOUNDS;
	arena->tiles[(size_t)y * arena->width + x] = tile;
	return GAME_OK;
}

bool arena_find_spawn(const Arena *arena, PlayerId player, Vec2i *out_spawn) {
	if (arena == NULL || arena->tiles == NULL) return false;

	TileType target = (player == PLAYER_ONE) ? TILE_SPAWN_P1 : TILE_SPAWN_P2;
	for (uint16_t y = 0; y < arena->height; y++) {
		for (uint16_t x = 0; x < arena->width; x++) {
			if (arena->tiles[(size_t)y * arena->width + x] == target) {
				if (out_spawn) { out_spawn->x = (int32_t)x; out_spawn->y = (int32_t)y; }
				return true;
			}
		}
	}
	return false;
}
