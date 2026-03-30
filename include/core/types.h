#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float    f32;
typedef double   f64;

typedef struct Vec2 {
    f32 x;
    f32 y;
} Vec2;

typedef struct IVec2 {
    i32 x;
    i32 y;
} IVec2;

typedef struct RectF {
    f32 x;
    f32 y;
    f32 w;
    f32 h;
} RectF;

typedef enum GameError {
    GAME_OK = 0,
    GAME_ERROR_INVALID_ARGUMENT,
    GAME_ERROR_INITIALIZATION,
    GAME_ERROR_NOT_FOUND,
    GAME_ERROR_INTERNAL
} GameError;

typedef enum GamePhase {
    GAME_PHASE_BOOT = 0,
    GAME_PHASE_MENU,
    GAME_PHASE_MATCH,
    GAME_PHASE_ROUND_END,
    GAME_PHASE_PAUSED,
    GAME_PHASE_VICTORY,
    GAME_PHASE_GAME_OVER
} GamePhase;

#endif
