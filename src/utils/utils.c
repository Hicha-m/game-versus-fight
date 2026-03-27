#include "utils/utils.h"

f32 clampf(f32 v, f32 lo, f32 hi)
{
    if (v < lo) {
        return lo;
    }

    if (v > hi) {
        return hi;
    }

    return v;
}

void clamp_min_zero_f32(f32* value)
{
    if (!value) {
        return;
    }

    if (*value < 0.0f) {
        *value = 0.0f;
    }
}
