#ifndef UI_MENU_H
#define UI_MENU_H

#include "core/types.h"
#include "engine/input.h"
#include "game/config.h"
#include "game/arena/arena.h"

typedef enum {
    MENU_STATE_MAIN = 0,
    MENU_STATE_PLAY_MODE,
    MENU_STATE_DIFFICULTY,
    MENU_STATE_OPTIONS,
    MENU_STATE_OPTIONS_AUDIO,
    MENU_STATE_OPTIONS_KEYBINDS,
    MENU_STATE_PAUSE,

    MENU_STATE_ARENA_MODE,
    MENU_STATE_ARENA_DIFFICULTY,
    MENU_STATE_ARENA_SEED,
} MenuState;

typedef enum {
    MENU_ACTION_NONE = 0,
    MENU_ACTION_PLAY_SOLO,
    MENU_ACTION_PLAY_MULTIPLAYER,
    MENU_ACTION_OPEN_OPTIONS,
    MENU_ACTION_START_MATCH,
    MENU_ACTION_QUIT,
    MENU_ACTION_RESUME,
    MENU_ACTION_BACK_TO_MENU,
} MenuAction;

typedef struct {
    MenuState current_state;
    i32 selected_option;
    f32 selection_timer;

    bool is_solo;
    AIDifficulty selected_difficulty;

    GameConfig config;

    ArenaOptions arena_options;
    u32 seed_input_buffer;
} MenuContext;

bool menu_init(MenuContext* menu);
void menu_shutdown(MenuContext* menu);

void menu_update(MenuContext* menu, const FrameInput* input, f32 dt);

MenuAction menu_get_action(MenuContext* menu);

void menu_set_state(MenuContext* menu, MenuState state);

MenuState menu_get_state(const MenuContext* menu);
i32 menu_get_selected_option(const MenuContext* menu);
const GameConfig* menu_get_config(const MenuContext* menu);
bool menu_is_solo(const MenuContext* menu);
AIDifficulty menu_get_selected_difficulty(const MenuContext* menu);
const ArenaOptions* menu_get_arena_options(const MenuContext* menu);

#endif
