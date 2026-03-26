#include "menu.h"
#include "config.h"
#include "engine.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>

#define MAP_RESULTS_PAGE_SIZE 8U

static void format_round_time(char *buffer, size_t buffer_size, uint16_t seconds) {
	snprintf(buffer, buffer_size, "Round Time: %02u:%02u", seconds / 60U, seconds % 60U);
}

static const char *binding_name(int32_t scancode) {
	const char *name = SDL_GetScancodeName((SDL_Scancode)scancode);
	return (name != NULL && name[0] != '\0') ? name : "Unbound";
}

static void draw_line(SDL_Renderer *renderer, float x, float y, const char *text, bool selected) {
	if (selected) {
		SDL_SetRenderDrawColor(renderer, 230, 185, 70, 255);
		SDL_FRect marker = {x - 20.0f, y - 2.0f, 12.0f, 12.0f};
		SDL_RenderFillRect(renderer, &marker);
	}
	SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
	SDL_RenderDebugText(renderer, x, y, text);
}

static void draw_map_preview(SDL_Renderer *renderer, const GameState *state, float x, float y, float w, float h);

static void render_main_menu(SDL_Renderer *renderer, const GameState *state) {
	const char *items[] = {"Start", "Map", "Options", "Quit"};
	SDL_RenderDebugText(renderer, 80.0f, 88.0f, "BLADE RUSH");
	SDL_RenderDebugText(renderer, 80.0f, 112.0f, "Push to the enemy base.");
	for (int index = 0; index < 4; ++index) {
		draw_line(renderer, 110.0f, 180.0f + (float)index * 26.0f,
			items[index], state->menu.main_menu_index == (uint8_t)index);
	}
}

static void render_map_menu(SDL_Renderer *renderer, const GameState *state) {
	const char *items[] = {"Generate", "Results", "Back"};
	SDL_RenderDebugText(renderer, 80.0f, 88.0f, "MAP MENU");
	SDL_RenderDebugText(renderer, 80.0f, 112.0f, "Create and test procedural arenas.");
	for (int index = 0; index < 3; ++index) {
		draw_line(renderer, 110.0f, 180.0f + (float)index * 26.0f,
			items[index], state->menu.map_menu_index == (uint8_t)index);
	}
}

static void render_mode_menu(SDL_Renderer *renderer, const GameState *state) {
	char buffer[96];
	SDL_RenderDebugText(renderer, 80.0f, 88.0f, "MATCH SETUP");
	snprintf(buffer, sizeof(buffer), "Mode: %s", config_game_mode_label(state->config.game_mode));
	draw_line(renderer, 110.0f, 164.0f, buffer, state->menu.mode_menu_index == 0);
	snprintf(buffer, sizeof(buffer), "AI: %s", config_difficulty_label(state->config.ai_difficulty));
	draw_line(renderer, 110.0f, 190.0f, buffer, state->menu.mode_menu_index == 1);
	format_round_time(buffer, sizeof(buffer), state->config.max_round_time_seconds);
	draw_line(renderer, 110.0f, 216.0f, buffer, state->menu.mode_menu_index == 2);
	if (state->menu.selected_map_name[0] != '\0') {
		snprintf(buffer, sizeof(buffer), "Start map: %.50s", state->menu.selected_map_name);
	} else {
		snprintf(buffer, sizeof(buffer), "Start map: procedural");
	}
	draw_line(renderer, 110.0f, 242.0f, buffer, state->menu.mode_menu_index == 3);
	draw_line(renderer, 110.0f, 268.0f, "Play", state->menu.mode_menu_index == 4);
	draw_line(renderer, 110.0f, 294.0f, "Back", state->menu.mode_menu_index == 5);
	if (state->menu.selected_map_name[0] != '\0') {
		snprintf(buffer, sizeof(buffer), "Selected saved map: %.60s", state->menu.selected_map_name);
		SDL_RenderDebugText(renderer, 80.0f, 340.0f, buffer);
	}
	SDL_RenderDebugText(renderer, 80.0f, 316.0f, "Use left/right on Start map to choose saved maps.");
	SDL_RenderDebugText(renderer, 80.0f, 364.0f, "P1 goes left. P2 goes right.");
}

