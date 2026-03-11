#include "game.h"
#include "arena.h"
#include "combat.h"
#include "ai.h"
#include "constants.h"


static GameError start_match(GameState *state) {
GameConfig cfg = {
.arena_width            = 16,
.arena_height           = 12,
.arena_seed             = 42,
.max_round_time_seconds = 99,
.ai_difficulty          = state->ai_difficulty,
};

GameError err = arena_generate(&state->arena, 16, 12, 42);
if (err != GAME_OK) return err;

err = combat_init(&state->combat, &state->arena, &cfg);
if (err != GAME_OK) return err;

state->game_phase  = GAME_PHASE_MATCH;
state->match_phase = MATCH_PHASE_FIGHT;
return GAME_OK;
}

GameError game_create(const GameConfig *config, GameState *out_state) {
if (out_state == NULL) return GAME_ERROR_INVALID_ARGUMENT;

*out_state = (GameState){0};
out_state->running     = true;
out_state->frame_index = 0;
out_state->game_phase  = GAME_PHASE_BOOT;

if (config != NULL)
out_state->ai_difficulty = config->ai_difficulty;

return GAME_OK;
}

GameError game_update(GameState *state, const FrameInput *input) {
if (state == NULL || input == NULL)
return GAME_ERROR_INVALID_ARGUMENT;

state->frame_index++;

switch (state->game_phase) {

case GAME_PHASE_BOOT:
state->game_phase = GAME_PHASE_PRESS_START;
break;

case GAME_PHASE_PRESS_START:
if (input->start_pressed)
start_match(state);
break;

case GAME_PHASE_MAIN_MENU:
if (input->start_pressed)
start_match(state);
break;

case GAME_PHASE_MATCH: {
/* Player 1 input -> action */
const PlayerCommand *cmd = &input->commands[0];
Action p1 = {
.type         = ACTION_NONE,
.sword_height = cmd->target_height,
.issued_frame = (uint32_t)state->frame_index,
};

if (cmd->attack)             p1.type = ACTION_ATTACK;
else if (cmd->parry)         p1.type = ACTION_PARRY;
else if (cmd->jump)          p1.type = ACTION_JUMP;
else if (cmd->move_axis < 0) p1.type = ACTION_MOVE_LEFT;
else if (cmd->move_axis > 0) p1.type = ACTION_MOVE_RIGHT;
else if (cmd->target_height != state->combat.fighters[0].sword_height)
p1.type = ACTION_HEIGHT_CHANGE;

combat_apply_action(&state->combat, PLAYER_ONE, &p1);

/* AI -> Player 2 action */
Action p2 = ai_get_action(state, state->ai_difficulty);
combat_apply_action(&state->combat, PLAYER_TWO, &p2);

/* Physics step */
combat_step(&state->combat, &state->arena, FIXED_TIMESTEP_MS);

/* Check round end */
PlayerId winner = PLAYER_ONE;
if (combat_is_round_over(&state->combat, &winner)) {
state->combat.score[winner]++;
if (state->combat.score[winner] >= 3) {
state->game_phase = GAME_PHASE_GAME_OVER;
} else {
GameConfig cfg = {.ai_difficulty = state->ai_difficulty};
combat_reset_round(&state->combat, &state->arena, &cfg);
}
}

if (input->pause_pressed)
state->match_phase = MATCH_PHASE_PAUSED;
break;
}

case GAME_PHASE_GAME_OVER:
if (input->start_pressed)
start_match(state);
break;

default:
break;
}

if (input->quit_requested)
state->running = false;

return GAME_OK;
}

GameError game_destroy(GameState *state) {
if (state == NULL) return GAME_ERROR_INVALID_ARGUMENT;

arena_destroy(&state->arena);
state->running = false;
return GAME_OK;
}
