#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include "core/types.h"

f32 math_clampf(f32 v, f32 lo, f32 hi);
void math_clamp_min_zero_f32(f32* value);

#endif