static void render_map_generate(SDL_Renderer *renderer, const GameState *state) {
	char buffer[128];
	SDL_RenderDebugText(renderer, 70.0f, 68.0f, "MAP GENERATE - tweak options, then save");

	snprintf(buffer, sizeof(buffer), "Archetype: %s", config_archetype_label(state->menu.generate_archetype));
	draw_line(renderer, 90.0f, 130.0f, buffer, state->menu.map_generate_index == 0);
	snprintf(buffer, sizeof(buffer), "Width: %u", state->menu.generate_width);
	draw_line(renderer, 90.0f, 156.0f, buffer, state->menu.map_generate_index == 1);
	snprintf(buffer, sizeof(buffer), "Height: %u", state->menu.generate_height);
	draw_line(renderer, 90.0f, 182.0f, buffer, state->menu.map_generate_index == 2);
	snprintf(buffer, sizeof(buffer), "Seed: %u", state->menu.generate_seed);
	draw_line(renderer, 90.0f, 208.0f, buffer, state->menu.map_generate_index == 3);
	snprintf(buffer, sizeof(buffer), "Platform density: %u%%", state->menu.generate_platform_density);
	draw_line(renderer, 90.0f, 234.0f, buffer, state->menu.map_generate_index == 4);
	snprintf(buffer, sizeof(buffer), "Holes: %u", state->menu.generate_hole_count);
	draw_line(renderer, 90.0f, 260.0f, buffer, state->menu.map_generate_index == 5);
	snprintf(buffer, sizeof(buffer), "Hole max width: %u", state->menu.generate_hole_max_width);
	draw_line(renderer, 90.0f, 286.0f, buffer, state->menu.map_generate_index == 6);
	snprintf(buffer, sizeof(buffer), "Hazards: %u", state->menu.generate_hazard_count);
	draw_line(renderer, 90.0f, 312.0f, buffer, state->menu.map_generate_index == 7);
	snprintf(buffer, sizeof(buffer), "Force symmetry: %s", state->menu.generate_force_symmetry ? "ON" : "OFF");
	draw_line(renderer, 90.0f, 338.0f, buffer, state->menu.map_generate_index == 8);
	draw_line(renderer, 90.0f, 370.0f, "Generate Preview", state->menu.map_generate_index == 9);
	draw_line(renderer, 90.0f, 396.0f, "Generate and Save", state->menu.map_generate_index == 10);
	draw_line(renderer, 90.0f, 422.0f, "Back", state->menu.map_generate_index == 11);

	SDL_RenderDebugText(renderer, 70.0f, 456.0f, "Use left/right arrows to tweak values.");
	SDL_RenderDebugText(renderer, 70.0f, 480.0f, "Preview lets you inspect a generated map before saving.");
	draw_map_preview(renderer, state, 760.0f, 120.0f, 450.0f, 440.0f);
}

