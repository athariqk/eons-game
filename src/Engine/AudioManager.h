#pragma once

#include <string>

#include <SDL3/SDL.h>

class AudioManager {
public:
    void Init();
    void Clear();

    void PlayWAV(const char *path);

private:
    SDL_AudioStream *stream{};
    SDL_AudioSpec wavSpec{};
    Uint32 wavLength{};
    Uint8 *wavBuffer{};
};
