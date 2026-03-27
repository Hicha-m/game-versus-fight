#ifndef ENGINE_H
#define ENGINE_H

#include "core/types.h"
#include "engine/input.h"

typedef struct Engine Engine;

typedef struct EngineConfig {
	const char* title;
	i32 window_width;
	i32 window_height;
} EngineConfig;

/* Initialisation / destruction */
Engine* engine_create(const EngineConfig* config);
void engine_destroy(Engine* engine);

/* State */
bool engine_is_running(const Engine* engine);
void engine_request_stop(Engine* engine);

/* Input */
void engine_poll_input(Engine* engine, FrameInput* out_input);

/* Native handles (engine/render boundary) */
void* engine_get_window_handle(Engine* engine);
void* engine_get_renderer_handle(Engine* engine);

/* Timing */
u64 engine_now_counter(void);
f64 engine_counter_seconds(u64 counter_delta);

#endif