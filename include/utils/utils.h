#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include "core/types.h"

i32 clamp(i32 v, i32 lo, i32 hi);
f32 clampf(f32 v, f32 lo, f32 hi);
void clamp_min_zero_f32(f32* value);

#endif
