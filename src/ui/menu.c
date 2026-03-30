#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "ui/menu.h"
#include "utils/log.h"

/* État global de l'action */
static MenuAction current_action = MENU_ACTION_NONE;
static bool menu_rng_seeded = false;

static void menu_seed_rng_once(void)
{
    if (menu_rng_seeded) {
        return;
    }

    srand((unsigned int)time(NULL));
    menu_rng_seeded = true;
}

static u32 menu_random_seed(void)
{
    i32 r;

    menu_seed_rng_once();
    r = rand();
    if (r < 0) {
        r = -r;
    }
    return (u32)r;
}

/* ========================================
   Lifecycle
   ======================================== */

bool menu_init(MenuContext* menu)
{
    if (!menu) {
        return false;
    }

    memset(menu, 0, sizeof(*menu));
    menu_seed_rng_once();

    menu->current_state = MENU_STATE_MAIN;
    menu->selected_option = 0;
    menu->selection_timer = 0.0f;
    menu->is_solo = true;
    menu->selected_difficulty = AI_DIFFICULTY_MEDIUM;

    /* Charger la configuration */
    if (!config_load(&menu->config)) {
        log_warn("Failed to load config, using defaults");
        config_set_defaults(&menu->config);
    }

    /* NEW: Initialize arena options */
    menu->arena_options = arena_options_default();
    menu->arena_options.seed = menu_random_seed();
    menu->seed_input_buffer = 0;

    current_action = MENU_ACTION_NONE;
    return true;
}

void menu_shutdown(MenuContext* menu)
{
    if (!menu) {
        return;
    }

    /* Sauvegarder la configuration */
    if (!config_save(&menu->config)) {
        log_warn("Failed to save config");
    }
}

MenuAction menu_get_action(MenuContext* menu)
{
    MenuAction action = current_action;
    current_action = MENU_ACTION_NONE;
    (void)menu;
    return action;
}

void menu_set_state(MenuContext* menu, MenuState state)
{
    if (!menu) {
        return;
    }
    menu->current_state = state;
    menu->selected_option = 0;
    menu->selection_timer = 0.0f;

    if (state == MENU_STATE_ARENA_SEED) {
        menu->arena_options.seed = menu_random_seed();
    }
}

MenuState menu_get_state(const MenuContext* menu)
{
    if (!menu) {
        return MENU_STATE_MAIN;
    }
    return menu->current_state;
}

i32 menu_get_selected_option(const MenuContext* menu)
{
    if (!menu) {
        return 0;
    }
    return menu->selected_option;
}

const GameConfig* menu_get_config(const MenuContext* menu)
{
    if (!menu) {
        return NULL;
    }
    return &menu->config;
}

bool menu_is_solo(const MenuContext* menu)
{
    if (!menu) {
        return true;
    }
    return menu->is_solo;
}

AIDifficulty menu_get_selected_difficulty(const MenuContext* menu)
{
    if (!menu) {
        return AI_DIFFICULTY_MEDIUM;
    }
    return menu->selected_difficulty;
}

/* NEW: Get arena options accessor */
const ArenaOptions* menu_get_arena_options(const MenuContext* menu)
{
    if (!menu) {
        return NULL;
    }
    return &menu->arena_options;
}

/* ========================================
   Update - Pure Logic (No SDL)
   ======================================== */

static void menu_update_main(MenuContext* menu, const FrameInput* input)
{
    i32 option_count = 3;  /* Play, Options, Quit */

    if (input->players[0].actions[ACTION_DOWN].pressed) {
        menu->selected_option = (menu->selected_option + 1) % option_count;
    }
    if (input->players[0].actions[ACTION_UP].pressed) {
        menu->selected_option = (menu->selected_option - 1 + option_count) % option_count;
    }

    if (input->players[0].actions[ACTION_JUMP].pressed || input->players[0].actions[ACTION_THRUST].pressed) {
        switch (menu->selected_option) {
            case 0:
                menu_set_state(menu, MENU_STATE_PLAY_MODE);
                break;
            case 1:
                menu_set_state(menu, MENU_STATE_OPTIONS);
                break;
            case 2:
                current_action = MENU_ACTION_QUIT;
                break;
            default:
                break;
        }
    }
}

