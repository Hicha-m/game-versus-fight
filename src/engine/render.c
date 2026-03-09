#include "render.h"
#include "engine.h"
#include "game/menu.h"
#include "game/hud.h"

void render_begin(void) {
    SDL_Renderer *renderer = engine_renderer_get();
    if (!renderer) return;

    SDL_SetRenderDrawColor(renderer, 20, 24, 32, 255);
    SDL_RenderClear(renderer);
}

void render_end(void) {
    SDL_Renderer *renderer = engine_renderer_get();
    if (!renderer) return;

    SDL_RenderPresent(renderer);
}

GameError render_frame(const GameState *state) {
    if (state == NULL) return GAME_ERROR_INVALID_ARGUMENT;

    render_begin();

    // Dessiner selon l’état du jeu
    switch (state->game_phase) {
        case GAME_PHASE_MAIN_MENU:
        case GAME_PHASE_PRESS_START:
        case GAME_PHASE_MODE_SELECT:
            menu_render(state);
            break;

        case GAME_PHASE_MATCH:
            render_world(state);
            hud_render(state);
            break;

        default:
            break;
    }

    render_end();
    return GAME_OK;
}

GameError render_world(const GameState *state) {
    (void)state;
    // TODO: dessiner l’arène + les fighters
    return GAME_OK;
}
