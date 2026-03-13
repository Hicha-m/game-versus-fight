#include "config.h"

#include "constants.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const BindingAction REBINDABLE_ACTIONS[] = {
	BIND_MOVE_LEFT,
	BIND_MOVE_RIGHT,
	BIND_JUMP,
	BIND_ATTACK,
	BIND_PARRY,
	BIND_STANCE_UP,
	BIND_STANCE_DOWN,
};

static void build_config_path(char *buffer, size_t buffer_size) {
	const char *base_path = SDL_GetBasePath();

	if (base_path == NULL) {
		snprintf(buffer, buffer_size, "config.json");
		return;
	}

	size_t len = strlen(base_path);
	bool ends_with_build = len >= 6 && strcmp(base_path + len - 6, "build/") == 0;
	snprintf(buffer, buffer_size, ends_with_build ? "%s../config.json" : "%sconfig.json", base_path);
}

static void config_set_default_bindings(GameConfig *config) {
	config->bindings[PLAYER_ONE].scancodes[BIND_MOVE_LEFT] = SDL_SCANCODE_LEFT;
	config->bindings[PLAYER_ONE].scancodes[BIND_MOVE_RIGHT] = SDL_SCANCODE_RIGHT;
	config->bindings[PLAYER_ONE].scancodes[BIND_JUMP] = SDL_SCANCODE_RSHIFT;
	config->bindings[PLAYER_ONE].scancodes[BIND_ATTACK] = SDL_SCANCODE_RETURN;
	config->bindings[PLAYER_ONE].scancodes[BIND_PARRY] = SDL_SCANCODE_RCTRL;
	config->bindings[PLAYER_ONE].scancodes[BIND_STANCE_UP] = SDL_SCANCODE_UP;
	config->bindings[PLAYER_ONE].scancodes[BIND_STANCE_DOWN] = SDL_SCANCODE_DOWN;

	config->bindings[PLAYER_TWO].scancodes[BIND_MOVE_LEFT] = SDL_SCANCODE_A;
	config->bindings[PLAYER_TWO].scancodes[BIND_MOVE_RIGHT] = SDL_SCANCODE_D;
	config->bindings[PLAYER_TWO].scancodes[BIND_JUMP] = SDL_SCANCODE_W;
	config->bindings[PLAYER_TWO].scancodes[BIND_ATTACK] = SDL_SCANCODE_F;
	config->bindings[PLAYER_TWO].scancodes[BIND_PARRY] = SDL_SCANCODE_G;
	config->bindings[PLAYER_TWO].scancodes[BIND_STANCE_UP] = SDL_SCANCODE_Q;
	config->bindings[PLAYER_TWO].scancodes[BIND_STANCE_DOWN] = SDL_SCANCODE_S;
}

uint16_t config_clamp_round_time(uint16_t seconds) {
	if (seconds < MIN_ROUND_TIME_SECONDS) {
		return MIN_ROUND_TIME_SECONDS;
	}
	if (seconds > MAX_ROUND_TIME_SECONDS) {
		return MAX_ROUND_TIME_SECONDS;
	}
	return seconds;
}

void config_set_defaults(GameConfig *config) {
	if (config == NULL) {
		return;
	}

	memset(config, 0, sizeof(*config));
	config->arena_width = ARENA_DEFAULT_WIDTH;
	config->arena_height = ARENA_DEFAULT_HEIGHT;
	config->arena_archetype = ARCHETYPE_MEDIUM;
	config->max_round_time_seconds = DEFAULT_ROUND_TIME_SECONDS;
	config->ai_difficulty = DIFFICULTY_NORMAL;
	config->game_mode = GAME_MODE_VERSUS;
	config_set_default_bindings(config);
}

static int parse_int_field(const char *buffer, const char *key, int fallback) {
	const char *start = strstr(buffer, key);
	if (start == NULL) {
		return fallback;
	}

	start = strchr(start, ':');
	if (start == NULL) {
		return fallback;
	}

	return (int)strtol(start + 1, NULL, 10);
}

