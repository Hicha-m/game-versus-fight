#ifndef COMBAT_H
#define COMBAT_H


#include "core/types.h"
#include "core/constants.h"
#include "game/arena/arena.h"

typedef enum SwordLine {
    SWORD_LINE_HIGH = 0,
    SWORD_LINE_MID,
    SWORD_LINE_LOW
} SwordLine;

typedef enum FighterAction {
    FIGHTER_ACTION_IDLE = 0,
    FIGHTER_ACTION_RUN,
    FIGHTER_ACTION_JUMP,
    FIGHTER_ACTION_FALL,
    FIGHTER_ACTION_ATTACK_HIGH,
    FIGHTER_ACTION_ATTACK_MID,
    FIGHTER_ACTION_ATTACK_LOW,
    FIGHTER_ACTION_THROW,
    FIGHTER_ACTION_ROLL,
    FIGHTER_ACTION_STUN,
    FIGHTER_ACTION_DEAD
} FighterAction;

typedef enum ControllerType {
    CONTROLLER_HUMAN = 0,
    CONTROLLER_AI
} ControllerType;

/* Intention gameplay d'un joueur pour le tick courant */
typedef struct PlayerCommand {
    bool right_pressed;
    bool left_pressed;
    bool up_pressed;
    bool down_pressed;
    bool jump_pressed;
    bool roll_pressed;
    bool thrust_pressed;
    bool throw_pressed;
    SwordLine target_sword_line;
} PlayerCommand;

typedef struct FighterStats {
    f32 move_speed;
    f32 jump_speed;
    f32 gravity;
    f32 max_fall_speed;
} FighterStats;


typedef struct FighterController {
    ControllerType type;
} FighterController;

typedef struct FighterState {
    /* --- Physique --- */
    Vec2 pos;
    Vec2 vel;
    bool grounded;
    bool facing_right;

    /* --- Épée / posture --- */
    bool has_sword;
    SwordLine sword_line;

    /* --- Action / animation --- */
    FighterAction action;
    f32 action_time;
    f32 action_duration;

    /* --- Combat --- */
    bool is_attacking;
    bool is_parrying;
    bool is_stunned;

    /* --- Timers --- */
    f32 stun_timer;
    f32 invincibility_timer;
    f32 attack_cooldown;
    f32 coyote_time;
    f32 move_hold_time;
    f32 thrust_buffer_timer;

    /* --- Gameplay --- */
    bool alive;
} FighterState;

typedef struct Fighter {
    FighterState state;
    FighterController controller;
    FighterStats stats;
} Fighter;

typedef struct ThrownSword {
    bool active;
    i32 owner_index;
    SwordLine line;
    Vec2 pos;
    Vec2 vel;
    f32 lifetime;
    f32 rotation_deg;
} ThrownSword;

typedef struct CombatState {
    Fighter fighters[MAX_PLAYERS];
    f32 respawn_timers[MAX_PLAYERS];
    ThrownSword thrown_sword;

    bool kill_happened;
    i32 kill_attacker_index;
    i32 kill_victim_index;
    bool kill_counts_for_progress;
    f32 kill_attacker_timer;   /* Time since kill, for camera priority window */

    bool round_over;
    i32 winner_index;
} CombatState;

/* Lifecycle */
bool combat_init(CombatState* combat);
void combat_shutdown(CombatState* combat);

/* Round */
void combat_reset_round(CombatState* combat, const Arena* arena);

/* Tick gameplay */
void combat_step(
    CombatState* combat,
    Arena* arena,
    const PlayerCommand* p1_cmd,
    const PlayerCommand* p2_cmd,
    f32 dt
);

#endif