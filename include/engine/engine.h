#ifndef ENGINE_H
#define ENGINE_H

#include "core/types.h"
#include "engine/input.h"

typedef struct Engine Engine;

typedef struct EngineConfig
{
    const char *title;
    i32 window_width;
    i32 window_height;
    bool fullscreen;
} EngineConfig;

Engine *engine_create(const EngineConfig *config);
void engine_destroy(Engine *engine);

bool engine_is_running(const Engine *engine);
void engine_request_stop(Engine *engine);

void engine_poll_input(Engine *engine, FrameInput *out_input);

void *engine_get_window_handle(Engine *engine);
void *engine_get_renderer_handle(Engine *engine);

u64 engine_now_counter(void);
f64 engine_counter_seconds(u64 counter_delta);

void engine_set_keys(Engine *engine, const u16 keys[MAX_PLAYERS][7]);

void engine_set_fullscreen(Engine *engine, bool enabled);
void engine_audio_update(Engine *engine, f32 master, f32 music, f32 effects, bool playing,
                         u32 events);
void engine_end_frame(Engine *engine);

#endif
