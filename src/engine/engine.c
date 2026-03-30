#include <stdio.h>
#include <stdlib.h>

#include "engine/engine.h"
#include "engine/engine_internal.h"

static void engine_toggle_fullscreen(Engine* engine)
{
    bool is_fullscreen;

    if (!engine || !engine->window) {
        return;
    }

    is_fullscreen = (SDL_GetWindowFlags(engine->window) & SDL_WINDOW_FULLSCREEN) != 0;
    if (!SDL_SetWindowFullscreen(engine->window, !is_fullscreen)) {
        fprintf(stderr, "SDL_SetWindowFullscreen failed: %s\n", SDL_GetError());
    }
}

Engine* engine_create(const EngineConfig* config)
{
    Engine* engine;
    u32 sdl_init_flags;
    SDL_WindowFlags window_flags;

    if (!config) { 
        return NULL;
    }

    engine = (Engine*)calloc(1, sizeof(*engine));
    if (!engine) {
        return NULL;
    }

    #if defined(GAME_TEST_MODE)
    sdl_init_flags = SDL_INIT_VIDEO;
    #else
    sdl_init_flags = SDL_INIT_VIDEO | SDL_INIT_GAMEPAD;
    #endif

    if (!SDL_Init(sdl_init_flags)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        free(engine);
        return NULL;
    }

    window_flags = config->fullscreen ? SDL_WINDOW_FULLSCREEN : 0;

    engine->window = SDL_CreateWindow(
        config->title,
        config->window_width,
        config->window_height,
        window_flags
    );

    if (!engine->window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        free(engine);
        return NULL;
    }

    engine->renderer = SDL_CreateRenderer(engine->window, NULL);
    if (!engine->renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(engine->window);
        engine->window = NULL;
        SDL_Quit();
        free(engine);
        return NULL;
    }

    input_set_default_keybinds(engine->keybinds);
    input_reset_frame(&engine->previous_input);

    engine->running = true;
    return engine;
}

void engine_destroy(Engine* engine)
{
    if (!engine) {
        return;
    }

    if (engine->renderer) {
        SDL_DestroyRenderer(engine->renderer);
        engine->renderer = NULL;
    }

    if (engine->window) {
        SDL_DestroyWindow(engine->window);
        engine->window = NULL;
    }

    engine->running = false;
    SDL_Quit();
    free(engine);
}

bool engine_is_running(const Engine* engine)
{
    if (!engine) {
        return false;
    }

    return engine->running;
}

void engine_request_stop(Engine* engine)
{
    if (!engine) {
        return;
    }

    engine->running = false;
}

void engine_apply_keyboard_to_frame_input(
    Engine* engine,
    const bool* keys,
    FrameInput* out_input
)
{
    const PlayerInput* prev_p1;
    const PlayerInput* prev_p2;
    bool pause_down;

    if (!engine || !keys || !out_input) {
        return;
    }

    prev_p1 = &engine->previous_input.players[0];
    prev_p2 = &engine->previous_input.players[1];

    fill_player_input_from_keybind(keys, &engine->keybinds[0], prev_p1, &out_input->players[0]);
    fill_player_input_from_keybind(keys, &engine->keybinds[1], prev_p2, &out_input->players[1]);

    pause_down = keys[SDL_SCANCODE_ESCAPE];
    update_button(&out_input->pause, pause_down, engine->previous_input.pause.down);
}

void engine_poll_input(Engine* engine, FrameInput* out_input)
{
    SDL_Event event;

    if (!engine || !out_input) {
        return;
    }

    input_reset_frame(out_input);

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            out_input->quit_requested = true;
            engine->running = false;
            continue;
        }

        if (event.type == SDL_EVENT_KEY_DOWN &&
            event.key.scancode == SDL_SCANCODE_F11 &&
            !event.key.repeat) {
            engine_toggle_fullscreen(engine);
        }
    }

    {
        const bool* keys = SDL_GetKeyboardState(NULL);
        engine_apply_keyboard_to_frame_input(engine, keys, out_input);
    }

    engine->previous_input = *out_input;
}

void* engine_get_window_handle(Engine* engine)
{
    if (!engine) {
        return NULL;
    }

    return (void*)engine->window;
}

void* engine_get_renderer_handle(Engine* engine)
{
    if (!engine) {
        return NULL;
    }

    return (void*)engine->renderer;
}

u64 engine_now_counter(void)
{
    return (u64)SDL_GetPerformanceCounter();
}

f64 engine_counter_seconds(u64 counter_delta)
{
    u64 freq = (u64)SDL_GetPerformanceFrequency();
    if (freq == 0) {
        return 0.0;
    }

    return (f64)counter_delta / (f64)freq;
}
