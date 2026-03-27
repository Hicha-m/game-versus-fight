#include <string.h>

#include "game/game.h"
#include "game_internal.h"

void game_start_match(Game* game)
{
    game->phase = GAME_PHASE_MATCH;
    combat_reset_round(&game->combat, &game->arena);
}

static SwordLine input_to_sword_line(i32 move_y)
{
    if (move_y > 0) {
        return SWORD_LINE_HIGH;
    }
    if (move_y < 0) {
        return SWORD_LINE_LOW;
    }
    return SWORD_LINE_MID;
}

void game_build_player_command_from_input(
    PlayerCommand* out_cmd,
    const PlayerInput* input
)
{
    if (!out_cmd || !input) {
        return;
    }

    memset(out_cmd, 0, sizeof(*out_cmd));

    out_cmd->move_x = input->move_x;
    out_cmd->jump_pressed = input->jump.down;
    out_cmd->thrust_pressed = input->thrust.pressed;
    out_cmd->throw_pressed = input->throw_weapon.pressed;
    out_cmd->roll_pressed = false;
    out_cmd->target_sword_line = input_to_sword_line(input->move_y);
}

void game_collect_commands(
    Game* game,
    const FrameInput* input,
    PlayerCommand* out_p1,
    PlayerCommand* out_p2,
    f32 dt
)
{
    Fighter* f1;
    Fighter* f2;

    if (!game || !input || !out_p1 || !out_p2) {
        return;
    }

    f1 = &game->combat.fighters[0];
    f2 = &game->combat.fighters[1];

    if (f1->controller.type == CONTROLLER_HUMAN) {
        game_build_player_command_from_input(out_p1, &input->players[0]);
    } else {
        *out_p1 = ai_think(&game->ai_controllers[0], &game->arena, &game->combat, 0, dt);
    }

    if (f2->controller.type == CONTROLLER_HUMAN) {
        game_build_player_command_from_input(out_p2, &input->players[1]);
    } else {
        *out_p2 = ai_think(&game->ai_controllers[1], &game->arena, &game->combat, 1, dt);
    }
}

void game_handle_room_transitions(Game* game)
{
    Fighter* p1;
    Fighter* p2;
    bool moved = false;

    if (!game) {
        return;
    }

    p1 = &game->combat.fighters[0];
    p2 = &game->combat.fighters[1];

    if (arena_can_transition_right(&game->arena, &p1->state.pos) ||
        arena_can_transition_right(&game->arena, &p2->state.pos)) {
        moved = arena_transition_right(&game->arena);
    } else if (arena_can_transition_left(&game->arena, &p1->state.pos) ||
               arena_can_transition_left(&game->arena, &p2->state.pos)) {
        moved = arena_transition_left(&game->arena);
    }

    if (moved) {
        game->match_stats.room_progress = game->arena.current_room;
        combat_reset_round(&game->combat, &game->arena);
    }
}

void game_handle_round_end(Game* game)
{
    i32 winner;
    i32 loser;

    if (!game) {
        return;
    }

    winner = game->combat.winner_index;
    loser = (winner == 0) ? 1 : 0;

    game->match_stats.rounds_played++;

    if (winner >= 0 && winner < MAX_PLAYERS) {
        game->match_stats.wins[winner]++;
        game->match_stats.kills[winner]++;
    }

    if (loser >= 0 && loser < MAX_PLAYERS) {
        if (!game->combat.fighters[loser].state.has_sword) {
            game->match_stats.disarms[loser]++;
        }
    }

    combat_reset_round(&game->combat, &game->arena);
}

bool game_init(Game* game)
{
    if (!game) {
        return false;
    }

    memset(game, 0, sizeof(*game));

    game->phase = GAME_PHASE_BOOT;

    if (!arena_init(&game->arena)) {
        return false;
    }

    arena_build_default(&game->arena);

    if (!combat_init(&game->combat)) {
        arena_shutdown(&game->arena);
        return false;
    }

    if (!ai_init(&game->ai_controllers[0]) || !ai_init(&game->ai_controllers[1])) {
        combat_shutdown(&game->combat);
        arena_shutdown(&game->arena);
        return false;
    }

    game->combat.fighters[0].controller.type = CONTROLLER_HUMAN;
    ai_set_difficulty(&game->ai_controllers[0], AI_DIFFICULTY_EASY);
    ai_set_algorithm(&game->ai_controllers[0], AI_ALGO_SCRIPTED);

    game->combat.fighters[1].controller.type = CONTROLLER_AI;
    ai_set_difficulty(&game->ai_controllers[1], AI_DIFFICULTY_EXPERT);
    ai_set_algorithm(&game->ai_controllers[1], AI_ALGO_MINIMAX_ALPHA_BETA);

    game_start_match(game);
    return true;
}

void game_shutdown(Game* game)
{
    if (!game) {
        return;
    }

    ai_shutdown(&game->ai_controllers[0]);
    ai_shutdown(&game->ai_controllers[1]);
    combat_shutdown(&game->combat);
    arena_shutdown(&game->arena);
}

void game_update(Game* game, const FrameInput* input, f32 dt)
{
    PlayerCommand p1_cmd;
    PlayerCommand p2_cmd;

    if (!game || !input) {
        return;
    }

    if (input->pause.pressed) {
        if (game->phase == GAME_PHASE_MATCH) {
            game->phase = GAME_PHASE_PAUSED;
        } else if (game->phase == GAME_PHASE_PAUSED) {
            game->phase = GAME_PHASE_MATCH;
        }
    }

    if (game->phase != GAME_PHASE_MATCH) {
        return;
    }

    game_collect_commands(game, input, &p1_cmd, &p2_cmd, dt);

    combat_step(&game->combat, &game->arena, &p1_cmd, &p2_cmd, dt);

    if (game->combat.round_over) {
        game_handle_round_end(game);
        return;
    }

    game_handle_room_transitions(game);
}
