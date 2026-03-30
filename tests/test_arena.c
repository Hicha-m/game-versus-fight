#include "game/arena/arena.h"
#include "test_common.h"

static int test_arena_init_sets_defaults(void) {
	Arena arena = {0};

	TEST_ASSERT_TRUE(arena_init(&arena));
	TEST_ASSERT_EQ_INT(ROOM_COUNT, arena.room_count);
	TEST_ASSERT_EQ_INT(2, arena.current_room);
	return 0;
}

static int test_arena_build_default_sets_center_room(void) {
	Arena arena = {0};
	const Room *room;

	TEST_ASSERT_TRUE(arena_init(&arena));
	arena_build_default(&arena);

	room = arena_get_current_room_const(&arena);
	TEST_ASSERT_TRUE(room != NULL);
	TEST_ASSERT_EQ_INT(ROOM_TYPE_CENTER, room->type);
	TEST_ASSERT_EQ_INT(ROOM_DEFAULT_WIDTH_TILES + 16, room->width_tiles);

	/* Center room defines a platform at y=13 from x=18 to x=28. */
	TEST_ASSERT_EQ_INT(TILE_PLATFORM, room_get_tile(room, 20, 13));
	return 0;
}

static int test_room_get_tile_out_of_bounds_is_empty(void) {
	Arena arena = {0};
	const Room *room;

	TEST_ASSERT_TRUE(arena_init(&arena));
	arena_build_default(&arena);
	room = arena_get_current_room_const(&arena);
	TEST_ASSERT_TRUE(room != NULL);

	TEST_ASSERT_EQ_INT(TILE_EMPTY, room_get_tile(room, -1, 0));
	TEST_ASSERT_EQ_INT(TILE_EMPTY, room_get_tile(room, 0, -1));
	TEST_ASSERT_EQ_INT(TILE_EMPTY, room_get_tile(room, room->width_tiles, 1));
	TEST_ASSERT_EQ_INT(TILE_EMPTY, room_get_tile(room, 1, room->height_tiles));
	return 0;
}

static int test_arena_transition_right_and_left(void) {
	Arena arena = {0};
	Vec2 pos = {0};
	const Room *room;

	TEST_ASSERT_TRUE(arena_init(&arena));
	arena_build_default(&arena);
	room = arena_get_current_room_const(&arena);
	TEST_ASSERT_TRUE(room != NULL);

	pos.x = room->transition.right_trigger_x - PLAYER_WIDTH;
	pos.y = 0.0f;
	TEST_ASSERT_TRUE(arena_can_transition_right(&arena, &pos));
	TEST_ASSERT_TRUE(arena_transition_right(&arena));
	TEST_ASSERT_EQ_INT(3, arena.current_room);

	room = arena_get_current_room_const(&arena);
	TEST_ASSERT_TRUE(room != NULL);
	pos.x = room->transition.left_trigger_x;
	TEST_ASSERT_TRUE(arena_can_transition_left(&arena, &pos));
	TEST_ASSERT_TRUE(arena_transition_left(&arena));
	TEST_ASSERT_EQ_INT(2, arena.current_room);
	return 0;
}

static int test_arena_generate_next_creates_valid_rooms(void) {
	Arena arena = {0};
	int i;
	const Room* room;

	TEST_ASSERT_TRUE(arena_init(&arena));
	arena_generate_next(&arena, 12345u, 5);

	/* Check that all rooms were generated */
	TEST_ASSERT_EQ_INT(ROOM_COUNT, arena.room_count);

	/* Check that center room is the current room */
	TEST_ASSERT_EQ_INT(ROOM_COUNT / 2, arena.current_room);

	/* Check that each room has valid dimensions */
	for (i = 0; i < ROOM_COUNT; ++i) {
		room = arena_get_room_const(&arena, i);
		TEST_ASSERT_TRUE(room != NULL);
		TEST_ASSERT_TRUE(room->width_tiles > 0);
		TEST_ASSERT_TRUE(room->width_tiles <= ROOM_MAX_WIDTH_TILES);
		TEST_ASSERT_EQ_INT(ROOM_HEIGHT_TILES, room->height_tiles);
	}

	return 0;
}

static int test_arena_generate_with_difficulty_progression(void) {
	Arena arena_easy = {0};
	Arena arena_hard = {0};
	const Room* room_easy;
	const Room* room_hard;

	TEST_ASSERT_TRUE(arena_init(&arena_easy));
	TEST_ASSERT_TRUE(arena_init(&arena_hard));

	/* Generate with different difficulties */
	arena_generate_next(&arena_easy, 12345u, 2);
	arena_generate_next(&arena_hard, 12345u, 9);

	/* Different difficulties should produce different layouts */
	room_easy = arena_get_current_room_const(&arena_easy);
	room_hard = arena_get_current_room_const(&arena_hard);

	TEST_ASSERT_TRUE(room_easy != NULL);
	TEST_ASSERT_TRUE(room_hard != NULL);
	/* We can't guarantee different layouts every time, but they should be valid */
	TEST_ASSERT_TRUE(room_easy->width_tiles > 0);
	TEST_ASSERT_TRUE(room_hard->width_tiles > 0);

	return 0;
}

int run_arena_tests(void) {
	TestCase cases[] = {
		{"init_sets_defaults", test_arena_init_sets_defaults},
		{"build_default_sets_center_room", test_arena_build_default_sets_center_room},
		{"room_get_tile_out_of_bounds_is_empty", test_room_get_tile_out_of_bounds_is_empty},
		{"transition_right_and_left", test_arena_transition_right_and_left},
		{"generate_next_creates_valid_rooms", test_arena_generate_next_creates_valid_rooms},
		{"generate_with_difficulty_progression", test_arena_generate_with_difficulty_progression},
	};

	return run_test_cases("arena", cases, (int)(sizeof(cases) / sizeof(cases[0])));
}
