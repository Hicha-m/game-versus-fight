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
		if (out->platform_density < 35U) out->platform_density = 35U;
		if (out->hole_count > 2U) out->hole_count = 2U;
		if (out->hazard_count > 2U) out->hazard_count = 2U;
		break;
	case ARCHETYPE_LARGE:
		if (out->platform_density < 16U) out->platform_density = 16U;
		if (out->hole_count > 4U) out->hole_count = 4U;
		if (out->hazard_count > 6U) out->hazard_count = 6U;
		break;
	case ARCHETYPE_MEDIUM:
	default:
		if (out->platform_density < 22U) out->platform_density = 22U;
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

static uint16_t mirror_platform_x(uint16_t width, uint16_t x, uint16_t length) {
	if (length >= width) {
		return 0U;
	}
	return (uint16_t)(width - x - length);
}

static void place_platform_span(TileType *tiles, uint16_t width, uint16_t height,
	uint16_t x, uint16_t y, uint16_t length) {
	if (tiles == NULL || width == 0U || height == 0U || length == 0U) {
		return;
	}
	if (y >= height || x >= width) {
		return;
	}
	uint16_t end = (uint16_t)(x + length);
	if (end > width) {
		end = width;
	}
	for (uint16_t px = x; px < end; ++px) {
		if (tiles[y * width + px] == TILE_EMPTY) {
			tiles[y * width + px] = TILE_PLATFORM;
		}
	}
}

static void place_mirrored_platform_span(TileType *tiles, uint16_t width, uint16_t height,
	uint16_t x, uint16_t y, uint16_t length, bool mirror) {
	place_platform_span(tiles, width, height, x, y, length);
	if (!mirror) {
		return;
	}
	uint16_t mx = mirror_platform_x(width, x, length);
	if (mx != x) {
		place_platform_span(tiles, width, height, mx, y, length);
	}
}

static uint16_t lane_y(uint16_t height, uint16_t lane) {
	uint16_t base = (height > 4U) ? (uint16_t)(height - 4U) : 2U;
	uint16_t step = 2U;
	uint16_t offset = (uint16_t)(lane * step);
	if (base <= offset + 1U) {
		return 2U;
	}
	return (uint16_t)(base - offset);
}

static uint16_t clamped_region_length(uint16_t region_width, uint16_t min_length, uint16_t max_length) {
	uint16_t max_allowed = region_width > 1U ? (uint16_t)(region_width - 1U) : 1U;
	if (max_length > max_allowed) {
		max_length = max_allowed;
	}
	if (min_length > max_length) {
		min_length = max_length;
	}
	return (uint16_t)random_between(min_length, max_length);
}

static void place_region_pattern(TileType *tiles, uint16_t width, uint16_t height,
	uint16_t region_x, uint16_t region_width, bool mirror) {
	uint16_t motif = (uint16_t)random_between(0U, 4U);
	uint16_t length = 0U;
	uint16_t start_x = 0U;
	uint16_t lower = lane_y(height, 0U);
	uint16_t middle = lane_y(height, 1U);
	uint16_t upper = lane_y(height, 2U);

	if (region_width < 4U) {
		return;
	}

	switch (motif) {
	case 0U:
		length = clamped_region_length(region_width, 3U, 5U);
		start_x = region_x;
		place_mirrored_platform_span(tiles, width, height, start_x, lower, length, mirror);
		start_x = (uint16_t)(region_x + region_width / 3U);
		place_mirrored_platform_span(tiles, width, height, start_x, middle, length, mirror);
		start_x = (uint16_t)(region_x + region_width / 2U);
		place_mirrored_platform_span(tiles, width, height, start_x, upper, (uint16_t)(length - 1U), mirror);
		break;
	case 1U:
		length = clamped_region_length(region_width, 4U, 7U);
		start_x = (uint16_t)(region_x + region_width / 4U);
		place_mirrored_platform_span(tiles, width, height, start_x, middle, length, mirror);
		place_mirrored_platform_span(tiles, width, height, region_x, upper, (uint16_t)(length > 2U ? length - 2U : length), mirror);
		break;
	case 2U:
		length = clamped_region_length(region_width, 3U, 4U);
		place_mirrored_platform_span(tiles, width, height, region_x, lower, length, mirror);
		start_x = (uint16_t)(region_x + region_width - length);
		place_mirrored_platform_span(tiles, width, height, start_x, lower, length, mirror);
		start_x = (uint16_t)(region_x + region_width / 3U);
		place_mirrored_platform_span(tiles, width, height, start_x, upper, length, mirror);
		break;
	case 3U:
		length = clamped_region_length(region_width, 3U, 5U);
		start_x = (uint16_t)(region_x + region_width / 5U);
		place_mirrored_platform_span(tiles, width, height, start_x, lower, length, mirror);
		place_mirrored_platform_span(tiles, width, height, start_x, upper, length, mirror);
		break;
	case 4U:
	default:
		length = clamped_region_length(region_width, 5U, 8U);
		start_x = (uint16_t)(region_x + region_width / 6U);
		place_mirrored_platform_span(tiles, width, height, start_x, middle, length, mirror);
		if (region_width > 7U) {
			place_mirrored_platform_span(tiles, width, height, region_x, upper, 3U, mirror);
		}
		break;
	}
}

