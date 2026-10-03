#include "sdl_audio_compiler.h"

#include <SDL3/SDL_audio.h>

#include <ncore/core/collection.h>
#include <ncore/resources/resource_archive.h>
#include <ncore/utils/log.h>

namespace nc {

StringView SDLAudioCompiler::get_format_name() const
{
    return "Audio";
}

bool SDLAudioCompiler::is_handling_extension( const String& ext )
{
    return ext == ".wav";
}

Error SDLAudioCompiler::compile( StringView p_input, StringView p_output, AssetImportContext p_ctx )
{
    uint8_t* raw_buf = nullptr;
    uint32_t wav_len = 0;

    SDL_AudioSpec spec{};
    spec.format   = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq     = 44100;

    if (!SDL_LoadWAV( p_input.data(), &spec, &raw_buf, &wav_len )) {
        NC_LOG_ERROR_C( log::AUDIO, "WAV import FAIL: {}", SDL_GetError() );
        return Error::FAIL;
    }

    auto result =
        Ref<AudioClip>::create( raw_buf, wav_len, spec.channels, spec.freq, SDL_AUDIO_BITSIZE( spec.format ) );
    SDL_free( raw_buf );

    ResourceArchive archive;
    if (auto err = archive.serialize( result, p_output.data() ); err != Error::OK) {
        NC_LOG_ERROR_C( log::AUDIO, "WAV serialize FAIL for '{}'", p_input );
        return err;
    }

    return Error::OK;
}

} // namespace nc
