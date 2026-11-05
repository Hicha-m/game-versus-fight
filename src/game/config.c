#include <stdio.h>
#include <string.h>
#include <math.h>



#include "game/config.h"
#include "utils/log.h"

static void keybind_set_defaults_p1(PlayerKeybindConfig* kb)
{
    if (!kb) return;
    kb->keys[ACTION_RIGHT] = 7;
    kb->keys[ACTION_LEFT] = 4;
    kb->keys[ACTION_UP] = 26;
    kb->keys[ACTION_DOWN] = 22;
    kb->keys[ACTION_JUMP] = 44;
    kb->keys[ACTION_THRUST] = 13;
    kb->keys[ACTION_THROW] = 14;
}

static void keybind_set_defaults_p2(PlayerKeybindConfig* kb)
{
    if (!kb) return;
    kb->keys[ACTION_RIGHT] = 79;
    kb->keys[ACTION_LEFT] = 80;
    kb->keys[ACTION_UP] = 82;
    kb->keys[ACTION_DOWN] = 81;
    kb->keys[ACTION_JUMP] = 40;
    kb->keys[ACTION_THRUST] = 228;
    kb->keys[ACTION_THROW] = 229;
}

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

    if (!isfinite(config->master_volume) || !isfinite(config->music_volume) || !isfinite(config->sfx_volume)) return false;
    for(i32 p=0;p<MAX_PLAYERS;p++) for(i32 a=0;a<ACTION_COUNT;a++) if(config->keybinds[p].keys[a]>=512) return false;
    if (config->master_volume < 0.0f || config->master_volume > 1.0f) {
        return false;
    }
    if (config->music_volume < 0.0f || config->music_volume > 1.0f) {
        return false;
    }
    if (config->sfx_volume < 0.0f || config->sfx_volume > 1.0f) {
        return false;
    }

    if (config->ai_difficulty < AI_DIFFICULTY_EASY || config->ai_difficulty > AI_DIFFICULTY_EXPERT) {
        return false;
    }

    return true;
}

bool config_load(GameConfig* config)
{
    FILE* f;
    char buffer[2048];
    i32 ai_diff, fullscreen;
    i32 player = 0;
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

    while (fgets(buffer, sizeof(buffer), f)) {

        if (sscanf(buffer, "    \"master_volume\": %f", &config->master_volume) == 1) {
            continue;
        }
        if (sscanf(buffer, "    \"music_volume\": %f", &config->music_volume) == 1) {
            continue;
        }
        if (sscanf(buffer, "    \"sfx_volume\": %f", &config->sfx_volume) == 1) {
            continue;
        }

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

        if (strstr(buffer, "\"player1\"")) player = 0;
        if (strstr(buffer, "\"player2\"")) player = 1;
        i32 action, scancode;
        if (sscanf(buffer, "      \"key_%d\": %d", &action, &scancode) == 2) {
            if (action < 0 || action >= ACTION_COUNT || scancode < 0 || scancode >= 512) {
                fclose(f); config_set_defaults(config); return false;
            }
            config->keybinds[player].keys[action] = (u16)scancode;
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
