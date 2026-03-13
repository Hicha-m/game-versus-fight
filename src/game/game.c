#include "game.h"

#include "arena.h"
#include "constants.h"
#include "engine/audio.h"
#include "game/config.h"
#include "game/match.h"

#include <SDL3/SDL.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAIN_MENU_ITEM_COUNT 4U
#define MAP_MENU_ITEM_COUNT 3U
#define MAP_GENERATE_ITEM_COUNT 12U
#define MODE_MENU_ITEM_COUNT 6U
#define MAP_RESULTS_PAGE_SIZE 8U

static uint8_t options_item_count(void) {
	return (uint8_t)(config_rebindable_action_count() * 2U + 2U);
}

static uint8_t map_results_item_count(const GameState *state) {
	if (state == NULL) {
		return 1U;
	}
	return (uint8_t)(state->menu.saved_map_filtered_count + 1U);
}

static void generate_preview_map(GameState *state);

static bool contains_case_insensitive(const char *text, const char *needle) {
	if (needle == NULL || needle[0] == '\0') {
		return true;
	}
	if (text == NULL) {
		return false;
	}

	size_t text_len = strlen(text);
	size_t needle_len = strlen(needle);
	if (needle_len > text_len) {
		return false;
	}

	for (size_t i = 0; i + needle_len <= text_len; ++i) {
		bool match = true;
		for (size_t j = 0; j < needle_len; ++j) {
			unsigned char a = (unsigned char)text[i + j];
			unsigned char b = (unsigned char)needle[j];
			if (tolower(a) != tolower(b)) {
				match = false;
				break;
			}
		}
		if (match) {
			return true;
		}
	}

	return false;
}

static int map_results_selected_saved_index(const GameState *state) {
	if (state == NULL) {
		return -1;
	}
	if (state->menu.map_results_index >= state->menu.saved_map_filtered_count) {
		return -1;
	}
	return (int)state->menu.saved_map_filtered_indices[state->menu.map_results_index];
}

static void update_map_preview(GameState *state) {
	if (state == NULL) {
		return;
	}

	int selected = map_results_selected_saved_index(state);
	if (selected < 0) {
		arena_destroy(&state->menu.map_preview);
		state->menu.map_preview_loaded = false;
		state->menu.map_preview_name[0] = '\0';
		return;
	}

	const char *name = state->menu.saved_map_names[selected];
	if (state->menu.map_preview_loaded && strcmp(state->menu.map_preview_name, name) == 0) {
		return;
	}

	arena_destroy(&state->menu.map_preview);
	ArenaSaveSummary summary = {0};
	GameError err = arena_load_from_file(&state->menu.map_preview, name, &summary);
	if (err != GAME_OK) {
		state->menu.map_preview_loaded = false;
		state->menu.map_preview_name[0] = '\0';
		return;
	}

	state->menu.map_preview_loaded = true;
	snprintf(state->menu.map_preview_name, sizeof(state->menu.map_preview_name), "%s", name);
}

static void rebuild_map_results_filter(GameState *state) {
	if (state == NULL) {
		return;
	}

	state->menu.saved_map_filtered_count = 0;
	for (uint8_t i = 0; i < state->menu.saved_map_count; ++i) {
		if (!contains_case_insensitive(state->menu.saved_map_names[i], state->menu.map_results_query)) {
			continue;
		}
		uint8_t out = state->menu.saved_map_filtered_count;
		state->menu.saved_map_filtered_indices[out] = i;
		state->menu.saved_map_filtered_count++;
	}

	if (state->menu.map_results_index >= map_results_item_count(state)) {
		state->menu.map_results_index = 0;
	}

	if (state->menu.saved_map_filtered_count == 0U) {
		state->menu.map_results_page = 0;
	} else {
		state->menu.map_results_page = (uint8_t)(state->menu.map_results_index / MAP_RESULTS_PAGE_SIZE);
	}

	update_map_preview(state);
}

static bool append_scancode_char(char *buffer, size_t buffer_size, int32_t scancode) {
	if (buffer == NULL || buffer_size == 0U || scancode < 0) {
		return false;
	}

	char c = '\0';
	if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
		c = (char)('a' + (scancode - SDL_SCANCODE_A));
	} else if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9) {
		c = (char)('1' + (scancode - SDL_SCANCODE_1));
	} else if (scancode == SDL_SCANCODE_0) {
		c = '0';
	} else if (scancode == SDL_SCANCODE_MINUS) {
		c = '-';
	} else if (scancode == SDL_SCANCODE_SPACE) {
		c = '_';
	}

	if (c == '\0') {
		return false;
	}

	size_t len = strlen(buffer);
	if (len + 1U >= buffer_size) {
		return false;
	}
	buffer[len] = c;
	buffer[len + 1U] = '\0';
	return true;
}

