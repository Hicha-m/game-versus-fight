#include <stdio.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "game/config.h"
#include "utils/log.h"

/* ========================================
   Default Keybindings
   ======================================== */

static void keybind_set_defaults_p1(PlayerKeybindConfig* kb)
{
    if (!kb) return;
    kb->keys[ACTION_RIGHT] = SDL_SCANCODE_D;
    kb->keys[ACTION_LEFT] = SDL_SCANCODE_A;
    kb->keys[ACTION_UP] = SDL_SCANCODE_W;
    kb->keys[ACTION_DOWN] = SDL_SCANCODE_S;
    kb->keys[ACTION_JUMP] = SDL_SCANCODE_SPACE;
    kb->keys[ACTION_THRUST] = SDL_SCANCODE_J;
    kb->keys[ACTION_THROW] = SDL_SCANCODE_K;
}

static void keybind_set_defaults_p2(PlayerKeybindConfig* kb)
{
    if (!kb) return;
    kb->keys[ACTION_RIGHT] = SDL_SCANCODE_RIGHT;
    kb->keys[ACTION_LEFT] = SDL_SCANCODE_LEFT;
    kb->keys[ACTION_UP] = SDL_SCANCODE_UP;
    kb->keys[ACTION_DOWN] = SDL_SCANCODE_DOWN;
    kb->keys[ACTION_JUMP] = SDL_SCANCODE_RETURN;
    kb->keys[ACTION_THRUST] = SDL_SCANCODE_RCTRL;
    kb->keys[ACTION_THROW] = SDL_SCANCODE_RSHIFT;
}

/* ========================================
   Public API
   ======================================== */

void config_set_defaults(GameConfig* config)
{
    if (!config) {
        return;
    }

    config->master_volume = 0.8f;
    config->music_volume = 0.7f;
    config->sfx_volume = 0.9f;

    keybind_set_defaults_p1(&config->keybinds[0]);
    keybind_set_defaults_p2(&config->keybinds[1]);

    config->ai_difficulty = AI_DIFFICULTY_MEDIUM;
    config->fullscreen = false;
}

void config_reset(GameConfig* config)
{
    config_set_defaults(config);
}

bool config_validate(const GameConfig* config)
{
    if (!config) {
        return false;
    }

    /* Validate volumes */
    if (config->master_volume < 0.0f || config->master_volume > 1.0f) {
        return false;
    }
    if (config->music_volume < 0.0f || config->music_volume > 1.0f) {
        return false;
    }
    if (config->sfx_volume < 0.0f || config->sfx_volume > 1.0f) {
        return false;
    }

    /* Validate difficulty */
    if (config->ai_difficulty < AI_DIFFICULTY_EASY || config->ai_difficulty > AI_DIFFICULTY_EXPERT) {
        return false;
    }

    return true;
}

/* ========================================
   I/O - Loading/Saving JSON Config
   ======================================== */

bool config_load(GameConfig* config)
{
    FILE* f;
    char buffer[2048];
    i32 ai_diff, fullscreen;
    char fullscreen_token[16];

    if (!config) {
        return false;
    }

    config_set_defaults(config);

    f = fopen(CONFIG_FILENAME, "r");
    if (!f) {
        log_info("Config file not found, using defaults");
        return true;
    }

    /* Very simple JSON parsing - look for key patterns */
    while (fgets(buffer, sizeof(buffer), f)) {
        /* Parse audio settings */
        if (sscanf(buffer, "    \"master_volume\": %f", &config->master_volume) == 1) {
            continue;
        }
        if (sscanf(buffer, "    \"music_volume\": %f", &config->music_volume) == 1) {
            continue;
        }
        if (sscanf(buffer, "    \"sfx_volume\": %f", &config->sfx_volume) == 1) {
            continue;
        }

        /* Parse gameplay settings */
        if (sscanf(buffer, "    \"ai_difficulty\": %d", &ai_diff) == 1) {
            config->ai_difficulty = (AIDifficulty)ai_diff;
            continue;
        }
        if (sscanf(buffer, "    \"fullscreen\": %15[^,\n\r ]", fullscreen_token) == 1) {
            if (strcmp(fullscreen_token, "true") == 0) {
                config->fullscreen = true;
                continue;
            }
            if (strcmp(fullscreen_token, "false") == 0) {
                config->fullscreen = false;
                continue;
            }
        }
        if (sscanf(buffer, "    \"fullscreen\": %d", &fullscreen) == 1) {
            config->fullscreen = (bool)fullscreen;
            continue;
        }

        /* Parse player 1 keybinds */
        for (i32 i = 0; i < ACTION_COUNT; i++) {
            i32 scancode;
            if (sscanf(buffer, "      \"key_%d\": %d", &i, &scancode) == 2 && i < ACTION_COUNT) {
                config->keybinds[0].keys[i] = (u16)scancode;
                break;
            }
        }

        /* Parse player 2 keybinds */
        for (i32 i = 0; i < ACTION_COUNT; i++) {
            i32 scancode;
            if (sscanf(buffer, "      \"key_%d\": %d", &i, &scancode) == 2 && i < ACTION_COUNT) {
                config->keybinds[1].keys[i] = (u16)scancode;
                break;
            }
        }
    }

    fclose(f);

    if (!config_validate(config)) {
        log_warn("Config validation failed, resetting to defaults");
        config_set_defaults(config);
        return false;
    }

    log_info("Config loaded from %s", CONFIG_FILENAME);
    return true;
}

bool config_save(const GameConfig* config)
{
    FILE* f;

    if (!config) {
        return false;
    }

    if (!config_validate(config)) {
        log_warn("Config validation failed, not saving");
        return false;
    }

    f = fopen(CONFIG_FILENAME, "w");
    if (!f) {
        log_error("Could not open %s for writing", CONFIG_FILENAME);
        return false;
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"version\": \"1.0\",\n");
    fprintf(f, "  \"audio\": {\n");
    fprintf(f, "    \"master_volume\": %.2f,\n", config->master_volume);
    fprintf(f, "    \"music_volume\": %.2f,\n", config->music_volume);
    fprintf(f, "    \"sfx_volume\": %.2f\n", config->sfx_volume);
    fprintf(f, "  },\n");
    fprintf(f, "  \"gameplay\": {\n");
    fprintf(f, "    \"ai_difficulty\": %d,\n", (i32)config->ai_difficulty);
    fprintf(f, "    \"fullscreen\": %s\n", config->fullscreen ? "true" : "false");
    fprintf(f, "  },\n");
    fprintf(f, "  \"keybinds\": {\n");
    fprintf(f, "    \"player1\": {\n");
    for (i32 i = 0; i < ACTION_COUNT; i++) {
        fprintf(f, "      \"key_%d\": %d%s\n", i, config->keybinds[0].keys[i],
                (i < ACTION_COUNT - 1) ? "," : "");
    }
    fprintf(f, "    },\n");
    fprintf(f, "    \"player2\": {\n");
    for (i32 i = 0; i < ACTION_COUNT; i++) {
        fprintf(f, "      \"key_%d\": %d%s\n", i, config->keybinds[1].keys[i],
                (i < ACTION_COUNT - 1) ? "," : "");
    }
    fprintf(f, "    }\n");
    fprintf(f, "  }\n");
    fprintf(f, "}\n");

    fclose(f);
    log_info("Config saved to %s", CONFIG_FILENAME);
    return true;
}
