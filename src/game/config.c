#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "game/config.h"
#include "utils/log.h"

static void keybind_set_defaults_p1(PlayerKeybindConfig *kb)
{
    if (!kb)
        return;
    kb->keys[ACTION_RIGHT] = 7;
    kb->keys[ACTION_LEFT] = 4;
    kb->keys[ACTION_UP] = 26;
    kb->keys[ACTION_DOWN] = 22;
    kb->keys[ACTION_JUMP] = 44;
    kb->keys[ACTION_THRUST] = 13;
    kb->keys[ACTION_THROW] = 14;
}

static void keybind_set_defaults_p2(PlayerKeybindConfig *kb)
{
    if (!kb)
        return;
    kb->keys[ACTION_RIGHT] = 79;
    kb->keys[ACTION_LEFT] = 80;
    kb->keys[ACTION_UP] = 82;
    kb->keys[ACTION_DOWN] = 81;
    kb->keys[ACTION_JUMP] = 40;
    kb->keys[ACTION_THRUST] = 228;
    kb->keys[ACTION_THROW] = 229;
}

void config_set_defaults(GameConfig *config)
{
    if (!config)
    {
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

void config_reset(GameConfig *config)
{
    config_set_defaults(config);
}

bool config_validate(const GameConfig *config)
{
    if (!config)
    {
        return false;
    }

    if (!isfinite(config->master_volume) || !isfinite(config->music_volume) ||
        !isfinite(config->sfx_volume))
        return false;
    for (i32 p = 0; p < MAX_PLAYERS; p++)
        for (i32 a = 0; a < ACTION_COUNT; a++)
            if (config->keybinds[p].keys[a] >= 512)
                return false;
    if (config->master_volume < 0.0f || config->master_volume > 1.0f)
    {
        return false;
    }
    if (config->music_volume < 0.0f || config->music_volume > 1.0f)
    {
        return false;
    }
    if (config->sfx_volume < 0.0f || config->sfx_volume > 1.0f)
    {
        return false;
    }

    if (config->ai_difficulty < AI_DIFFICULTY_EASY || config->ai_difficulty > AI_DIFFICULTY_EXPERT)
    {
        return false;
    }

    return true;
}

/* Bounded JSON reader for the configuration schema. Parse into a temporary
 * object so malformed input cannot partially change live settings. */
static void skip_space(const char **text)
{
    while (isspace((unsigned char)**text))
        ++*text;
}

static bool read_string(const char **text, char *out, size_t capacity)
{
    skip_space(text);
    if (*(*text)++ != '\"')
        return false;
    size_t length = 0;
    while (**text && **text != '\"')
    {
        char value = *(*text)++;
        if ((unsigned char)value < 32 || value == '\\' || length + 1 >= capacity)
            return false;
        out[length++] = value;
    }
    if (**text != '\"')
        return false;
    ++*text;
    out[length] = 0;
    return true;
}

static bool read_object(const char **text, GameConfig *config, const char *section, i32 player,
                        i32 depth)
{
    if (depth > 8)
        return false;
    skip_space(text);
    if (**text != '{')
        return false;
    ++*text;
    skip_space(text);
    if (**text == '}')
    {
        ++*text;
        return true;
    }
    do
    {
        char key[64];
        if (!read_string(text, key, sizeof(key)))
            return false;
        skip_space(text);
        if (**text != ':')
            return false;
        ++*text;
        skip_space(text);
        if (**text == '{')
        {
            i32 child_player = strcmp(key, "player1") == 0   ? 0
                               : strcmp(key, "player2") == 0 ? 1
                                                             : -1;
            if (!read_object(text, config, key, child_player, depth + 1))
                return false;
        }
        else if (**text == '\"')
        {
            char value[128];
            if (!read_string(text, value, sizeof(value)))
                return false;
            if (strcmp(key, "version") != 0)
                return false;
        }
        else
        {
            const char *begin = *text;
            while (**text && **text != ',' && **text != '}' && !isspace((unsigned char)**text))
                ++*text;
            size_t length = (size_t)(*text - begin);
            char value[64];
            if (length == 0 || length >= sizeof(value))
                return false;
            memcpy(value, begin, length);
            value[length] = 0;
            bool boolean = strcmp(value, "true") == 0 || strcmp(value, "false") == 0;
            char *end = NULL;
            errno = 0;
            double number = boolean ? (strcmp(value, "true") == 0) : strtod(value, &end);
            if (!boolean && (errno || !end || *end || !isfinite(number)))
                return false;
            if (strcmp(section, "audio") == 0)
            {
                if (boolean || number < 0 || number > 1)
                    return false;
                if (strcmp(key, "master_volume") == 0)
                    config->master_volume = (f32)number;
                else if (strcmp(key, "music_volume") == 0)
                    config->music_volume = (f32)number;
                else if (strcmp(key, "sfx_volume") == 0)
                    config->sfx_volume = (f32)number;
                else
                    return false;
            }
            else if (strcmp(section, "gameplay") == 0)
            {
                if (strcmp(key, "fullscreen") == 0)
                {
                    if (number != 0 && number != 1)
                        return false;
                    config->fullscreen = number != 0;
                }
                else if (strcmp(key, "ai_difficulty") == 0)
                {
                    if (boolean || number < 0 || number > 3 || floor(number) != number)
                        return false;
                    config->ai_difficulty = (AIDifficulty)number;
                }
                else
                    return false;
            }
            else if (player >= 0)
            {
                if (strncmp(key, "key_", 4) != 0)
                    return false;
                errno = 0;
                char *index_end;
                long action = strtol(key + 4, &index_end, 10);
                if (errno || index_end == key + 4 || *index_end || action < 0 ||
                    action >= ACTION_COUNT || boolean || number <= 0 || number >= 512 ||
                    floor(number) != number)
                    return false;
                config->keybinds[player].keys[action] = (u16)number;
            }
            else
                return false;
        }
        skip_space(text);
        if (**text == '}')
        {
            ++*text;
            return true;
        }
        if (**text != ',')
            return false;
        ++*text;
    } while (**text);
    return false;
}

bool config_load(GameConfig *config)
{
    if (!config)
        return false;
    config_set_defaults(config);
    FILE *file = fopen(CONFIG_FILENAME, "rb");
    if (!file)
        return true;
    char buffer[8193];
    size_t size = fread(buffer, 1, sizeof(buffer) - 1, file);
    bool failed = ferror(file) || !feof(file);
    fclose(file);
    buffer[size] = 0;
    if (failed || memchr(buffer, 0, size))
        return false;
    GameConfig candidate;
    config_set_defaults(&candidate);
    const char *cursor = buffer;
    if (!read_object(&cursor, &candidate, "", -1, 0))
        return false;
    skip_space(&cursor);
    if (*cursor || !config_validate(&candidate))
        return false;
    *config = candidate;
    return true;
}

bool config_save(const GameConfig *config)
{
    FILE *f;

    if (!config)
    {
        return false;
    }

    if (!config_validate(config))
    {
        log_warn("Config validation failed, not saving");
        return false;
    }

    f = fopen(CONFIG_FILENAME ".tmp", "w");
    if (!f)
    {
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
    for (i32 i = 0; i < ACTION_COUNT; i++)
    {
        fprintf(f, "      \"key_%d\": %d%s\n", i, config->keybinds[0].keys[i],
                (i < ACTION_COUNT - 1) ? "," : "");
    }
    fprintf(f, "    },\n");
    fprintf(f, "    \"player2\": {\n");
    for (i32 i = 0; i < ACTION_COUNT; i++)
    {
        fprintf(f, "      \"key_%d\": %d%s\n", i, config->keybinds[1].keys[i],
                (i < ACTION_COUNT - 1) ? "," : "");
    }
    fprintf(f, "    }\n");
    fprintf(f, "  }\n");
    fprintf(f, "}\n");

    bool failed = ferror(f) != 0;
    if (fclose(f) != 0)
        failed = true;
    if (failed)
    {
        remove(CONFIG_FILENAME ".tmp");
        return false;
    }
    if (rename(CONFIG_FILENAME ".tmp", CONFIG_FILENAME) != 0)
        return false;
    log_info("Config saved to %s", CONFIG_FILENAME);
    return true;
}