static void draw_map_preview(SDL_Renderer *renderer, const GameState *state, float x, float y, float w, float h) {
	SDL_SetRenderDrawColor(renderer, 28, 34, 46, 255);
	SDL_FRect panel = {x, y, w, h};
	SDL_RenderFillRect(renderer, &panel);
	SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
	SDL_RenderRect(renderer, &panel);

	if (!state->menu.map_preview_loaded || state->menu.map_preview.tiles == NULL) {
		SDL_RenderDebugText(renderer, x + 14.0f, y + 16.0f, "No preview");
		return;
	}

	const Arena *arena = &state->menu.map_preview;
	float scale_x = (w - 16.0f) / (float)arena->width;
	float scale_y = (h - 36.0f) / (float)arena->height;
	float tile = fminf(scale_x, scale_y);
	if (tile < 1.0f) {
		tile = 1.0f;
	}
	float grid_w = tile * arena->width;
	float ox = x + (w - grid_w) * 0.5f;
	float oy = y + 24.0f;

	for (uint16_t ty = 0; ty < arena->height; ++ty) {
		for (uint16_t tx = 0; tx < arena->width; ++tx) {
			TileType tile_type = arena->tiles[ty * arena->width + tx];
			switch (tile_type) {
			case TILE_EMPTY: SDL_SetRenderDrawColor(renderer, 33, 42, 56, 255); break;
			case TILE_SOLID: SDL_SetRenderDrawColor(renderer, 132, 126, 118, 255); break;
			case TILE_PLATFORM: SDL_SetRenderDrawColor(renderer, 156, 110, 67, 255); break;
			case TILE_SPAWN_P1: SDL_SetRenderDrawColor(renderer, 42, 110, 224, 255); break;
			case TILE_SPAWN_P2: SDL_SetRenderDrawColor(renderer, 220, 72, 55, 255); break;
			case TILE_HAZARD: SDL_SetRenderDrawColor(renderer, 218, 132, 35, 255); break;
			default: SDL_SetRenderDrawColor(renderer, 96, 96, 96, 255); break;
			}

			SDL_FRect cell = {
				ox + (float)tx * tile,
				oy + (float)ty * tile,
				tile,
				tile
			};
			SDL_RenderFillRect(renderer, &cell);
		}
	}

	SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
	SDL_RenderDebugText(renderer, x + 14.0f, y + 8.0f, state->menu.map_preview_name);
}

static void render_map_results(SDL_Renderer *renderer, const GameState *state) {
	char line[128];
	SDL_RenderDebugText(renderer, 70.0f, 68.0f, "MAP RESULTS - launch a saved map");
	snprintf(line, sizeof(line), "Search: %s", state->menu.map_results_query[0] != '\0' ? state->menu.map_results_query : "(none)");
	SDL_RenderDebugText(renderer, 70.0f, 92.0f, line);

	if (state->menu.saved_map_filtered_count == 0) {
		SDL_RenderDebugText(renderer, 90.0f, 126.0f, "No saved maps yet");
		draw_line(renderer, 90.0f, 160.0f, "Back", state->menu.map_results_index == 0);
		SDL_RenderDebugText(renderer, 70.0f, 442.0f, "F: search, Enter: launch, Esc: back");
		draw_map_preview(renderer, state, 760.0f, 120.0f, 450.0f, 440.0f);
		return;
	}

	uint8_t total = state->menu.saved_map_filtered_count;
	uint8_t page = state->menu.map_results_page;
	uint8_t max_page = (uint8_t)((total - 1U) / MAP_RESULTS_PAGE_SIZE);
	if (page > max_page) {
		page = max_page;
	}
	uint8_t start = (uint8_t)(page * MAP_RESULTS_PAGE_SIZE);
	uint8_t end = (uint8_t)(start + MAP_RESULTS_PAGE_SIZE);
	if (end > total) {
		end = total;
	}

	float y = 130.0f;
	for (uint8_t filtered = start; filtered < end; ++filtered) {
		uint8_t index = state->menu.saved_map_filtered_indices[filtered];
		snprintf(line, sizeof(line), "%s (%ux%u) %s seed=%u",
			state->menu.saved_map_names[index],
			state->menu.saved_map_width[index],
			state->menu.saved_map_height[index],
			config_archetype_label(state->menu.saved_map_archetype[index]),
			state->menu.saved_map_seed[index]);
		draw_line(renderer, 90.0f, y, line, state->menu.map_results_index == filtered);
		y += 24.0f;
	}

	draw_line(renderer, 90.0f, y + 10.0f, "Back", state->menu.map_results_index == state->menu.saved_map_filtered_count);
	snprintf(line, sizeof(line), "Page %u/%u", (unsigned)(page + 1U), (unsigned)(max_page + 1U));
	SDL_RenderDebugText(renderer, 90.0f, y + 38.0f, line);

	if (state->menu.map_results_search_edit) {
		snprintf(line, sizeof(line), "Search edit: %s_", state->menu.map_results_edit_buffer);
		SDL_RenderDebugText(renderer, 70.0f, 466.0f, line);
	} else if (state->menu.map_results_rename_edit) {
		snprintf(line, sizeof(line), "Rename edit: %s_", state->menu.map_results_edit_buffer);
		SDL_RenderDebugText(renderer, 70.0f, 466.0f, line);
	} else {
		SDL_RenderDebugText(renderer, 70.0f, 466.0f, "Enter: launch | F: search | R: rename | Delete: remove | Left/Right: pages");
	}

	draw_map_preview(renderer, state, 760.0f, 120.0f, 450.0f, 440.0f);
}

