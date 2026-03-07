#include "engine.h"
#include "test_common.h"

static int test_engine_init_sets_initial_state(void) {
	GameState state = {0};
	GameConfig config = {0};
	config.ai_difficulty = DIFFICULTY_NORMAL;

	GameError err = engine_init(&state, &config);
	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_EQ_INT(MATCH_PHASE_MENU, state.phase);
	TEST_ASSERT_EQ_INT(0, state.frame_index);
	TEST_ASSERT_TRUE(state.running);
	TEST_ASSERT_EQ_INT(DIFFICULTY_NORMAL, state.ai_difficulty);
	return 0;
}

static int test_engine_simulate_and_tick_increment_frame(void) {
	GameState state = {0};

	TEST_ASSERT_EQ_INT(GAME_OK, engine_simulate_frame(&state));
	TEST_ASSERT_EQ_INT(1, state.frame_index);

	TEST_ASSERT_EQ_INT(GAME_OK, engine_tick(&state, NULL));
	TEST_ASSERT_EQ_INT(2, state.frame_index);
	return 0;
}

static int test_engine_shutdown_stops_running(void) {
	GameState state = {0};
	state.running = true;

	GameError err = engine_shutdown(&state);
	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_TRUE(!state.running);
	return 0;
}

int run_engine_tests(void) {
	TestCase cases[] = {
		{"init_sets_initial_state", test_engine_init_sets_initial_state},
		{"simulate_and_tick_increment_frame", test_engine_simulate_and_tick_increment_frame},
		{"shutdown_stops_running", test_engine_shutdown_stops_running},
	};

	return run_test_cases("engine", cases, (int)(sizeof(cases) / sizeof(cases[0])));
}
