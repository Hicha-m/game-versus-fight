#include <stdio.h>
#include <string.h>

#include "render_internal.h"
#include "core/constants.h"
#include "game/game.h"

#define VICTORY_CENTER_X (WINDOW_WIDTH / 2)
#define VICTORY_CENTER_Y (WINDOW_HEIGHT / 2)
#define TITLE_Y 80.0f
#define STATS_Y_START 180.0f
#define STATS_LINE_HEIGHT 40.0f
#define BUTTON_Y (WINDOW_HEIGHT - 100.0f)
#define BUTTON_WIDTH 150.0f
#define BUTTON_HEIGHT 50.0f

/* Helper: Draw text centered */
static void render_victory_text_centered(
    SDL_Renderer* renderer,
    f32 x,
    f32 y,
    const char* text,
    u8 r,
    u8 g,
    u8 b
)
{
    if (!renderer || !text) {
        return;
    }
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderDebugText(renderer, x - 50.0f, y, text);
}

/* Helper: Draw rectangle with border */
static void render_victory_button(
    SDL_Renderer* renderer,
    f32 x,
    f32 y,
    f32 w,
    f32 h,
    const char* text,
    bool highlighted
)
{
    SDL_FRect rect = {x - w / 2, y, w, h};

    /* Draw background */
    if (highlighted) {
        SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    }
    SDL_RenderFillRect(renderer, &rect);

    /* Draw border */
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderRect(renderer, &rect);

    /* Draw text */
    render_victory_text_centered(renderer, x, y + 15.0f, text, 240, 240, 240);
}

void render_victory_screen(SDL_Renderer* renderer, const Game* game, i32 winner_index)
{
    char buffer[256];
    f32 y_offset;
    const MatchStats* stats;

    if (!renderer || !game || winner_index < 0 || winner_index >= MAX_PLAYERS) {
        return;
    }

    stats = &game->match_stats;

    /* Clear background */
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    /* Draw title */
    if (winner_index == 0) {
        render_victory_text_centered(renderer, VICTORY_CENTER_X, TITLE_Y, "PLAYER 1 WINS!", 0, 255, 100);
    } else {
        render_victory_text_centered(renderer, VICTORY_CENTER_X, TITLE_Y, "PLAYER 2 WINS!", 255, 100, 0);
    }

    /* Draw separator line */
    y_offset = TITLE_Y + 40.0f;
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_FRect line_rect = {100.0f, y_offset, WINDOW_WIDTH - 200.0f, 2.0f};
    SDL_RenderFillRect(renderer, &line_rect);

    /* Draw stats */
    y_offset = STATS_Y_START;

    snprintf(buffer, sizeof(buffer), "Rounds Won: %d", stats->wins[winner_index]);
    render_victory_text_centered(renderer, VICTORY_CENTER_X, y_offset, buffer, 100, 200, 255);
    y_offset += STATS_LINE_HEIGHT;

    snprintf(buffer, sizeof(buffer), "Total Kills: %d", stats->kills[winner_index]);
    render_victory_text_centered(renderer, VICTORY_CENTER_X, y_offset, buffer, 100, 200, 255);
    y_offset += STATS_LINE_HEIGHT;

    snprintf(buffer, sizeof(buffer), "Opponent Disarms: %d", stats->disarms[winner_index]);
    render_victory_text_centered(renderer, VICTORY_CENTER_X, y_offset, buffer, 100, 200, 255);
    y_offset += STATS_LINE_HEIGHT;

    snprintf(buffer, sizeof(buffer), "Rooms Conquered: %d", stats->room_progress);
    render_victory_text_centered(renderer, VICTORY_CENTER_X, y_offset, buffer, 100, 200, 255);

    /* Draw continue button */
    render_victory_button(
        renderer,
        VICTORY_CENTER_X,
        BUTTON_Y,
        BUTTON_WIDTH,
        BUTTON_HEIGHT,
        "CONTINUE",
        true
    );
}