GameError config_load(GameConfig *config) {
	if (config == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	config_set_defaults(config);
	char path[512];
	build_config_path(path, sizeof(path));

	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		return config_save(config);
	}

	if (fseek(file, 0L, SEEK_END) != 0) {
		fclose(file);
		return GAME_ERROR_IO;
	}

	long size = ftell(file);
	if (size < 0) {
		fclose(file);
		return GAME_ERROR_IO;
	}

	if (fseek(file, 0L, SEEK_SET) != 0) {
		fclose(file);
		return GAME_ERROR_IO;
	}

	char *buffer = (char *)malloc((size_t)size + 1U);
	if (buffer == NULL) {
		fclose(file);
		return GAME_ERROR_OUT_OF_MEMORY;
	}

	size_t read_count = fread(buffer, 1U, (size_t)size, file);
	buffer[read_count] = '\0';
	fclose(file);

	config->arena_width = (uint16_t)parse_int_field(buffer, "\"arena_width\"", config->arena_width);
	config->arena_height = (uint16_t)parse_int_field(buffer, "\"arena_height\"", config->arena_height);
	config->arena_archetype = (ArchetypeType)parse_int_field(buffer, "\"arena_archetype\"", config->arena_archetype);
	config->max_round_time_seconds = config_clamp_round_time((uint16_t)parse_int_field(buffer, "\"max_round_time_seconds\"", config->max_round_time_seconds));
	config->ai_difficulty = (DifficultyLevel)parse_int_field(buffer, "\"ai_difficulty\"", config->ai_difficulty);
	config->game_mode = (GameMode)parse_int_field(buffer, "\"game_mode\"", config->game_mode);

	config->bindings[PLAYER_ONE].scancodes[BIND_MOVE_LEFT] = parse_int_field(buffer, "\"p1_left\"", config->bindings[PLAYER_ONE].scancodes[BIND_MOVE_LEFT]);
	config->bindings[PLAYER_ONE].scancodes[BIND_MOVE_RIGHT] = parse_int_field(buffer, "\"p1_right\"", config->bindings[PLAYER_ONE].scancodes[BIND_MOVE_RIGHT]);
	config->bindings[PLAYER_ONE].scancodes[BIND_JUMP] = parse_int_field(buffer, "\"p1_jump\"", config->bindings[PLAYER_ONE].scancodes[BIND_JUMP]);
	config->bindings[PLAYER_ONE].scancodes[BIND_ATTACK] = parse_int_field(buffer, "\"p1_attack\"", config->bindings[PLAYER_ONE].scancodes[BIND_ATTACK]);
	config->bindings[PLAYER_ONE].scancodes[BIND_PARRY] = parse_int_field(buffer, "\"p1_parry\"", config->bindings[PLAYER_ONE].scancodes[BIND_PARRY]);
	config->bindings[PLAYER_ONE].scancodes[BIND_STANCE_UP] = parse_int_field(buffer, "\"p1_stance_up\"", config->bindings[PLAYER_ONE].scancodes[BIND_STANCE_UP]);
	config->bindings[PLAYER_ONE].scancodes[BIND_STANCE_DOWN] = parse_int_field(buffer, "\"p1_stance_down\"", config->bindings[PLAYER_ONE].scancodes[BIND_STANCE_DOWN]);

	config->bindings[PLAYER_TWO].scancodes[BIND_MOVE_LEFT] = parse_int_field(buffer, "\"p2_left\"", config->bindings[PLAYER_TWO].scancodes[BIND_MOVE_LEFT]);
	config->bindings[PLAYER_TWO].scancodes[BIND_MOVE_RIGHT] = parse_int_field(buffer, "\"p2_right\"", config->bindings[PLAYER_TWO].scancodes[BIND_MOVE_RIGHT]);
	config->bindings[PLAYER_TWO].scancodes[BIND_JUMP] = parse_int_field(buffer, "\"p2_jump\"", config->bindings[PLAYER_TWO].scancodes[BIND_JUMP]);
	config->bindings[PLAYER_TWO].scancodes[BIND_ATTACK] = parse_int_field(buffer, "\"p2_attack\"", config->bindings[PLAYER_TWO].scancodes[BIND_ATTACK]);
	config->bindings[PLAYER_TWO].scancodes[BIND_PARRY] = parse_int_field(buffer, "\"p2_parry\"", config->bindings[PLAYER_TWO].scancodes[BIND_PARRY]);
	config->bindings[PLAYER_TWO].scancodes[BIND_STANCE_UP] = parse_int_field(buffer, "\"p2_stance_up\"", config->bindings[PLAYER_TWO].scancodes[BIND_STANCE_UP]);
	config->bindings[PLAYER_TWO].scancodes[BIND_STANCE_DOWN] = parse_int_field(buffer, "\"p2_stance_down\"", config->bindings[PLAYER_TWO].scancodes[BIND_STANCE_DOWN]);

	free(buffer);
	return GAME_OK;
}

