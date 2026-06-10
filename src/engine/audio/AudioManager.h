#pragma once

#include <string>

#include <SDL3/SDL.h>

namespace ncore {

class AudioManager {
public:
    AudioManager();

    void clean();

    void play_wav(const char *path);

private:
    SDL_AudioStream *stream{};
    SDL_AudioSpec src_spec;
    Uint32 wav_len{};
    Uint8 *wav_buffer{};
};

} // namespace ncore
