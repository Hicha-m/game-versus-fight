#include "engine/engine.h"
#include "test_common.h"

#include <SDL3/SDL.h>

static int try_init_engine(Engine *engine) {
	EngineConfig config = {
		.title = "test",
		.window_width = 320,
		.window_height = 240,
	};

	/* Keep tests runnable in headless environments (CI, containers). */
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_CRITICAL);
	SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
	return (int)engine_init(engine, &config);
}

static int test_engine_init_creates_renderer(void) {
	Engine engine = {0};
	if (!try_init_engine(&engine)) {
		/* SDL renderer may be unavailable on this machine. */
		return 0;
	}

	TEST_ASSERT_TRUE(engine.running);
	TEST_ASSERT_TRUE(engine.window != NULL);
	TEST_ASSERT_TRUE(engine.renderer != NULL);
	engine_shutdown(&engine);
	return 0;
}

static int test_engine_shutdown_stops_running(void) {
	Engine engine = {0};
	if (!try_init_engine(&engine)) {
		return 0;
	}

	engine_shutdown(&engine);
	TEST_ASSERT_TRUE(!engine.running);
	TEST_ASSERT_TRUE(engine.window == NULL);
	TEST_ASSERT_TRUE(engine.renderer == NULL);
	return 0;
}

static int test_engine_shutdown_is_idempotent(void) {
	Engine engine = {0};
	engine_shutdown(&engine);
	TEST_ASSERT_TRUE(engine.window == NULL);
	TEST_ASSERT_TRUE(engine.renderer == NULL);
	TEST_ASSERT_TRUE(!engine.running);
	return 0;
}

static int test_engine_init_rejects_invalid_arguments(void) {
	TEST_ASSERT_TRUE(!engine_init(NULL, NULL));
	return 0;
}

int run_engine_tests(void) {
	TestCase cases[] = {
		{"init_creates_renderer", test_engine_init_creates_renderer},
		{"shutdown_stops_running", test_engine_shutdown_stops_running},
		{"shutdown_is_idempotent", test_engine_shutdown_is_idempotent},
		{"init_rejects_invalid_arguments", test_engine_init_rejects_invalid_arguments},
	};

	return run_test_cases("engine", cases, (int)(sizeof(cases) / sizeof(cases[0])));
}
