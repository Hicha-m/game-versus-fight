#ifndef COMBAT_INTERNAL_H
#define COMBAT_INTERNAL_H

#include "game/combat/combat.h"

void fighter_apply_command(Fighter* fighter, const PlayerCommand* cmd, f32 dt);
void fighter_update_timers(Fighter* fighter, f32 dt);
void fighter_apply_gravity(Fighter* fighter, f32 dt);
void fighter_integrate(Fighter* fighter, f32 dt);
void fighter_resolve_world_collision(Fighter* fighter, const Room* room, f32 dt);

void combat_resolve_facing(Fighter* left, Fighter* right);
void combat_resolve_attacks(CombatState* combat);
void combat_check_fall_deaths(CombatState* combat, const Room* room);

RectF fighter_get_body_rect(const Fighter* fighter);
bool fighters_in_attack_range(const Fighter* attacker, const Fighter* defender, bool armed);

#endif