#ifndef GAME_INTERNAL_H
#define GAME_INTERNAL_H

#include "game/game.h"

void game_start_match(Game* game);

void game_build_player_command_from_input(
    PlayerCommand* out_cmd,
    const PlayerInput* input
);

void game_collect_commands(
    Game* game,
    const FrameInput* input,
    PlayerCommand* out_p1,
    PlayerCommand* out_p2,
    f32 dt
);

void game_handle_room_transitions(Game* game);
void game_handle_round_end(Game* game);

#endif