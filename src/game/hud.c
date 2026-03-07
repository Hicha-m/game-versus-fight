#include "ui.h"

GameError ui_init(void) {
	return GAME_OK;
}

GameError ui_render_hud(const GameState *state) {
	(void)state;
	return GAME_OK;
}

GameError ui_shutdown(void) {
	return GAME_OK;
}