static void remove_last_char(char *buffer) {
	if (buffer == NULL) {
		return;
	}
	size_t len = strlen(buffer);
	if (len > 0U) {
		buffer[len - 1U] = '\0';
	}
}

static void set_status(GameState *state, const char *text) {
	if (state == NULL || text == NULL) {
		return;
	}

	snprintf(state->menu.status_text, sizeof(state->menu.status_text), "%s", text);
	state->menu.status_frames = 240;
}

static void sync_runtime_config(GameState *state) {
	state->config.max_round_time_seconds = config_clamp_round_time(state->config.max_round_time_seconds);
	state->ai_difficulty = state->config.ai_difficulty;
	state->game_mode = state->config.game_mode;
	if (state->config.arena_archetype < ARCHETYPE_SMALL || state->config.arena_archetype > ARCHETYPE_LARGE) {
		state->config.arena_archetype = ARCHETYPE_MEDIUM;
	}
}

static void ensure_defaults(GameState *state) {
	if (state->config.arena_width == 0 || state->config.arena_height == 0) {
		config_set_defaults(&state->config);
		sync_runtime_config(state);
	}
	if (state->menu.generate_width == 0 || state->menu.generate_height == 0) {
		state->menu.generate_archetype = state->config.arena_archetype;
		state->menu.generate_width = state->config.arena_width;
		state->menu.generate_height = state->config.arena_height;
		ArenaGenerationOptions defaults;
		arena_generation_options_defaults(&defaults, state->menu.generate_archetype);
		state->menu.generate_platform_density = defaults.platform_density;
		state->menu.generate_hazard_count = defaults.hazard_count;
		state->menu.generate_hole_count = defaults.hole_count;
		state->menu.generate_hole_max_width = defaults.hole_max_width;
		state->menu.generate_force_symmetry = defaults.force_symmetry;
	}
}

static void move_selection(uint8_t *index, int delta, uint8_t max_value) {
	if (index == NULL || max_value == 0) {
		return;
	}

	int value = (int)(*index) + delta;
	if (value < 0) {
		value = (int)max_value - 1;
	}
	if (value >= (int)max_value) {
		value = 0;
	}
	*index = (uint8_t)value;
}

static uint16_t clamp_u16_range(uint16_t value, uint16_t min_value, uint16_t max_value) {
	if (value < min_value) return min_value;
	if (value > max_value) return max_value;
	return value;
}

static uint8_t clamp_u8_range(uint8_t value, uint8_t min_value, uint8_t max_value) {
	if (value < min_value) return min_value;
	if (value > max_value) return max_value;
	return value;
}

static ArchetypeType cycle_archetype(ArchetypeType current, int delta) {
	int next = (int)current + delta;
	if (next < (int)ARCHETYPE_SMALL) {
		next = (int)ARCHETYPE_LARGE;
	}
	if (next > (int)ARCHETYPE_LARGE) {
		next = (int)ARCHETYPE_SMALL;
	}
	return (ArchetypeType)next;
}

static void refresh_saved_maps(GameState *state) {
	if (state == NULL) {
		return;
	}

	ArenaSaveSummary summaries[MENU_MAX_SAVED_MAPS] = {0};
	uint8_t count = 0;
	if (arena_list_saved_maps(summaries, MENU_MAX_SAVED_MAPS, &count) != GAME_OK) {
		state->menu.saved_map_count = 0;
		state->menu.saved_map_filtered_count = 0;
		state->menu.map_results_index = 0;
		state->menu.map_results_page = 0;
		arena_destroy(&state->menu.map_preview);
		state->menu.map_preview_loaded = false;
		return;
	}

	state->menu.saved_map_count = count;
	for (uint8_t i = 0; i < count; ++i) {
		snprintf(state->menu.saved_map_names[i], MENU_MAX_SAVED_MAP_NAME, "%s", summaries[i].name);
		state->menu.saved_map_width[i] = summaries[i].width;
		state->menu.saved_map_height[i] = summaries[i].height;
		state->menu.saved_map_seed[i] = summaries[i].seed;
		state->menu.saved_map_archetype[i] = summaries[i].archetype;
	}

	rebuild_map_results_filter(state);
}

static void adjust_mode(GameState *state, int delta) {
	int next = (int)state->config.game_mode + delta;
	if (next < (int)GAME_MODE_VERSUS) {
		next = (int)GAME_MODE_VS_AI;
	}
	if (next > (int)GAME_MODE_VS_AI) {
		next = (int)GAME_MODE_VERSUS;
	}
	state->config.game_mode = (GameMode)next;
	sync_runtime_config(state);
	config_save(&state->config);
}

