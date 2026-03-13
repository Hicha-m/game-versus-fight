#include "arena/generator.h"
#include "utils.h"
#include "constants.h"

static uint32_t random_between(uint32_t min_v, uint32_t max_v) {
	if (max_v <= min_v) {
		return min_v;
	}
	return min_v + (math_random_u32() % (max_v - min_v + 1U));
}

static uint16_t clamp_u16(uint16_t value, uint16_t min_v, uint16_t max_v) {
	if (value < min_v) return min_v;
	if (value > max_v) return max_v;
	return value;
}

static uint8_t clamp_u8(uint8_t value, uint8_t min_v, uint8_t max_v) {
	if (value < min_v) return min_v;
	if (value > max_v) return max_v;
	return value;
}

static void archetype_defaults(const ArenaGenerationOptions *in, ArenaGenerationOptions *out) {
	*out = *in;

	out->platform_density = clamp_u8(out->platform_density, 5U, 90U);
	out->hazard_count = clamp_u8(out->hazard_count, 0U, 12U);
	out->hole_count = clamp_u8(out->hole_count, 0U, 8U);
	out->hole_max_width = clamp_u8(out->hole_max_width, 1U, 6U);

	switch (out->archetype) {
	case ARCHETYPE_SMALL:
		if (out->platform_density < 45U) out->platform_density = 45U;
		if (out->hole_count > 2U) out->hole_count = 2U;
		if (out->hazard_count > 2U) out->hazard_count = 2U;
		break;
	case ARCHETYPE_LARGE:
		if (out->platform_density < 20U) out->platform_density = 20U;
		if (out->hole_count > 4U) out->hole_count = 4U;
		if (out->hazard_count > 6U) out->hazard_count = 6U;
		break;
	case ARCHETYPE_MEDIUM:
	default:
		if (out->platform_density < 30U) out->platform_density = 30U;
		if (out->hole_count > 3U) out->hole_count = 3U;
		if (out->hazard_count > 4U) out->hazard_count = 4U;
		break;
	}
}

static uint16_t spawn_left_x(uint16_t width) {
	return (uint16_t)(width / 6U);
}

static uint16_t spawn_right_x(uint16_t width) {
	uint16_t left = spawn_left_x(width);
	if (width <= left + 2U) {
		return (uint16_t)(width - 2U);
	}
	return (uint16_t)(width - left - 1U);
}

static void place_floor(TileType *tiles, uint16_t width, uint16_t height) {
	for (uint16_t x = 0; x < width; ++x) {
		tiles[(height - 1U) * width + x] = TILE_SOLID;
	}
}

static void carve_hole(TileType *tiles, uint16_t width, uint16_t height, uint16_t x, uint16_t hole_width) {
	uint16_t end = (uint16_t)(x + hole_width);
	if (end >= width) {
		end = (uint16_t)(width - 1U);
	}
	for (uint16_t hx = x; hx <= end; ++hx) {
		tiles[(height - 1U) * width + hx] = TILE_EMPTY;
	}
}

static void place_random_platforms(TileType *tiles, uint16_t width, uint16_t height,
	const ArenaGenerationOptions *options) {
	uint16_t min_y = 2U;
	uint16_t max_y = (height > 5U) ? (uint16_t)(height - 4U) : 2U;
	uint16_t min_x = 2U;
	uint16_t max_x = (width > 6U) ? (uint16_t)(width - 3U) : (uint16_t)(width - 1U);
	uint16_t iterations = (uint16_t)((uint32_t)width * (uint32_t)options->platform_density / 10U);

	for (uint16_t i = 0; i < iterations; ++i) {
		uint16_t y = (uint16_t)random_between(min_y, max_y);
		uint16_t x = (uint16_t)random_between(min_x, max_x);
		uint16_t length = (uint16_t)random_between(1U, 4U);
		for (uint16_t p = 0; p < length && (uint16_t)(x + p) < width - 1U; ++p) {
			tiles[y * width + x + p] = TILE_PLATFORM;
			if (options->force_symmetry) {
				uint16_t mx = (uint16_t)(width - 1U - (x + p));
				tiles[y * width + mx] = TILE_PLATFORM;
			}
		}
	}
}

