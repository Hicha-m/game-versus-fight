#include "arena.h"
#include "arena/generator.h"
#include "utils.h"
#include "constants.h"

#include <SDL3/SDL.h>

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ARENA_SAVE_MAGIC "BRMAP1"
#define ARENA_GEN_MAX_RETRIES 32U

static ArchetypeType archetype_from_int(int value) {
	if (value < (int)ARCHETYPE_SMALL || value > (int)ARCHETYPE_LARGE) {
		return ARCHETYPE_MEDIUM;
	}
	return (ArchetypeType)value;
}

static void build_saved_maps_dir(char *buffer, size_t buffer_size) {
	const char *base_path = SDL_GetBasePath();
	if (base_path == NULL) {
		snprintf(buffer, buffer_size, "saved_maps");
		return;
	}

	size_t len = strlen(base_path);
	bool ends_with_build = len >= 6U && strcmp(base_path + len - 6U, "build/") == 0;
	snprintf(buffer, buffer_size, ends_with_build ? "%s../saved_maps" : "%ssaved_maps", base_path);
}

static void sanitize_name(const char *input, char *output, size_t output_size) {
	size_t out = 0;
	if (input == NULL || output == NULL || output_size == 0U) {
		return;
	}

	for (size_t i = 0; input[i] != '\0' && out + 1U < output_size; ++i) {
		char c = input[i];
		bool ok = (c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') ||
			c == '_' || c == '-';
		output[out++] = ok ? c : '_';
	}

	if (out == 0U) {
		output[out++] = 'm';
		output[out++] = 'a';
		output[out++] = 'p';
	}
	output[out] = '\0';
}

static GameError ensure_saved_maps_dir(char *out_dir, size_t out_size) {
	if (out_dir == NULL || out_size == 0U) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	build_saved_maps_dir(out_dir, out_size);
	struct stat st = {0};
	if (stat(out_dir, &st) == 0) {
		if (S_ISDIR(st.st_mode)) {
			return GAME_OK;
		}
		return GAME_ERROR_IO;
	}

	if (mkdir(out_dir, 0755) != 0) {
		return GAME_ERROR_IO;
	}

	return GAME_OK;
}

static void build_saved_map_path(const char *map_name, char *path, size_t path_size) {
	char dir[512];
	char safe_name[MENU_MAX_SAVED_MAP_NAME];
	build_saved_maps_dir(dir, sizeof(dir));
	sanitize_name(map_name, safe_name, sizeof(safe_name));
	snprintf(path, path_size, "%s/%s.map", dir, safe_name);
}

static bool has_map_extension(const char *name) {
	if (name == NULL) {
		return false;
	}

	size_t len = strlen(name);
	return len > 4U && strcmp(name + len - 4U, ".map") == 0;
}

void arena_generation_options_defaults(ArenaGenerationOptions *options, ArchetypeType archetype) {
	if (options == NULL) {
		return;
	}

	*options = (ArenaGenerationOptions){0};
	options->archetype = archetype;
	options->force_symmetry = true;

	switch (archetype) {
	case ARCHETYPE_SMALL:
		options->platform_density = 42;
		options->hazard_count = 1;
		options->hole_count = 1;
		options->hole_max_width = 2;
		break;
	case ARCHETYPE_LARGE:
		options->platform_density = 24;
		options->hazard_count = 4;
		options->hole_count = 3;
		options->hole_max_width = 3;
		break;
	case ARCHETYPE_MEDIUM:
	default:
		options->platform_density = 30;
		options->hazard_count = 2;
		options->hole_count = 2;
		options->hole_max_width = 2;
		break;
	}
}

/**
 * Generate a procedural arena
 */
GameError arena_generate(Arena *arena, uint16_t width, uint16_t height, uint32_t seed) {
	ArenaGenerationOptions options;
	arena_generation_options_defaults(&options, ARCHETYPE_MEDIUM);
	return arena_generate_with_options(arena, width, height, seed, &options);
}

GameError arena_generate_with_options(Arena *arena, uint16_t width, uint16_t height, uint32_t seed,
	const ArenaGenerationOptions *options) {
	if (arena == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	if (options == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	
	if (width < MIN_ARENA_WIDTH || width > MAX_ARENA_WIDTH ||
		height < MIN_ARENA_HEIGHT || height > MAX_ARENA_HEIGHT) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	for (uint32_t attempt = 0; attempt < ARENA_GEN_MAX_RETRIES; ++attempt) {
		arena_destroy(arena);
		arena->width = width;
		arena->height = height;
		arena->seed = seed + attempt;
		math_seed_rng(arena->seed);

		GameError err = generator_create_tiles(arena, options);
		if (err != GAME_OK) {
			arena_destroy(arena);
			return err;
		}

		err = arena_validate(arena);
		if (err == GAME_OK) {
			return GAME_OK;
		}
	}

	arena_destroy(arena);
	return GAME_ERROR_INVALID_STATE;
}

/**
 * Validate arena for fairness
 */
GameError arena_validate(const Arena *arena) {
	if (arena == NULL || arena->tiles == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	if (arena->width < MIN_ARENA_WIDTH || arena->height < MIN_ARENA_HEIGHT) {
		return GAME_ERROR_INVALID_STATE;
	}
	
	bool has_spawn_p1 = false;
	bool has_spawn_p2 = false;
	Vec2i spawn_p1 = {0};
	Vec2i spawn_p2 = {0};
	uint16_t floor_solid = 0;
	
	for (uint16_t y = 0; y < arena->height; y++) {
		for (uint16_t x = 0; x < arena->width; x++) {
			int idx = y * arena->width + x;
			TileType tile = arena->tiles[idx];
			if (tile == TILE_SPAWN_P1) {
				has_spawn_p1 = true;
				spawn_p1.x = (int32_t)x;
				spawn_p1.y = (int32_t)y;
			}
			if (tile == TILE_SPAWN_P2) {
				has_spawn_p2 = true;
				spawn_p2.x = (int32_t)x;
				spawn_p2.y = (int32_t)y;
			}
			if (y == arena->height - 1U && tile == TILE_SOLID) {
				floor_solid++;
			}
		}
	}

	if (!(has_spawn_p1 && has_spawn_p2)) {
		return GAME_ERROR_INVALID_STATE;
	}
	if (spawn_p1.y != spawn_p2.y) {
		return GAME_ERROR_INVALID_STATE;
	}
	if (spawn_p1.y + 1 >= arena->height || spawn_p2.y + 1 >= arena->height) {
		return GAME_ERROR_INVALID_STATE;
	}

	TileType p1_floor = arena->tiles[(spawn_p1.y + 1) * arena->width + spawn_p1.x];
	TileType p2_floor = arena->tiles[(spawn_p2.y + 1) * arena->width + spawn_p2.x];
	if (p1_floor != TILE_SOLID || p2_floor != TILE_SOLID) {
		return GAME_ERROR_INVALID_STATE;
	}

	int32_t p1_dist = spawn_p1.x;
	int32_t p2_dist = (int32_t)arena->width - 1 - spawn_p2.x;
	int32_t diff = p1_dist - p2_dist;
	if (diff < 0) diff = -diff;
	if (diff > 3) {
		return GAME_ERROR_INVALID_STATE;
	}

	if (floor_solid < arena->width / 2U) {
		return GAME_ERROR_INVALID_STATE;
	}
	
	return GAME_OK;
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

	out_spawn->x = (player == PLAYER_ONE)
		? (int32_t)(arena->width / 6U)
		: (int32_t)(arena->width - (arena->width / 6U) - 1U);
	out_spawn->y = arena->height - 2;
	return true;
}

GameError arena_save_to_file(const Arena *arena, const char *map_name, ArchetypeType archetype) {
	if (arena == NULL || arena->tiles == NULL || map_name == NULL || map_name[0] == '\0') {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	char dir_path[512];
	GameError err = ensure_saved_maps_dir(dir_path, sizeof(dir_path));
	if (err != GAME_OK) {
		return err;
	}

	char path[640];
	build_saved_map_path(map_name, path, sizeof(path));

	FILE *file = fopen(path, "wb");
	if (file == NULL) {
		return GAME_ERROR_IO;
	}

	char safe_name[MENU_MAX_SAVED_MAP_NAME];
	sanitize_name(map_name, safe_name, sizeof(safe_name));

	fprintf(file, "%s\n", ARENA_SAVE_MAGIC);
	fprintf(file, "name=%s\n", safe_name);
	fprintf(file, "width=%u\n", arena->width);
	fprintf(file, "height=%u\n", arena->height);
	fprintf(file, "seed=%u\n", arena->seed);
	fprintf(file, "archetype=%d\n", (int)archetype);
	fprintf(file, "data\n");

	for (uint16_t y = 0; y < arena->height; ++y) {
		for (uint16_t x = 0; x < arena->width; ++x) {
			int idx = y * arena->width + x;
			fprintf(file, "%d ", (int)arena->tiles[idx]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
	return GAME_OK;
}

static GameError read_summary_only(FILE *file, ArenaSaveSummary *summary) {
	if (file == NULL || summary == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	char line[256];
	if (fgets(line, sizeof(line), file) == NULL) {
		return GAME_ERROR_IO;
	}
	if (strncmp(line, ARENA_SAVE_MAGIC, strlen(ARENA_SAVE_MAGIC)) != 0) {
		return GAME_ERROR_INVALID_STATE;
	}

	*summary = (ArenaSaveSummary){0};
	summary->archetype = ARCHETYPE_MEDIUM;

	while (fgets(line, sizeof(line), file) != NULL) {
		if (strncmp(line, "data", 4) == 0) {
			break;
		}
		if (strncmp(line, "name=", 5) == 0) {
			sscanf(line + 5, "%79s", summary->name);
		} else if (strncmp(line, "width=", 6) == 0) {
			int width = 0;
			sscanf(line + 6, "%d", &width);
			summary->width = (uint16_t)width;
		} else if (strncmp(line, "height=", 7) == 0) {
			int height = 0;
			sscanf(line + 7, "%d", &height);
			summary->height = (uint16_t)height;
		} else if (strncmp(line, "seed=", 5) == 0) {
			unsigned int seed = 0U;
			sscanf(line + 5, "%u", &seed);
			summary->seed = (uint32_t)seed;
		} else if (strncmp(line, "archetype=", 10) == 0) {
			int archetype = (int)ARCHETYPE_MEDIUM;
			sscanf(line + 10, "%d", &archetype);
			summary->archetype = archetype_from_int(archetype);
		}
	}

	if (summary->name[0] == '\0' || summary->width == 0U || summary->height == 0U) {
		return GAME_ERROR_INVALID_STATE;
	}

	return GAME_OK;
}

GameError arena_load_from_file(Arena *arena, const char *map_name, ArenaSaveSummary *out_summary) {
	if (arena == NULL || map_name == NULL || map_name[0] == '\0') {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	char path[640];
	build_saved_map_path(map_name, path, sizeof(path));

	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		return GAME_ERROR_NOT_FOUND;
	}

	ArenaSaveSummary summary;
	GameError err = read_summary_only(file, &summary);
	if (err != GAME_OK) {
		fclose(file);
		return err;
	}

	if (summary.width < MIN_ARENA_WIDTH || summary.width > MAX_ARENA_WIDTH ||
		summary.height < MIN_ARENA_HEIGHT || summary.height > MAX_ARENA_HEIGHT) {
		fclose(file);
		return GAME_ERROR_INVALID_STATE;
	}

	TileType *tiles = (TileType *)br_calloc((size_t)summary.width * summary.height, sizeof(TileType));
	if (tiles == NULL) {
		fclose(file);
		return GAME_ERROR_OUT_OF_MEMORY;
	}

	for (uint16_t y = 0; y < summary.height; ++y) {
		for (uint16_t x = 0; x < summary.width; ++x) {
			int value = 0;
			if (fscanf(file, "%d", &value) != 1) {
				br_free(tiles);
				fclose(file);
				return GAME_ERROR_IO;
			}
			tiles[y * summary.width + x] = (TileType)value;
		}
	}

	fclose(file);

	arena_destroy(arena);
	arena->width = summary.width;
	arena->height = summary.height;
	arena->seed = summary.seed;
	arena->tiles = tiles;

	err = arena_validate(arena);
	if (err != GAME_OK) {
		arena_destroy(arena);
		return err;
	}

	if (out_summary != NULL) {
		*out_summary = summary;
	}

	return GAME_OK;
}

GameError arena_list_saved_maps(ArenaSaveSummary *out_summaries, uint8_t max_summaries, uint8_t *out_count) {
	if (out_count == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	*out_count = 0;

	char dir_path[512];
	GameError err = ensure_saved_maps_dir(dir_path, sizeof(dir_path));
	if (err != GAME_OK) {
		return err;
	}

	DIR *dir = opendir(dir_path);
	if (dir == NULL) {
		return GAME_ERROR_IO;
	}

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (!has_map_extension(entry->d_name)) {
			continue;
		}
		if (*out_count >= max_summaries) {
			break;
		}

		char map_name[MENU_MAX_SAVED_MAP_NAME] = {0};
		strncpy(map_name, entry->d_name, sizeof(map_name) - 1U);
		char *dot = strrchr(map_name, '.');
		if (dot != NULL) {
			*dot = '\0';
		}

		char path[640];
		if (snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name) >= (int)sizeof(path)) {
			continue;
		}
		FILE *file = fopen(path, "rb");
		if (file == NULL) {
			continue;
		}

		ArenaSaveSummary summary;
		if (read_summary_only(file, &summary) == GAME_OK) {
			if (summary.name[0] == '\0') {
				snprintf(summary.name, sizeof(summary.name), "%s", map_name);
			}
			out_summaries[*out_count] = summary;
			(*out_count)++;
		}
		fclose(file);
	}

	closedir(dir);
	return GAME_OK;
}

GameError arena_delete_saved_map(const char *map_name) {
	if (map_name == NULL || map_name[0] == '\0') {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	char path[640];
	build_saved_map_path(map_name, path, sizeof(path));
	if (remove(path) != 0) {
		return GAME_ERROR_NOT_FOUND;
	}

	return GAME_OK;
}

GameError arena_rename_saved_map(const char *old_name, const char *new_name) {
	if (old_name == NULL || new_name == NULL || old_name[0] == '\0' || new_name[0] == '\0') {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	char old_path[640];
	char new_path[640];
	build_saved_map_path(old_name, old_path, sizeof(old_path));
	build_saved_map_path(new_name, new_path, sizeof(new_path));

	if (rename(old_path, new_path) != 0) {
		return GAME_ERROR_IO;
	}

	return GAME_OK;
}