static void adjust_difficulty(GameState *state, int delta) {
	int next = (int)state->config.ai_difficulty + delta;
	if (next < (int)DIFFICULTY_EASY) {
		next = (int)DIFFICULTY_EXPERT;
	}
	if (next > (int)DIFFICULTY_EXPERT) {
		next = (int)DIFFICULTY_EASY;
	}
	state->config.ai_difficulty = (DifficultyLevel)next;
	sync_runtime_config(state);
	config_save(&state->config);
}

static void adjust_round_time(GameState *state, int delta) {
	int next = (int)state->config.max_round_time_seconds + delta * (int)ROUND_TIME_STEP_SECONDS;
	if (next < (int)MIN_ROUND_TIME_SECONDS) {
		next = (int)MAX_ROUND_TIME_SECONDS;
	}
	if (next > (int)MAX_ROUND_TIME_SECONDS) {
		next = (int)MIN_ROUND_TIME_SECONDS;
	}
	state->config.max_round_time_seconds = (uint16_t)next;
	sync_runtime_config(state);
	config_save(&state->config);
}

static void adjust_start_map_selection(GameState *state, int delta) {
	if (state == NULL || state->menu.saved_map_count == 0U || delta == 0) {
		return;
	}

	int index = -1;
	if (state->menu.selected_map_name[0] != '\0') {
		for (uint8_t i = 0; i < state->menu.saved_map_count; ++i) {
			if (strcmp(state->menu.saved_map_names[i], state->menu.selected_map_name) == 0) {
				index = (int)i;
				break;
			}
		}
	}

	int count_plus_default = (int)state->menu.saved_map_count + 1;
	int next = index + delta;
	if (next < -1) {
		next = (int)state->menu.saved_map_count - 1;
	}
	if (next >= count_plus_default - 1) {
		next = -1;
	}

	if (next < 0) {
		state->menu.selected_map_name[0] = '\0';
		set_status(state, "Start map: procedural");
		return;
	}

	snprintf(state->menu.selected_map_name,
		sizeof(state->menu.selected_map_name),
		"%s",
		state->menu.saved_map_names[next]);
	set_status(state, "Start map: saved selection");
}

static GameError begin_match_from_menu(GameState *state) {
	state->rng_state++;
	state->menu.use_saved_map = state->menu.selected_map_name[0] != '\0';
	sync_runtime_config(state);
	GameError err = match_start(state);
	if (err != GAME_OK) {
		set_status(state, "Match start failed");
	}
	return err;
}

static void handle_main_menu(GameState *state, const FrameInput *input) {
	if (input->menu_up_pressed) {
		move_selection(&state->menu.main_menu_index, -1, MAIN_MENU_ITEM_COUNT);
	}
	if (input->menu_down_pressed) {
		move_selection(&state->menu.main_menu_index, 1, MAIN_MENU_ITEM_COUNT);
	}
	if (!input->menu_confirm_pressed) {
		return;
	}

	switch (state->menu.main_menu_index) {
	case 0:
		refresh_saved_maps(state);
		state->game_phase = GAME_PHASE_MODE_SELECT;
		break;
	case 1:
		state->game_phase = GAME_PHASE_MAP_MENU;
		break;
	case 2:
		state->game_phase = GAME_PHASE_OPTIONS;
		break;
	case 3:
		state->running = false;
		break;
	default:
		break;
	}
}

static void handle_map_menu(GameState *state, const FrameInput *input) {
	if (input->menu_up_pressed) {
		move_selection(&state->menu.map_menu_index, -1, MAP_MENU_ITEM_COUNT);
	}
	if (input->menu_down_pressed) {
		move_selection(&state->menu.map_menu_index, 1, MAP_MENU_ITEM_COUNT);
	}
	if (input->menu_back_pressed) {
		state->game_phase = GAME_PHASE_MAIN_MENU;
		return;
	}
	if (!input->menu_confirm_pressed) {
		return;
	}

	if (state->menu.map_menu_index == 0U) {
		generate_preview_map(state);
		state->game_phase = GAME_PHASE_MAP_GENERATE;
		return;
	}
	if (state->menu.map_menu_index == 1U) {
		refresh_saved_maps(state);
		state->menu.map_results_index = 0U;
		state->menu.map_results_page = 0U;
		state->menu.map_results_search_edit = false;
		state->menu.map_results_rename_edit = false;
		state->game_phase = GAME_PHASE_MAP_RESULTS;
		return;
	}

	state->game_phase = GAME_PHASE_MAIN_MENU;
}