static void place_hazards(TileType *tiles, uint16_t width, uint16_t height,
	const ArenaGenerationOptions *options) {
	if (height < 4U) {
		return;
	}

	for (uint8_t i = 0; i < options->hazard_count; ++i) {
		uint16_t y = (uint16_t)random_between((uint32_t)(height - 4U), (uint32_t)(height - 2U));
		uint16_t x = (uint16_t)random_between(2U, (uint32_t)(width - 3U));
		if (tiles[y * width + x] == TILE_EMPTY) {
			tiles[y * width + x] = TILE_HAZARD;
			if (options->force_symmetry) {
				uint16_t mx = (uint16_t)(width - 1U - x);
				tiles[y * width + mx] = TILE_HAZARD;
			}
		}
	}
}

static void place_holes(TileType *tiles, uint16_t width, uint16_t height,
	const ArenaGenerationOptions *options, uint16_t left_spawn, uint16_t right_spawn) {
	for (uint8_t i = 0; i < options->hole_count; ++i) {
		uint16_t hole_width = (uint16_t)random_between(1U, options->hole_max_width);
		uint16_t min_x = 2U;
		uint16_t max_x = (uint16_t)(width - hole_width - 3U);
		if (max_x <= min_x) {
			break;
		}
		uint16_t x = (uint16_t)random_between(min_x, max_x);

		if ((x >= left_spawn - 3U && x <= left_spawn + 3U) ||
			(x >= right_spawn - 3U && x <= right_spawn + 3U)) {
			continue;
		}

		carve_hole(tiles, width, height, x, hole_width);
		if (options->force_symmetry) {
			uint16_t mx = (uint16_t)(width - 1U - x - hole_width + 1U);
			carve_hole(tiles, width, height, mx, hole_width);
		}
	}
}

/**
 * Simple procedural arena generation
 * 
 * Algorithm:
 * 1. Create bottom platform (solid floor)
 * 2. Create floating platforms scattered randomly
 * 3. Create spawn zones for both players
 * 4. Ensure basic fairness (roughly symmetric)
 */
GameError generator_create_tiles(Arena *arena, const ArenaGenerationOptions *options) {
	if (arena == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	if (options == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	ArenaGenerationOptions tuned;
	archetype_defaults(options, &tuned);

	size_t tile_count = (size_t)arena->width * arena->height;
	arena->tiles = (TileType *)br_calloc(tile_count, sizeof(TileType));

	if (arena->tiles == NULL) {
		return GAME_ERROR_OUT_OF_MEMORY;
	}

	arena->width = clamp_u16(arena->width, MIN_ARENA_WIDTH, MAX_ARENA_WIDTH);
	arena->height = clamp_u16(arena->height, MIN_ARENA_HEIGHT, MAX_ARENA_HEIGHT);

	place_floor(arena->tiles, arena->width, arena->height);

	uint16_t left_spawn = spawn_left_x(arena->width);
	uint16_t right_spawn = spawn_right_x(arena->width);
	uint16_t spawn_y = (uint16_t)(arena->height - 2U);
	arena->tiles[spawn_y * arena->width + left_spawn] = TILE_SPAWN_P1;
	arena->tiles[spawn_y * arena->width + right_spawn] = TILE_SPAWN_P2;

	place_holes(arena->tiles, arena->width, arena->height, &tuned, left_spawn, right_spawn);
	place_random_platforms(arena->tiles, arena->width, arena->height, &tuned);
	place_hazards(arena->tiles, arena->width, arena->height, &tuned);

	/* Always guarantee a central platform anchor for duel readability. */
	if (arena->width > 8U && arena->height > 6U) {
		uint16_t cx = (uint16_t)(arena->width / 2U);
		uint16_t py = (uint16_t)(arena->height - 5U);
		arena->tiles[py * arena->width + cx] = TILE_PLATFORM;
		if (cx > 0U) {
			arena->tiles[py * arena->width + (uint16_t)(cx - 1U)] = TILE_PLATFORM;
		}
	}
	
	return GAME_OK;
}
