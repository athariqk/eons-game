#include "AudioManager.h"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_init.h>

#include "modules/utils/Logger.h"

namespace ncore {

AudioManager::AudioManager() {}

void AudioManager::clean() {
    for (auto &[handle, stream]: streams) {
        if (stream != nullptr) {
            SDL_DestroyAudioStream(stream);
            stream = nullptr;
        }
    }

    streams.clear();
}

void AudioManager::play_wav(const AudioClip *p_clip) {
    auto it = streams.find(p_clip);
    if (it == streams.end()) {
        SDL_AudioSpec spec{.format = SDL_AUDIO_F32, .channels = p_clip->channels, .freq = p_clip->frequency};
        SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
        if (!stream) {
            LOG_ERROR(log::AUDIO, "Failed to open audio stream: {}", SDL_GetError());
            return;
        }
        it = streams.emplace(p_clip, stream).first;
    }

    SDL_PutAudioStreamData(it->second, p_clip->data.data(), p_clip->length);
    SDL_ResumeAudioStreamDevice(it->second);
}

} // namespace ncore
