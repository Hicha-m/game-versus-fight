#ifndef GAME_H
#define GAME_H

#include "core/types.h"
#include "engine/input.h"
#include "game/arena/arena.h"
#include "game/combat/combat.h"
#include "game/ai/ai.h"
#include "ui/menu.h"
#include "game/config.h"

typedef struct MatchStats {
    i32 rounds_played;
    i32 wins[MAX_PLAYERS];
    i32 kills[MAX_PLAYERS];
    i32 disarms[MAX_PLAYERS];
    i32 room_progress;
} MatchStats;

typedef struct Game {
    GamePhase phase;

    MenuContext menu;

    Arena arena;
    ArenaOptions arena_options;
    CombatState combat;
    AIController ai_controllers[MAX_PLAYERS];
    MatchStats match_stats;

    i32 room_push_direction;
    i32 victory_winner_index;
} Game;

bool game_init(Game* game);
void game_shutdown(Game* game);

void game_update(Game* game, const FrameInput* input, f32 dt);

void game_set_arena_options(Game* game, const ArenaOptions* options);
bool game_regenerate_arena(Game* game);

bool game_is_quit_requested(void);

#endif