static void clear_spawn_corridor(TileType *tiles, uint16_t width, uint16_t height, uint16_t spawn_x, uint16_t spawn_y) {
	if (tiles == NULL || width == 0U || height == 0U) {
		return;
	}
	uint16_t min_x = spawn_x > 3U ? (uint16_t)(spawn_x - 3U) : 0U;
	uint16_t max_x = (uint16_t)(spawn_x + 3U);
	if (max_x >= width) {
		max_x = (uint16_t)(width - 1U);
	}
	uint16_t min_y = spawn_y > 3U ? (uint16_t)(spawn_y - 3U) : 0U;
	for (uint16_t y = min_y; y <= spawn_y; ++y) {
		for (uint16_t x = min_x; x <= max_x; ++x) {
			TileType tile = tiles[y * width + x];
			if (tile == TILE_PLATFORM || tile == TILE_HAZARD) {
				tiles[y * width + x] = TILE_EMPTY;
			}
		}
	}
}

static void place_center_feature(TileType *tiles, uint16_t width, uint16_t height) {
	if (width < 10U || height < 7U) {
		return;
	}

	uint16_t center_x = (uint16_t)(width / 2U);
	uint16_t feature = (uint16_t)random_between(0U, 2U);
	uint16_t middle = lane_y(height, 1U);
	uint16_t upper = lane_y(height, 2U);

	switch (feature) {
	case 0U:
		place_platform_span(tiles, width, height, (uint16_t)(center_x - 2U), middle, 4U);
		break;
	case 1U:
		place_platform_span(tiles, width, height, (uint16_t)(center_x - 3U), middle, 3U);
		place_platform_span(tiles, width, height, center_x, upper, 3U);
		break;
	case 2U:
	default:
		place_platform_span(tiles, width, height, (uint16_t)(center_x - 1U), middle, 2U);
		place_platform_span(tiles, width, height, (uint16_t)(center_x - 2U), upper, 4U);
		break;
	}
}

static void place_structured_platforms(TileType *tiles, uint16_t width, uint16_t height,
	const ArenaGenerationOptions *options, uint16_t left_spawn, uint16_t right_spawn) {
	uint16_t region_width = clamp_u16((uint16_t)(width / 10U), 8U, 18U);
	uint16_t left_limit = (uint16_t)(width / 2U);
	uint16_t center_margin = clamp_u16((uint16_t)(width / 16U), 3U, 8U);
	uint16_t region_count = 0U;
	uint16_t target_regions = 0U;
	uint16_t start_x = 2U;
	uint16_t max_x = 0U;

	if (left_limit <= center_margin + region_width + start_x) {
		place_center_feature(tiles, width, height);
		clear_spawn_corridor(tiles, width, height, left_spawn, (uint16_t)(height - 2U));
		clear_spawn_corridor(tiles, width, height, right_spawn, (uint16_t)(height - 2U));
		return;
	}

	max_x = (uint16_t)(left_limit - center_margin - region_width);
	for (uint16_t x = start_x; x <= max_x; x = (uint16_t)(x + region_width)) {
		region_count++;
	}

	target_regions = (uint16_t)(((uint32_t)region_count * (uint32_t)options->platform_density + 34U) / 70U);
	if (target_regions == 0U) {
		target_regions = 1U;
	}
	if (target_regions > region_count) {
		target_regions = region_count;
	}

	for (uint16_t x = start_x; x <= max_x; x = (uint16_t)(x + region_width)) {
		uint16_t remaining_regions = (uint16_t)(((max_x - x) / region_width) + 1U);
		if (target_regions == 0U) {
			break;
		}
		if (remaining_regions == target_regions || random_between(0U, 99U) < options->platform_density) {
			place_region_pattern(tiles, width, height, x, region_width, options->force_symmetry);
			target_regions--;
		}
	}

	if (!options->force_symmetry) {
		uint16_t right_start = (uint16_t)(left_limit + center_margin);
		uint16_t right_end = width > region_width + 2U ? (uint16_t)(width - region_width - 2U) : right_start;
		for (uint16_t x = right_start; x <= right_end; x = (uint16_t)(x + region_width)) {
			if (random_between(0U, 99U) < options->platform_density / 2U + 10U) {
				place_region_pattern(tiles, width, height, x, region_width, false);
			}
		}
	}

	place_center_feature(tiles, width, height);
	clear_spawn_corridor(tiles, width, height, left_spawn, (uint16_t)(height - 2U));
	clear_spawn_corridor(tiles, width, height, right_spawn, (uint16_t)(height - 2U));
}

static void place_hazards(TileType *tiles, uint16_t width, uint16_t height,
	const ArenaGenerationOptions *options) {
	if (height < 4U) {
		return;
	}

	for (uint8_t i = 0; i < options->hazard_count; ++i) {
		uint16_t y = (uint16_t)random_between((uint32_t)(height - 4U), (uint32_t)(height - 2U));
		uint16_t x = (uint16_t)random_between(2U, (uint32_t)(width - 3U));
		if (tiles[y * width + x] == TILE_EMPTY && tiles[(height - 1U) * width + x] == TILE_SOLID) {
			tiles[y * width + x] = TILE_HAZARD;
			if (options->force_symmetry) {
				uint16_t mx = (uint16_t)(width - 1U - x);
				if (tiles[(height - 1U) * width + mx] == TILE_SOLID) {
					tiles[y * width + mx] = TILE_HAZARD;
				}
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
	place_structured_platforms(arena->tiles, arena->width, arena->height, &tuned, left_spawn, right_spawn);
	place_hazards(arena->tiles, arena->width, arena->height, &tuned);
	
	return GAME_OK;
}
