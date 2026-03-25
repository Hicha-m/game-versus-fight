#ifndef ARENA_INTERNAL_H
#define ARENA_INTERNAL_H

#include "game/arena/arena.h"

void room_fill(Room* room, TileType type);
void room_build_floor(Room* room, i32 floor_row);
void room_build_platform(Room* room, i32 y, i32 x0, i32 x1);
bool room_is_inside(const Room* room, i32 tx, i32 ty);

#endif