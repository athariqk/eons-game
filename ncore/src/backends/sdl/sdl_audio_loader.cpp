#include "sdl_audio_loader.h"

#include <SDL3/SDL_audio.h>

#include <ncore/kernel/collection.h>
#include <ncore/modules/audio/resources/audio_clip.h>
#include <ncore/utils/log.h>

namespace nc {

bool SDLAudioLoader::is_handling_extension( const std::string& ext )
{
    return ext == ".wav";
}

Ref<IResource> SDLAudioLoader::import( const std::string_view path )
{
    uint8_t* raw_buf = nullptr;
    uint32_t wav_len = 0;

    SDL_AudioSpec spec{};
    spec.format   = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq     = 44100;

    if (!SDL_LoadWAV( path.data(), &spec, &raw_buf, &wav_len )) {
        NC_LOG_ERROR_C( log::AUDIO, "WAV import FAIL: {}", SDL_GetError() );
        return {};
    }

    auto result =
        Ref<AudioClip>::create( raw_buf, wav_len, spec.channels, spec.freq, SDL_AUDIO_BITSIZE( spec.format ) );
    SDL_free( raw_buf );
    return result.as<IResource>();
}

} // namespace nc
