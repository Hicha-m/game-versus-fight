#include <stdio.h>
#include <string.h>
#include <math.h>

#include "render_internal.h"
#include "assets_private.h"
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

static void render_victory_text_centered(SDL_Renderer *renderer, f32 x, f32 y, const char *text,
                                         u8 r, u8 g, u8 b)
{
    if (!renderer || !text)
    {
        return;
    }
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderDebugText(renderer, x - 50.0f, y, text);
}

static void render_victory_button(SDL_Renderer *renderer, f32 x, f32 y, f32 w, f32 h,
                                  const char *text, bool highlighted)
{
    SDL_FRect rect = {x - w / 2, y, w, h};

    if (highlighted)
    {
        SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    }
    SDL_RenderFillRect(renderer, &rect);

    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderRect(renderer, &rect);

    render_victory_text_centered(renderer, x, y + 15.0f, text, 240, 240, 240);
}

void render_victory_screen(SDL_Renderer *renderer, const Game *game, i32 winner_index)
{
    char buffer[256];
    f32 y_offset;
    const MatchStats *stats;

    if (!renderer || !game || winner_index < 0 || winner_index >= MAX_PLAYERS)
    {
        return;
    }

    if (game->victory_time < 4.0f)
    {
        f32 t = game->victory_time;
        SDL_SetRenderDrawColor(renderer, 57, 28, 50, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 155, 126, 43, 255);
        SDL_FRect floor = {0, 580, WINDOW_WIDTH, 140};
        SDL_RenderFillRect(renderer, &floor);
        /* An original horned serpent silhouette celebrates the terminal run. */
        for (i32 i = 0; i < 22; i++)
        {
            f32 head = WINDOW_WIDTH + 180 - fminf(t, 2.4f) * 400 + fmaxf(0, t - 2.4f) * 1000;
            f32 x = head + i * 44, y = 475 + sinf(t * 4 + i * 0.3f) * 18;
            SDL_FRect segment = {x, y, 52, 75};
            SDL_SetRenderDrawColor(renderer, 190 + (i % 2) * 20, 84, 136, 255);
            for (i32 band = 0; band < 76; band += 4)
            {
                f32 relative = (band - 37.5f) / 37.5f;
                f32 half = 26 * sqrtf(fmaxf(0, 1 - relative * relative));
                SDL_FRect curve = {x + 26 - half, y + band, half * 2, 4};
                SDL_RenderFillRect(renderer, &curve);
            }
            SDL_SetRenderDrawColor(renderer, 107, 44, 91, 255);
            (void)segment;
            if (i == 0)
            {
                SDL_FRect mouth = {x - 25, y + 20, 35, 45};
                SDL_SetRenderDrawColor(renderer, 43, 17, 34, 255);
                SDL_RenderFillRect(renderer, &mouth);
                SDL_FRect horn = {x + 12, y - 30, 9, 35};
                SDL_SetRenderDrawColor(renderer, 240, 224, 172, 255);
                SDL_RenderFillRect(renderer, &horn);
                SDL_FRect eye = {x + 5, y + 10, 7, 7};
                SDL_RenderFillRect(renderer, &eye);
            }
        }
        for (i32 i = 0; i < 45; i++)
        {
            SDL_SetRenderDrawColor(renderer, 160 + i % 3 * 25, 85 + i % 4 * 14, 65, 255);
            SDL_FRect spectator = {(f32)(i * 30), 540 + sinf(t * 10 + i) * 5, 9, 30};
            SDL_RenderFillRect(renderer, &spectator);
        }
        if (t < 1.8f)
        {
            SDL_SetRenderDrawColor(renderer, 255, winner_index == 0 ? 235 : 142,
                                   winner_index == 0 ? 0 : 43, 255);
            SDL_FRect runner = {WINDOW_WIDTH * 0.5f + t * 55, 484, 48, 96};
            SDL_Texture *figure = assets_animation(0, t, true);
            if (figure)
            {
                SDL_SetTextureColorMod(figure, 255, winner_index == 0 ? 235 : 142,
                                       winner_index == 0 ? 0 : 43);
                SDL_RenderTexture(renderer, figure, NULL, &runner);
            }
            else
                SDL_RenderFillRect(renderer, &runner);
        }
        render_victory_text_centered(renderer, VICTORY_CENTER_X, 150, "THE FINAL RUN", 255, 230,
                                     40);
        return;
    }

    stats = &game->match_stats;

    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    if (winner_index == 0)
    {
        render_victory_text_centered(renderer, VICTORY_CENTER_X, TITLE_Y, "PLAYER 1 WINS!", 0, 255,
                                     100);
    }
    else
    {
        render_victory_text_centered(renderer, VICTORY_CENTER_X, TITLE_Y, "PLAYER 2 WINS!", 255,
                                     100, 0);
    }

    y_offset = TITLE_Y + 40.0f;
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_FRect line_rect = {100.0f, y_offset, WINDOW_WIDTH - 200.0f, 2.0f};
    SDL_RenderFillRect(renderer, &line_rect);

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

    render_victory_button(renderer, VICTORY_CENTER_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT,
                          "CONTINUE", true);
}
