#ifndef GAME_H
#define GAME_H

#include "core/types.h"
#include "engine/input.h"
#include "game/arena/arena.h"
#include "game/combat/combat.h"
#include "game/ai/ai.h"

typedef struct MatchStats {
    i32 rounds_played;
    i32 wins[MAX_PLAYERS];
    i32 kills[MAX_PLAYERS];
    i32 disarms[MAX_PLAYERS];
    i32 room_progress;
} MatchStats;

typedef struct Game {
    GamePhase phase;

    Arena arena;
    CombatState combat;
    AIController ai_controllers[MAX_PLAYERS];
    MatchStats match_stats;
} Game;

/* Lifecycle */
bool game_init(Game* game);
void game_shutdown(Game* game);

/* Tick gameplay global */
void game_update(Game* game, const FrameInput* input, f32 dt);

#endif
