#include <ncore/core/collection.h>
#include <ncore/services/audio/audio_service.h>
#include <utils/profiler.h>

namespace nc {

struct AudioService::Impl {
    struct SoundEntry {
        std::span<const std::byte> data;
        int length;
        SDL_AudioStream* stream;
    };
    RIDPool<SoundEntry> sounds;
};

AudioService::AudioService() : pImpl( std::make_unique<Impl>() ) {}

AudioService::~AudioService() = default;

Error AudioService::init( ConfFile& cfg_file )
{
    return Error::OK;
}

void AudioService::shutdown() {}

RID AudioService::create_stream( const AudioStreamDesc& p_desc )
{
    RID handle = pImpl->sounds.acquire();

    SDL_AudioSpec spec{ .format = SDL_AUDIO_F32, .channels = p_desc.channels, .freq = p_desc.frequency };
    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream( SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr );
    if (!stream) {
        NC_LOG_ERROR_C( log::AUDIO, "Failed to open audio sound: {}", SDL_GetError() );
        return RID();
    }

    auto entry = pImpl->sounds.get( handle );
    if (entry) {
        entry->data   = p_desc.data;
        entry->length = static_cast<int>( p_desc.data.size() );
        entry->stream = stream;
    }

    return RID( handle );
}

void AudioService::play_sound( RID stream_rid )
{
    NC_ZONE_COLOR( "AudioService::play_sound", NC_COLOR_AUDIO )
    NC_FAIL_MSG_RET( stream_rid.is_valid(), "Failed to play sound: invalid handle" );

    auto sound = pImpl->sounds.get( stream_rid );
    if (sound) {
        SDL_PutAudioStreamData( sound->stream, sound->data.data(), sound->length );
        SDL_ResumeAudioStreamDevice( sound->stream );
    }
}

void AudioService::destroy_rid( RID handle )
{
    if (auto entry = pImpl->sounds.get( handle )) {
        SDL_DestroyAudioStream( entry->stream );
        pImpl->sounds.release( handle );
    }
}

} // namespace nc
