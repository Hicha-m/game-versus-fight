#include "combat.h"

GameError combat_init(CombatState *combat, const Arena *arena, const GameConfig *config) {
	(void)arena;
	(void)config;
	if (combat != NULL) {
		combat->duel_active = true;
		combat->round_time_frames = 0;
		combat->score[0] = 0;
		combat->score[1] = 0;
	}
	return GAME_OK;
}

GameError combat_apply_action(CombatState *combat, PlayerId actor, const Action *action) {
	(void)combat;
	(void)actor;
	(void)action;
	return GAME_OK;
}

GameError combat_step(CombatState *combat, const Arena *arena, uint32_t fixed_dt_ms) {
	(void)arena;
	(void)fixed_dt_ms;
	if (combat != NULL) {
		combat->round_time_frames++;
	}
	return GAME_OK;
}

GameError combat_reset_round(CombatState *combat, const Arena *arena, const GameConfig *config) {
	(void)arena;
	(void)config;
	if (combat != NULL) {
		combat->round_time_frames = 0;
		combat->duel_active = true;
	}
	return GAME_OK;
}

bool combat_is_round_over(const CombatState *combat, PlayerId *out_winner) {
	(void)combat;
	if (out_winner != NULL) {
		*out_winner = PLAYER_ONE;
	}
	return false;
}
