#include <math.h>
#include "engine_internal.h"

/* Small synthesized score and effects: no external files or codec dependency. */
void engine_audio_start(Engine *engine)
{
    SDL_AudioSpec spec = {.format = SDL_AUDIO_F32, .channels = 1, .freq = 24000};
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
        return;
    engine->audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (engine->audio)
        SDL_ResumeAudioStreamDevice(engine->audio);
}

void engine_audio_update(Engine *engine, f32 master, f32 music, f32 effects, bool playing,
                         u32 events)
{
    if (!engine || !engine->audio)
        return;
    if (events)
    {
        engine->effect_samples = 3600;
        engine->effect_frequency = (events & 1u) ? 110.0f : 660.0f;
    }
    if (SDL_GetAudioStreamQueued(engine->audio) >= 2400 * (int)sizeof(float))
        return;
    float samples[600];
    static const float notes[] = {220, 261.63f, 329.63f, 293.66f, 220, 196, 246.94f, 293.66f};
    for (int i = 0; i < 600; ++i)
    {
        u64 t = engine->audio_sample++;
        float beat = (float)(t % 12000) / 12000.0f;
        float melody = sinf((float)(t % 240000) * notes[(t / 12000) % 8] * 6.2831853f / 24000.0f);
        float sample = playing ? melody * (1.0f - beat) * 0.06f * music : 0;
        if (engine->effect_samples > 0)
        {
            float envelope = engine->effect_samples / 3600.0f;
            sample += sinf((float)(t % 240000) * engine->effect_frequency * 6.2831853f / 24000.0f) *
                      envelope * 0.22f * effects;
            --engine->effect_samples;
        }
        samples[i] = sample * master;
    }
    SDL_PutAudioStreamData(engine->audio, samples, sizeof(samples));
}
