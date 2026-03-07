#include "game.h"
#include "test_common.h"

static int test_game_create_initializes_state(void) {
	GameState state = {0};
	GameConfig config = {0};

	GameError err = game_create(&config, &state);
	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_TRUE(state.running);
	TEST_ASSERT_EQ_INT(0, state.frame_index);
	return 0;
}

static int test_game_update_increments_frame(void) {
	GameState state = {0};
	FrameInput input = {0};

	TEST_ASSERT_EQ_INT(GAME_OK, game_update(&state, &input));
	TEST_ASSERT_EQ_INT(1, state.frame_index);
	return 0;
}

static int test_game_destroy_and_error_string(void) {
	GameState state = {0};
	state.running = true;

	TEST_ASSERT_EQ_INT(GAME_OK, game_destroy(&state));
	TEST_ASSERT_TRUE(!state.running);
	TEST_ASSERT_TRUE(game_error_string(GAME_OK) != NULL);
	return 0;
}

int run_game_tests(void) {
	TestCase cases[] = {
		{"create_initializes_state", test_game_create_initializes_state},
		{"update_increments_frame", test_game_update_increments_frame},
		{"destroy_and_error_string", test_game_destroy_and_error_string},
	};

	return run_test_cases("game", cases, (int)(sizeof(cases) / sizeof(cases[0])));
}
