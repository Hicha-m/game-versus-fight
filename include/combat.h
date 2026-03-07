#ifndef COMBAT_H
#define COMBAT_H

#include <stdbool.h>
#include <stdint.h>

#include "types.h"

GameError combat_init(CombatState *combat, const Arena *arena, const GameConfig *config);
GameError combat_apply_action(CombatState *combat, PlayerId actor, const Action *action);
GameError combat_step(CombatState *combat, const Arena *arena, uint32_t fixed_dt_ms);
GameError combat_reset_round(CombatState *combat, const Arena *arena, const GameConfig *config);
bool combat_is_round_over(const CombatState *combat, PlayerId *out_winner);

#endif
