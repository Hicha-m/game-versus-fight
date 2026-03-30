#ifndef ARENA_H
#define ARENA_H

#include "core/types.h"
#include "core/constants.h"


typedef enum TileType {
    TILE_EMPTY = 0,
    TILE_SOLID,
    TILE_PLATFORM,
    TILE_HAZARD
} TileType;

typedef enum RoomType {
    ROOM_TYPE_START_A = 0,
    ROOM_TYPE_MID_LEFT,
    ROOM_TYPE_CENTER,
    ROOM_TYPE_MID_RIGHT,
    ROOM_TYPE_END_B
} RoomType;

typedef enum RoomDirection {
    ROOM_DIR_LEFT_TO_RIGHT = 0,
    ROOM_DIR_RIGHT_TO_LEFT
} RoomDirection;

typedef enum ArenaGenerationMode {
    ARENA_MODE_DEFAULT = 0,      /* Hardcoded static layout */
    ARENA_MODE_PROCEDURAL,        /* Random with seed + difficulty */
    ARENA_MODE_CORRIDOR          /* Generate corridor */
} ArenaGenerationMode;

typedef struct ArenaOptions {
    ArenaGenerationMode mode;
    u32 seed;                     /* Used if mode == ARENA_MODE_PROCEDURAL */
    i32 difficulty;               /* 0-10, used if mode == ARENA_MODE_PROCEDURAL */
} ArenaOptions;

typedef struct RoomSpawn {
    Vec2 attacker_spawn;
    Vec2 defender_spawn;
} RoomSpawn;

typedef struct RoomTransition {
    f32 left_trigger_x;
    f32 right_trigger_x;
} RoomTransition;

typedef struct Room {
    RoomType type;
    RoomDirection direction;

    i32 width_tiles;
    i32 height_tiles;

    TileType tiles[ROOM_HEIGHT_TILES][ROOM_MAX_WIDTH_TILES];

    RoomSpawn spawns;
    RoomTransition transition;
} Room;

typedef struct Arena {
    Room rooms[ROOM_COUNT];
    i32 room_count;
    i32 current_room;
} Arena;

/* Lifecycle */
bool arena_init(Arena* arena);
void arena_shutdown(Arena* arena);

/* Build / génération */
void arena_build_default(Arena* arena);
void arena_generate_corridor(Arena* arena, u32 seed);
void arena_generate_next(Arena* arena, u32 seed, i32 difficulty);

/* Initialization with options (main entry point) */
bool arena_init_with_options(Arena* arena, const ArenaOptions* options);

/* Preset option builders (helpers) */
ArenaOptions arena_options_default(void);
ArenaOptions arena_options_procedural(u32 seed, i32 difficulty);
ArenaOptions arena_options_corridor(u32 seed);

/* Access */
Room* arena_get_current_room(Arena* arena);
const Room* arena_get_current_room_const(const Arena* arena);
const Room* arena_get_room_const(const Arena* arena, i32 room_index);
i32 arena_middle_room_index(const Arena* arena);
bool arena_set_middle_room(Arena* arena);

/* Queries */
TileType room_get_tile(const Room* room, i32 tx, i32 ty);
bool room_is_solid_world(const Room* room, f32 world_x, f32 world_y);

/* Transition */
bool arena_can_transition_left(const Arena* arena, const Vec2* player_pos);
bool arena_can_transition_right(const Arena* arena, const Vec2* player_pos);
bool arena_transition_left(Arena* arena);
bool arena_transition_right(Arena* arena);

/* Validation */
bool room_validate_complete(const Room* room);
bool room_validate_spawns(const Room* room);
bool room_validate_accessibility(const Room* room);

#endif