#ifndef ARENA_H
#define ARENA_H

#include <stdbool.h>
#include <stdint.h>

#include "types.h"

GameError arena_generate(Arena *arena, uint16_t width, uint16_t height, uint32_t seed);
GameError arena_validate(const Arena *arena);
void arena_destroy(Arena *arena);

TileType arena_get_tile(const Arena *arena, uint16_t x, uint16_t y, GameError *error);
GameError arena_set_tile(Arena *arena, uint16_t x, uint16_t y, TileType tile);
bool arena_find_spawn(const Arena *arena, PlayerId player, Vec2i *out_spawn);

#endif
