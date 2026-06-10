#include "AudioManager.h"

#include "Logger.h"

namespace Aeon {

AudioManager::AudioManager() {
    if (!SDL_Init(SDL_INIT_AUDIO)) {
        LOG_ERROR(Log::Audio, "Failed to initialize SDL audio submodule");
    } else {
        LOG_INFO(Log::Audio, "SDL Audio initialized");
    }

    SDL_zero(srcSpec);
    srcSpec.format = SDL_AUDIO_F32;
    srcSpec.channels = 2;
    srcSpec.freq = 44100;

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &srcSpec, nullptr, nullptr);
    if (stream == nullptr) {
        LOG_ERROR(Log::Audio, "Failed to open audio stream: {}", SDL_GetError());
        return;
    }

    SDL_ResumeAudioStreamDevice(stream);
}

void AudioManager::Clear() {
    if (stream != nullptr) {
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
    }

    SDL_free(wavBuffer);
    wavBuffer = nullptr;
    wavLength = 0;
}

void AudioManager::PlayWAV(const char *path) {
    if (stream == nullptr) {
        LOG_ERROR(Log::Audio, "Can't play audio without an initialized audio stream");
        return;
    }

    SDL_free(wavBuffer);
    wavBuffer = nullptr;
    wavLength = 0;

    if (!SDL_LoadWAV(path, &srcSpec, &wavBuffer, &wavLength)) {
        LOG_ERROR(Log::Audio, "Couldn't open audio: {}", SDL_GetError());
        return;
    } else {
        LOG_INFO(Log::Audio, "Playing audio path: {}", path);
    }

    if (SDL_GetAudioStreamQueued(stream) < static_cast<int>(wavLength)) {
        SDL_PutAudioStreamData(stream, wavBuffer, wavLength);
    }
}

} // namespace Aeon