static void apply_archetype_preset(GameState *state) {
	ArenaGenerationOptions defaults;
	arena_generation_options_defaults(&defaults, state->menu.generate_archetype);
	state->menu.generate_platform_density = defaults.platform_density;
	state->menu.generate_hazard_count = defaults.hazard_count;
	state->menu.generate_hole_count = defaults.hole_count;
	state->menu.generate_hole_max_width = defaults.hole_max_width;
	state->menu.generate_force_symmetry = defaults.force_symmetry;

	switch (state->menu.generate_archetype) {
	case ARCHETYPE_SMALL:
		state->menu.generate_width = 96;
		state->menu.generate_height = 14;
		break;
	case ARCHETYPE_LARGE:
		state->menu.generate_width = 240;
		state->menu.generate_height = 20;
		break;
	case ARCHETYPE_MEDIUM:
	default:
		state->menu.generate_width = ARENA_DEFAULT_WIDTH;
		state->menu.generate_height = ARENA_DEFAULT_HEIGHT;
		break;
	}
}

static void generate_preview_map(GameState *state) {
	ArenaGenerationOptions options = {
		.archetype = state->menu.generate_archetype,
		.platform_density = state->menu.generate_platform_density,
		.hazard_count = state->menu.generate_hazard_count,
		.hole_count = state->menu.generate_hole_count,
		.hole_max_width = state->menu.generate_hole_max_width,
		.force_symmetry = state->menu.generate_force_symmetry,
	};

	state->menu.generate_width = clamp_u16_range(state->menu.generate_width, MIN_ARENA_WIDTH, MAX_ARENA_WIDTH);
	state->menu.generate_height = clamp_u16_range(state->menu.generate_height, MIN_ARENA_HEIGHT, MAX_ARENA_HEIGHT);
	state->menu.generate_platform_density = clamp_u8_range(state->menu.generate_platform_density, 5U, 90U);
	state->menu.generate_hole_count = clamp_u8_range(state->menu.generate_hole_count, 0U, 8U);
	state->menu.generate_hole_max_width = clamp_u8_range(state->menu.generate_hole_max_width, 1U, 6U);
	state->menu.generate_hazard_count = clamp_u8_range(state->menu.generate_hazard_count, 0U, 12U);

	if (state->menu.generate_seed == 0U) {
		state->menu.generate_seed = state->rng_state + (uint32_t)state->frame_index + 13U;
	}

	arena_destroy(&state->menu.map_preview);
	GameError err = arena_generate_with_options(&state->menu.map_preview,
		state->menu.generate_width,
		state->menu.generate_height,
		state->menu.generate_seed,
		&options);
	if (err != GAME_OK) {
		state->menu.map_preview_loaded = false;
		state->menu.map_preview_name[0] = '\0';
		set_status(state, "Generation failed");
		return;
	}

	state->menu.map_preview_loaded = true;
	snprintf(state->menu.map_preview_name,
		sizeof(state->menu.map_preview_name),
		"preview_%s_%ux%u_s%u",
		config_archetype_label(state->menu.generate_archetype),
		(unsigned)state->menu.map_preview.width,
		(unsigned)state->menu.map_preview.height,
		(unsigned)state->menu.map_preview.seed);
	set_status(state, "Preview generated");
}

static void generate_and_save_map(GameState *state) {
	generate_preview_map(state);
	if (!state->menu.map_preview_loaded || state->menu.map_preview.tiles == NULL) {
		return;
	}

	char map_name[MENU_MAX_SAVED_MAP_NAME];
	snprintf(map_name, sizeof(map_name), "%s_%ux%u_%u",
		config_archetype_label(state->menu.generate_archetype),
		(unsigned)state->menu.map_preview.width,
		(unsigned)state->menu.map_preview.height,
		(unsigned)state->menu.map_preview.seed);

	GameError err = arena_save_to_file(&state->menu.map_preview, map_name, state->menu.generate_archetype);
	if (err != GAME_OK) {
		set_status(state, "Save failed");
		return;
	}

	snprintf(state->menu.selected_map_name, sizeof(state->menu.selected_map_name), "%s", map_name);
	state->config.arena_width = state->menu.generate_width;
	state->config.arena_height = state->menu.generate_height;
	state->config.arena_seed = state->menu.generate_seed;
	state->config.arena_archetype = state->menu.generate_archetype;
	config_save(&state->config);
	state->rng_state = state->menu.generate_seed + 1U;
	state->menu.generate_seed += 1U;

	refresh_saved_maps(state);
	set_status(state, "Map generated and saved");
}

