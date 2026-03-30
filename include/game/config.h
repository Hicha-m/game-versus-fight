#ifndef CONFIG_H
#define CONFIG_H


#include "core/types.h"
#include "core/constants.h"
#include "engine/input.h"
#include "game/ai/ai_types.h"

#define CONFIG_FILENAME "game_config.json"

/* Keybindings pour un joueur */
typedef struct {
    u16 keys[ACTION_COUNT];  /* SDL_Scancode values */
} PlayerKeybindConfig;

/* Configuration générale du jeu */
typedef struct {
    /* Audio */
    f32 master_volume;      /* 0.0 - 1.0 */
    f32 music_volume;       /* 0.0 - 1.0 */
    f32 sfx_volume;         /* 0.0 - 1.0 */

    /* Contrôles - MAX_PLAYERS joueurs */
    PlayerKeybindConfig keybinds[MAX_PLAYERS];

    /* Gameplay */
    AIDifficulty ai_difficulty;
    bool fullscreen;
} GameConfig;

/* Lifecycle */
void config_set_defaults(GameConfig* config);
void config_reset(GameConfig* config);
bool config_validate(const GameConfig* config);

/* I/O */
bool config_load(GameConfig* config);
bool config_save(const GameConfig* config);

#endif

