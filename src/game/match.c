#include "match.h"

#include "arena.h"
#include "combat.h"
#include "combat/character.h"
#include "combat/sword.h"
#include "constants.h"

#include <math.h>
#include <string.h>

#define BLOCK_SPARK_FRAMES 6U
#define BLOCK_RECOIL_SPEED 4.0f

static float spawn_y_for_arena(const Arena *arena) {
	return (float)(arena->height - 1) - PLAYER_HEIGHT;
}

static float segment_start_x(int segment) {
	return (float)(segment * (int)MAP_SEGMENT_TILES);
}

static float segment_end_x(int segment) {
	return segment_start_x(segment) + (float)MAP_SEGMENT_TILES;
}

static void place_fighters_for_map_spawns(GameState *state) {
	Vec2i p1 = {0};
	Vec2i p2 = {0};
	float spawn_y = spawn_y_for_arena(&state->arena);
	arena_find_spawn(&state->arena, PLAYER_ONE, &p1);
	arena_find_spawn(&state->arena, PLAYER_TWO, &p2);

	character_init(&state->combat.fighters[PLAYER_ONE], (float)p1.x, spawn_y);
	character_init(&state->combat.fighters[PLAYER_TWO], (float)p2.x, spawn_y);
	state->combat.fighters[PLAYER_ONE].facing = (p1.x <= p2.x) ? FACING_RIGHT : FACING_LEFT;
	state->combat.fighters[PLAYER_TWO].facing = (p2.x >= p1.x) ? FACING_LEFT : FACING_RIGHT;
	state->combat.respawn_frames[PLAYER_ONE] = 0;
	state->combat.respawn_frames[PLAYER_TWO] = 0;
}

static void place_fighters_for_segment(GameState *state, int segment) {
	float left = segment_start_x(segment);
	float right = segment_end_x(segment);
	float spawn_y = spawn_y_for_arena(&state->arena);

	character_init(&state->combat.fighters[PLAYER_ONE], right - PLAYER_WIDTH - MAP_ENTRY_OFFSET, spawn_y);
	character_init(&state->combat.fighters[PLAYER_TWO], left + MAP_ENTRY_OFFSET, spawn_y);
	state->combat.fighters[PLAYER_ONE].facing = FACING_LEFT;
	state->combat.fighters[PLAYER_TWO].facing = FACING_RIGHT;
	state->combat.respawn_frames[PLAYER_ONE] = 0;
	state->combat.respawn_frames[PLAYER_TWO] = 0;
}

static void respawn_player(GameState *state, PlayerId player) {
	float spawn_x;
	FacingDirection face;
	if (state->combat.segment_goal_mode) {
		float left = segment_start_x(state->active_segment);
		float right = segment_end_x(state->active_segment);
		if (player == PLAYER_ONE) {
			spawn_x = right - PLAYER_WIDTH - MAP_ENTRY_OFFSET;
			face = FACING_LEFT;
		} else {
			spawn_x = left + MAP_ENTRY_OFFSET;
			face = FACING_RIGHT;
		}
	} else {
		Vec2i spawn = {0};
		arena_find_spawn(&state->arena, player, &spawn);
		spawn_x = (float)spawn.x;
		PlayerId other = (player == PLAYER_ONE) ? PLAYER_TWO : PLAYER_ONE;
		if (state->combat.fighters[other].alive) {
			face = (spawn_x <= state->combat.fighters[other].position.x) ? FACING_RIGHT : FACING_LEFT;
		} else {
			face = (player == PLAYER_ONE) ? FACING_RIGHT : FACING_LEFT;
		}
	}

	float spawn_y = spawn_y_for_arena(&state->arena);
	character_init(&state->combat.fighters[player], spawn_x, spawn_y);
	state->combat.fighters[player].facing = face;
	state->combat.respawn_frames[player] = 0;
	state->combat.death_popup_frames[player] = DEATH_POPUP_FRAMES;
}

static void start_transition(GameState *state, uint8_t target_segment) {
	state->pending_segment = target_segment;
	state->transition_frames = MAP_TRANSITION_FRAMES;
	state->combat.respawn_frames[PLAYER_ONE] = 0;
	state->combat.respawn_frames[PLAYER_TWO] = 0;
}