static void handle_map_generate(GameState *state, const FrameInput *input) {
	if (input->menu_up_pressed) {
		move_selection(&state->menu.map_generate_index, -1, MAP_GENERATE_ITEM_COUNT);
	}
	if (input->menu_down_pressed) {
		move_selection(&state->menu.map_generate_index, 1, MAP_GENERATE_ITEM_COUNT);
	}

	int delta = 0;
	if (input->menu_left_pressed) delta = -1;
	if (input->menu_right_pressed) delta = 1;

	if (delta != 0) {
		switch (state->menu.map_generate_index) {
		case 0:
			state->menu.generate_archetype = cycle_archetype(state->menu.generate_archetype, delta);
			apply_archetype_preset(state);
			break;
		case 1:
			state->menu.generate_width = clamp_u16_range((uint16_t)((int)state->menu.generate_width + delta * 8), MIN_ARENA_WIDTH, MAX_ARENA_WIDTH);
			break;
		case 2:
			state->menu.generate_height = clamp_u16_range((uint16_t)((int)state->menu.generate_height + delta), MIN_ARENA_HEIGHT, MAX_ARENA_HEIGHT);
			break;
		case 3:
			if (delta < 0 && state->menu.generate_seed > 0U) {
				state->menu.generate_seed--;
			} else if (delta > 0) {
				state->menu.generate_seed++;
			}
			break;
		case 4:
			state->menu.generate_platform_density = clamp_u8_range((uint8_t)((int)state->menu.generate_platform_density + delta * 5), 5U, 90U);
			break;
		case 5:
			state->menu.generate_hole_count = clamp_u8_range((uint8_t)((int)state->menu.generate_hole_count + delta), 0U, 8U);
			break;
		case 6:
			state->menu.generate_hole_max_width = clamp_u8_range((uint8_t)((int)state->menu.generate_hole_max_width + delta), 1U, 6U);
			break;
		case 7:
			state->menu.generate_hazard_count = clamp_u8_range((uint8_t)((int)state->menu.generate_hazard_count + delta), 0U, 12U);
			break;
		case 8:
			state->menu.generate_force_symmetry = !state->menu.generate_force_symmetry;
			break;
		default:
			break;
		}
	}

	if (input->menu_back_pressed) {
		state->game_phase = GAME_PHASE_MAP_MENU;
		return;
	}

	if (!input->menu_confirm_pressed) {
		return;
	}

	if (state->menu.map_generate_index == 9U) {
		generate_preview_map(state);
		return;
	}
	if (state->menu.map_generate_index == 10U) {
		generate_and_save_map(state);
		return;
	}
	if (state->menu.map_generate_index == 11U) {
		state->game_phase = GAME_PHASE_MAP_MENU;
		return;
	}

	if (state->menu.map_generate_index == 0U) {
		state->menu.generate_archetype = cycle_archetype(state->menu.generate_archetype, 1);
		apply_archetype_preset(state);
	}
}

