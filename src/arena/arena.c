#include "arena.h"

GameError arena_generate(Arena *arena, uint16_t width, uint16_t height, uint32_t seed) {
	if (arena != NULL) {
		arena->width = width;
		arena->height = height;
		arena->seed = seed;
		arena->tiles = NULL;
	}
	return GAME_OK;
}

GameError arena_validate(const Arena *arena) {
	(void)arena;
	return GAME_OK;
}

void arena_destroy(Arena *arena) {
	if (arena != NULL) {
		arena->tiles = NULL;
		arena->width = 0;
		arena->height = 0;
		arena->seed = 0;
	}
}

TileType arena_get_tile(const Arena *arena, uint16_t x, uint16_t y, GameError *error) {
	(void)arena;
	(void)x;
	(void)y;
	if (error != NULL) {
		*error = GAME_OK;
	}
	return TILE_EMPTY;
}

GameError arena_set_tile(Arena *arena, uint16_t x, uint16_t y, TileType tile) {
	(void)arena;
	(void)x;
	(void)y;
	(void)tile;
	return GAME_OK;
}

bool arena_find_spawn(const Arena *arena, PlayerId player, Vec2i *out_spawn) {
	(void)arena;
	(void)player;
	if (out_spawn != NULL) {
		out_spawn->x = 0;
		out_spawn->y = 0;
	}
	return true;
}