static void complete_transition(GameState *state) {
	state->active_segment = state->pending_segment;
	if (state->combat.segment_goal_mode) {
		place_fighters_for_segment(state, state->active_segment);
	} else {
		place_fighters_for_map_spawns(state);
	}
	state->combat.sword_in_flight[PLAYER_ONE] = false;
	state->combat.sword_in_flight[PLAYER_TWO] = false;
	state->combat.death_popup_frames[PLAYER_ONE] = 0;
	state->combat.death_popup_frames[PLAYER_TWO] = 0;
}

static void resolve_fighter_overlap(GameState *state) {
	FighterState *p1 = &state->combat.fighters[PLAYER_ONE];
	FighterState *p2 = &state->combat.fighters[PLAYER_TWO];
	if (!p1->alive || !p2->alive) {
		return;
	}

	float p1_left = p1->position.x;
	float p1_right = p1->position.x + PLAYER_WIDTH;
	float p2_left = p2->position.x;
	float p2_right = p2->position.x + PLAYER_WIDTH;

	float overlap_x = (p1_right < p2_right ? p1_right : p2_right) - (p1_left > p2_left ? p1_left : p2_left);
	if (overlap_x <= 0.0f) {
		return;
	}

	float p1_top = p1->position.y;
	float p1_bottom = p1->position.y + PLAYER_HEIGHT;
	float p2_top = p2->position.y;
	float p2_bottom = p2->position.y + PLAYER_HEIGHT;
	bool overlap_y = p1_bottom >= p2_top && p2_bottom >= p1_top;
	if (!overlap_y) {
		return;
	}

	float push = overlap_x * 0.5f + 0.01f;
	if (p1->position.x <= p2->position.x) {
		p1->position.x -= push;
		p2->position.x += push;
	} else {
		p1->position.x += push;
		p2->position.x -= push;
	}

	p1->velocity.x = 0.0f;
	p2->velocity.x = 0.0f;
}

static void drop_sword(GameState *state, PlayerId owner, float x, float y) {
	state->combat.sword_in_flight[owner] = false;
	state->combat.sword_on_ground[owner] = true;
	state->combat.sword_drop_position[owner].x = x;
	state->combat.sword_drop_position[owner].y = y;
}

static bool pickup_nearby_sword(GameState *state, PlayerId player, const FighterState *fighter) {
	for (int owner = 0; owner < 2; ++owner) {
		if (!state->combat.sword_on_ground[owner]) {
			continue;
		}
		float dx = fighter->position.x - state->combat.sword_drop_position[owner].x;
		if (dx < 0.0f) dx = -dx;
		float dy = fighter->position.y - state->combat.sword_drop_position[owner].y;
		if (dy < 0.0f) dy = -dy;
		if (dx <= 1.2f && dy <= 1.2f) {
			state->combat.fighters[player].has_sword = true;
			state->combat.fighters[player].sword_recover_frames = 0;
			state->combat.sword_on_ground[owner] = false;
			return true;
		}
	}
	return false;
}

static void try_pickup_ground_sword(GameState *state, PlayerId player, const PlayerCommand *command) {
	FighterState *fighter = &state->combat.fighters[player];

	if (command == NULL || !command->crouch || !fighter->alive || fighter->has_sword) {
		return;
	}

	pickup_nearby_sword(state, player, fighter);
}

static void register_kill(GameState *state, PlayerId killer, PlayerId victim, bool by_throw, bool by_neck_snap);
static void register_double_kill(GameState *state);

static void drop_fighter_sword(GameState *state, PlayerId player) {
	FighterState *fighter = &state->combat.fighters[player];

	if (!fighter->has_sword) {
		return;
	}

	fighter->has_sword = false;
	fighter->sword_recover_frames = 0;
	drop_sword(state, player,
		fighter->position.x + PLAYER_WIDTH * 0.5f,
		fighter->position.y + PLAYER_HEIGHT * 0.5f);
}

static void kill_fighter(GameState *state, PlayerId killer, PlayerId victim, bool by_throw, bool by_neck_snap) {
	drop_fighter_sword(state, victim);
	character_kill(&state->combat.fighters[victim]);
	register_kill(state, killer, victim, by_throw, by_neck_snap);
}

static void kill_both_fighters(GameState *state) {
	drop_fighter_sword(state, PLAYER_ONE);
	drop_fighter_sword(state, PLAYER_TWO);
	character_kill(&state->combat.fighters[PLAYER_ONE]);
	character_kill(&state->combat.fighters[PLAYER_TWO]);
	register_double_kill(state);
}