static void handle_map_results(GameState *state, const FrameInput *input) {
	uint8_t item_count = map_results_item_count(state);
	if (state->menu.map_results_index >= item_count) {
		state->menu.map_results_index = 0;
	}

	if (state->menu.map_results_search_edit || state->menu.map_results_rename_edit) {
		if (input->menu_back_pressed) {
			state->menu.map_results_search_edit = false;
			state->menu.map_results_rename_edit = false;
			set_status(state, "Map text edit cancelled");
			return;
		}

		if (input->pressed_scancode == SDL_SCANCODE_BACKSPACE) {
			remove_last_char(state->menu.map_results_edit_buffer);
			if (state->menu.map_results_search_edit) {
				snprintf(state->menu.map_results_query,
					sizeof(state->menu.map_results_query),
					"%s",
					state->menu.map_results_edit_buffer);
				rebuild_map_results_filter(state);
			}
			return;
		}

		if (input->menu_confirm_pressed) {
			if (state->menu.map_results_search_edit) {
				snprintf(state->menu.map_results_query,
					sizeof(state->menu.map_results_query),
					"%s",
					state->menu.map_results_edit_buffer);
				rebuild_map_results_filter(state);
				set_status(state, "Search query applied");
			}
			if (state->menu.map_results_rename_edit) {
				int selected = map_results_selected_saved_index(state);
				if (selected >= 0 && state->menu.map_results_edit_buffer[0] != '\0') {
					GameError rename_err = arena_rename_saved_map(state->menu.saved_map_names[selected], state->menu.map_results_edit_buffer);
					if (rename_err == GAME_OK) {
						snprintf(state->menu.selected_map_name,
							sizeof(state->menu.selected_map_name),
							"%s",
							state->menu.map_results_edit_buffer);
						refresh_saved_maps(state);
						set_status(state, "Map renamed");
					} else {
						set_status(state, "Rename failed");
					}
				}
			}

			state->menu.map_results_search_edit = false;
			state->menu.map_results_rename_edit = false;
			return;
		}

		append_scancode_char(state->menu.map_results_edit_buffer,
			sizeof(state->menu.map_results_edit_buffer),
			input->pressed_scancode);
		if (state->menu.map_results_search_edit) {
			snprintf(state->menu.map_results_query,
				sizeof(state->menu.map_results_query),
				"%s",
				state->menu.map_results_edit_buffer);
			rebuild_map_results_filter(state);
		}
		return;
	}

	if (input->menu_up_pressed) {
		move_selection(&state->menu.map_results_index, -1, item_count);
		state->menu.map_results_page = (uint8_t)(state->menu.map_results_index / MAP_RESULTS_PAGE_SIZE);
		update_map_preview(state);
	}
	if (input->menu_down_pressed) {
		move_selection(&state->menu.map_results_index, 1, item_count);
		state->menu.map_results_page = (uint8_t)(state->menu.map_results_index / MAP_RESULTS_PAGE_SIZE);
		update_map_preview(state);
	}
	if (input->menu_left_pressed && state->menu.saved_map_filtered_count > 0U && state->menu.map_results_page > 0U) {
		state->menu.map_results_page--;
		state->menu.map_results_index = (uint8_t)(state->menu.map_results_page * MAP_RESULTS_PAGE_SIZE);
		if (state->menu.map_results_index >= state->menu.saved_map_filtered_count) {
			state->menu.map_results_index = state->menu.saved_map_filtered_count - 1U;
		}
		update_map_preview(state);
	}
	if (input->menu_right_pressed && state->menu.saved_map_filtered_count > 0U) {
		uint8_t max_page = (uint8_t)((state->menu.saved_map_filtered_count - 1U) / MAP_RESULTS_PAGE_SIZE);
		if (state->menu.map_results_page < max_page) {
			state->menu.map_results_page++;
			state->menu.map_results_index = (uint8_t)(state->menu.map_results_page * MAP_RESULTS_PAGE_SIZE);
			update_map_preview(state);
		}
	}
	if (input->menu_back_pressed) {
		state->game_phase = GAME_PHASE_MAP_MENU;
		return;
	}

	if (input->pressed_scancode == SDL_SCANCODE_F) {
		state->menu.map_results_search_edit = true;
		snprintf(state->menu.map_results_edit_buffer,
			sizeof(state->menu.map_results_edit_buffer),
			"%s",
			state->menu.map_results_query);
		set_status(state, "Search: type name, Enter confirm");
		return;
	}

	if (input->pressed_scancode == SDL_SCANCODE_R) {
		int selected = map_results_selected_saved_index(state);
		if (selected >= 0) {
			state->menu.map_results_rename_edit = true;
			snprintf(state->menu.map_results_edit_buffer,
				sizeof(state->menu.map_results_edit_buffer),
				"%s",
				state->menu.saved_map_names[selected]);
			set_status(state, "Rename: edit name, Enter confirm");
		}
		return;
	}

	if (input->pressed_scancode == SDL_SCANCODE_DELETE) {
		int selected = map_results_selected_saved_index(state);
		if (selected >= 0) {
			char deleted_name[MENU_MAX_SAVED_MAP_NAME] = {0};
			snprintf(deleted_name, sizeof(deleted_name), "%s", state->menu.saved_map_names[selected]);
			GameError del_err = arena_delete_saved_map(deleted_name);
			if (del_err == GAME_OK) {
				if (strcmp(state->menu.selected_map_name, deleted_name) == 0) {
					state->menu.selected_map_name[0] = '\0';
				}
				refresh_saved_maps(state);
				set_status(state, "Map deleted");
			} else {
				set_status(state, "Delete failed");
			}
		}
		return;
	}

	if (!input->menu_confirm_pressed) {
		return;
	}

	if (state->menu.map_results_index < state->menu.saved_map_filtered_count) {
		uint8_t index = state->menu.saved_map_filtered_indices[state->menu.map_results_index];
		snprintf(state->menu.selected_map_name, sizeof(state->menu.selected_map_name), "%s", state->menu.saved_map_names[index]);
		state->menu.use_saved_map = true;
		if (begin_match_from_menu(state) != GAME_OK) {
			state->menu.use_saved_map = false;
		}
		return;
	}

	state->game_phase = GAME_PHASE_MAP_MENU;
}

