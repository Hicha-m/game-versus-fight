#ifndef CONFIG_H
#define CONFIG_H

#include "core/types.h"
#include "core/constants.h"
#include "engine/input.h"
#include "game/ai/ai_types.h"

#define CONFIG_FILENAME "game_config.json"

typedef struct {
    u16 keys[ACTION_COUNT];
} PlayerKeybindConfig;

typedef struct {

    f32 master_volume;
    f32 music_volume;
    f32 sfx_volume;

    PlayerKeybindConfig keybinds[MAX_PLAYERS];

    AIDifficulty ai_difficulty;
    bool fullscreen;
} GameConfig;

void config_set_defaults(GameConfig* config);
void config_reset(GameConfig* config);
bool config_validate(const GameConfig* config);

bool config_load(GameConfig* config);
bool config_save(const GameConfig* config);

#endif
