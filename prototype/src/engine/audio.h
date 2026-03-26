#ifndef ENGINE_AUDIO_H
#define ENGINE_AUDIO_H

#include "types.h"

typedef enum AudioSfxId {
	AUDIO_SFX_MENU_CONFIRM,
	AUDIO_SFX_MENU_BACK,
	AUDIO_SFX_HIT,
	AUDIO_SFX_BLOCK,
	AUDIO_SFX_PICKUP,
	AUDIO_SFX_DEATH,
	AUDIO_SFX_SPAWN,
	AUDIO_SFX_JUMP,
	AUDIO_SFX_LAND,
	AUDIO_SFX_STEP,
	AUDIO_SFX_THROW,
	AUDIO_SFX_COUNT
} AudioSfxId;

GameError audio_init(void);
void audio_shutdown(void);
void audio_play_sfx(AudioSfxId id);

#endif