static void register_kill(GameState *state, PlayerId killer, PlayerId victim, bool by_throw, bool by_neck_snap) {
	state->combat.kill_count[killer]++;
	state->combat.death_count[victim]++;
	state->combat.has_priority = true;
	state->combat.priority_owner = killer;
	if (by_throw) {
		state->combat.throw_kill_count[killer]++;
	} else {
		state->combat.thrust_kill_count[killer]++;
	}
	if (by_neck_snap) {
		state->combat.neck_snap_count[killer]++;
	}
}

static void register_double_kill(GameState *state) {
	state->combat.kill_count[PLAYER_ONE]++;
	state->combat.kill_count[PLAYER_TWO]++;
	state->combat.death_count[PLAYER_ONE]++;
	state->combat.death_count[PLAYER_TWO]++;
	state->combat.has_priority = false;
}

static void build_ai_command(const GameState *state, PlayerCommand *command) {
	const FighterState *ai = &state->combat.fighters[PLAYER_TWO];
	const FighterState *player = &state->combat.fighters[PLAYER_ONE];
	float delta = player->position.x - ai->position.x;

	memset(command, 0, sizeof(*command));
	command->target_height = player->sword_height;

	if (delta > 0.8f) {
		command->move_axis = 1;
	} else if (delta < -0.8f) {
		command->move_axis = -1;
	}

	if (ai->has_sword && delta > -2.5f && delta < 2.5f) {
		command->attack = true;
	}
	if (ai->has_sword && (state->frame_index % 90U) == 0U) {
		command->parry = true;
	}

	if (player->position.y + 0.5f < ai->position.y && ai->grounded) {
		command->jump = true;
	}
}

static float sword_lane_y(const FighterState *fighter) {
	switch (fighter->sword_height) {
	case SWORD_HEIGHT_HIGH: return fighter->position.y + 0.45f;
	case SWORD_HEIGHT_LOW: return fighter->position.y + 1.55f;
	case SWORD_HEIGHT_MID:
	default:
		return fighter->position.y + 1.0f;
	}
}

static float sword_base_x(const FighterState *fighter) {
	return fighter->facing == FACING_RIGHT ? fighter->position.x + PLAYER_WIDTH : fighter->position.x;
}

static float sword_tip_x(const FighterState *fighter) {
	float reach = BASE_SWORD_REACH;
	return sword_base_x(fighter) + (fighter->facing == FACING_RIGHT ? reach : -reach);
}

static bool segments_overlap(float a0, float a1, float b0, float b1) {
	float min_a = a0 < a1 ? a0 : a1;
	float max_a = a0 > a1 ? a0 : a1;
	float min_b = b0 < b1 ? b0 : b1;
	float max_b = b0 > b1 ? b0 : b1;
	return max_a >= min_b && max_b >= min_a;
}

static bool thrust_is_blocked(const FighterState *attacker, const FighterState *defender) {
	if (!attacker->has_sword || !defender->has_sword) {
		return false;
	}
	if (attacker->sword_height != defender->sword_height) {
		return false;
	}

	return fabsf(sword_lane_y(attacker) - sword_lane_y(defender)) < 0.2f
		&& segments_overlap(sword_base_x(attacker), sword_tip_x(attacker), sword_base_x(defender), sword_tip_x(defender));
}

static bool sword_hits_body(const FighterState *attacker, const FighterState *defender, float reach) {
	if (!attacker->has_sword) {
		return false;
	}

	float lane_y = sword_lane_y(attacker);
	float defender_top = defender->position.y + 0.15f;
	float defender_bottom = defender->position.y + PLAYER_HEIGHT - 0.15f;
	if (lane_y < defender_top || lane_y > defender_bottom) {
		return false;
	}

	float tip_x = sword_base_x(attacker) + (attacker->facing == FACING_RIGHT ? reach : -reach);

	return segments_overlap(sword_base_x(attacker), tip_x,
		defender->position.x, defender->position.x + PLAYER_WIDTH);
}

static bool body_contact(const FighterState *a, const FighterState *b, float horizontal_range) {
	float dx = a->position.x - b->position.x;
	if (dx < 0.0f) dx = -dx;
	if (dx > horizontal_range) return false;

	float ay0 = a->position.y;
	float ay1 = a->position.y + PLAYER_HEIGHT;
	float by0 = b->position.y;
	float by1 = b->position.y + PLAYER_HEIGHT;
	return ay1 >= by0 && by1 >= ay0;
}

