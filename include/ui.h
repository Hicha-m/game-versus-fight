#ifndef UI_H
#define UI_H

#include "types.h"

GameError ui_init(void);
GameError ui_render_menu(const GameState *state);
GameError ui_render_hud(const GameState *state);
GameError ui_shutdown(void);

#endif
