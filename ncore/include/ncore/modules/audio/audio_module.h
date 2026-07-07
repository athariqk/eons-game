#pragma once

#include <ncore/modules/module.h>

namespace nc {

class AudioClip;

/**
 * @brief AudioModule provides base functionality for
 * audio playback and management.
 */
class AudioModule : public IModule {
    NCLASS( AudioModule, IModule )

public:
    AudioModule();
    ~AudioModule() override;

    Error init() override;
    void finalize() override;

    RID create_stream( const AudioClip& p_clip );
    void play_sound( RID stream_rid );

    void destroy_resource( RID handle );

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace nc