static void set_block_feedback(GameState *state, const FighterState *p1, const FighterState *p2) {
	state->combat.block_spark_frames = BLOCK_SPARK_FRAMES;
	state->combat.block_spark_position.x = (p1->position.x + p2->position.x) * 0.5f + PLAYER_WIDTH * 0.5f;
	state->combat.block_spark_position.y = (sword_lane_y(p1) + sword_lane_y(p2)) * 0.5f;

	state->combat.fighters[PLAYER_ONE].velocity.x -= BLOCK_RECOIL_SPEED;
	state->combat.fighters[PLAYER_TWO].velocity.x += BLOCK_RECOIL_SPEED;
}

static bool try_apply_disarm(GameState *state,
	PlayerId attacker,
	PlayerId defender,
	const PlayerCommand *attacker_command) {
	FighterState *atk = &state->combat.fighters[attacker];
	FighterState *def = &state->combat.fighters[defender];

	if (attacker_command == NULL || !attacker_command->attack) {
		return false;
	}
	if (!atk->alive || !def->alive || !def->has_sword) {
		return false;
	}

	/* Disarm stays move-tag based (air kick variants), never from sword height clashes. */
	bool dive_or_wall_kick = !atk->grounded;
	if (!dive_or_wall_kick) {
		return false;
	}
	if (!body_contact(atk, def, 1.2f)) {
		return false;
	}

	def->has_sword = false;
	def->sword_recover_frames = SWORD_RECOVER_FRAMES;
	drop_sword(state, defender, def->position.x, def->position.y + PLAYER_HEIGHT * 0.5f);
	def->downed = true;
	def->downed_frames = DOWNED_FRAMES;
	state->combat.disarm_count[attacker]++;
	return true;
}

static void apply_wall_interaction(FighterState *fighter,
	const PlayerCommand *command,
	float left,
	float right) {
	if (fighter->grounded || fighter->downed) {
		return;
	}

	bool at_left_wall = fighter->position.x <= left + 0.1f;
	bool at_right_wall = fighter->position.x + PLAYER_WIDTH >= right - 0.1f;

	if ((at_left_wall || at_right_wall) && fighter->velocity.y > 4.0f) {
		fighter->velocity.y = 4.0f;
	}

	if (!command->jump) {
		return;
	}

	if (at_left_wall) {
		fighter->velocity.x = MAX_HORIZONTAL_VEL * 0.9f;
		fighter->velocity.y = -JUMP_FORCE * 0.9f;
		fighter->facing = FACING_RIGHT;
	}
	if (at_right_wall) {
		fighter->velocity.x = -MAX_HORIZONTAL_VEL * 0.9f;
		fighter->velocity.y = -JUMP_FORCE * 0.9f;
		fighter->facing = FACING_LEFT;
	}
}

static void launch_sword_if_requested(GameState *state, PlayerId owner, const FighterState *attacker, const PlayerCommand *command) {
	if (!command->parry || !attacker->has_sword || attacker->sword_recover_frames > 0) {
		return;
	}
	if (state->combat.sword_in_flight[owner]) {
		return;
	}

	state->combat.sword_in_flight[owner] = true;
	state->combat.sword_flight_height[owner] = attacker->sword_height;
	state->combat.sword_flight_position[owner].x = sword_base_x(attacker);
	state->combat.sword_flight_position[owner].y = sword_lane_y(attacker);
	state->combat.sword_flight_velocity[owner] = (attacker->facing == FACING_RIGHT) ? THROW_SPEED : -THROW_SPEED;
	state->combat.fighters[owner].has_sword = false;
	state->combat.fighters[owner].sword_recover_frames = SWORD_RECOVER_FRAMES;
}

static bool projectile_hits_block(const CombatState *combat, PlayerId owner, PlayerId defender) {
	const FighterState *def = &combat->fighters[defender];
	if (!def->has_sword || !def->alive) {
		return false;
	}
	if (combat->sword_flight_height[owner] != def->sword_height) {
		return false;
	}

	float lane = combat->sword_flight_position[owner].y;
	float def_lane = sword_lane_y(def);
	if (fabsf(lane - def_lane) > 0.2f) {
		return false;
	}

	float px = combat->sword_flight_position[owner].x;
	float s0 = sword_base_x(def);
	float s1 = sword_tip_x(def);
	return segments_overlap(px - 0.25f, px + 0.25f, s0, s1);
}

