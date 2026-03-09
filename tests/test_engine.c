#include "engine.h"
#include "test_common.h"

static int test_engine_init_creates_renderer(void) {
	GameError err = engine_init();
	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_TRUE(engine_renderer_get() != NULL);
	return 0;
}

static int test_engine_shutdown_stops_running(void) {
	GameError err = engine_shutdown();
	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_TRUE(engine_renderer_get() == NULL);
	return 0;
}

static int test_engine_shutdown_is_idempotent(void) {
	TEST_ASSERT_EQ_INT(GAME_OK, engine_shutdown());
	TEST_ASSERT_TRUE(engine_renderer_get() == NULL);
	return 0;
}

int run_engine_tests(void) {
	TestCase cases[] = {
		{"init_creates_renderer", test_engine_init_creates_renderer},
		{"shutdown_stops_running", test_engine_shutdown_stops_running},
		{"shutdown_is_idempotent", test_engine_shutdown_is_idempotent},
	};

	return run_test_cases("engine", cases, (int)(sizeof(cases) / sizeof(cases[0])));
}
