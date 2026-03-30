#include "game/arena/arena.h"
#include "game/combat/combat.h"
#include "test_common.h"

static void setup_arena_and_combat(Arena *arena, CombatState *combat) {
	arena_init(arena);
	arena_build_default(arena);
	combat_init(combat);
	combat_reset_round(combat, arena);
}

static int test_combat_init_sets_defaults(void) {
	CombatState combat = {0};

	TEST_ASSERT_TRUE(combat_init(&combat));
	TEST_ASSERT_TRUE(!combat.round_over);
	TEST_ASSERT_EQ_INT(-1, combat.winner_index);
	return 0;
}

static int test_combat_reset_round_uses_room_spawns(void) {
	Arena arena = {0};
	CombatState combat = {0};
	const Room *room;

	setup_arena_and_combat(&arena, &combat);
	room = arena_get_current_room_const(&arena);
	TEST_ASSERT_TRUE(room != NULL);

	TEST_ASSERT_EQ_INT((int)room->spawns.attacker_spawn.x, (int)combat.fighters[0].state.pos.x);
	TEST_ASSERT_EQ_INT((int)room->spawns.defender_spawn.x, (int)combat.fighters[1].state.pos.x);
	TEST_ASSERT_TRUE(combat.fighters[0].state.alive);
	TEST_ASSERT_TRUE(combat.fighters[1].state.alive);
	TEST_ASSERT_TRUE(combat.fighters[0].state.has_sword);
	TEST_ASSERT_TRUE(combat.fighters[1].state.has_sword);
	TEST_ASSERT_TRUE(!combat.fighters[1].state.facing_right);
	return 0;
}

static int test_combat_step_sword_hit_does_not_end_round(void) {
	Arena arena = {0};
	CombatState combat = {0};
	PlayerCommand p1 = {0};
	PlayerCommand p2 = {0};

	setup_arena_and_combat(&arena, &combat);

	combat.fighters[0].state.pos.x = 100.0f;
	combat.fighters[0].state.pos.y = 100.0f;
	combat.fighters[0].state.facing_right = true;

	combat.fighters[1].state.pos.x = 124.0f;
	combat.fighters[1].state.pos.y = 100.0f;
	combat.fighters[1].state.facing_right = false;

	p1.thrust_pressed = true;
	p1.target_sword_line = SWORD_LINE_MID;
	p2.target_sword_line = SWORD_LINE_HIGH;

	combat_step(&combat, &arena, &p1, &p2, FIXED_DT);

	TEST_ASSERT_TRUE(!combat.round_over);
	TEST_ASSERT_EQ_INT(-1, combat.winner_index);
	TEST_ASSERT_TRUE(!combat.fighters[1].state.alive);
	return 0;
}

static int test_combat_killed_fighter_respawns_after_delay(void) {
	Arena arena = {0};
	CombatState combat = {0};
	PlayerCommand p1 = {0};
	PlayerCommand p2 = {0};
	const Room *room;
	f32 elapsed = 0.0f;

	setup_arena_and_combat(&arena, &combat);
	room = arena_get_current_room_const(&arena);
	TEST_ASSERT_TRUE(room != NULL);

	combat.fighters[1].state.alive = false;

	while (elapsed < (DEFAULT_RESPAWN_DELAY + FIXED_DT)) {
		combat_step(&combat, &arena, &p1, &p2, FIXED_DT);
		elapsed += FIXED_DT;
	}

	TEST_ASSERT_TRUE(combat.fighters[1].state.alive);
	TEST_ASSERT_EQ_INT((int)room->spawns.defender_spawn.x, (int)combat.fighters[1].state.pos.x);
	return 0;
}

static int test_combat_step_matching_line_parries_attacker(void) {
	Arena arena = {0};
	CombatState combat = {0};
	PlayerCommand p1 = {0};
	PlayerCommand p2 = {0};

	setup_arena_and_combat(&arena, &combat);

	combat.fighters[0].state.pos.x = 100.0f;
	combat.fighters[0].state.pos.y = 100.0f;
	combat.fighters[0].state.facing_right = true;

	combat.fighters[1].state.pos.x = 124.0f;
	combat.fighters[1].state.pos.y = 100.0f;
	combat.fighters[1].state.facing_right = false;

	p1.thrust_pressed = true;
	p1.target_sword_line = SWORD_LINE_MID;
	p2.target_sword_line = SWORD_LINE_MID;

	combat_step(&combat, &arena, &p1, &p2, FIXED_DT);

	TEST_ASSERT_TRUE(!combat.round_over);
	TEST_ASSERT_TRUE(combat.fighters[0].state.is_stunned);
	TEST_ASSERT_TRUE(!combat.fighters[0].state.has_sword);
	TEST_ASSERT_TRUE(combat.fighters[1].state.is_parrying);
	return 0;
}

int run_combat_tests(void) {
	TestCase cases[] = {
		{"init_sets_defaults", test_combat_init_sets_defaults},
		{"reset_round_uses_room_spawns", test_combat_reset_round_uses_room_spawns},
		{"step_sword_hit_does_not_end_round", test_combat_step_sword_hit_does_not_end_round},
		{"killed_fighter_respawns_after_delay", test_combat_killed_fighter_respawns_after_delay},
		{"step_matching_line_parries_attacker", test_combat_step_matching_line_parries_attacker},
	};

	return run_test_cases("combat", cases, (int)(sizeof(cases) / sizeof(cases[0])));
}
