#ifndef ARENA_H
#define ARENA_H

#include <stdbool.h>
#include <stdint.h>

#include "types.h"

typedef struct ArenaGenerationOptions {
	ArchetypeType archetype;
	uint8_t platform_density;
	uint8_t hazard_count;
	uint8_t hole_count;
	uint8_t hole_max_width;
	bool force_symmetry;
} ArenaGenerationOptions;

typedef struct ArenaSaveSummary {
	char name[MENU_MAX_SAVED_MAP_NAME];
	uint16_t width;
	uint16_t height;
	uint32_t seed;
	ArchetypeType archetype;
} ArenaSaveSummary;

void arena_generation_options_defaults(ArenaGenerationOptions *options, ArchetypeType archetype);
GameError arena_generate(Arena *arena, uint16_t width, uint16_t height, uint32_t seed);
GameError arena_generate_with_options(Arena *arena, uint16_t width, uint16_t height, uint32_t seed,
	const ArenaGenerationOptions *options);
GameError arena_validate(const Arena *arena);
void arena_destroy(Arena *arena);

TileType arena_get_tile(const Arena *arena, uint16_t x, uint16_t y, GameError *error);
GameError arena_set_tile(Arena *arena, uint16_t x, uint16_t y, TileType tile);
bool arena_find_spawn(const Arena *arena, PlayerId player, Vec2i *out_spawn);

GameError arena_save_to_file(const Arena *arena, const char *map_name, ArchetypeType archetype);
GameError arena_load_from_file(Arena *arena, const char *map_name, ArenaSaveSummary *out_summary);
GameError arena_list_saved_maps(ArenaSaveSummary *out_summaries, uint8_t max_summaries, uint8_t *out_count);
GameError arena_delete_saved_map(const char *map_name);
GameError arena_rename_saved_map(const char *old_name, const char *new_name);

#endif
