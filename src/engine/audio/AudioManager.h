#pragma once

#include <string>

#include <SDL3/SDL.h>

namespace Aeon {

class AudioManager {
public:
    AudioManager();

    void Clear();

    void PlayWAV(const char *path);

private:
    SDL_AudioStream *stream{};
    SDL_AudioSpec srcSpec;
    Uint32 wavLength{};
    Uint8 *wavBuffer{};
};

} // namespace Aeon
