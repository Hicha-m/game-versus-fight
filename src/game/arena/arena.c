#include <string.h>

#include "game/arena/arena.h"
#include "arena_internal.h"

static u32 lcg_next(u32* state)
{
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

void room_fill(Room* room, TileType type)
{
    i32 y;
    i32 x;

    if (!room) {
        return;
    }

    for (y = 0; y < ROOM_HEIGHT_TILES; ++y) {
        for (x = 0; x < ROOM_MAX_WIDTH_TILES; ++x) {
            room->tiles[y][x] = type;
        }
    }
}

void room_build_floor(Room* room, i32 floor_row)
{
    i32 x;

    if (!room || floor_row < 0 || floor_row >= room->height_tiles) {
        return;
    }

    for (x = 0; x < room->width_tiles; ++x) {
        room->tiles[floor_row][x] = TILE_SOLID;
        if (floor_row + 1 < room->height_tiles) {
            room->tiles[floor_row + 1][x] = TILE_SOLID;
        }
    }
}

void room_build_platform(Room* room, i32 y, i32 x0, i32 x1)
{
    i32 x;

    if (!room || y < 0 || y >= room->height_tiles) {
        return;
    }

    if (x0 < 0) x0 = 0;
    if (x1 >= room->width_tiles) x1 = room->width_tiles - 1;

    for (x = x0; x <= x1; ++x) {
        room->tiles[y][x] = TILE_PLATFORM;
    }
}

bool room_is_inside(const Room* room, i32 tx, i32 ty)
{
    if (!room) {
        return false;
    }

    return (tx >= 0 && tx < room->width_tiles &&
            ty >= 0 && ty < room->height_tiles);
}

static void room_setup_base(Room* room, RoomType type, RoomDirection direction, i32 width_tiles)
{
    room->type = type;
    room->direction = direction;
    room->width_tiles = width_tiles;
    room->height_tiles = ROOM_HEIGHT_TILES;

    room_fill(room, TILE_EMPTY);
    room_build_floor(room, ROOM_HEIGHT_TILES - 2);

    room->transition.left_trigger_x = 12.0f;
    room->transition.right_trigger_x = (f32)(width_tiles * TILE_SIZE - 12);

    room->spawns.attacker_spawn.x = 4.0f * TILE_SIZE;
    room->spawns.attacker_spawn.y = (f32)((ROOM_HEIGHT_TILES - 3) * TILE_SIZE) - PLAYER_HEIGHT;

    room->spawns.defender_spawn.x = (f32)((width_tiles - 5) * TILE_SIZE);
    room->spawns.defender_spawn.y = (f32)((ROOM_HEIGHT_TILES - 3) * TILE_SIZE) - PLAYER_HEIGHT;
}

bool arena_init(Arena* arena)
{
    if (!arena) {
        return false;
    }

    memset(arena, 0, sizeof(*arena));
    arena->room_count = ROOM_COUNT;
    arena->current_room = ROOM_COUNT / 2;
    return true;
}

void arena_shutdown(Arena* arena)
{
    (void)arena;
}

void arena_build_default(Arena* arena)
{
    Room* r;

    if (!arena) {
        return;
    }

    arena->room_count = ROOM_COUNT;
    arena->current_room = ROOM_COUNT / 2;

    r = &arena->rooms[0];
    room_setup_base(r, ROOM_TYPE_START_A, ROOM_DIR_LEFT_TO_RIGHT, ROOM_DEFAULT_WIDTH_TILES);
    room_build_platform(r, 13, 8, 14);

    r = &arena->rooms[1];
    room_setup_base(r, ROOM_TYPE_MID_LEFT, ROOM_DIR_LEFT_TO_RIGHT, ROOM_DEFAULT_WIDTH_TILES + 8);
    room_build_platform(r, 12, 14, 20);
    room_build_platform(r, 15, 28, 34);

    r = &arena->rooms[2];
    room_setup_base(r, ROOM_TYPE_CENTER, ROOM_DIR_LEFT_TO_RIGHT, ROOM_DEFAULT_WIDTH_TILES + 16);
    room_build_platform(r, 13, 18, 28);
    room_build_platform(r, 10, 34, 42);

    r = &arena->rooms[3];
    room_setup_base(r, ROOM_TYPE_MID_RIGHT, ROOM_DIR_RIGHT_TO_LEFT, ROOM_DEFAULT_WIDTH_TILES + 8);
    room_build_platform(r, 12, 10, 16);
    room_build_platform(r, 15, 24, 30);

    r = &arena->rooms[4];
    room_setup_base(r, ROOM_TYPE_END_B, ROOM_DIR_RIGHT_TO_LEFT, ROOM_DEFAULT_WIDTH_TILES);
    room_build_platform(r, 13, 22, 30);
}

void arena_generate_corridor(Arena* arena, u32 seed)
{
    i32 i;
    u32 s = seed;

    if (!arena) {
        return;
    }

    arena->room_count = ROOM_COUNT;
    arena->current_room = ROOM_COUNT / 2;

    for (i = 0; i < ROOM_COUNT; ++i) {
        Room* r = &arena->rooms[i];
        RoomType type;
        i32 width_tiles = ROOM_DEFAULT_WIDTH_TILES + (i % 3) * 6;
        i32 p0;
        i32 p1;

        type = (RoomType)i;

        room_setup_base(
            r,
            type,
            (i <= 3) ? ROOM_DIR_LEFT_TO_RIGHT : ROOM_DIR_RIGHT_TO_LEFT,
            width_tiles
        );

        p0 = 6 + (i32)(lcg_next(&s) % 10u);
        p1 = p0 + 6 + (i32)(lcg_next(&s) % 8u);
        room_build_platform(r, 13 - (i % 2), p0, p1);
        room_build_platform(r, 9 + (i % 3), width_tiles / 2 - 3, width_tiles / 2 + 3);
    }
}

bool arena_init_with_options(Arena* arena, const ArenaOptions* options)
{
    if (!arena || !options) {
        return false;
    }

    /* Initialize base arena */
    if (!arena_init(arena)) {
        return false;
    }

    /* Choose generation mode */
    switch (options->mode) {
    case ARENA_MODE_DEFAULT:
        /* Static hardcoded layout */
        arena_build_default(arena);
        return true;

    case ARENA_MODE_PROCEDURAL:
        /* Procedural with seed and difficulty */
        arena_generate_next(arena, options->seed, options->difficulty);
        return true;

    case ARENA_MODE_CORRIDOR:
        /* Corridor generation */
        arena_generate_corridor(arena, options->seed);
        return true;

    default:
        /* Fallback to default */
        arena_build_default(arena);
        return true;
    }
}

/* Helper: Create preset options */
ArenaOptions arena_options_default(void)
{
    ArenaOptions opts = {0};
    opts.mode = ARENA_MODE_DEFAULT;
    opts.seed = 0;
    opts.difficulty = 0;
    return opts;
}

ArenaOptions arena_options_procedural(u32 seed, i32 difficulty)
{
    ArenaOptions opts = {0};
    opts.mode = ARENA_MODE_PROCEDURAL;
    opts.seed = seed;
    opts.difficulty = (difficulty < 0) ? 0 : (difficulty > 10) ? 10 : difficulty;
    return opts;
}

ArenaOptions arena_options_corridor(u32 seed)
{
    ArenaOptions opts = {0};
    opts.mode = ARENA_MODE_CORRIDOR;
    opts.seed = seed;
    opts.difficulty = 0;
    return opts;
}

Room* arena_get_current_room(Arena* arena)
{
    if (!arena || arena->current_room < 0 || arena->current_room >= arena->room_count) {
        return NULL;
    }

    return &arena->rooms[arena->current_room];
}

const Room* arena_get_current_room_const(const Arena* arena)
{
    if (!arena || arena->current_room < 0 || arena->current_room >= arena->room_count) {
        return NULL;
    }

    return &arena->rooms[arena->current_room];
}

const Room* arena_get_room_const(const Arena* arena, i32 room_index)
{
    if (!arena || room_index < 0 || room_index >= arena->room_count) {
        return NULL;
    }

    return &arena->rooms[room_index];
}

i32 arena_middle_room_index(const Arena* arena)
{
    if (!arena || arena->room_count <= 0) {
        return 0;
    }

    return arena->room_count / 2;
}

bool arena_set_middle_room(Arena* arena)
{
    i32 middle;

    if (!arena || arena->room_count <= 0) {
        return false;
    }

    middle = arena_middle_room_index(arena);
    if (middle < 0 || middle >= arena->room_count) {
        return false;
    }

    arena->current_room = middle;
    return true;
}

TileType room_get_tile(const Room* room, i32 tx, i32 ty)
{
    if (!room_is_inside(room, tx, ty)) {
        return TILE_EMPTY;
    }

    return room->tiles[ty][tx];
}

bool room_is_solid_world(const Room* room, f32 world_x, f32 world_y)
{
    i32 tx;
    i32 ty;
    TileType tile;

    if (!room) {
        return false;
    }

    tx = (i32)(world_x / TILE_SIZE);
    ty = (i32)(world_y / TILE_SIZE);

    tile = room_get_tile(room, tx, ty);
    return (tile == TILE_SOLID);
}

bool arena_can_transition_left(const Arena* arena, const Vec2* player_pos)
{
    const Room* room;

    if (!arena || !player_pos) {
        return false;
    }

    room = arena_get_current_room_const(arena);
    if (!room || arena->current_room <= 0) {
        return false;
    }

    return (player_pos->x <= room->transition.left_trigger_x);
}

bool arena_can_transition_right(const Arena* arena, const Vec2* player_pos)
{
    const Room* room;

    if (!arena || !player_pos) {
        return false;
    }

    room = arena_get_current_room_const(arena);
    if (!room || arena->current_room >= arena->room_count - 1) {
        return false;
    }

    return (player_pos->x + PLAYER_WIDTH >= room->transition.right_trigger_x);
}

bool arena_transition_left(Arena* arena)
{
    if (!arena || arena->current_room <= 0) {
        return false;
    }

    arena->current_room--;
    return true;
}

bool arena_transition_right(Arena* arena)
{
    if (!arena || arena->current_room >= arena->room_count - 1) {
        return false;
    }

    arena->current_room++;
    return true;
}
