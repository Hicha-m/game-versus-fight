#ifndef CONSTANTS_H
#define CONSTANTS_H

/* =========================
   Fenêtre / rendu
   ========================= */

#define WINDOW_WIDTH            1280
#define WINDOW_HEIGHT           720
#define WINDOW_TITLE            "Blade Rush"

#define TARGET_TPS             60
#define FIXED_DT               (1.0f / 60.0f)

#define MAX_PLAYERS      2
#define ROOM_COUNT       5

/* =========================
   Monde / arène
   ========================= */

#define ROOM_MAX_WIDTH_TILES    96
#define ROOM_HEIGHT_TILES       22
#define TILE_SIZE               32

#define ROOM_DEFAULT_WIDTH_TILES 40

/* =========================
   Joueurs / combat
   ========================= */

#define PLAYER_WIDTH           24.0f
#define PLAYER_HEIGHT          48.0f

#define PLAYER_MOVE_SPEED      260.0f
#define PLAYER_JUMP_SPEED     -620.0f
#define PLAYER_GRAVITY        1700.0f
#define PLAYER_MAX_FALL_SPEED 1200.0f

#define SWORD_THRUST_RANGE_X    40.0f
#define SWORD_HIT_RANGE_Y       22.0f

#define DEFAULT_ATTACK_DURATION   0.12f
#define DEFAULT_ATTACK_COOLDOWN   0.20f
#define DEFAULT_ROLL_DURATION     0.18f
#define DEFAULT_STUN_DURATION     0.35f
#define DEFAULT_COYOTE_TIME       0.08f

/* IA / minimax */
#define AI_MAX_DEPTH            8
#define AI_MAX_ACTIONS          8


#endif