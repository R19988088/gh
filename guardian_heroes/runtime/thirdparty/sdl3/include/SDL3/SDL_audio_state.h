/* Project extension: exact software stream state, excluding the host device. */
#ifndef SDL_audio_state_h_
#define SDL_audio_state_h_
#include <SDL3/SDL_audio.h>
#ifdef __cplusplus
extern "C" {
#endif
extern SDL_DECLSPEC bool SDLCALL GH_SDL_SaveAudioStreamState(SDL_AudioStream *, void **, size_t *);
extern SDL_DECLSPEC bool SDLCALL GH_SDL_RestoreAudioStreamState(SDL_AudioStream *, const void *, size_t);
#ifdef __cplusplus
}
#endif
#endif
