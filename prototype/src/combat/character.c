#include "combat/character.h"
#include "combat/physics.h"
#include "constants.h"
#include "types.h"

/**
 * Initialize a fighter character
 * 
 * Responsible for setting initial state:
 * - Position (spawn point)
 * - Health/alive status
 * - Sword height
 * - Physics state (velocity, grounded)
 */
void character_init(FighterState *fighter, float spawn_x, float spawn_y) {
	if (fighter == NULL) return;
	
	fighter->position = (Vec2f){.x = spawn_x, .y = spawn_y};
	fighter->velocity = (Vec2f){.x = 0.0f, .y = 0.0f};
	fighter->facing = FACING_RIGHT;
	fighter->sword_height = SWORD_HEIGHT_MID;
	fighter->has_sword = true;
	fighter->sword_ready = true;
	fighter->sword_recover_frames = 0;
	fighter->alive = true;
	fighter->downed = false;
	fighter->downed_frames = 0;
	fighter->rolling = false;
	fighter->roll_frames = 0;
	fighter->cartwheeling = false;
	fighter->cartwheel_frames = 0;
	fighter->momentum_frames = 0;
	fighter->successful_parries = 0;
	fighter->attack_was_held = false;
	fighter->grounded = false;
	fighter->drop_through_frames = 0;
	fighter->invulnerability_frames = 0;
	fighter->stun_frames = 0;
}

/**
 * Update character position and velocity
 * Integrates physics: gravity, movement, collision
 */
void character_update(FighterState *fighter, const PlayerCommand *input, uint32_t dt_ms) {
	if (fighter == NULL || !fighter->alive) return;
	
	float dt_sec = dt_ms / 1000.0f;
	PlayerCommand neutral_input = {0};
	if (input == NULL) {
		input = &neutral_input;
	}
	if (fighter->stun_frames > 0) {
		neutral_input.target_height = fighter->sword_height;
		input = &neutral_input;
	}

	if (fighter->downed) {
		fighter->velocity.x = 0.0f;
		fighter->velocity.y = 0.0f;
		if (fighter->downed_frames > 0) {
			fighter->downed_frames--;
		}
		if (fighter->downed_frames == 0) {
			fighter->downed = false;
		}
		return;
	}

	if (fighter->drop_through_frames > 0) {
		fighter->drop_through_frames--;
	}
	
	// Apply gravity
	fighter->velocity.y += GRAVITY * dt_sec;
	
	// Horizontal movement with acceleration/deceleration
	int8_t move_input = input->move_axis;  // -1 (left), 0 (none), 1 (right)
	bool attack_pressed_this_frame = input->attack && !fighter->attack_was_held;

	if (fighter->grounded && move_input != 0 && input->crouch) {
		if (attack_pressed_this_frame) {
			fighter->cartwheeling = true;
			fighter->cartwheel_frames = CARTWHEEL_FRAMES;
			fighter->velocity.x = move_input * CARTWHEEL_SPEED;
		} else {
			fighter->rolling = true;
			fighter->roll_frames = ROLL_FRAMES;
			fighter->velocity.x = move_input * ROLL_SPEED;
		}
	}

	/* Keep crouch movement, but do not force platform drop-through every frame. */
	if (fighter->grounded && input->crouch && input->parry) {
		fighter->drop_through_frames = 8;
	}

	if (fighter->rolling) {
		if (fighter->roll_frames > 0) {
			fighter->roll_frames--;
		} else {
			fighter->rolling = false;
		}
	}
	if (fighter->cartwheeling) {
		if (fighter->cartwheel_frames > 0) {
			fighter->cartwheel_frames--;
		} else {
			fighter->cartwheeling = false;
		}
	}

	if (input->crouch && move_input != 0 && fighter->grounded && !fighter->rolling && !fighter->cartwheeling) {
		fighter->velocity.x = move_input * CRAWL_SPEED;
	}
	
	if (move_input != 0 && !fighter->rolling && !fighter->cartwheeling && !(input->crouch && fighter->grounded)) {
		float target_vx = move_input * MAX_HORIZONTAL_VEL;
		// Smooth acceleration towards target
		float accel = ACCELERATION * dt_sec;
		if (fighter->velocity.x < target_vx) {
			fighter->velocity.x = (fighter->velocity.x + accel > target_vx) 
				? target_vx 
				: fighter->velocity.x + accel;
		} else if (fighter->velocity.x > target_vx) {
			fighter->velocity.x = (fighter->velocity.x - accel < target_vx) 
				? target_vx 
				: fighter->velocity.x - accel;
		}
		
		// Update facing direction
		if (move_input > 0) fighter->facing = FACING_RIGHT;
		else if (move_input < 0) fighter->facing = FACING_LEFT;
	} else if (!fighter->rolling && !fighter->cartwheeling) {
		// Deceleration
		float decel = DECELERATION * dt_sec;
		if (fighter->velocity.x > 0) {
			fighter->velocity.x = (fighter->velocity.x - decel < 0) 
				? 0 
				: fighter->velocity.x - decel;
		} else if (fighter->velocity.x < 0) {
			fighter->velocity.x = (fighter->velocity.x + decel > 0) 
				? 0 
				: fighter->velocity.x + decel;
		}
	}
	
	// Apply jumping
	if (input->jump && fighter->grounded) {
		fighter->velocity.y = -JUMP_FORCE;  // Negative = up
		fighter->grounded = false;
	}
	
	// Update position based on velocity
	fighter->position.x += fighter->velocity.x * dt_sec;
	fighter->position.y += fighter->velocity.y * dt_sec;
	
	// Update sword height
	if (input->target_height != fighter->sword_height) {
		fighter->sword_height = input->target_height;
	}

	if (fighter->has_sword && input->attack) {
		float thrust_speed = MAX_HORIZONTAL_VEL * 1.2f;
		fighter->velocity.x = (fighter->facing == FACING_RIGHT) ? thrust_speed : -thrust_speed;
	}

	if (!fighter->has_sword && fighter->sword_recover_frames > 0) {
		fighter->sword_recover_frames--;
		if (fighter->sword_recover_frames == 0) {
			fighter->has_sword = true;
		}
	}
	
	// Decay invulnerability and stun
	if (fighter->invulnerability_frames > 0) {
		fighter->invulnerability_frames--;
	}
	if (fighter->stun_frames > 0) {
		fighter->stun_frames--;
	}

	fighter->attack_was_held = input->attack;
}

/**
 * Apply knockback from hit
 */
void character_apply_knockback(FighterState *fighter, float force_x, float force_y) {
	if (fighter == NULL || !fighter->alive) return;
	
	fighter->velocity.x += force_x;
	fighter->velocity.y += force_y;
	fighter->invulnerability_frames = INVULNERABILITY_FRAMES;
}

/**
 * Kill a fighter (death handling)
 */
void character_kill(FighterState *fighter) {
	if (fighter == NULL) return;
	
	fighter->alive = false;
	fighter->velocity = (Vec2f){.x = 0.0f, .y = 0.0f};
	fighter->has_sword = false;
	fighter->sword_ready = false;
	fighter->sword_recover_frames = 0;
	fighter->downed = false;
	fighter->downed_frames = 0;
	fighter->rolling = false;
	fighter->roll_frames = 0;
	fighter->cartwheeling = false;
	fighter->cartwheel_frames = 0;
	fighter->momentum_frames = 0;  // Reset momentum on death
	fighter->successful_parries = 0;  // Reset sword evolution on death
	fighter->attack_was_held = false;
}