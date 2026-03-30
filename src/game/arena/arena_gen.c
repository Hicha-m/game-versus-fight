#include <stdlib.h>
#include <math.h>

#include "game/arena/arena.h"
#include "arena_internal.h"

/* =========================
   LCG PRNG (reproducible)
   ========================= */

static u32 lcg_next(u32* state)
{
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

static i32 lcg_range(u32* state, i32 min, i32 max)
{
    if (min >= max) return min;
    return min + (i32)(lcg_next(state) % (u32)(max - min + 1));
}

static i32 clamp_i32(i32 v, i32 lo, i32 hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void room_build_ground_column(Room* room, i32 x, i32 ground_y)
{
    i32 y;

    if (!room || x < 0 || x >= room->width_tiles) {
        return;
    }

    for (y = clamp_i32(ground_y, 0, room->height_tiles - 1); y < room->height_tiles; ++y) {
        room->tiles[y][x] = TILE_SOLID;
    }
}

static void room_apply_terrain_profile(Room* room, u32* seed, i32 difficulty)
{
    i32 x;
    i32 base_ground = ROOM_HEIGHT_TILES - 2;
    i32 slope_every = 4;
    i32 min_ground = 15;
    i32 max_ground = ROOM_HEIGHT_TILES - 2;
    i32 holes;
    i32 i;
    i32 protected_left = 8;
    i32 protected_right = 10;
    i32 current_ground;

    if (!room || !seed) {
        return;
    }

    current_ground = base_ground - (difficulty / 4);
    current_ground = clamp_i32(current_ground, min_ground, max_ground);

    for (x = 0; x < room->width_tiles; ++x) {
        if (x > protected_left && x < room->width_tiles - protected_right) {
            if ((x % slope_every) == 0) {
                i32 delta = lcg_range(seed, -1, 1);
                current_ground = clamp_i32(current_ground + delta, min_ground, max_ground);
            }
        } else {
            current_ground = base_ground;
        }

        room_build_ground_column(room, x, current_ground);
    }

    holes = 1 + (difficulty / 4);
    if (holes > 3) {
        holes = 3;
    }

    for (i = 0; i < holes; ++i) {
        i32 hole_width = 2 + (difficulty >= 7 ? 1 : 0);
        i32 hx = lcg_range(seed, protected_left + 4, room->width_tiles - protected_right - hole_width - 4);
        i32 y;

        for (x = hx; x < hx + hole_width; ++x) {
            for (y = 0; y < room->height_tiles; ++y) {
                if (room->tiles[y][x] == TILE_SOLID) {
                    room->tiles[y][x] = TILE_HAZARD;
                }
            }
        }
    }
}

static void room_add_end_landmark(Room* room)
{
    i32 floor_y = ROOM_HEIGHT_TILES - 2;
    i32 gate_x;
    i32 y;

    if (!room || room->width_tiles < 20) {
        return;
    }

    gate_x = room->width_tiles - 3;

    for (y = floor_y - 6; y <= floor_y; ++y) {
        if (y >= 0 && y < room->height_tiles) {
            room->tiles[y][gate_x] = TILE_SOLID;
        }
    }

    room_build_platform(room, floor_y - 5, gate_x - 8, gate_x - 2);
}

static void room_set_camera_safe_spawns(Room* room)
{
    i32 camera_tiles;
    i32 left_spawn_tx;
    i32 right_spawn_tx;
    i32 left_found_tx;
    i32 right_found_tx;
    bool left_found = false;
    bool right_found = false;
    i32 dx;
    i32 y;

    if (!room) {
        return;
    }

    camera_tiles = WINDOW_WIDTH / TILE_SIZE;
    left_spawn_tx = 4;
    right_spawn_tx = left_spawn_tx + (camera_tiles - 12);
    right_spawn_tx = clamp_i32(right_spawn_tx, left_spawn_tx + 8, room->width_tiles - 6);

    left_found_tx = left_spawn_tx;
    right_found_tx = right_spawn_tx;

    for (dx = 0; dx <= 4 && !left_found; ++dx) {
        i32 candidates[2] = {left_spawn_tx + dx, left_spawn_tx - dx};
        i32 c;

        for (c = 0; c < 2; ++c) {
            i32 tx = candidates[c];
            if (tx < 1 || tx >= room->width_tiles - 1) {
                continue;
            }
            for (y = ROOM_HEIGHT_TILES - 2; y >= 0; --y) {
                TileType t = room_get_tile(room, tx, y);
                if (t == TILE_SOLID || t == TILE_PLATFORM) {
                    left_found = true;
                    left_found_tx = tx;
                    room->spawns.attacker_spawn.y = (f32)(y * TILE_SIZE) - PLAYER_HEIGHT;
                    break;
                }
            }
            if (left_found) {
                break;
            }
        }
    }

    for (dx = 0; dx <= 5 && !right_found; ++dx) {
        i32 candidates[2] = {right_spawn_tx - dx, right_spawn_tx + dx};
        i32 c;

        for (c = 0; c < 2; ++c) {
            i32 tx = candidates[c];
            if (tx < 1 || tx >= room->width_tiles - 1) {
                continue;
            }
            for (y = ROOM_HEIGHT_TILES - 2; y >= 0; --y) {
                TileType t = room_get_tile(room, tx, y);
                if (t == TILE_SOLID || t == TILE_PLATFORM) {
                    right_found = true;
                    right_found_tx = tx;
                    room->spawns.defender_spawn.y = (f32)(y * TILE_SIZE) - PLAYER_HEIGHT;
                    break;
                }
            }
            if (right_found) {
                break;
            }
        }
    }

    room->spawns.attacker_spawn.x = (f32)(left_found_tx * TILE_SIZE);
    room->spawns.defender_spawn.x = (f32)(right_found_tx * TILE_SIZE);
}

/* =========================
   Gameplay type selection
   (difficulty influences type)
   ========================= */

static GameplayType room_select_gameplay_type(RoomType type, i32 difficulty)
{
    /* difficulty: 0 (easy) to 10 (hard) */
    
    switch (type) {
    case ROOM_TYPE_START_A:
        return GAMEPLAY_DUEL;    /* Always start simple */
    
    case ROOM_TYPE_END_B:
        return GAMEPLAY_DUEL;    /* Always end simple */
    
    case ROOM_TYPE_MID_LEFT:
    case ROOM_TYPE_MID_RIGHT:
        /* Ramps up gradually */
        if (difficulty < 3)      return GAMEPLAY_DUEL;
        if (difficulty < 6)      return GAMEPLAY_VERTICAL;
        return GAMEPLAY_PLATFORM;
    
    case ROOM_TYPE_CENTER:
        /* Peak difficulty: always complex */
        if (difficulty < 4)      return GAMEPLAY_VERTICAL;
        if (difficulty < 7)      return GAMEPLAY_PLATFORM;
        return GAMEPLAY_CHOKEPOINT;
    
    default:
        return GAMEPLAY_DUEL;
    }
}

/* =========================
   Parameter calculation
   (based on difficulty + gameplay type)
   ========================= */

static void room_calc_params(
    RoomParams* params,
    GameplayType type,
    i32 difficulty,
    i32 base_width)
{
    /* difficulty: 0-10 */
    params->width_tiles = base_width;
    params->allow_ledges = true;
    
    switch (type) {
    case GAMEPLAY_DUEL:
        params->platform_count = 1;
        params->min_platform_height = 12;
        params->max_platform_height = 13;
        params->height_variance = 0.5f;
        break;
    
    case GAMEPLAY_VERTICAL:
        params->platform_count = 2 + (difficulty / 3);
        params->min_platform_height = 7;
        params->max_platform_height = 15;
        params->height_variance = 2.0f;
        break;
    
    case GAMEPLAY_PLATFORM:
        params->platform_count = 3 + (difficulty / 2);
        params->min_platform_height = 5;
        params->max_platform_height = 17;
        params->height_variance = 3.0f;
        break;
    
    case GAMEPLAY_CHOKEPOINT:
        params->platform_count = 2;
        params->min_platform_height = 10;
        params->max_platform_height = 14;
        params->height_variance = 1.5f;
        params->allow_ledges = false;
        break;
    
    default:
        params->platform_count = 1;
        params->min_platform_height = 12;
        params->max_platform_height = 13;
        params->height_variance = 0.5f;
        break;
    }
}

/* =========================
   Room generation by type
   ========================= */

static void room_gen_duel(Room* room, const RoomParams* params, u32 seed)
{
    room_fill(room, TILE_EMPTY);
    room_apply_terrain_profile(room, &seed, 2);
    
    /* Single platform, slight offset */
    i32 plat_y = lcg_range(&seed, params->min_platform_height, params->max_platform_height);
    i32 plat_x0 = params->width_tiles / 4;
    i32 plat_x1 = (3 * params->width_tiles) / 4;
    room_build_platform(room, plat_y, plat_x0, plat_x1);
}

static void room_gen_vertical(Room* room, const RoomParams* params, u32 seed)
{
    i32 i;
    i32 platform_count = params->platform_count;
    
    room_fill(room, TILE_EMPTY);
    room_apply_terrain_profile(room, &seed, 4);
    
    /* Vertical staircase: platforms at increasing heights */
    for (i = 0; i < platform_count; ++i) {
        i32 plat_y = lcg_range(&seed,
            params->min_platform_height,
            params->max_platform_height);
        
        /* Alternate left/right */
        i32 plat_x0, plat_x1;
        if (i % 2 == 0) {
            plat_x0 = params->width_tiles / 6;
            plat_x1 = params->width_tiles / 3;
        } else {
            plat_x0 = (2 * params->width_tiles) / 3;
            plat_x1 = (5 * params->width_tiles) / 6;
        }
        
        room_build_platform(room, plat_y, plat_x0, plat_x1);
    }
}

static void room_gen_platform(Room* room, const RoomParams* params, u32 seed)
{
    i32 i;
    i32 platform_count = params->platform_count;
    f32 x_step = (f32)params->width_tiles / (platform_count + 1);
    
    room_fill(room, TILE_EMPTY);
    room_apply_terrain_profile(room, &seed, 7);
    
    /* Scattered platforms: complex layout */
    for (i = 0; i < platform_count; ++i) {
        i32 plat_y = lcg_range(&seed,
            params->min_platform_height,
            params->max_platform_height);
        
        i32 plat_x0 = (i32)((i + 1) * x_step) - 4 + lcg_range(&seed, -2, 2);
        i32 plat_x1 = plat_x0 + 8 + lcg_range(&seed, 0, 4);
        
        if (plat_x1 >= params->width_tiles) {
            plat_x1 = params->width_tiles - 1;
        }
        
        room_build_platform(room, plat_y, plat_x0, plat_x1);
    }
}

static void room_gen_chokepoint(Room* room, const RoomParams* params, u32 seed)
{
    i32 narrow_x0, narrow_x1;
    i32 variant;
    
    room_fill(room, TILE_EMPTY);
    room_apply_terrain_profile(room, &seed, 8);
    
    /* Narrow passage in the middle */
    narrow_x0 = (params->width_tiles / 2) - 3;
    narrow_x1 = (params->width_tiles / 2) + 3;
    
    /* Variant: side platforms create pressure */
    variant = lcg_next(&seed) % 2;
    if (variant == 0) {
        room_build_platform(room, 10, 2, narrow_x0 - 2);
        room_build_platform(room, 10, narrow_x1 + 2, params->width_tiles - 3);
    } else {
        room_build_platform(room, 12, 1, narrow_x0 - 3);
        room_build_platform(room, 12, narrow_x1 + 3, params->width_tiles - 2);
    }
}

/* =========================
   Main generation API
   ========================= */

bool room_generate_by_type(Room* room, GameplayType type, const RoomParams* params, u32 seed)
{
    if (!room || !params) {
        return false;
    }
    
    switch (type) {
    case GAMEPLAY_DUEL:
        room_gen_duel(room, params, seed);
        break;
    
    case GAMEPLAY_VERTICAL:
        room_gen_vertical(room, params, seed);
        break;
    
    case GAMEPLAY_PLATFORM:
        room_gen_platform(room, params, seed);
        break;
    
    case GAMEPLAY_CHOKEPOINT:
        room_gen_chokepoint(room, params, seed);
        break;
    
    default:
        return false;
    }
    
    return true;
}

/* =========================
   Full arena generation
   ========================= */

static void gen_room_setup_base(Room* room, RoomType type, RoomDirection direction, i32 width_tiles)
{
    i32 camera_tiles;

    room->type = type;
    room->direction = direction;
    room->width_tiles = width_tiles;
    room->height_tiles = ROOM_HEIGHT_TILES;
    
    room_fill(room, TILE_EMPTY);
    room_build_floor(room, ROOM_HEIGHT_TILES - 2);
    
    room->transition.left_trigger_x = 12.0f;
    room->transition.right_trigger_x = (f32)(width_tiles * TILE_SIZE - 12);
    
    camera_tiles = WINDOW_WIDTH / TILE_SIZE;

    room->spawns.attacker_spawn.x = 4.0f * TILE_SIZE;
    room->spawns.attacker_spawn.y = (f32)((ROOM_HEIGHT_TILES - 3) * TILE_SIZE) - PLAYER_HEIGHT;
    
    room->spawns.defender_spawn.x = (f32)((clamp_i32(camera_tiles - 8, 12, width_tiles - 5)) * TILE_SIZE);
    room->spawns.defender_spawn.y = (f32)((ROOM_HEIGHT_TILES - 3) * TILE_SIZE) - PLAYER_HEIGHT;
}

void arena_generate_next(Arena* arena, u32 seed, i32 difficulty)
{
    i32 i;
    u32 s = seed;
    
    if (!arena || difficulty < 0 || difficulty > 10) {
        return;
    }
    
    arena->room_count = ROOM_COUNT;
    arena->current_room = ROOM_COUNT / 2;
    
    for (i = 0; i < ROOM_COUNT; ++i) {
        Room* r = &arena->rooms[i];
        RoomType room_type = (RoomType)i;
        GameplayType gameplay_type;
        RoomParams params;
        i32 base_width = ROOM_DEFAULT_WIDTH_TILES + 10 + difficulty * 2 + (i % 3) * 8;
        if (room_type == ROOM_TYPE_CENTER) {
            base_width += 10;
        }
        base_width = clamp_i32(base_width, ROOM_DEFAULT_WIDTH_TILES + 8, ROOM_MAX_WIDTH_TILES);
        
        /* Select gameplay type based on room position + difficulty */
        gameplay_type = room_select_gameplay_type(room_type, difficulty);
        
        /* Calculate parameters for this gameplay type */
        room_calc_params(&params, gameplay_type, difficulty, base_width);
        
        /* Setup base structure */
        gen_room_setup_base(
            r,
            room_type,
            (i <= 2) ? ROOM_DIR_LEFT_TO_RIGHT : ROOM_DIR_RIGHT_TO_LEFT,
            params.width_tiles
        );
        
        /* Generate content */
        room_generate_by_type(r, gameplay_type, &params, lcg_next(&s));
        room_add_end_landmark(r);
        room_set_camera_safe_spawns(r);
    }
}
