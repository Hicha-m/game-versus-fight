#ifndef UI_MENU_H
#define UI_MENU_H

#include "core/types.h"
#include "engine/input.h"
#include "game/config.h"
#include "game/arena/arena.h"  /* For ArenaOptions */

typedef enum {
    MENU_STATE_MAIN = 0,           /* Menu principal */
    MENU_STATE_PLAY_MODE,          /* Sélection Solo/Multiplayer */
    MENU_STATE_DIFFICULTY,         /* Sélection difficulté IA */
    MENU_STATE_OPTIONS,            /* Paramètres (son, keybindings) */
    MENU_STATE_OPTIONS_AUDIO,      /* Contrôle audio */
    MENU_STATE_OPTIONS_KEYBINDS,   /* Contrôle keybindings */
    MENU_STATE_PAUSE,              /* Menu de pause */
    /* NEW: Arena configuration */
    MENU_STATE_ARENA_MODE,         /* Choisir mode génération */
    MENU_STATE_ARENA_DIFFICULTY,   /* Difficultée 0-10 */
    MENU_STATE_ARENA_SEED,         /* Seed input */
} MenuState;

typedef enum {
    MENU_ACTION_NONE = 0,
    MENU_ACTION_PLAY_SOLO,
    MENU_ACTION_PLAY_MULTIPLAYER,
    MENU_ACTION_OPEN_OPTIONS,
    MENU_ACTION_START_MATCH,
    MENU_ACTION_QUIT,
    MENU_ACTION_RESUME,            /* Reprendre le match */
    MENU_ACTION_BACK_TO_MENU,      /* Retourner au menu principal */
} MenuAction;

typedef struct {
    MenuState current_state;
    i32 selected_option;           /* Index de l'option sélectionnée */
    f32 selection_timer;           /* Pour animation */

    /* Mode de jeu sélectionné */
    bool is_solo;
    AIDifficulty selected_difficulty;

    /* Configuration */
    GameConfig config;

    /* NEW: Arena options */
    ArenaOptions arena_options;
    u32 seed_input_buffer;         /* For seed text input */
} MenuContext;

/* Lifecycle */
bool menu_init(MenuContext* menu);
void menu_shutdown(MenuContext* menu);

/* Update - logique pure, PAS DE SDL */
void menu_update(MenuContext* menu, const FrameInput* input, f32 dt);

/* Obtenir l'action demandée */
MenuAction menu_get_action(MenuContext* menu);

/* Changer d'état */
void menu_set_state(MenuContext* menu, MenuState state);

/* Accesseurs pour le rendu */
MenuState menu_get_state(const MenuContext* menu);
i32 menu_get_selected_option(const MenuContext* menu);
const GameConfig* menu_get_config(const MenuContext* menu);
bool menu_is_solo(const MenuContext* menu);
AIDifficulty menu_get_selected_difficulty(const MenuContext* menu);
const ArenaOptions* menu_get_arena_options(const MenuContext* menu);

#endif
