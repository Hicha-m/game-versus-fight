#include <stdio.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "core/constants.h"
#include "engine/engine.h"
#include "engine/engine_internal.h"

static InputButton make_button(bool current_down, bool previous_down)
{
    InputButton b;
    b.down = current_down;
    b.pressed = (current_down && !previous_down);
    b.released = (!current_down && previous_down);
    return b;
}

static void fill_player_input_from_keybind(
    const bool* keys,
    const PlayerKeybind* bind,
    const PlayerInput* previous,
    PlayerInput* out_current
)
{
    bool right_down = keys[bind->right];
    bool left_down = keys[bind->left];
    bool up_down = keys[bind->up];
    bool down_down = keys[bind->down];

    bool jump_down = keys[bind->jump];
    bool thrust_down = keys[bind->thrust];
    bool throw_down = keys[bind->throw_weapon];

    out_current->move_x = (right_down ? 1 : 0) - (left_down ? 1 : 0);
    out_current->move_y = (down_down ? 1 : 0) - (up_down ? 1 : 0);

    out_current->jump = make_button(jump_down, previous->jump.down);
    out_current->thrust = make_button(thrust_down, previous->thrust.down);
    out_current->throw_weapon = make_button(throw_down, previous->throw_weapon.down);
}

bool engine_init(Engine* engine, const EngineConfig* config)
{
    u32 sdl_init_flags;

    if (!engine || !config) {
        return false;
    }

    memset(engine, 0, sizeof(*engine));

    #if defined(GAME_TEST_MODE)
    sdl_init_flags = SDL_INIT_VIDEO;
    #else
    sdl_init_flags = SDL_INIT_VIDEO | SDL_INIT_GAMEPAD;
    #endif

    if (!SDL_Init(sdl_init_flags)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    engine->window = SDL_CreateWindow(
        config->title,
        config->window_width,
        config->window_height,
        0
    );

    if (!engine->window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    engine->renderer = SDL_CreateRenderer(engine->window, NULL);
    if (!engine->renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(engine->window);
        engine->window = NULL;
        SDL_Quit();
        return false;
    }

    input_set_default_keybinds(engine->keybinds);
    input_reset_frame(&engine->previous_input);

    engine->running = true;
    return true;
}

void engine_shutdown(Engine* engine)
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
    out_input->pause = make_button(pause_down, engine->previous_input.pause.down);
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
        }
    }

    {
        const bool* keys = SDL_GetKeyboardState(NULL);
        engine_apply_keyboard_to_frame_input(engine, keys, out_input);
    }

    engine->previous_input = *out_input;
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
