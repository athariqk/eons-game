#include "AudioManager.h"

#include "Logger.h"

namespace ncore {

AudioManager::AudioManager() {
    if (!SDL_Init(SDL_INIT_AUDIO)) {
        LOG_ERROR(log::AUDIO, "Failed to initialize SDL audio submodule");
    } else {
        LOG_INFO(log::AUDIO, "SDL Audio initialized");
    }

    SDL_zero(src_spec);
    src_spec.format = SDL_AUDIO_F32;
    src_spec.channels = 2;
    src_spec.freq = 44100;

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &src_spec, nullptr, nullptr);
    if (stream == nullptr) {
        LOG_ERROR(log::AUDIO, "Failed to open audio stream: {}", SDL_GetError());
        return;
    }

    SDL_ResumeAudioStreamDevice(stream);
}

void AudioManager::clean() {
    if (stream != nullptr) {
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
    }

    SDL_free(wav_buffer);
    wav_buffer = nullptr;
    wav_len = 0;
}

void AudioManager::play_wav(const char *path) {
    if (stream == nullptr) {
        LOG_ERROR(log::AUDIO, "Can't play audio without an initialized audio stream");
        return;
    }

    SDL_free(wav_buffer);
    wav_buffer = nullptr;
    wav_len = 0;

    if (!SDL_LoadWAV(path, &src_spec, &wav_buffer, &wav_len)) {
        LOG_ERROR(log::AUDIO, "Couldn't open audio: {}", SDL_GetError());
        return;
    } else {
        LOG_INFO(log::AUDIO, "Playing audio path: {}", path);
    }

    if (SDL_GetAudioStreamQueued(stream) < static_cast<int>(wav_len)) {
        SDL_PutAudioStreamData(stream, wav_buffer, wav_len);
    }
}

} // namespace ncore