static void menu_update_play_mode(MenuContext* menu, const FrameInput* input)
{
    i32 option_count = 2;  /* Solo, Multiplayer */

    if (input->players[0].actions[ACTION_DOWN].pressed) {
        menu->selected_option = (menu->selected_option + 1) % option_count;
    }
    if (input->players[0].actions[ACTION_UP].pressed) {
        menu->selected_option = (menu->selected_option - 1 + option_count) % option_count;
    }

    if (input->players[0].actions[ACTION_JUMP].pressed || input->players[0].actions[ACTION_THRUST].pressed) {
        switch (menu->selected_option) {
            case 0:
                menu->is_solo = true;
                menu_set_state(menu, MENU_STATE_DIFFICULTY);
                break;
            case 1:
                menu->is_solo = false;
                menu_set_state(menu, MENU_STATE_ARENA_MODE);
                break;
            default:
                break;
        }
    }

    if (input->players[0].actions[ACTION_LEFT].pressed || input->players[0].actions[ACTION_THROW].pressed) {
        menu_set_state(menu, MENU_STATE_MAIN);
    }
}

static void menu_update_difficulty(MenuContext* menu, const FrameInput* input)
{
    i32 option_count = 4;  /* Easy, Medium, Hard, Expert */

    if (input->players[0].actions[ACTION_DOWN].pressed) {
        menu->selected_option = (menu->selected_option + 1) % option_count;
    }
    if (input->players[0].actions[ACTION_UP].pressed) {
        menu->selected_option = (menu->selected_option - 1 + option_count) % option_count;
    }

    if (input->players[0].actions[ACTION_JUMP].pressed || input->players[0].actions[ACTION_THRUST].pressed) {
        menu->selected_difficulty = (AIDifficulty)menu->selected_option;
        menu->config.ai_difficulty = menu->selected_difficulty;
        menu_set_state(menu, MENU_STATE_ARENA_MODE);
    }

    if (input->players[0].actions[ACTION_LEFT].pressed || input->players[0].actions[ACTION_THROW].pressed) {
        menu_set_state(menu, MENU_STATE_PLAY_MODE);
    }
}

static void menu_update_options(MenuContext* menu, const FrameInput* input)
{
    i32 option_count = 2;  /* Audio, Keybinds */

    if (input->players[0].actions[ACTION_DOWN].pressed) {
        menu->selected_option = (menu->selected_option + 1) % option_count;
    }
    if (input->players[0].actions[ACTION_UP].pressed) {
        menu->selected_option = (menu->selected_option - 1 + option_count) % option_count;
    }

    if (input->players[0].actions[ACTION_JUMP].pressed || input->players[0].actions[ACTION_THRUST].pressed) {
        switch (menu->selected_option) {
            case 0:
                menu_set_state(menu, MENU_STATE_OPTIONS_AUDIO);
                break;
            case 1:
                menu_set_state(menu, MENU_STATE_OPTIONS_KEYBINDS);
                break;
            default:
                break;
        }
    }

    if (input->players[0].actions[ACTION_LEFT].pressed || input->players[0].actions[ACTION_THROW].pressed) {
        menu_set_state(menu, MENU_STATE_MAIN);
    }
}

