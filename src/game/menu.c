#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "ui/menu.h"
#include "utils/log.h"

static u32 menu_random_seed(MenuContext *menu)
{
    menu->rng_state = menu->rng_state * 1664525u + 1013904223u;
    return menu->rng_state;
}

bool menu_init(MenuContext *menu)
{
    if (!menu)
    {
        return false;
    }

    memset(menu, 0, sizeof(*menu));
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    menu->rng_state = (u32)now.tv_nsec ^ (u32)now.tv_sec;

    menu->current_state = MENU_STATE_MAIN;
    menu->selected_option = 0;
    menu->selection_timer = 0.0f;
    menu->is_solo = true;
    menu->selected_difficulty = AI_DIFFICULTY_MEDIUM;

    if (!config_load(&menu->config))
    {
        log_warn("Failed to load config, using defaults");
        config_set_defaults(&menu->config);
    }

    menu->selected_difficulty = menu->config.ai_difficulty;
    menu->arena_options = arena_options_default();
    menu->arena_options.seed = menu_random_seed(menu);
    menu->seed_input_buffer = 0;

    menu->pending_action = MENU_ACTION_NONE;
    return true;
}

void menu_shutdown(MenuContext *menu)
{
    if (!menu)
    {
        return;
    }

    if (!config_save(&menu->config))
    {
        log_warn("Failed to save config");
    }
}

MenuAction menu_get_action(MenuContext *menu)
{
    MenuAction action = menu->pending_action;
    menu->pending_action = MENU_ACTION_NONE;
    (void)menu;
    return action;
}

void menu_set_state(MenuContext *menu, MenuState state)
{
    if (!menu)
    {
        return;
    }
    menu->capturing_key = false;
    if (menu->current_state == MENU_STATE_OPTIONS_KEYBINDS ||
        menu->current_state == MENU_STATE_OPTIONS_AUDIO)
        config_save(&menu->config);
    menu->current_state = state;
    menu->selected_option = 0;
    menu->selection_timer = 0.0f;

    if (state == MENU_STATE_ARENA_SEED)
    {
        menu->arena_options.seed = menu_random_seed(menu);
    }
}

MenuState menu_get_state(const MenuContext *menu)
{
    if (!menu)
    {
        return MENU_STATE_MAIN;
    }
    return menu->current_state;
}

i32 menu_get_selected_option(const MenuContext *menu)
{
    if (!menu)
    {
        return 0;
    }
    return menu->selected_option;
}

const GameConfig *menu_get_config(const MenuContext *menu)
{
    if (!menu)
    {
        return NULL;
    }
    return &menu->config;
}

bool menu_is_solo(const MenuContext *menu)
{
    if (!menu)
    {
        return true;
    }
    return menu->is_solo;
}

AIDifficulty menu_get_selected_difficulty(const MenuContext *menu)
{
    if (!menu)
    {
        return AI_DIFFICULTY_MEDIUM;
    }
    return menu->selected_difficulty;
}

const ArenaOptions *menu_get_arena_options(const MenuContext *menu)
{
    if (!menu)
    {
        return NULL;
    }
    return &menu->arena_options;
}

static void menu_update_main(MenuContext *menu, const FrameInput *input)
{
    i32 option_count = 3;

    if (input->players[0].actions[ACTION_DOWN].pressed)
    {
        menu->selected_option = (menu->selected_option + 1) % option_count;
    }
    if (input->players[0].actions[ACTION_UP].pressed)
    {
        menu->selected_option = (menu->selected_option - 1 + option_count) % option_count;
    }

    if (input->players[0].actions[ACTION_JUMP].pressed ||
        input->players[0].actions[ACTION_THRUST].pressed)
    {
        switch (menu->selected_option)
        {
        case 0:
            menu_set_state(menu, MENU_STATE_PLAY_MODE);
            break;
        case 1:
            menu_set_state(menu, MENU_STATE_OPTIONS);
            break;
        case 2:
            menu->pending_action = MENU_ACTION_QUIT;
            break;
        default:
            break;
        }
    }
}

