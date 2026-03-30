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

#define PLAYER_MOVE_SPEED      320.0f
#define PLAYER_JUMP_SPEED     -620.0f
#define PLAYER_GRAVITY        1700.0f
#define PLAYER_MAX_FALL_SPEED 1200.0f

/* Gameplay hit reach (collision/query), not the rendered sword pixel length. */
#define SWORD_BODY_REACH_X      24.0f
#define SWORD_THRUST_RANGE_X    40.0f
#define SWORD_HIT_RANGE_Y       22.0f

#define DEFAULT_ATTACK_DURATION   0.12f
#define DEFAULT_ATTACK_COOLDOWN   0.10f
#define DEFAULT_ROLL_DURATION     0.18f
#define DEFAULT_STUN_DURATION     0.35f
#define DEFAULT_COYOTE_TIME       0.08f
#define DEFAULT_RESPAWN_DELAY     1.50f

#define RUN_HOLSTER_DELAY              0.18f
#define WALK_STEP_SPEED_MULTIPLIER     0.42f
#define WALK_STEP_MOMENTUM_BLEND       0.35f
#define WALK_STEP_MAX_BOOST_MULTIPLIER 1.35f
#define THRUST_INPUT_BUFFER_TIME       1.12f
#define THRUST_LUNGE_SPEED_MULTIPLIER  0.30f
#define THRUST_LUNGE_MAX_MULTIPLIER    1.20f
#define THRUST_MOMENTUM_START_MULTIPLIER 0.80f
#define THRUST_MOMENTUM_DAMP_MULTIPLIER  0.90f

#define ATTACK_DURATION_MULTIPLIER      1.40f
#define ATTACK_RANGE_ARMED_MULTIPLIER   1.45f
#define ATTACK_RANGE_UNARMED_MULTIPLIER 0.95f
#define ATTACK_HEIGHT_ARMED_MULTIPLIER  1.45f
#define ATTACK_HEIGHT_UNARMED_MULTIPLIER 1.65f

#define PARRY_STUN_MULTIPLIER           0.85f
#define PARRY_KNOCKBACK_MULTIPLIER      1.35f
#define PARRY_VERTICAL_RECOIL           -220.0f

#define THROW_ACTION_DURATION            0.18f
#define THROW_PROJECTILE_SPEED_X         520.0f
#define THROW_PROJECTILE_SPEED_Y_HIGH   -180.0f
#define THROW_PROJECTILE_SPEED_Y_MID     -40.0f
#define THROW_PROJECTILE_SPEED_Y_LOW     140.0f
#define THROW_PROJECTILE_GRAVITY         900.0f
#define THROW_PROJECTILE_LIFETIME        1.25f
#define THROW_PROJECTILE_SPIN_DPS        720.0f
#define THROW_PROJECTILE_WIDTH           20.0f
#define THROW_PROJECTILE_HEIGHT           8.0f

#define THRUST_RENDER_BONUS_MIN         18.0f
#define THRUST_RENDER_BONUS_PEAK        36.0f
#define SWORD_RENDER_BASE_WIDTH         20.0f
#define SWORD_RENDER_BASE_HEIGHT        5.0f
#define SWORD_RENDER_THRUST_HEIGHT      6.0f
#define THRUST_RENDER_EXTEND_FRACTION   0.30f

/* IA / minimax */
#define AI_MAX_DEPTH            8
#define AI_MAX_ACTIONS          8

/* Camera follow rules - killer priority & distance limit */
#define CAMERA_KILL_PRIORITY_WINDOW     2.0f    /* Time (sec) to prioritize killer after kill */
#define CAMERA_MAX_DISTANCE             (WINDOW_WIDTH * 1.2f)  /* Max distance from camera before enemy dies */


#endif