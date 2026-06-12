#pragma once

#include <unordered_map>

#include <modules/resources/Resource.h>

struct SDL_AudioStream;

namespace ncore {

class AudioManager {
public:
    AudioManager();

    void clean();

    void play_wav(const AudioClip *p_clip);

private:
    std::unordered_map<const AudioClip *, SDL_AudioStream *> streams;
};

} // namespace ncore