static void menu_update_play_mode(MenuContext *menu, const FrameInput *input)
{
    i32 option_count = 2;

    if (input->players[0].actions[ACTION_DOWN].pressed)
    {
        menu->selected_option = (menu->selected_option + 1) % option_count;
    }
    if (input->players[0].actions[ACTION_UP].pressed)
    {
        menu->selected_option = (menu->selected_option - 1 + option_count) % option_count;
    }

    if (input->players[0].actions[ACTION_JUMP].pressed ||
        input->players[0].actions[ACTION_THRUST].pressed)
    {
        switch (menu->selected_option)
        {
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

    if (input->players[0].actions[ACTION_LEFT].pressed ||
        input->players[0].actions[ACTION_THROW].pressed)
    {
        menu_set_state(menu, MENU_STATE_MAIN);
    }
}

static void menu_update_difficulty(MenuContext *menu, const FrameInput *input)
{
    i32 option_count = 4;

    if (input->players[0].actions[ACTION_DOWN].pressed)
    {
        menu->selected_option = (menu->selected_option + 1) % option_count;
    }
    if (input->players[0].actions[ACTION_UP].pressed)
    {
        menu->selected_option = (menu->selected_option - 1 + option_count) % option_count;
    }

    if (input->players[0].actions[ACTION_JUMP].pressed ||
        input->players[0].actions[ACTION_THRUST].pressed)
    {
        menu->selected_difficulty = (AIDifficulty)menu->selected_option;
        menu->config.ai_difficulty = menu->selected_difficulty;
        menu_set_state(menu, MENU_STATE_ARENA_MODE);
    }

    if (input->players[0].actions[ACTION_LEFT].pressed ||
        input->players[0].actions[ACTION_THROW].pressed)
    {
        menu_set_state(menu, MENU_STATE_PLAY_MODE);
    }
}

static void menu_update_options(MenuContext *menu, const FrameInput *input)
{
    i32 option_count = 3;

    if (input->players[0].actions[ACTION_DOWN].pressed)
    {
        menu->selected_option = (menu->selected_option + 1) % option_count;
    }
    if (input->players[0].actions[ACTION_UP].pressed)
    {
        menu->selected_option = (menu->selected_option - 1 + option_count) % option_count;
    }

    if (input->players[0].actions[ACTION_JUMP].pressed ||
        input->players[0].actions[ACTION_THRUST].pressed)
    {
        switch (menu->selected_option)
        {
        case 0:
            menu_set_state(menu, MENU_STATE_OPTIONS_AUDIO);
            break;
        case 1:
            menu_set_state(menu, MENU_STATE_OPTIONS_KEYBINDS);
            break;
        case 2:
            menu->config.fullscreen = !menu->config.fullscreen;
            config_save(&menu->config);
            break;
        default:
            break;
        }
    }

    if (input->players[0].actions[ACTION_LEFT].pressed ||
        input->players[0].actions[ACTION_THROW].pressed)
    {
        menu_set_state(menu, MENU_STATE_MAIN);
    }
}

static void menu_update_options_audio(MenuContext *menu, const FrameInput *input)
{
    i32 option_count = 3;

    if (input->players[0].actions[ACTION_DOWN].pressed)
    {
        menu->selected_option = (menu->selected_option + 1) % option_count;
    }
    if (input->players[0].actions[ACTION_UP].pressed)
    {
        menu->selected_option = (menu->selected_option - 1 + option_count) % option_count;
    }

    if (input->players[0].actions[ACTION_RIGHT].down)
    {
        switch (menu->selected_option)
        {
        case 0:
            menu->config.master_volume += 0.01f;
            if (menu->config.master_volume > 1.0f)
            {
                menu->config.master_volume = 1.0f;
            }
            break;
        case 1:
            menu->config.music_volume += 0.01f;
            if (menu->config.music_volume > 1.0f)
            {
                menu->config.music_volume = 1.0f;
            }
            break;
        case 2:
            menu->config.sfx_volume += 0.01f;
            if (menu->config.sfx_volume > 1.0f)
            {
                menu->config.sfx_volume = 1.0f;
            }
            break;
        default:
            break;
        }
    }

    if (input->players[0].actions[ACTION_LEFT].down)
    {
        switch (menu->selected_option)
        {
        case 0:
            menu->config.master_volume -= 0.01f;
            if (menu->config.master_volume < 0.0f)
            {
                menu->config.master_volume = 0.0f;
            }
            break;
        case 1:
            menu->config.music_volume -= 0.01f;
            if (menu->config.music_volume < 0.0f)
            {
                menu->config.music_volume = 0.0f;
            }
            break;
        case 2:
            menu->config.sfx_volume -= 0.01f;
            if (menu->config.sfx_volume < 0.0f)
            {
                menu->config.sfx_volume = 0.0f;
            }
            break;
        default:
            break;
        }
    }

    if (input->players[0].actions[ACTION_THROW].pressed)
    {
        menu_set_state(menu, MENU_STATE_OPTIONS);
    }
}

static void menu_update_options_keybinds(MenuContext *menu, const FrameInput *input)
{
    if (menu->capturing_key)
    {
        if (input->pause.pressed)
        {
            menu->capturing_key = false;
            return;
        }
        u16 key = input->key_pressed;
        if (key > 0 && key < 512 && key != 41 && key != 68)
        {
            u16 *bindings = menu->config.keybinds[menu->keybind_player].keys;
            u16 previous = bindings[menu->selected_option];
            for (i32 i = 0; i < ACTION_COUNT; ++i)
                if (bindings[i] == key)
                    bindings[i] = previous;
            bindings[menu->selected_option] = key;
            menu->capturing_key = false;
            config_save(&menu->config);
        }
        return;
    }
    if (input->players[0].actions[ACTION_DOWN].pressed)
        menu->selected_option = (menu->selected_option + 1) % ACTION_COUNT;
    if (input->players[0].actions[ACTION_UP].pressed)
        menu->selected_option = (menu->selected_option + ACTION_COUNT - 1) % ACTION_COUNT;
    if (input->players[0].actions[ACTION_LEFT].pressed ||
        input->players[0].actions[ACTION_RIGHT].pressed)
        menu->keybind_player = 1 - menu->keybind_player;
    if (input->players[0].actions[ACTION_THRUST].pressed ||
        input->players[0].actions[ACTION_JUMP].pressed)
        menu->capturing_key = true;
    if (input->players[0].actions[ACTION_THROW].pressed || input->pause.pressed)
        menu_set_state(menu, MENU_STATE_OPTIONS);
}

static void menu_update_pause(MenuContext *menu, const FrameInput *input)
{
    i32 option_count = 2;

    if (input->players[0].actions[ACTION_DOWN].pressed)
    {
        menu->selected_option = (menu->selected_option + 1) % option_count;
    }
    if (input->players[0].actions[ACTION_UP].pressed)
    {
        menu->selected_option = (menu->selected_option - 1 + option_count) % option_count;
    }

    if (input->players[0].actions[ACTION_JUMP].pressed ||
        input->players[0].actions[ACTION_THRUST].pressed)
    {
        switch (menu->selected_option)
        {
        case 0:
            menu->pending_action = MENU_ACTION_RESUME;
            break;
        case 1:
            menu->pending_action = MENU_ACTION_BACK_TO_MENU;
            break;
        default:
            break;
        }
    }

    if (input->pause.pressed)
    {
        menu->pending_action = MENU_ACTION_RESUME;
    }
}

static void menu_update_arena_mode(MenuContext *menu, const FrameInput *input)
{
    i32 option_count = 3;

    if (input->players[0].actions[ACTION_DOWN].pressed)
    {
        menu->selected_option = (menu->selected_option + 1) % option_count;
    }
    if (input->players[0].actions[ACTION_UP].pressed)
    {
        menu->selected_option = (menu->selected_option - 1 + option_count) % option_count;
    }

    menu->arena_options.mode = (ArenaGenerationMode)menu->selected_option;

    if (input->players[0].actions[ACTION_JUMP].pressed ||
        input->players[0].actions[ACTION_THRUST].pressed)
    {
        if (menu->arena_options.mode == ARENA_MODE_PROCEDURAL)
        {
            menu_set_state(menu, MENU_STATE_ARENA_DIFFICULTY);
        }
        else if (menu->arena_options.mode == ARENA_MODE_CORRIDOR)
        {
            menu_set_state(menu, MENU_STATE_ARENA_SEED);
        }
        else
        {

            menu->pending_action = MENU_ACTION_START_MATCH;
        }
    }

    if (input->players[0].actions[ACTION_THROW].pressed)
    {
        menu_set_state(menu, MENU_STATE_MAIN);
    }
}

static void menu_update_arena_difficulty(MenuContext *menu, const FrameInput *input)
{
    if (input->players[0].actions[ACTION_LEFT].pressed)
    {
        menu->arena_options.difficulty--;
        if (menu->arena_options.difficulty < 0)
        {
            menu->arena_options.difficulty = 0;
        }
    }
    if (input->players[0].actions[ACTION_RIGHT].pressed)
    {
        menu->arena_options.difficulty++;
        if (menu->arena_options.difficulty > 10)
        {
            menu->arena_options.difficulty = 10;
        }
    }

    if (input->players[0].actions[ACTION_JUMP].pressed ||
        input->players[0].actions[ACTION_THRUST].pressed)
    {
        menu_set_state(menu, MENU_STATE_ARENA_SEED);
    }

    if (input->players[0].actions[ACTION_THROW].pressed)
    {
        menu_set_state(menu, MENU_STATE_ARENA_MODE);
    }
}

static void menu_update_arena_seed(MenuContext *menu, const FrameInput *input)
{
    u16 key = input->key_pressed;
    if (key >= 30 && key <= 39)
    {
        u32 digit = key == 39 ? 0 : (u32)(key - 29);
        if (menu->arena_options.seed <= (UINT32_MAX - digit) / 10)
            menu->arena_options.seed = menu->arena_options.seed * 10 + digit;
        return;
    }
    if (key == 42)
    {
        menu->arena_options.seed /= 10;
        return;
    }

    if (input->players[0].actions[ACTION_LEFT].pressed ||
        input->players[0].actions[ACTION_RIGHT].pressed ||
        input->players[0].actions[ACTION_UP].pressed ||
        input->players[0].actions[ACTION_DOWN].pressed)
    {
        menu->arena_options.seed = menu_random_seed(menu);
    }

    if (input->players[0].actions[ACTION_JUMP].pressed ||
        input->players[0].actions[ACTION_THRUST].pressed)
    {

        menu->pending_action = MENU_ACTION_START_MATCH;
    }

    if (input->players[0].actions[ACTION_THROW].pressed)
    {
        menu_set_state(menu, MENU_STATE_ARENA_DIFFICULTY);
    }
}

void menu_update(MenuContext *menu, const FrameInput *input, f32 dt)
{
    if (!menu || !input)
    {
        return;
    }

    menu->selection_timer += dt;

    switch (menu->current_state)
    {
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