static void menu_update_options_audio(MenuContext* menu, const FrameInput* input)
{
    i32 option_count = 3;  /* Master, Music, SFX */

    if (input->players[0].actions[ACTION_DOWN].pressed) {
        menu->selected_option = (menu->selected_option + 1) % option_count;
    }
    if (input->players[0].actions[ACTION_UP].pressed) {
        menu->selected_option = (menu->selected_option - 1 + option_count) % option_count;
    }

    /* Adjust selected volume */
    if (input->players[0].actions[ACTION_RIGHT].down) {
        switch (menu->selected_option) {
            case 0:
                menu->config.master_volume += 0.01f;
                if (menu->config.master_volume > 1.0f) {
                    menu->config.master_volume = 1.0f;
                }
                break;
            case 1:
                menu->config.music_volume += 0.01f;
                if (menu->config.music_volume > 1.0f) {
                    menu->config.music_volume = 1.0f;
                }
                break;
            case 2:
                menu->config.sfx_volume += 0.01f;
                if (menu->config.sfx_volume > 1.0f) {
                    menu->config.sfx_volume = 1.0f;
                }
                break;
            default:
                break;
        }
    }

    if (input->players[0].actions[ACTION_LEFT].down) {
        switch (menu->selected_option) {
            case 0:
                menu->config.master_volume -= 0.01f;
                if (menu->config.master_volume < 0.0f) {
                    menu->config.master_volume = 0.0f;
                }
                break;
            case 1:
                menu->config.music_volume -= 0.01f;
                if (menu->config.music_volume < 0.0f) {
                    menu->config.music_volume = 0.0f;
                }
                break;
            case 2:
                menu->config.sfx_volume -= 0.01f;
                if (menu->config.sfx_volume < 0.0f) {
                    menu->config.sfx_volume = 0.0f;
                }
                break;
            default:
                break;
        }
    }

    if (input->players[0].actions[ACTION_THROW].pressed) {
        menu_set_state(menu, MENU_STATE_OPTIONS);
    }
}

static void menu_update_options_keybinds(MenuContext* menu, const FrameInput* input)
{
    i32 action_count = ACTION_COUNT;

    if (input->players[0].actions[ACTION_DOWN].pressed) {
        menu->selected_option = (menu->selected_option + 1) % action_count;
    }
    if (input->players[0].actions[ACTION_UP].pressed) {
        menu->selected_option = (menu->selected_option - 1 + action_count) % action_count;
    }

    if (input->players[0].actions[ACTION_THROW].pressed) {
        menu_set_state(menu, MENU_STATE_OPTIONS);
    }
}

static void menu_update_pause(MenuContext* menu, const FrameInput* input)
{
    i32 option_count = 2;  /* Resume, Back to Menu */

    if (input->players[0].actions[ACTION_DOWN].pressed) {
        menu->selected_option = (menu->selected_option + 1) % option_count;
    }
    if (input->players[0].actions[ACTION_UP].pressed) {
        menu->selected_option = (menu->selected_option - 1 + option_count) % option_count;
    }

    if (input->players[0].actions[ACTION_JUMP].pressed || input->players[0].actions[ACTION_THRUST].pressed) {
        switch (menu->selected_option) {
            case 0:
                current_action = MENU_ACTION_RESUME;
                break;
            case 1:
                current_action = MENU_ACTION_BACK_TO_MENU;
                break;
            default:
                break;
        }
    }

    /* ESC aussi pour résumer */
    if (input->pause.pressed) {
        current_action = MENU_ACTION_RESUME;
    }
}

