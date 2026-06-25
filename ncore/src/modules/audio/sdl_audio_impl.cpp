#include "sdl_audio_impl.h"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_error.h>

#include <modules/assets/asset_manager.h>
#include <modules/audio/audio_clip.h>
#include <ncore/utils/log.h>

namespace ncore {

Error SDLAudioImpl::init()
{
    // Initialization code for SDL audio plugin
    return Error::OK;
}

void SDLAudioImpl::finalize()
{
    for (auto& [handle, stream] : streams) {
        if (stream != nullptr) {
            SDL_DestroyAudioStream(stream);
            stream = nullptr;
        }
    }

    streams.clear();
}

void SDLAudioImpl::play_sound(const AudioClip* p_sound)
{
    NC_ASSERT_RET(p_sound != nullptr, "Failed to play sound: invalid handle or type mismatch");

    auto it = streams.find(p_sound);
    if (it == streams.end()) {
        SDL_AudioSpec spec{.format = SDL_AUDIO_F32, .channels = p_sound->channels, .freq = p_sound->frequency};
        SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
        if (!stream) {
            NC_LOG_ERROR_C(log::AUDIO, "Failed to open audio stream: {}", SDL_GetError());
            return;
        }
        it = streams.emplace(p_sound, stream).first;
    }

    SDL_PutAudioStreamData(it->second, p_sound->data.data(), p_sound->length);
    SDL_ResumeAudioStreamDevice(it->second);
}

} // namespace ncore