static bool projectile_hits_body(const CombatState *combat, PlayerId owner, PlayerId defender) {
	const FighterState *def = &combat->fighters[defender];
	if (!def->alive || def->downed) {
		return false;
	}

	float lane = combat->sword_flight_position[owner].y;
	float top = def->position.y + 0.15f;
	float bottom = def->position.y + PLAYER_HEIGHT - 0.15f;
	if (lane < top || lane > bottom) {
		return false;
	}

	float px = combat->sword_flight_position[owner].x;
	return px >= def->position.x && px <= def->position.x + PLAYER_WIDTH;
}

static void update_thrown_swords(GameState *state) {
	for (int owner = 0; owner < 2; ++owner) {
		if (!state->combat.sword_in_flight[owner]) {
			continue;
		}

		PlayerId own = (PlayerId)owner;
		PlayerId opp = owner == PLAYER_ONE ? PLAYER_TWO : PLAYER_ONE;
		state->combat.sword_flight_position[owner].x += state->combat.sword_flight_velocity[owner] * FIXED_TIMESTEP_SEC;

		float x = state->combat.sword_flight_position[owner].x;
		if (x < 0.0f || x > (float)state->arena.width) {
			drop_sword(state, own, x < 0.0f ? 0.0f : (float)state->arena.width, state->combat.sword_flight_position[owner].y);
			continue;
		}

		if (projectile_hits_block(&state->combat, own, opp)) {
			drop_sword(state, own, state->combat.sword_flight_position[owner].x, state->combat.sword_flight_position[owner].y);
			continue;
		}

		if (projectile_hits_body(&state->combat, own, opp)) {
			kill_fighter(state, own, opp, true, false);
			drop_sword(state, own, state->combat.sword_flight_position[owner].x, state->combat.sword_flight_position[owner].y);
		}
	}
}

static void resolve_sword_duel(GameState *state,
	const PlayerCommand *p1_command,
	const PlayerCommand *p2_command) {
	FighterState *p1 = &state->combat.fighters[PLAYER_ONE];
	FighterState *p2 = &state->combat.fighters[PLAYER_TWO];

	if (!p1->alive || !p2->alive) {
		return;
	}
	if (p1->stun_frames > 0 || p2->stun_frames > 0) {
		return;
	}
	if (p1->invulnerability_frames > 0 || p2->invulnerability_frames > 0) {
		return;
	}

	launch_sword_if_requested(state, PLAYER_ONE, p1, p1_command);
	launch_sword_if_requested(state, PLAYER_TWO, p2, p2_command);

	if (p2->downed && p1_command->attack && body_contact(p1, p2, 1.0f)) {
		kill_fighter(state, PLAYER_ONE, PLAYER_TWO, false, true);
		return;
	}
	if (p1->downed && p2_command->attack && body_contact(p2, p1, 1.0f)) {
		kill_fighter(state, PLAYER_TWO, PLAYER_ONE, false, true);
		return;
	}

	bool p1_thrust = p1_command->attack;
	bool p2_thrust = p2_command->attack;
	bool p1_blocked = p1_thrust && thrust_is_blocked(p1, p2);
	bool p2_blocked = p2_thrust && thrust_is_blocked(p2, p1);
	bool p1_hits = p1_thrust && !p1_blocked && sword_hits_body(p1, p2, BASE_SWORD_REACH);
	bool p2_hits = p2_thrust && !p2_blocked && sword_hits_body(p2, p1, BASE_SWORD_REACH);

	if (p1_blocked && p2_blocked) {
		set_block_feedback(state, p1, p2);
	}

	if (p1_hits && p2_hits) {
		kill_both_fighters(state);
		return;
	}

	if (p1_hits) {
		kill_fighter(state, PLAYER_ONE, PLAYER_TWO, false, false);
		return;
	}

	if (p2_hits) {
		kill_fighter(state, PLAYER_TWO, PLAYER_ONE, false, false);
		return;
	}

	if (!p1->has_sword && p1_command->attack && body_contact(p1, p2, 1.1f)) {
		kill_fighter(state, PLAYER_ONE, PLAYER_TWO, false, false);
		return;
	}
	if (!p2->has_sword && p2_command->attack && body_contact(p2, p1, 1.1f)) {
		kill_fighter(state, PLAYER_TWO, PLAYER_ONE, false, false);
		return;
	}

	try_apply_disarm(state, PLAYER_ONE, PLAYER_TWO, p1_command);
	try_apply_disarm(state, PLAYER_TWO, PLAYER_ONE, p2_command);
}

