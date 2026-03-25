#ifndef ENGINE_H
#define ENGINE_H

#include <SDL3/SDL.h>
#include "core/types.h"
#include "engine/input.h"


typedef struct EngineConfig {
	const char *title;
	i32 window_width;
	i32 window_height;
} EngineConfig;

typedef struct Engine {
	SDL_Window *window;
	SDL_Renderer *renderer;
	bool running;
	PlayerKeybind keybinds[MAX_PLAYERS];
	FrameInput previous_input;
} Engine;

/* Initialisation / destruction */
bool engine_init(Engine* engine, const EngineConfig* config);
void engine_shutdown(Engine* engine);

/* Input */
void engine_poll_input(Engine* engine, FrameInput* out_input);

/* Timing */
u64 engine_now_counter(void);
f64 engine_counter_seconds(u64 counter_delta);

#endif