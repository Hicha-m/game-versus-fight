#include <stdio.h>

#include "core/constants.h"
#include "core/types.h"
#include "engine/engine.h"
#include "game/game.h"
#include "render/render.h"
#include "utils/log.h"
#include "utils/memory.h"

int main(void)
{
    Engine* engine;
    Game game;
    RenderContext render;

    EngineConfig config;
    f64 accumulator = 0.0;
    u64 last_counter;

    memory_init();

    config.title = WINDOW_TITLE;
    config.window_width = WINDOW_WIDTH;
    config.window_height = WINDOW_HEIGHT;

    engine = engine_create(&config);
    if (!engine) {
        log_error("engine_create failed");
        return 1;
    }

    if (!render_init(&render, engine)) {
        log_error("render_init failed");
        engine_destroy(engine);
        return 1;
    }

    if (!game_init(&game)) {
        log_error("game_init failed");
        render_shutdown(&render);
        engine_destroy(engine);
        return 1;
    }

    last_counter = engine_now_counter();

    while (engine_is_running(engine)) {
        u64 now = engine_now_counter();
        f64 frame_time = engine_counter_seconds(now - last_counter);
        FrameInput input;

        last_counter = now;

        if (frame_time > 0.25) {
            frame_time = 0.25;
        }

        accumulator += frame_time;

        engine_poll_input(engine, &input);

        if (input.quit_requested) {
            engine_request_stop(engine);
        }

        while (accumulator >= FIXED_DT) {
            game_update(&game, &input, FIXED_DT);
            accumulator -= FIXED_DT;
        }

        render_frame(&render, engine, &game, (f32)(accumulator / FIXED_DT));
    }

    game_shutdown(&game);
    render_shutdown(&render);
    engine_destroy(engine);

    if (memory_has_leaks()) {
        MemoryStats stats = memory_get_stats();
        log_warn(
            "Memory leaks detected: alloc=%llu free=%llu current=%llu peak=%llu",
            (unsigned long long)stats.allocation_count,
            (unsigned long long)stats.free_count,
            (unsigned long long)stats.bytes_current,
            (unsigned long long)stats.bytes_peak
        );
    } else {
        MemoryStats stats = memory_get_stats();
        log_info(
            "No memory leaks. peak=%llu bytes",
            (unsigned long long)stats.bytes_peak
        );
    }

    memory_shutdown();
    return 0;
}