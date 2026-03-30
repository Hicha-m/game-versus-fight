#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "game/game.h"
#include "game_internal.h"
#include "utils/log.h"

/* Global flag to request quit from menu */
static bool game_quit_requested = false;
static bool game_rng_seeded = false;

static void game_seed_rng_once(void)
{
    if (game_rng_seeded) {
        return;
    }

    srand((unsigned int)time(NULL));
    game_rng_seeded = true;
}

static u32 game_random_seed(void)
{
    i32 r;

    game_seed_rng_once();
    r = rand();
    if (r < 0) {
        r = -r;
    }
    return (u32)r;
}

void game_start_match(Game* game)
{
    arena_set_middle_room(&game->arena);
    game->room_push_direction = 0;
    game->phase = GAME_PHASE_MATCH;
    combat_reset_round(&game->combat, &game->arena);
}

/* Allow menu to change arena options before match starts */
void game_set_arena_options(Game* game, const ArenaOptions* options)
{
    if (game && options) {
        game->arena_options = *options;
    }
}

/* Regenerate arena with current options (useful for menu preview) */
bool game_regenerate_arena(Game* game)
{
    if (!game) {
        return false;
    }

    arena_shutdown(&game->arena);
    return arena_init_with_options(&game->arena, &game->arena_options);
}

static i32 fighter_target_room_direction(i32 fighter_index)
{
    return (fighter_index == 0) ? 1 : -1;
}

static void game_update_room_push_from_kill(Game* game)
{
    if (!game || !game->combat.kill_happened) {
        return;
    }

    if (game->combat.kill_counts_for_progress &&
        game->combat.kill_attacker_index >= 0 &&
        game->combat.kill_attacker_index < MAX_PLAYERS) {
        game->room_push_direction = fighter_target_room_direction(game->combat.kill_attacker_index);
    } else {
        game->room_push_direction = 0;
    }

    game->combat.kill_happened = false;
}

