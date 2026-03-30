#include <stdio.h>

#include "render_internal.h"
#include "engine/input.h"
#include "core/constants.h"

#define MENU_ITEM_HEIGHT 50
#define MENU_ITEM_Y_START 200
#define MENU_CENTER_X (WINDOW_WIDTH / 2)

/* ========================================
   Helper Functions - Text Rendering
   ======================================== */

static void render_menu_text(SDL_Renderer* renderer, f32 x, f32 y, const char* text)
{
    if (!renderer || !text) {
        return;
    }
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
    SDL_RenderDebugText(renderer, x, y, text);
}

static void render_menu_option(
    SDL_Renderer* renderer,
    f32 x,
    f32 y,
    f32 w,
    f32 h,
    const char* text,
    bool selected
)
{
    SDL_FRect rect = {x, y, w, h};

    /* Draw background */
    if (selected) {
        SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    }
    SDL_RenderFillRect(renderer, &rect);

    /* Draw text */
    render_menu_text(renderer, x + 10.0f, y + 15.0f, text);
}

/* ========================================
   Menu State Renderers
   ======================================== */

static void render_menu_main(SDL_Renderer* renderer, const MenuContext* menu)
{
    const char* labels[] = {
        "PLAY",
        "OPTIONS",
        "QUIT"
    };
    i32 option_count = 3;
    i32 selected = menu_get_selected_option(menu);

    for (i32 i = 0; i < option_count; i++) {
        render_menu_option(
            renderer,
            MENU_CENTER_X - 100.0f,
            MENU_ITEM_Y_START + i * MENU_ITEM_HEIGHT,
            200.0f,
            40.0f,
            labels[i],
            i == selected
        );
    }
}

static void render_menu_play_mode(SDL_Renderer* renderer, const MenuContext* menu)
{
    const char* labels[] = {
        "SOLO",
        "MULTIPLAYER"
    };
    i32 option_count = 2;
    i32 selected = menu_get_selected_option(menu);

    render_menu_text(renderer, MENU_CENTER_X - 150.0f, 100.0f, "SELECT MODE");

    for (i32 i = 0; i < option_count; i++) {
        render_menu_option(
            renderer,
            MENU_CENTER_X - 100.0f,
            MENU_ITEM_Y_START + i * MENU_ITEM_HEIGHT,
            200.0f,
            40.0f,
            labels[i],
            i == selected
        );
    }
}

static void render_menu_difficulty(SDL_Renderer* renderer, const MenuContext* menu)
{
    const char* labels[] = {
        "EASY",
        "MEDIUM",
        "HARD",
        "EXPERT"
    };
    i32 option_count = 4;
    i32 selected = menu_get_selected_option(menu);

    render_menu_text(renderer, MENU_CENTER_X - 200.0f, 100.0f, "SELECT DIFFICULTY");

    for (i32 i = 0; i < option_count; i++) {
        render_menu_option(
            renderer,
            MENU_CENTER_X - 100.0f,
            MENU_ITEM_Y_START + i * MENU_ITEM_HEIGHT,
            200.0f,
            40.0f,
            labels[i],
            i == selected
        );
    }
}

static void render_menu_options(SDL_Renderer* renderer, const MenuContext* menu)
{
    const char* labels[] = {
        "AUDIO",
        "KEYBINDS"
    };
    i32 option_count = 2;
    i32 selected = menu_get_selected_option(menu);

    render_menu_text(renderer, MENU_CENTER_X - 150.0f, 100.0f, "OPTIONS");

    for (i32 i = 0; i < option_count; i++) {
        render_menu_option(
            renderer,
            MENU_CENTER_X - 100.0f,
            MENU_ITEM_Y_START + i * MENU_ITEM_HEIGHT,
            200.0f,
            40.0f,
            labels[i],
            i == selected
        );
    }
}

static void render_menu_options_audio(SDL_Renderer* renderer, const MenuContext* menu)
{
    const char* labels[] = {
        "MASTER",
        "MUSIC",
        "SFX"
    };
    const GameConfig* config = menu_get_config(menu);
    f32* volumes[] = {
        (f32*)&config->master_volume,
        (f32*)&config->music_volume,
        (f32*)&config->sfx_volume
    };
    i32 option_count = 3;
    i32 selected = menu_get_selected_option(menu);
    char buffer[64];

    render_menu_text(renderer, MENU_CENTER_X - 200.0f, 100.0f, "AUDIO SETTINGS");

    for (i32 i = 0; i < option_count; i++) {
        snprintf(buffer, sizeof(buffer), "%s: %d%%", labels[i], (i32)(*volumes[i] * 100.0f));

        render_menu_option(
            renderer,
            MENU_CENTER_X - 150.0f,
            MENU_ITEM_Y_START + i * MENU_ITEM_HEIGHT,
            300.0f,
            40.0f,
            buffer,
            i == selected
        );
    }
}

static void render_menu_options_keybinds(SDL_Renderer* renderer, const MenuContext* menu)
{
    const char* action_names[] = {
        "RIGHT",
        "LEFT",
        "UP",
        "DOWN",
        "JUMP",
        "THRUST",
        "THROW"
    };
    i32 action_count = ACTION_COUNT;
    i32 selected = menu_get_selected_option(menu);

    render_menu_text(renderer, MENU_CENTER_X - 200.0f, 100.0f, "KEYBINDS");

    for (i32 i = 0; i < action_count; i++) {
        render_menu_option(
            renderer,
            MENU_CENTER_X - 150.0f,
            MENU_ITEM_Y_START + i * MENU_ITEM_HEIGHT,
            300.0f,
            40.0f,
            action_names[i],
            i == selected
        );
    }
}

