#pragma once

#include <ncore/kernel/resource.h>
#include <ncore/modules/audio/audio_service.h>

struct SDL_AudioStream;

namespace ncore {

class AssetManager;
struct AudioClip;

class SDLAudioImpl : public IAudioService {
    NCLASS( SDLAudioImpl, IAudioService )

public:
    SDLAudioImpl() = default;

    Error init() override;
    void finalize() override;

    void play_sound( const AudioClip* p_sound ) override;

private:
    std::unordered_map<const AudioClip*, SDL_AudioStream*> streams;
};

} // namespace ncore