static void render_options_menu(SDL_Renderer *renderer, const GameState *state) {
	char buffer[128];
	float y = 96.0f;
	uint8_t action_count = config_rebindable_action_count();
	SDL_RenderDebugText(renderer, 60.0f, 62.0f, "OPTIONS / PERSISTENT CONTROLS");

	for (int player = 0; player < 2; ++player) {
		for (uint8_t action = 0; action < action_count; ++action) {
			BindingAction binding = config_rebindable_action_at(action);
			int index = player * action_count + action;
			snprintf(buffer, sizeof(buffer), "P%d %-12s : %s",
				player + 1,
				config_binding_label(binding),
				binding_name(state->config.bindings[player].scancodes[binding]));
			draw_line(renderer, 70.0f, y, buffer, state->menu.options_index == (uint8_t)index);
			y += 22.0f;
		}
		y += 8.0f;
	}

	draw_line(renderer, 70.0f, y, "Reset Defaults", state->menu.options_index == (uint8_t)(action_count * 2));
	y += 24.0f;
	draw_line(renderer, 70.0f, y, "Back", state->menu.options_index == (uint8_t)(action_count * 2 + 1));

	if (state->menu.waiting_for_rebind) {
		SDL_SetRenderDrawColor(renderer, 10, 10, 10, 220);
		SDL_FRect box = {620.0f, 120.0f, 520.0f, 90.0f};
		SDL_RenderFillRect(renderer, &box);
		SDL_SetRenderDrawColor(renderer, 255, 220, 90, 255);
		SDL_RenderDebugText(renderer, 648.0f, 148.0f, "Press a new key or ESC to cancel");
	}
}


GameError menu_render(const GameState *state) {
	if (state == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	SDL_Renderer *renderer = engine_renderer_get();
	if (renderer == NULL) return GAME_ERROR_INVALID_STATE;

	SDL_SetRenderDrawColor(renderer, 18, 30, 44, 255);
	SDL_FRect background = {0.0f, 0.0f, 1280.0f, 720.0f};
	SDL_RenderFillRect(renderer, &background);

	SDL_SetRenderDrawColor(renderer, 48, 78, 110, 255);
	SDL_FRect panel = {40.0f, 40.0f, 1200.0f, 640.0f};
	SDL_RenderFillRect(renderer, &panel);

	switch (state->game_phase) {
	case GAME_PHASE_PRESS_START:
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderDebugText(renderer, 88.0f, 120.0f, "BLADE RUSH");
		if (((state->frame_index / 30U) % 2U) == 0U) {
			SDL_RenderDebugText(renderer, 88.0f, 164.0f, "PRESS ANY KEY TO START");
		}
		SDL_RenderDebugText(renderer, 88.0f, 192.0f, "Fight from the middle to the enemy base.");
		break;
	case GAME_PHASE_MAIN_MENU:
		 render_main_menu(renderer, state);
		 break;
	case GAME_PHASE_MAP_MENU:
		render_map_menu(renderer, state);
		break;
	case GAME_PHASE_MAP_GENERATE:
		render_map_generate(renderer, state);
		break;
	case GAME_PHASE_MAP_RESULTS:
		render_map_results(renderer, state);
		break;
	case GAME_PHASE_MODE_SELECT:
		 render_mode_menu(renderer, state);
		 break;
	case GAME_PHASE_OPTIONS:
		 render_options_menu(renderer, state);
		 break;
	default:
		break;
	}

	if (state->menu.status_frames > 0 && state->menu.status_text[0] != '\0') {
		SDL_SetRenderDrawColor(renderer, 255, 220, 90, 255);
		SDL_RenderDebugText(renderer, 70.0f, 650.0f, state->menu.status_text);
	}
	
	return GAME_OK;
}