GameError config_save(const GameConfig *config) {
	if (config == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	char path[512];
	build_config_path(path, sizeof(path));

	FILE *file = fopen(path, "wb");
	if (file == NULL) {
		return GAME_ERROR_IO;
	}

	int wrote = fprintf(file,
		"{\n"
		"  \"arena_width\": %u,\n"
		"  \"arena_height\": %u,\n"
		"  \"arena_archetype\": %d,\n"
		"  \"max_round_time_seconds\": %u,\n"
		"  \"ai_difficulty\": %d,\n"
		"  \"game_mode\": %d,\n"
		"  \"p1_left\": %d,\n"
		"  \"p1_right\": %d,\n"
		"  \"p1_jump\": %d,\n"
		"  \"p1_attack\": %d,\n"
		"  \"p1_parry\": %d,\n"
		"  \"p1_stance_up\": %d,\n"
		"  \"p1_stance_down\": %d,\n"
		"  \"p2_left\": %d,\n"
		"  \"p2_right\": %d,\n"
		"  \"p2_jump\": %d,\n"
		"  \"p2_attack\": %d,\n"
		"  \"p2_parry\": %d,\n"
		"  \"p2_stance_up\": %d,\n"
		"  \"p2_stance_down\": %d\n"
		"}\n",
		config->arena_width,
		config->arena_height,
		(int)config->arena_archetype,
		config_clamp_round_time(config->max_round_time_seconds),
		(int)config->ai_difficulty,
		(int)config->game_mode,
		config->bindings[PLAYER_ONE].scancodes[BIND_MOVE_LEFT],
		config->bindings[PLAYER_ONE].scancodes[BIND_MOVE_RIGHT],
		config->bindings[PLAYER_ONE].scancodes[BIND_JUMP],
		config->bindings[PLAYER_ONE].scancodes[BIND_ATTACK],
		config->bindings[PLAYER_ONE].scancodes[BIND_PARRY],
		config->bindings[PLAYER_ONE].scancodes[BIND_STANCE_UP],
		config->bindings[PLAYER_ONE].scancodes[BIND_STANCE_DOWN],
		config->bindings[PLAYER_TWO].scancodes[BIND_MOVE_LEFT],
		config->bindings[PLAYER_TWO].scancodes[BIND_MOVE_RIGHT],
		config->bindings[PLAYER_TWO].scancodes[BIND_JUMP],
		config->bindings[PLAYER_TWO].scancodes[BIND_ATTACK],
		config->bindings[PLAYER_TWO].scancodes[BIND_PARRY],
		config->bindings[PLAYER_TWO].scancodes[BIND_STANCE_UP],
		config->bindings[PLAYER_TWO].scancodes[BIND_STANCE_DOWN]);

	fclose(file);
	return wrote > 0 ? GAME_OK : GAME_ERROR_IO;
}

const char *config_binding_label(BindingAction action) {
	switch (action) {
	case BIND_MOVE_LEFT: return "Move Left";
	case BIND_MOVE_RIGHT: return "Move Right";
	case BIND_JUMP: return "Jump";
	case BIND_ATTACK: return "Thrust";
	case BIND_PARRY: return "Throw";
	case BIND_STANCE_UP: return "Stance Up";
	case BIND_STANCE_DOWN: return "Stance Down";
	default: return "Unknown";
	}
}

const char *config_game_mode_label(GameMode mode) {
	switch (mode) {
	case GAME_MODE_VERSUS: return "Local Versus";
	case GAME_MODE_VS_AI: return "Versus AI";
	default: return "Unknown";
	}
}

const char *config_difficulty_label(DifficultyLevel difficulty) {
	switch (difficulty) {
	case DIFFICULTY_EASY: return "Easy";
	case DIFFICULTY_NORMAL: return "Normal";
	case DIFFICULTY_HARD: return "Hard";
	case DIFFICULTY_EXPERT: return "Expert";
	default: return "Unknown";
	}
}

const char *config_archetype_label(ArchetypeType archetype) {
	switch (archetype) {
	case ARCHETYPE_SMALL: return "Small";
	case ARCHETYPE_MEDIUM: return "Medium";
	case ARCHETYPE_LARGE: return "Large";
	default: return "Medium";
	}
}

uint8_t config_rebindable_action_count(void) {
	return (uint8_t)(sizeof(REBINDABLE_ACTIONS) / sizeof(REBINDABLE_ACTIONS[0]));
}

BindingAction config_rebindable_action_at(uint8_t index) {
	uint8_t count = config_rebindable_action_count();
	if (index >= count) {
		return BIND_MOVE_LEFT;
	}

	return REBINDABLE_ACTIONS[index];
}