static void handle_mode_select(GameState *state, const FrameInput *input) {
	if (input->menu_up_pressed) {
		move_selection(&state->menu.mode_menu_index, -1, MODE_MENU_ITEM_COUNT);
	}
	if (input->menu_down_pressed) {
		move_selection(&state->menu.mode_menu_index, 1, MODE_MENU_ITEM_COUNT);
	}

	if (state->menu.mode_menu_index == 0) {
		if (input->menu_left_pressed) adjust_mode(state, -1);
		if (input->menu_right_pressed) adjust_mode(state, 1);
	}
	if (state->menu.mode_menu_index == 1) {
		if (input->menu_left_pressed) adjust_difficulty(state, -1);
		if (input->menu_right_pressed) adjust_difficulty(state, 1);
	}
	if (state->menu.mode_menu_index == 2) {
		if (input->menu_left_pressed) adjust_round_time(state, -1);
		if (input->menu_right_pressed) adjust_round_time(state, 1);
	}
	if (state->menu.mode_menu_index == 3) {
		if (input->menu_left_pressed) adjust_start_map_selection(state, -1);
		if (input->menu_right_pressed) adjust_start_map_selection(state, 1);
	}

	if (input->menu_back_pressed) {
		state->game_phase = GAME_PHASE_MAIN_MENU;
		return;
	}

	if (!input->menu_confirm_pressed) {
		return;
	}

	if (state->menu.mode_menu_index == 0) {
		adjust_mode(state, 1);
		return;
	}
	if (state->menu.mode_menu_index == 1) {
		adjust_difficulty(state, 1);
		return;
	}
	if (state->menu.mode_menu_index == 2) {
		adjust_round_time(state, 1);
		return;
	}
	if (state->menu.mode_menu_index == 3) {
		adjust_start_map_selection(state, 1);
		return;
	}
	if (state->menu.mode_menu_index == 4) {
		begin_match_from_menu(state);
		return;
	}

	state->game_phase = GAME_PHASE_MAIN_MENU;
}

static void begin_rebind(GameState *state) {
	uint8_t index = state->menu.options_index;
	uint8_t action_count = config_rebindable_action_count();
	if (action_count == 0U || index >= (uint8_t)(action_count * 2U)) {
		set_status(state, "Invalid binding slot");
		return;
	}
	state->menu.waiting_for_rebind = true;
	state->menu.rebind_player = (index < action_count) ? PLAYER_ONE : PLAYER_TWO;
	state->menu.rebind_action = config_rebindable_action_at((uint8_t)(index % action_count));
	set_status(state, "Press a new key");
}

static void complete_rebind(GameState *state, int32_t scancode) {
	if (scancode < 0) {
		return;
	}

	state->config.bindings[state->menu.rebind_player].scancodes[state->menu.rebind_action] = scancode;
	state->menu.waiting_for_rebind = false;
	config_save(&state->config);
	set_status(state, "Controls saved");
}

static void reset_bindings(GameState *state) {
	DifficultyLevel difficulty = state->config.ai_difficulty;
	GameMode mode = state->config.game_mode;
	ArchetypeType archetype = state->config.arena_archetype;
	uint16_t arena_width = state->config.arena_width;
	uint16_t arena_height = state->config.arena_height;
	config_set_defaults(&state->config);
	state->config.ai_difficulty = difficulty;
	state->config.game_mode = mode;
	state->config.arena_archetype = archetype;
	state->config.arena_width = arena_width;
	state->config.arena_height = arena_height;
	sync_runtime_config(state);
	config_save(&state->config);
	set_status(state, "Default controls restored");
}

static void handle_options(GameState *state, const FrameInput *input) {
	uint8_t item_count = options_item_count();
	uint8_t action_count = config_rebindable_action_count();
	uint8_t reset_index = (uint8_t)(action_count * 2U);
	uint8_t back_index = (uint8_t)(reset_index + 1U);

	if (item_count == 0U) {
		state->menu.options_index = 0;
		return;
	}
	if (state->menu.options_index >= item_count) {
		state->menu.options_index = (uint8_t)(item_count - 1U);
	}

	if (state->menu.waiting_for_rebind) {
		if (input->menu_back_pressed) {
			state->menu.waiting_for_rebind = false;
			set_status(state, "Rebind cancelled");
			return;
		}
		if (input->pressed_scancode >= 0) {
			complete_rebind(state, input->pressed_scancode);
		}
		return;
	}

	if (input->menu_up_pressed) {
		move_selection(&state->menu.options_index, -1, item_count);
	}
	if (input->menu_down_pressed) {
		move_selection(&state->menu.options_index, 1, item_count);
	}
	if (input->menu_back_pressed) {
		config_save(&state->config);
		state->game_phase = GAME_PHASE_MAIN_MENU;
		return;
	}
	if (!input->menu_confirm_pressed) {
		return;
	}

	if (state->menu.options_index < reset_index) {
		begin_rebind(state);
		return;
	}
	if (state->menu.options_index == reset_index) {
		reset_bindings(state);
		return;
	}
	if (state->menu.options_index == back_index) {
		config_save(&state->config);
		state->game_phase = GAME_PHASE_MAIN_MENU;
	}
}

