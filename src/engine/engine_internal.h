#ifndef ENGINE_INTERNAL_H
#define ENGINE_INTERNAL_H

#include "engine/engine.h"

void engine_apply_keyboard_to_frame_input(
    Engine* engine,
    const bool* keys,
    FrameInput* out_input
);

#endif 