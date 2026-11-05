#include <stdio.h>

#include "core/constants.h"
#include "core/types.h"
#include "engine/engine.h"
#include "game/config.h"
#include "game/game.h"
#include "render/render.h"
#include "utils/log.h"
#include "utils/memory.h"

int main(void)
{
    Engine *engine;
    Game game;
    RenderContext render;
    FrameInput input = {0};

    EngineConfig config;
    GameConfig game_config;
    f64 accumulator = 0.0;
    u64 last_counter;

    memory_init();

    config.title = WINDOW_TITLE;
    config.window_width = WINDOW_WIDTH;
    config.window_height = WINDOW_HEIGHT;
    config.fullscreen = false;

    config_set_defaults(&game_config);
    if (!config_load(&game_config))
    {
        log_warn("config_load failed, using default display settings");
    }
    config.fullscreen = game_config.fullscreen;

    engine = engine_create(&config);
    if (!engine)
    {
        log_error("engine_create failed");
        return 1;
    }

    u16 keys[MAX_PLAYERS][7];
    for (i32 p = 0; p < MAX_PLAYERS; p++)
        for (i32 a = 0; a < ACTION_COUNT; a++)
            keys[p][a] = game_config.keybinds[p].keys[a];
    engine_set_keys(engine, (const u16(*)[7])keys);

    if (!render_init(&render, engine))
    {
        log_error("render_init failed");
        engine_destroy(engine);
        return 1;
    }

    if (!game_init(&game))
    {
        log_error("game_init failed");
        render_shutdown(&render);
        engine_destroy(engine);
        return 1;
    }

    bool applied_fullscreen = game.menu.config.fullscreen;
    last_counter = engine_now_counter();

    while (engine_is_running(engine))
    {
        FrameInput sampled_input = {0};
        u64 now = engine_now_counter();
        f64 frame_time = engine_counter_seconds(now - last_counter);
        last_counter = now;

        if (frame_time > 0.25)
        {
            frame_time = 0.25;
        }

        accumulator += frame_time;

        engine_poll_input(engine, &sampled_input);
        input_accumulate_frame(&input, &sampled_input);
        if (sampled_input.key_pressed == 60)
            render.debug_draw = !render.debug_draw;

        if (input.quit_requested || game_is_quit_requested(&game))
        {
            engine_request_stop(engine);
        }

        while (accumulator >= FIXED_DT)
        {
            game_update(&game, &input, FIXED_DT);
            accumulator -= FIXED_DT;
            input_clear_edges(&input);
        }

        if (!game.menu.capturing_key)
        {
            for (i32 p = 0; p < MAX_PLAYERS; p++)
                for (i32 a = 0; a < ACTION_COUNT; a++)
                    keys[p][a] = game.menu.config.keybinds[p].keys[a];
            engine_set_keys(engine, (const u16(*)[7])keys);
        }
        if (applied_fullscreen != game.menu.config.fullscreen)
        {
            applied_fullscreen = game.menu.config.fullscreen;
            engine_set_fullscreen(engine, applied_fullscreen);
        }
        engine_audio_update(engine, game.menu.config.master_volume, game.menu.config.music_volume,
                            game.menu.config.sfx_volume, game.phase == GAME_PHASE_MATCH,
                            game.audio_events);
        game.audio_events = 0;
        render_frame(&render, engine, &game);
        engine_end_frame(engine);
    }

    game_shutdown(&game);
    render_shutdown(&render);
    engine_destroy(engine);

    if (memory_has_leaks())
    {
        MemoryStats stats = memory_get_stats();
        log_warn("Memory leaks detected: alloc=%llu free=%llu current=%llu peak=%llu",
                 (unsigned long long)stats.allocation_count, (unsigned long long)stats.free_count,
                 (unsigned long long)stats.bytes_current, (unsigned long long)stats.bytes_peak);
    }
    else
    {
        MemoryStats stats = memory_get_stats();
        log_info("Project tracker: no outstanding tracked allocations. peak=%llu bytes (SDL/system "
                 "allocations not measured)",
                 (unsigned long long)stats.bytes_peak);
    }

    memory_shutdown();
    return 0;
}