GameError game_create(const GameConfig *config, GameState *out_state) {
	if (out_state == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	*out_state = (GameState){0};
	out_state->running = true;
	out_state->game_phase = GAME_PHASE_BOOT;
	out_state->match_phase = MATCH_PHASE_MENU;
	out_state->rng_state = (uint32_t)time(NULL);
	config_set_defaults(&out_state->config);
	if (config != NULL) {
		out_state->config = *config;
	} else {
		config_load(&out_state->config);
	}
	sync_runtime_config(out_state);
	out_state->round_winner = PLAYER_ONE;
	out_state->active_segment = MAP_MIDDLE_SEGMENT_INDEX;
	out_state->pending_segment = MAP_MIDDLE_SEGMENT_INDEX;
	out_state->menu.generate_archetype = out_state->config.arena_archetype;
	out_state->menu.generate_width = out_state->config.arena_width;
	out_state->menu.generate_height = out_state->config.arena_height;
	out_state->menu.generate_seed = out_state->rng_state;
	ArenaGenerationOptions defaults;
	arena_generation_options_defaults(&defaults, out_state->menu.generate_archetype);
	out_state->menu.generate_platform_density = defaults.platform_density;
	out_state->menu.generate_hazard_count = defaults.hazard_count;
	out_state->menu.generate_hole_count = defaults.hole_count;
	out_state->menu.generate_hole_max_width = defaults.hole_max_width;
	out_state->menu.generate_force_symmetry = defaults.force_symmetry;
	refresh_saved_maps(out_state);
	return GAME_OK;
}

GameError game_update(GameState *state, const FrameInput *input) {
	if (state == NULL || input == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	ensure_defaults(state);
	state->frame_index++;

	if (state->menu.status_frames > 0) {
		state->menu.status_frames--;
	}
	if (input->quit_requested) {
		state->running = false;
		return GAME_OK;
	}

	if (state->game_phase != GAME_PHASE_MATCH && input->menu_confirm_pressed) {
		audio_play_sfx(AUDIO_SFX_MENU_CONFIRM);
	}
	if (state->game_phase != GAME_PHASE_MATCH && input->menu_back_pressed) {
		audio_play_sfx(AUDIO_SFX_MENU_BACK);
	}

	switch (state->game_phase) {
	case GAME_PHASE_BOOT:
		state->game_phase = GAME_PHASE_PRESS_START;
		state->phase_elapsed = 0;
		state->menu.press_start_armed = false;
		break;
	case GAME_PHASE_PRESS_START:
		if (state->phase_elapsed < 20U) {
			state->phase_elapsed++;
			break;
		}
		state->menu.press_start_armed = true;
		if (state->menu.press_start_armed && (input->menu_confirm_pressed || input->any_key_pressed)) {
			audio_play_sfx(AUDIO_SFX_MENU_CONFIRM);
			state->game_phase = GAME_PHASE_MAIN_MENU;
		}
		break;
	case GAME_PHASE_MAIN_MENU:
		handle_main_menu(state, input);
		break;
	case GAME_PHASE_MAP_MENU:
		handle_map_menu(state, input);
		break;
	case GAME_PHASE_MAP_GENERATE:
		handle_map_generate(state, input);
		break;
	case GAME_PHASE_MAP_RESULTS:
		handle_map_results(state, input);
		break;
	case GAME_PHASE_MODE_SELECT:
		handle_mode_select(state, input);
		break;
	case GAME_PHASE_OPTIONS:
		handle_options(state, input);
		break;
	case GAME_PHASE_MATCH:
		return match_update(state, input);
	case GAME_PHASE_GAME_OVER:
		if (input->menu_confirm_pressed || input->menu_back_pressed) {
			match_abort(state);
			state->game_phase = GAME_PHASE_MAIN_MENU;
		}
		break;
	case GAME_PHASE_QUIT:
		state->running = false;
		break;
	default:
		break;
	}

	return GAME_OK;
}

GameError game_destroy(GameState *state) {
	if (state == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	match_abort(state);
	arena_destroy(&state->menu.map_preview);
	state->running = false;
	return GAME_OK;
}


