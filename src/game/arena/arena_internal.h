#ifndef ARENA_INTERNAL_H
#define ARENA_INTERNAL_H

#include "game/arena/arena.h"

void room_fill(Room* room, TileType type);
void room_build_floor(Room* room, i32 floor_row);
void room_build_platform(Room* room, i32 y, i32 x0, i32 x1);
bool room_is_inside(const Room* room, i32 tx, i32 ty);

typedef enum GameplayType {
    GAMEPLAY_DUEL = 0,
    GAMEPLAY_VERTICAL,
    GAMEPLAY_PLATFORM,
    GAMEPLAY_CHOKEPOINT,
} GameplayType;

typedef struct RoomParams {
    i32 width_tiles;
    i32 platform_count;
    i32 min_platform_height;
    i32 max_platform_height;
    f32 height_variance;
    bool allow_ledges;
} RoomParams;

void arena_generate_next(Arena* arena, u32 seed, i32 difficulty);
bool room_generate_by_type(Room* room, GameplayType type, const RoomParams* params, u32 seed);

bool room_validate_accessibility(const Room* room);
bool room_validate_spawns(const Room* room);
bool room_validate_complete(const Room* room);

#endif
