#include "arena.h"
#include "test_common.h"

#include <string.h>

static uint16_t count_tiles(const Arena *arena, TileType tile) {
	uint16_t count = 0;
	for (uint16_t y = 0; y < arena->height; ++y) {
		for (uint16_t x = 0; x < arena->width; ++x) {
			if (arena->tiles[y * arena->width + x] == tile) {
				count++;
			}
		}
	}
	return count;
}

static int test_arena_generate_sets_basic_fields(void) {
	Arena arena = {0};
	GameError err = arena_generate(&arena, 32U, 12U, 42U);

	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_EQ_INT(32, arena.width);
	TEST_ASSERT_EQ_INT(12, arena.height);
	TEST_ASSERT_EQ_INT(42, arena.seed);
	TEST_ASSERT_TRUE(arena.tiles != NULL);  // Real implementation allocates tiles
	arena_destroy(&arena);  // Clean up
	return 0;
}

static int test_arena_get_tile_returns_empty_in_mock(void) {
	GameError err = GAME_ERROR_INTERNAL;
	TileType tile = arena_get_tile(NULL, 0U, 0U, &err);

	TEST_ASSERT_EQ_INT(GAME_OK, err);  // NULL arena returns GAME_OK with TILE_EMPTY
	TEST_ASSERT_EQ_INT(TILE_EMPTY, tile);
	return 0;
}

static int test_arena_find_spawn_returns_origin_in_mock(void) {
	Vec2i spawn = {-1, -1};
	bool found = arena_find_spawn(NULL, PLAYER_ONE, &spawn);

	TEST_ASSERT_TRUE(found);  // NULL arena returns safe defaults
	TEST_ASSERT_EQ_INT(0, spawn.x);
	TEST_ASSERT_EQ_INT(0, spawn.y);
	return 0;
}

static int test_arena_save_load_and_list_roundtrip(void) {
	Arena generated = {0};
	Arena loaded = {0};
	ArenaSaveSummary summary = {0};
	ArenaSaveSummary list[MENU_MAX_SAVED_MAPS] = {0};
	uint8_t count = 0;
	const char *name = "test_b1_roundtrip";
	const char *renamed = "test_b1_roundtrip_renamed";

	ArenaGenerationOptions options;
	arena_generation_options_defaults(&options, ARCHETYPE_SMALL);
	GameError err = arena_generate_with_options(&generated, 80U, 14U, 4242U, &options);
	TEST_ASSERT_EQ_INT(GAME_OK, err);

	err = arena_save_to_file(&generated, name, ARCHETYPE_SMALL);
	TEST_ASSERT_EQ_INT(GAME_OK, err);

	err = arena_load_from_file(&loaded, name, &summary);
	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_EQ_INT(generated.width, loaded.width);
	TEST_ASSERT_EQ_INT(generated.height, loaded.height);
	TEST_ASSERT_EQ_INT(generated.seed, loaded.seed);
	TEST_ASSERT_EQ_INT(ARCHETYPE_SMALL, summary.archetype);

	err = arena_list_saved_maps(list, MENU_MAX_SAVED_MAPS, &count);
	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_TRUE(count > 0);

	bool found = false;
	for (uint8_t i = 0; i < count; ++i) {
		if (strcmp(list[i].name, name) == 0) {
			found = true;
			break;
		}
	}
	TEST_ASSERT_TRUE(found);

	err = arena_rename_saved_map(name, renamed);
	TEST_ASSERT_EQ_INT(GAME_OK, err);

	err = arena_load_from_file(&loaded, renamed, &summary);
	TEST_ASSERT_EQ_INT(GAME_OK, err);

	err = arena_delete_saved_map(renamed);
	TEST_ASSERT_EQ_INT(GAME_OK, err);

	err = arena_load_from_file(&loaded, renamed, &summary);
	TEST_ASSERT_EQ_INT(GAME_ERROR_NOT_FOUND, err);

	arena_destroy(&generated);
	arena_destroy(&loaded);
	return 0;
}

static int test_arena_generation_changes_with_seed(void) {
	Arena first = {0};
	Arena second = {0};
	ArenaGenerationOptions options;
	arena_generation_options_defaults(&options, ARCHETYPE_MEDIUM);

	TEST_ASSERT_EQ_INT(GAME_OK, arena_generate_with_options(&first, 120U, 18U, 1234U, &options));
	TEST_ASSERT_EQ_INT(GAME_OK, arena_generate_with_options(&second, 120U, 18U, 9876U, &options));
	TEST_ASSERT_TRUE(memcmp(first.tiles, second.tiles,
		(size_t)first.width * first.height * sizeof(TileType)) != 0);

	arena_destroy(&first);
	arena_destroy(&second);
	return 0;
}

static int test_arena_generation_limits_platform_count(void) {
	Arena arena = {0};
	ArenaGenerationOptions options;
	uint16_t platform_count = 0;

	arena_generation_options_defaults(&options, ARCHETYPE_MEDIUM);
	TEST_ASSERT_EQ_INT(GAME_OK, arena_generate_with_options(&arena, 120U, 18U, 2026U, &options));

	platform_count = count_tiles(&arena, TILE_PLATFORM);
	TEST_ASSERT_TRUE(platform_count > 0U);
	TEST_ASSERT_TRUE(platform_count <= arena.width);

	arena_destroy(&arena);
	return 0;
}

int run_arena_tests(void) {
	TestCase cases[] = {
		{"generate_sets_basic_fields", test_arena_generate_sets_basic_fields},
		{"get_tile_returns_empty_in_mock", test_arena_get_tile_returns_empty_in_mock},
		{"find_spawn_returns_origin_in_mock", test_arena_find_spawn_returns_origin_in_mock},
		{"save_load_and_list_roundtrip", test_arena_save_load_and_list_roundtrip},
		{"generation_changes_with_seed", test_arena_generation_changes_with_seed},
		{"generation_limits_platform_count", test_arena_generation_limits_platform_count},
	};

	return run_test_cases("arena", cases, (int)(sizeof(cases) / sizeof(cases[0])));
}