static void render_menu_pause(SDL_Renderer* renderer, const MenuContext* menu)
{
    const char* labels[] = {
        "RESUME",
        "BACK TO MENU"
    };
    i32 option_count = 2;
    i32 selected = menu_get_selected_option(menu);

    render_menu_text(renderer, MENU_CENTER_X - 150.0f, 100.0f, "PAUSED");

    for (i32 i = 0; i < option_count; i++) {
        render_menu_option(
            renderer,
            MENU_CENTER_X - 100.0f,
            MENU_ITEM_Y_START + i * MENU_ITEM_HEIGHT,
            200.0f,
            40.0f,
            labels[i],
            i == selected
        );
    }
}

/* NEW: Arena mode selection (Default, Procedural, Corridor) */
static void render_menu_arena_mode(SDL_Renderer* renderer, const MenuContext* menu)
{
    const char* labels[] = {
        "DEFAULT",
        "PROCEDURAL",
        "CORRIDOR"
    };
    i32 option_count = 3;
    i32 selected = menu_get_selected_option(menu);

    render_menu_text(renderer, MENU_CENTER_X - 200.0f, 80.0f, "ARENA GENERATION MODE");

    for (i32 i = 0; i < option_count; i++) {
        render_menu_option(
            renderer,
            MENU_CENTER_X - 100.0f,
            MENU_ITEM_Y_START + i * MENU_ITEM_HEIGHT,
            200.0f,
            40.0f,
            labels[i],
            i == selected
        );
    }
}

/* NEW: Arena difficulty slider (0-10) */
static void render_menu_arena_difficulty(SDL_Renderer* renderer, const MenuContext* menu)
{
    char difficulty_text[64];
    i32 difficulty = menu->arena_options.difficulty;

    render_menu_text(renderer, MENU_CENTER_X - 250.0f, 100.0f, "ARENA DIFFICULTY");

    snprintf(difficulty_text, sizeof(difficulty_text), "DIFFICULTY: %d / 10", difficulty);
    render_menu_text(renderer, MENU_CENTER_X - 150.0f, 180.0f, difficulty_text);

    /* Simple visual bar */
    render_menu_text(renderer, MENU_CENTER_X - 150.0f, 250.0f, "LEFT/RIGHT: Adjust | SPACE/J: Next");
}

/* NEW: Arena seed display */
static void render_menu_arena_seed(SDL_Renderer* renderer, const MenuContext* menu)
{
    char seed_text[64];

    render_menu_text(renderer, MENU_CENTER_X - 200.0f, 100.0f, "ARENA SEED");

    snprintf(seed_text, sizeof(seed_text), "SEED: %u", menu->arena_options.seed);
    render_menu_text(renderer, MENU_CENTER_X - 100.0f, 180.0f, seed_text);

    render_menu_text(renderer, MENU_CENTER_X - 200.0f, 250.0f, "AUTO-GENERATED FOR RANDOMIZATION");
    render_menu_text(renderer, MENU_CENTER_X - 150.0f, 300.0f, "SPACE/J: Start | K: Back");
}

/* ========================================
   Public API
   ======================================== */

void render_menu(SDL_Renderer* renderer, const MenuContext* menu)
{
    MenuState state;
    const char* instructions = NULL;

    if (!renderer || !menu) {
        return;
    }

    state = menu_get_state(menu);

    switch (state) {
        case MENU_STATE_MAIN:
            render_menu_main(renderer, menu);
            instructions = "UP/DOWN: Move | SPACE/J: Select | K: Back | ESC: Quit";
            break;
        case MENU_STATE_PLAY_MODE:
            render_menu_play_mode(renderer, menu);
            instructions = "UP/DOWN: Choose | SPACE/J: Select | K: Back";
            break;
        case MENU_STATE_DIFFICULTY:
            render_menu_difficulty(renderer, menu);
            instructions = "UP/DOWN: Choose | SPACE/J: Select | K: Back";
            break;
        case MENU_STATE_OPTIONS:
            render_menu_options(renderer, menu);
            instructions = "UP/DOWN: Move | SPACE/J: Select | K: Back";
            break;
        case MENU_STATE_OPTIONS_AUDIO:
            render_menu_options_audio(renderer, menu);
            instructions = "UP/DOWN: Choose | LEFT/RIGHT: Adjust | K: Back";
            break;
        case MENU_STATE_OPTIONS_KEYBINDS:
            render_menu_options_keybinds(renderer, menu);
            instructions = "UP/DOWN: Move | K: Back";
            break;
        case MENU_STATE_PAUSE:
            render_menu_pause(renderer, menu);
            instructions = "UP/DOWN: Choose | SPACE/J: Select | ESC: Resume";
            break;
        /* NEW: Arena configuration states */
        case MENU_STATE_ARENA_MODE:
            render_menu_arena_mode(renderer, menu);
            instructions = "LEFT/RIGHT: Choose | SPACE/J: Select | K: Back";
            break;
        case MENU_STATE_ARENA_DIFFICULTY:
            render_menu_arena_difficulty(renderer, menu);
            instructions = "LEFT/RIGHT: Adjust | SPACE/J: Next | K: Back";
            break;
        case MENU_STATE_ARENA_SEED:
            render_menu_arena_seed(renderer, menu);
            instructions = "SPACE/J: Start | K: Back";
            break;
        default:
            render_menu_text(renderer, 100.0f, 100.0f, "Unknown menu state");
            break;
    }

    /* Display instructions at the bottom */
    if (instructions) {
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderDebugText(renderer, 50.0f, WINDOW_HEIGHT - 40.0f, instructions);
    }
}
