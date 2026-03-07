#include "arena.h"
#include "test_common.h"

static int test_arena_generate_sets_basic_fields(void) {
	Arena arena = {0};
	GameError err = arena_generate(&arena, 32U, 12U, 42U);

	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_EQ_INT(32, arena.width);
	TEST_ASSERT_EQ_INT(12, arena.height);
	TEST_ASSERT_EQ_INT(42, arena.seed);
	TEST_ASSERT_TRUE(arena.tiles == NULL);
	return 0;
}

static int test_arena_get_tile_returns_empty_in_mock(void) {
	GameError err = GAME_ERROR_INTERNAL;
	TileType tile = arena_get_tile(NULL, 0U, 0U, &err);

	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_EQ_INT(TILE_EMPTY, tile);
	return 0;
}

static int test_arena_find_spawn_returns_origin_in_mock(void) {
	Vec2i spawn = {-1, -1};
	bool found = arena_find_spawn(NULL, PLAYER_ONE, &spawn);

	TEST_ASSERT_TRUE(found);
	TEST_ASSERT_EQ_INT(0, spawn.x);
	TEST_ASSERT_EQ_INT(0, spawn.y);
	return 0;
}

int run_arena_tests(void) {
	TestCase cases[] = {
		{"generate_sets_basic_fields", test_arena_generate_sets_basic_fields},
		{"get_tile_returns_empty_in_mock", test_arena_get_tile_returns_empty_in_mock},
		{"find_spawn_returns_origin_in_mock", test_arena_find_spawn_returns_origin_in_mock},
	};

	return run_test_cases("arena", cases, (int)(sizeof(cases) / sizeof(cases[0])));
}