static bool handle_segment_flow(GameState *state) {
	if (!state->combat.segment_goal_mode) {
		return false;
	}

	FighterState *p1 = &state->combat.fighters[PLAYER_ONE];
	FighterState *p2 = &state->combat.fighters[PLAYER_TWO];
	float left = segment_start_x(state->active_segment);
	float right = segment_end_x(state->active_segment);

	if (state->combat.has_priority && state->combat.priority_owner == PLAYER_ONE && p1->alive && p1->position.x < left) {
		if (state->active_segment == 0) {
			state->round_winner = PLAYER_ONE;
			state->combat.score[PLAYER_ONE]++;
			state->match_phase = MATCH_PHASE_ROUND_END;
			state->phase_elapsed = 0;
			return true;
		}
		state->combat.zones_crossed[PLAYER_ONE]++;
		start_transition(state, (uint8_t)(state->active_segment - 1));
		return true;
	}
	if (state->combat.has_priority && state->combat.priority_owner == PLAYER_TWO &&
		p2->alive && p2->position.x + PLAYER_WIDTH > right) {
		if (state->active_segment + 1U >= MAP_TOTAL_SEGMENTS) {
			state->round_winner = PLAYER_TWO;
			state->combat.score[PLAYER_TWO]++;
			state->match_phase = MATCH_PHASE_ROUND_END;
			state->phase_elapsed = 0;
			return true;
		}
		state->combat.zones_crossed[PLAYER_TWO]++;
		start_transition(state, (uint8_t)(state->active_segment + 1));
		return true;
	}

	if (p1->position.x + PLAYER_WIDTH > right) {
		p1->position.x = right - PLAYER_WIDTH;
		p1->velocity.x = 0.0f;
	}
	if (p2->position.x < left) {
		p2->position.x = left;
		p2->velocity.x = 0.0f;
	}
	if (state->active_segment > 0 && p1->position.x < left) {
		p1->position.x = left;
		p1->velocity.x = 0.0f;
	}
	if (state->active_segment + 1U < MAP_TOTAL_SEGMENTS && p2->position.x + PLAYER_WIDTH > right) {
		p2->position.x = right - PLAYER_WIDTH;
		p2->velocity.x = 0.0f;
	}

	return false;
}

