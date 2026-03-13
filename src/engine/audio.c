#include "audio.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <string.h>

#define PATH_BUFFER_SIZE 512

typedef struct LoadedSfx {
	Uint8 *data;
	Uint32 length;
	SDL_AudioSpec spec;
	bool loaded;
} LoadedSfx;

static SDL_AudioStream *s_stream = NULL;
static LoadedSfx s_sfx[AUDIO_SFX_COUNT] = {0};
static bool s_audio_ready = false;

static const char *sfx_rel_paths[AUDIO_SFX_COUNT] = {
	"assets/sound/sndType.wav",
	"assets/sound/sndDrop.wav",
	"assets/sound/sndHit1.wav",
	"assets/sound/sndClash.wav",
	"assets/sound/sndPickup.wav",
	"assets/sound/sndDie.wav",
	"assets/sound/sndSpawn.wav",
	"assets/sound/sndJump.wav",
	"assets/sound/sndLand1.wav",
	"assets/sound/sndStep.wav",
	"assets/sound/sndFlyingSwordFlies.wav"
};

static bool file_exists(const char *path) {
	if (path == NULL) {
		return false;
	}
	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		return false;
	}
	fclose(file);
	return true;
}

static bool resolve_asset_path(const char *relative_path, char *out_path, size_t out_size) {
	if (relative_path == NULL || out_path == NULL || out_size == 0U) {
		return false;
	}

	const char *prefixes[] = {
		"",
		"../",
		"../../"
	};

	for (size_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); ++i) {
		snprintf(out_path, out_size, "%s%s", prefixes[i], relative_path);
		if (file_exists(out_path)) {
			return true;
		}
	}

	out_path[0] = '\0';
	return false;
}

static bool same_audio_spec(const SDL_AudioSpec *a, const SDL_AudioSpec *b) {
	if (a == NULL || b == NULL) {
		return false;
	}
	return a->format == b->format && a->channels == b->channels && a->freq == b->freq;
}

GameError audio_init(void) {
	audio_shutdown();

	SDL_AudioSpec stream_spec = {0};
	bool stream_spec_ready = false;

	for (int i = 0; i < (int)AUDIO_SFX_COUNT; ++i) {
		char path[PATH_BUFFER_SIZE] = {0};
		if (!resolve_asset_path(sfx_rel_paths[i], path, sizeof(path))) {
			SDL_Log("audio_init: missing sound '%s'", sfx_rel_paths[i]);
			continue;
		}

		SDL_AudioSpec spec = {0};
		Uint8 *data = NULL;
		Uint32 length = 0;
		if (!SDL_LoadWAV(path, &spec, &data, &length)) {
			SDL_Log("audio_init: failed loading '%s': %s", path, SDL_GetError());
			continue;
		}

		s_sfx[i].data = data;
		s_sfx[i].length = length;
		s_sfx[i].spec = spec;
		s_sfx[i].loaded = true;

		if (!stream_spec_ready) {
			stream_spec = spec;
			stream_spec_ready = true;
		}
	}

	if (!stream_spec_ready) {
		s_audio_ready = false;
		return GAME_OK;
	}

	s_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &stream_spec, NULL, NULL);
	if (s_stream == NULL) {
		SDL_Log("audio_init: SDL_OpenAudioDeviceStream failed: %s", SDL_GetError());
		s_audio_ready = false;
		return GAME_OK;
	}

	if (!SDL_ResumeAudioStreamDevice(s_stream)) {
		SDL_Log("audio_init: SDL_ResumeAudioStreamDevice failed: %s", SDL_GetError());
		SDL_DestroyAudioStream(s_stream);
		s_stream = NULL;
		s_audio_ready = false;
		return GAME_OK;
	}

	s_audio_ready = true;
	return GAME_OK;
}

void audio_shutdown(void) {
	if (s_stream != NULL) {
		SDL_DestroyAudioStream(s_stream);
		s_stream = NULL;
	}

	for (int i = 0; i < (int)AUDIO_SFX_COUNT; ++i) {
		if (s_sfx[i].data != NULL) {
			SDL_free(s_sfx[i].data);
			s_sfx[i].data = NULL;
		}
		s_sfx[i].length = 0;
		s_sfx[i].spec = (SDL_AudioSpec){0};
		s_sfx[i].loaded = false;
	}

	s_audio_ready = false;
}

void audio_play_sfx(AudioSfxId id) {
	if (!s_audio_ready || s_stream == NULL) {
		return;
	}
	if (id < 0 || id >= AUDIO_SFX_COUNT) {
		return;
	}
	if (!s_sfx[id].loaded || s_sfx[id].data == NULL || s_sfx[id].length == 0U) {
		return;
	}

	SDL_AudioSpec current = {0};
	if (!SDL_GetAudioStreamFormat(s_stream, &current, NULL)) {
		return;
	}
	if (!same_audio_spec(&current, &s_sfx[id].spec)) {
		return;
	}

	(void)SDL_PutAudioStreamData(s_stream, s_sfx[id].data, (int)s_sfx[id].length);
}