/* Check if a player has reached the enemy base */
static i32 game_check_victory(Game* game)
{
    if (!game) {
        return -1;
    }

    /* Player 0 wins if they reach room 4 (END_B) */
    if (game->arena.current_room == ROOM_COUNT - 1) {
        return 0;
    }

    /* Player 1 wins if they reach room 0 (START_A) */
    if (game->arena.current_room == 0) {
        return 1;
    }

    return -1;
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

    out_cmd->up_pressed = input->actions[ACTION_UP].pressed;
    out_cmd->down_pressed = input->actions[ACTION_DOWN].pressed;
    out_cmd->right_pressed = input->actions[ACTION_RIGHT].down;
    out_cmd->left_pressed = input->actions[ACTION_LEFT].down;
    out_cmd->jump_pressed = input->actions[ACTION_JUMP].down;
    out_cmd->thrust_pressed = input->actions[ACTION_THRUST].pressed;
    out_cmd->throw_pressed = input->actions[ACTION_THROW].pressed;
    out_cmd->roll_pressed = false;
    out_cmd->target_sword_line = SWORD_LINE_MID;
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
        *out_p1 = ai_think(&game->ai_controllers[0], &game->arena, &game->combat, 0, dt, game->match_stats.kills);
    }

    if (f2->controller.type == CONTROLLER_HUMAN) {
        game_build_player_command_from_input(out_p2, &input->players[1]);
    } else {
        *out_p2 = ai_think(&game->ai_controllers[1], &game->arena, &game->combat, 1, dt, game->match_stats.kills);
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

    if (game->room_push_direction > 0) {
        /* Only P1 (attacker) can progress to the right */
        if (arena_can_transition_right(&game->arena, &p1->state.pos)) {
            moved = arena_transition_right(&game->arena);
        }
    } else if (game->room_push_direction < 0) {
        /* Only P2 (defender) can progress to the left */
        if (arena_can_transition_left(&game->arena, &p2->state.pos)) {
            moved = arena_transition_left(&game->arena);
        }
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
    game_seed_rng_once();

    game->phase = GAME_PHASE_MENU;

    /* Initialize menu */
    if (!menu_init(&game->menu)) {
        return false;
    }

    /* Initialize arena options (default mode) */
    game->arena_options = arena_options_default();

    /* Initialize arena with options */
    if (!arena_init_with_options(&game->arena, &game->arena_options)) {
        menu_shutdown(&game->menu);
        return false;
    }

    if (!combat_init(&game->combat)) {
        arena_shutdown(&game->arena);
        menu_shutdown(&game->menu);
        return false;
    }

    if (!ai_init(&game->ai_controllers[0]) || !ai_init(&game->ai_controllers[1])) {
        combat_shutdown(&game->combat);
        arena_shutdown(&game->arena);
        menu_shutdown(&game->menu);
        return false;
    }

    game->combat.fighters[0].controller.type = CONTROLLER_HUMAN;
    ai_set_difficulty(&game->ai_controllers[0], AI_DIFFICULTY_EASY);
    ai_set_algorithm(&game->ai_controllers[0], AI_ALGO_SCRIPTED);

    game->combat.fighters[1].controller.type = CONTROLLER_AI;
    ai_set_difficulty(&game->ai_controllers[1], AI_DIFFICULTY_EXPERT);
    ai_set_algorithm(&game->ai_controllers[1], AI_ALGO_MINIMAX_ALPHA_BETA);

    /* Don't start match yet - wait for menu selection */
    return true;
}

void game_shutdown(Game* game)
{
    if (!game) {
        return;
    }

    menu_shutdown(&game->menu);
    ai_shutdown(&game->ai_controllers[0]);
    ai_shutdown(&game->ai_controllers[1]);
    combat_shutdown(&game->combat);
    arena_shutdown(&game->arena);
}

void game_update(Game* game, const FrameInput* input, f32 dt)
{
    PlayerCommand p1_cmd;
    PlayerCommand p2_cmd;
    MenuAction menu_action;

    if (!game || !input) {
        return;
    }

    /* Handle menu state */
    if (game->phase == GAME_PHASE_MENU) {
        menu_update(&game->menu, input, dt);
        menu_action = menu_get_action(&game->menu);

        switch (menu_action) {
            case MENU_ACTION_PLAY_SOLO:
                /* Setup solo match: P1 human, P2 AI */
                game->combat.fighters[0].controller.type = CONTROLLER_HUMAN;
                game->combat.fighters[1].controller.type = CONTROLLER_AI;
                ai_set_difficulty(&game->ai_controllers[1], game->menu.selected_difficulty);
                game_start_match(game);
                game->phase = GAME_PHASE_MATCH;
                break;

            case MENU_ACTION_PLAY_MULTIPLAYER:
                /* Setup multiplayer: both human */
                game->combat.fighters[0].controller.type = CONTROLLER_HUMAN;
                game->combat.fighters[1].controller.type = CONTROLLER_HUMAN;
                game_start_match(game);
                game->phase = GAME_PHASE_MATCH;
                break;

            case MENU_ACTION_START_MATCH: {
                /* NEW: Arena-configured match start */
                /* Apply arena options before starting */
                const ArenaOptions* opts = menu_get_arena_options(&game->menu);
                if (opts) {
                    ArenaOptions run_opts = *opts;

                    /* Procedural/corridor maps always use a fresh random seed. */
                    if (run_opts.mode == ARENA_MODE_PROCEDURAL || run_opts.mode == ARENA_MODE_CORRIDOR) {
                        run_opts.seed = game_random_seed();
                    }

                    game_set_arena_options(game, &run_opts);
                    game_regenerate_arena(game);
                }

                /* Setup based on solo vs multiplayer selection */
                if (game->menu.is_solo) {
                    game->combat.fighters[0].controller.type = CONTROLLER_HUMAN;
                    game->combat.fighters[1].controller.type = CONTROLLER_AI;
                    ai_set_difficulty(&game->ai_controllers[1], game->menu.selected_difficulty);
                } else {
                    game->combat.fighters[0].controller.type = CONTROLLER_HUMAN;
                    game->combat.fighters[1].controller.type = CONTROLLER_HUMAN;
                }

                game_start_match(game);
                game->phase = GAME_PHASE_MATCH;
                break;
            }

            case MENU_ACTION_QUIT:
                /* Set quit flag to be checked by engine */
                game_quit_requested = true;
                break;

            default:
                break;
        }

        return;
    }

    /* Handle pause menu */
    if (game->phase == GAME_PHASE_PAUSED) {
        menu_update(&game->menu, input, dt);
        menu_action = menu_get_action(&game->menu);

        switch (menu_action) {
            case MENU_ACTION_RESUME:
                game->phase = GAME_PHASE_MATCH;
                break;
            case MENU_ACTION_BACK_TO_MENU:
                game->phase = GAME_PHASE_MENU;
                menu_set_state(&game->menu, MENU_STATE_MAIN);
                break;
            default:
                break;
        }

        return;
    }

    /* Handle victory screen */
    if (game->phase == GAME_PHASE_VICTORY) {
        /* Wait for any input to return to menu */
        if (input->players[0].actions[ACTION_THRUST].pressed ||
            input->players[1].actions[ACTION_THRUST].pressed ||
            input->pause.pressed) {
            game->phase = GAME_PHASE_MENU;
            menu_set_state(&game->menu, MENU_STATE_MAIN);
        }
        return;
    }

    /* Handle entering pause menu */
    if (input->pause.pressed) {
        if (game->phase == GAME_PHASE_MATCH) {
            game->phase = GAME_PHASE_PAUSED;
            menu_set_state(&game->menu, MENU_STATE_PAUSE);
        }
    }

    if (game->phase != GAME_PHASE_MATCH) {
        return;
    }

    game_collect_commands(game, input, &p1_cmd, &p2_cmd, dt);

    combat_step(&game->combat, &game->arena, &p1_cmd, &p2_cmd, dt);
    game_update_room_push_from_kill(game);

    if (game->combat.round_over) {
        game_handle_round_end(game);
        return;
    }

    game_handle_room_transitions(game);

    /* Check if victory has been achieved */
    {
        i32 winner = game_check_victory(game);
        if (winner >= 0 && winner < MAX_PLAYERS) {
            /* Set winner before finalizing stats */
            game->combat.winner_index = winner;
            
            /* Record the final round stats (win + kill) */
            game->match_stats.rounds_played++;
            game->match_stats.wins[winner]++;
            game->match_stats.kills[winner]++;
            
            game->victory_winner_index = winner;
            game->phase = GAME_PHASE_VICTORY;
        }
    }
}

bool game_is_quit_requested(void)
{
    return game_quit_requested;
}