GameError match_start(GameState *state) {
	if (state == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	arena_destroy(&state->arena);
	state->config.arena_seed = state->rng_state;

	GameError err = GAME_OK;
	if (state->menu.use_saved_map && state->menu.selected_map_name[0] != '\0') {
		ArenaSaveSummary summary = {0};
		err = arena_load_from_file(&state->arena, state->menu.selected_map_name, &summary);
		if (err == GAME_OK) {
			state->config.arena_width = summary.width;
			state->config.arena_height = summary.height;
			state->config.arena_seed = summary.seed;
			state->config.arena_archetype = summary.archetype;
		}
		state->menu.use_saved_map = false;
	} else {
		ArenaGenerationOptions options;
		arena_generation_options_defaults(&options, state->config.arena_archetype);
		err = arena_generate_with_options(&state->arena,
			state->config.arena_width,
			state->config.arena_height,
			state->config.arena_seed,
			&options);
	}

	if (err != GAME_OK) {
		return err;
	}

	err = combat_init(&state->combat, &state->arena, &state->config);
	if (err != GAME_OK) {
		arena_destroy(&state->arena);
		return err;
	}
	state->active_segment = MAP_MIDDLE_SEGMENT_INDEX;
	state->pending_segment = MAP_MIDDLE_SEGMENT_INDEX;
	state->transition_frames = 0;
	state->combat.sword_on_ground[PLAYER_ONE] = false;
	state->combat.sword_on_ground[PLAYER_TWO] = false;
	state->combat.sword_in_flight[PLAYER_ONE] = false;
	state->combat.sword_in_flight[PLAYER_TWO] = false;
	if (state->combat.segment_goal_mode) {
		place_fighters_for_segment(state, state->active_segment);
	} else {
		place_fighters_for_map_spawns(state);
	}

	state->game_phase = GAME_PHASE_MATCH;
	state->match_phase = MATCH_PHASE_COUNTDOWN;
	state->phase_elapsed = 0;
	state->round_winner = PLAYER_ONE;
	state->combat.has_priority = false;
	return GAME_OK;
}

GameError match_update(GameState *state, const FrameInput *input) {
	if (state == NULL || input == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	if (input->menu_back_pressed) {
		match_abort(state);
		state->game_phase = GAME_PHASE_MAIN_MENU;
		state->match_phase = MATCH_PHASE_MENU;
		state->phase_elapsed = 0;
		return GAME_OK;
	}

	if (state->match_phase == MATCH_PHASE_COUNTDOWN) {
		if (++state->phase_elapsed >= MATCH_COUNTDOWN_FRAMES) {
			state->match_phase = MATCH_PHASE_FIGHT;
			state->phase_elapsed = 0;
		}
		return GAME_OK;
	}

	if (state->match_phase == MATCH_PHASE_ROUND_END) {
		if (++state->phase_elapsed >= ROUND_END_FRAMES) {
			state->game_phase = GAME_PHASE_GAME_OVER;
			state->phase_elapsed = 0;
		}
		return GAME_OK;
	}

	if (state->transition_frames > 0) {
		state->transition_frames--;
		if (state->transition_frames == 0) {
			complete_transition(state);
		}
		return GAME_OK;
	}

	PlayerCommand player_two_command = input->commands[PLAYER_TWO];
	if (state->game_mode == GAME_MODE_VS_AI) {
		build_ai_command(state, &player_two_command);
	}

	float left = segment_start_x(state->active_segment);
	float right = segment_end_x(state->active_segment);
	apply_wall_interaction(&state->combat.fighters[PLAYER_ONE], &input->commands[PLAYER_ONE], left, right);
	apply_wall_interaction(&state->combat.fighters[PLAYER_TWO], &player_two_command, left, right);

	character_update(&state->combat.fighters[PLAYER_ONE], &input->commands[PLAYER_ONE], FIXED_TIMESTEP_MS);
	character_update(&state->combat.fighters[PLAYER_TWO], &player_two_command, FIXED_TIMESTEP_MS);

	GameError err = combat_step(&state->combat, &state->arena, FIXED_TIMESTEP_MS);
	if (err != GAME_OK) {
		return err;
	}

	resolve_fighter_overlap(state);

	try_pickup_ground_sword(state, PLAYER_ONE, &input->commands[PLAYER_ONE]);
	try_pickup_ground_sword(state, PLAYER_TWO, &player_two_command);

	resolve_sword_duel(state, &input->commands[PLAYER_ONE], &player_two_command);
	update_thrown_swords(state);
	if (state->combat.block_spark_frames > 0) {
		state->combat.block_spark_frames--;
	}

	for (int player = 0; player < 2; ++player) {
		if (state->combat.fighters[player].has_sword && state->combat.sword_on_ground[player]) {
			state->combat.sword_on_ground[player] = false;
		}
		if (!state->combat.fighters[player].alive) {
			if (state->combat.respawn_frames[player] == 0) {
				state->combat.respawn_frames[player] = PLAYER_RESPAWN_FRAMES;
			} else {
				state->combat.respawn_frames[player]--;
				if (state->combat.respawn_frames[player] == 0) {
					respawn_player(state, (PlayerId)player);
				}
			}
		}
		if (state->combat.death_popup_frames[player] > 0) {
			state->combat.death_popup_frames[player]--;
		}
	}

	if (handle_segment_flow(state)) {
		return GAME_OK;
	}

	if (state->match_phase == MATCH_PHASE_ROUND_END) {
		return GAME_OK;
	}

	PlayerId winner = PLAYER_ONE;
	if (combat_is_round_over(&state->combat, &winner)) {
		state->round_winner = winner;
		state->combat.score[winner]++;
		state->match_phase = MATCH_PHASE_ROUND_END;
		state->phase_elapsed = 0;
	}

	return GAME_OK;
}

void match_abort(GameState *state) {
	if (state == NULL) {
		return;
	}

	arena_destroy(&state->arena);
	state->combat = (CombatState){0};
}
