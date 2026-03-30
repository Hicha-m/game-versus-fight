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

    /* Menu */
    MenuContext menu;

    /* Match */
    Arena arena;
    ArenaOptions arena_options;    /* Options for arena generation */
    CombatState combat;
    AIController ai_controllers[MAX_PLAYERS];
    MatchStats match_stats;

    i32 room_push_direction; /* +1 => right, -1 => left, 0 => locked (need kill) */
    i32 victory_winner_index; /* Index of winner when phase is GAME_PHASE_VICTORY */
} Game;

/* Lifecycle */
bool game_init(Game* game);
void game_shutdown(Game* game);

/* Tick gameplay global */
void game_update(Game* game, const FrameInput* input, f32 dt);

/* Arena options management */
void game_set_arena_options(Game* game, const ArenaOptions* options);
bool game_regenerate_arena(Game* game);

/* Check if quit was requested from menu */
bool game_is_quit_requested(void);

#endif
