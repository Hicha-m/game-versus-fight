#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include "types.h"

void config_set_defaults(GameConfig *config);
GameError config_load(GameConfig *config);
GameError config_save(const GameConfig *config);
uint16_t config_clamp_round_time(uint16_t seconds);
const char *config_binding_label(BindingAction action);
const char *config_game_mode_label(GameMode mode);
const char *config_difficulty_label(DifficultyLevel difficulty);
const char *config_archetype_label(ArchetypeType archetype);
uint8_t config_rebindable_action_count(void);
BindingAction config_rebindable_action_at(uint8_t index);

#endif