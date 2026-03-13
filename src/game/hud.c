#include "hud.h"
#include "engine.h"
#include "constants.h"

#include <SDL3/SDL.h>
#include <stdio.h>

GameError hud_render(const GameState *state) {
	if (state == NULL) return GAME_ERROR_INVALID_ARGUMENT;

	SDL_Renderer *renderer = engine_renderer_get();
	if (renderer == NULL) return GAME_ERROR_INVALID_STATE;

	const CombatState *c  = &state->combat;
	const uint16_t     aw = (c->arena_width > 0) ? c->arena_width : 1;
	char buf[64];

	/* ── HUD background strip ── */
	SDL_SetRenderDrawColor(renderer, 10, 12, 20, 255);
	SDL_FRect bg = {0.0f, 0.0f, (float)WINDOW_W, (float)HUD_HEIGHT_PX};
	SDL_RenderFillRect(renderer, &bg);

	/* ── Score labels ── */
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	snprintf(buf, sizeof(buf), "P1:%d", c->score[PLAYER_ONE]);
	SDL_RenderDebugText(renderer, 10.0f, 8.0f, buf);
	snprintf(buf, sizeof(buf), "P2:%d", c->score[PLAYER_TWO]);
	SDL_RenderDebugText(renderer, (float)WINDOW_W - 50.0f, 8.0f, buf);
	snprintf(buf, sizeof(buf), "%s", state->game_mode == GAME_MODE_VS_AI ? "VS AI" : "LOCAL VS");
	SDL_RenderDebugText(renderer, (float)WINDOW_W * 0.5f - 28.0f, 8.0f, buf);
	if (c->has_priority) {
		snprintf(buf, sizeof(buf), "GO: P%d", c->priority_owner + 1);
		SDL_SetRenderDrawColor(renderer, 255, 205, 80, 255);
		SDL_RenderDebugText(renderer, (float)WINDOW_W * 0.5f + 44.0f, 8.0f, buf);
	}

	/* ── Territorial progress bar ── */
	const float bar_x = 80.0f;
	const float bar_y = 6.0f;
	const float bar_w = (float)WINDOW_W - 160.0f;
	const float bar_h = 10.0f;

	SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
	SDL_FRect bar_bg = {bar_x, bar_y, bar_w, bar_h};
	SDL_RenderFillRect(renderer, &bar_bg);

	/* P1 (blue) advances toward the left base */
	float p1_adv = 1.0f - (c->fighters[PLAYER_ONE].position.x / (float)aw);
	if (p1_adv < 0.0f) p1_adv = 0.0f;
	if (p1_adv > 1.0f) p1_adv = 1.0f;
	if (p1_adv > 0.001f) {
		SDL_SetRenderDrawColor(renderer, 30, 100, 220, 255);
		SDL_FRect p1b = {bar_x + bar_w * (1.0f - p1_adv), bar_y, bar_w * p1_adv, bar_h};
		SDL_RenderFillRect(renderer, &p1b);
	}

	/* P2 (red) advances toward the right base */
	float p2_adv = c->fighters[PLAYER_TWO].position.x / (float)aw;
	if (p2_adv < 0.0f) p2_adv = 0.0f;
	if (p2_adv > 1.0f) p2_adv = 1.0f;
	if (p2_adv > 0.001f) {
		SDL_SetRenderDrawColor(renderer, 220, 30, 30, 255);
		SDL_FRect p2b = {bar_x, bar_y, bar_w * p2_adv, bar_h};
		SDL_RenderFillRect(renderer, &p2b);
	}

	SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
	SDL_RenderRect(renderer, &bar_bg);

	/* ── Timer ── */
	uint32_t total_sec = c->round_time_frames / 60;
	snprintf(buf, sizeof(buf), "%02d:%02d", total_sec / 60, total_sec % 60);
	SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
	SDL_RenderDebugText(renderer, (float)WINDOW_W * 0.5f - 20.0f,
	                    bar_y + bar_h + 4.0f, buf);

	/* ── Sword height indicators ── */
	const char *ht[3] = {"LOW", "MID", " HI"};
	SDL_SetRenderDrawColor(renderer, 220, 200, 80, 255);
	snprintf(buf, sizeof(buf), "P1 %s", ht[c->fighters[PLAYER_ONE].sword_height]);
	SDL_RenderDebugText(renderer, 10.0f, 28.0f, buf);
	snprintf(buf, sizeof(buf), "P2 %s", ht[c->fighters[PLAYER_TWO].sword_height]);
	SDL_RenderDebugText(renderer, (float)WINDOW_W - 55.0f, 28.0f, buf);

	if (!c->fighters[PLAYER_ONE].has_sword) {
		SDL_SetRenderDrawColor(renderer, 245, 120, 80, 255);
		SDL_RenderDebugText(renderer, 70.0f, 28.0f, "UNARMED");
	}
	if (!c->fighters[PLAYER_TWO].has_sword) {
		SDL_SetRenderDrawColor(renderer, 245, 120, 80, 255);
		SDL_RenderDebugText(renderer, (float)WINDOW_W - 128.0f, 28.0f, "UNARMED");
	}
	if (c->fighters[PLAYER_ONE].downed) {
		SDL_SetRenderDrawColor(renderer, 255, 80, 80, 255);
		SDL_RenderDebugText(renderer, 10.0f, 40.0f, "DOWN");
	}
	if (c->fighters[PLAYER_TWO].downed) {
		SDL_SetRenderDrawColor(renderer, 255, 80, 80, 255);
		SDL_RenderDebugText(renderer, (float)WINDOW_W - 55.0f, 40.0f, "DOWN");
	}

	/* ── Death popups ── */
	if (c->death_popup_frames[PLAYER_ONE] > 0) {
		if ((c->death_popup_frames[PLAYER_ONE] % 20) > 6) {
			SDL_SetRenderDrawColor(renderer, 240, 200, 0, 255);
			SDL_FRect popup = {8.0f, (float)HUD_HEIGHT_PX + 8.0f, 110.0f, 22.0f};
			SDL_RenderFillRect(renderer, &popup);
			SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
			SDL_RenderDebugText(renderer, 14.0f,
			                    (float)HUD_HEIGHT_PX + 14.0f, "<- GO LEFT");
		}
	}

	if (c->death_popup_frames[PLAYER_TWO] > 0) {
		if ((c->death_popup_frames[PLAYER_TWO] % 20) > 6) {
			SDL_SetRenderDrawColor(renderer, 240, 200, 0, 255);
			SDL_FRect popup = {(float)WINDOW_W - 118.0f, (float)HUD_HEIGHT_PX + 8.0f, 110.0f, 22.0f};
			SDL_RenderFillRect(renderer, &popup);
			SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
			SDL_RenderDebugText(renderer, (float)WINDOW_W - 104.0f,
			                    (float)HUD_HEIGHT_PX + 14.0f, "GO RIGHT ->");
		}
	}

	return GAME_OK;
}