/* NEW: Arena mode selection (Default, Procedural, Corridor) */
static void menu_update_arena_mode(MenuContext* menu, const FrameInput* input)
{
    i32 option_count = 3;  /* Default, Procedural, Corridor */

    if (input->players[0].actions[ACTION_DOWN].pressed) {
        menu->selected_option = (menu->selected_option + 1) % option_count;
    }
    if (input->players[0].actions[ACTION_UP].pressed) {
        menu->selected_option = (menu->selected_option - 1 + option_count) % option_count;
    }

    /* Update mode based on selection */
    menu->arena_options.mode = (ArenaGenerationMode)menu->selected_option;

    if (input->players[0].actions[ACTION_JUMP].pressed || input->players[0].actions[ACTION_THRUST].pressed) {
        if (menu->arena_options.mode == ARENA_MODE_PROCEDURAL) {
            menu_set_state(menu, MENU_STATE_ARENA_DIFFICULTY);
        } else if (menu->arena_options.mode == ARENA_MODE_CORRIDOR) {
            menu_set_state(menu, MENU_STATE_ARENA_SEED);
        } else {
            /* DEFAULT mode - go straight to play */
            current_action = MENU_ACTION_START_MATCH;
        }
    }

    if (input->players[0].actions[ACTION_THROW].pressed) {
        menu_set_state(menu, MENU_STATE_MAIN);
    }
}

/* NEW: Arena difficulty selection (0-10) */
static void menu_update_arena_difficulty(MenuContext* menu, const FrameInput* input)
{
    if (input->players[0].actions[ACTION_LEFT].pressed) {
        menu->arena_options.difficulty--;
        if (menu->arena_options.difficulty < 0) {
            menu->arena_options.difficulty = 0;
        }
    }
    if (input->players[0].actions[ACTION_RIGHT].pressed) {
        menu->arena_options.difficulty++;
        if (menu->arena_options.difficulty > 10) {
            menu->arena_options.difficulty = 10;
        }
    }

    if (input->players[0].actions[ACTION_JUMP].pressed || input->players[0].actions[ACTION_THRUST].pressed) {
        menu_set_state(menu, MENU_STATE_ARENA_SEED);
    }

    if (input->players[0].actions[ACTION_THROW].pressed) {
        menu_set_state(menu, MENU_STATE_ARENA_MODE);
    }
}

/* NEW: Arena seed selection */
static void menu_update_arena_seed(MenuContext* menu, const FrameInput* input)
{
    /* Reroll on directional input if user wants a different random seed. */
    if (input->players[0].actions[ACTION_LEFT].pressed ||
        input->players[0].actions[ACTION_RIGHT].pressed ||
        input->players[0].actions[ACTION_UP].pressed ||
        input->players[0].actions[ACTION_DOWN].pressed) {
        menu->arena_options.seed = menu_random_seed();
    }

    if (input->players[0].actions[ACTION_JUMP].pressed || input->players[0].actions[ACTION_THRUST].pressed) {
        /* Start the match */
        current_action = MENU_ACTION_START_MATCH;
    }

    if (input->players[0].actions[ACTION_THROW].pressed) {
        menu_set_state(menu, MENU_STATE_ARENA_DIFFICULTY);
    }
}

void menu_update(MenuContext* menu, const FrameInput* input, f32 dt)
{
    if (!menu || !input) {
        return;
    }

    menu->selection_timer += dt;

    switch (menu->current_state) {
        case MENU_STATE_MAIN:
            menu_update_main(menu, input);
            break;
        case MENU_STATE_PLAY_MODE:
            menu_update_play_mode(menu, input);
            break;
        case MENU_STATE_DIFFICULTY:
            menu_update_difficulty(menu, input);
            break;
        case MENU_STATE_OPTIONS:
            menu_update_options(menu, input);
            break;
        case MENU_STATE_OPTIONS_AUDIO:
            menu_update_options_audio(menu, input);
            break;
        case MENU_STATE_OPTIONS_KEYBINDS:
            menu_update_options_keybinds(menu, input);
            break;
        case MENU_STATE_PAUSE:
            menu_update_pause(menu, input);
            break;
        /* NEW: Arena configuration states */
        case MENU_STATE_ARENA_MODE:
            menu_update_arena_mode(menu, input);
            break;
        case MENU_STATE_ARENA_DIFFICULTY:
            menu_update_arena_difficulty(menu, input);
            break;
        case MENU_STATE_ARENA_SEED:
            menu_update_arena_seed(menu, input);
            break;
        default:
            break;
    }
}
