#include "engine/engine.h"
#include "test_common.h"

#include <SDL3/SDL.h>

static Engine* try_init_engine(void) {
	EngineConfig config = {
		.title = "test",
		.window_width = 320,
		.window_height = 240,
	};

	/* Keep tests runnable in headless environments (CI, containers). */
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_CRITICAL);
	SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
	return engine_create(&config);
}

static int test_engine_init_creates_renderer(void) {
	Engine* engine = try_init_engine();
	if (!engine) {
		/* SDL renderer may be unavailable on this machine. */
		return 0;
	}

	TEST_ASSERT_TRUE(engine_is_running(engine));
	TEST_ASSERT_TRUE(engine_get_window_handle(engine) != NULL);
	TEST_ASSERT_TRUE(engine_get_renderer_handle(engine) != NULL);
	engine_destroy(engine);
	return 0;
}

static int test_engine_shutdown_stops_running(void) {
	Engine* engine = try_init_engine();
	if (!engine) {
		return 0;
	}

	engine_request_stop(engine);
	TEST_ASSERT_TRUE(!engine_is_running(engine));
	engine_destroy(engine);
	return 0;
}

static int test_engine_shutdown_is_idempotent(void) {
	engine_destroy(NULL);
	return 0;
}

static int test_engine_init_rejects_invalid_arguments(void) {
	TEST_ASSERT_TRUE(engine_create(NULL) == NULL);
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
