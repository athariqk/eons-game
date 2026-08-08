#pragma once

#include <ncore/services/service.h>

namespace nc {

class AudioClip;

/**
 * @brief AudioService provides base functionality for
 * audio playback and management.
 */
class AudioService : public IService {
    NCLASS( AudioService, IService )

public:
    AudioService();
    ~AudioService() override;

    Error init( ConfFile& cfg_file ) override;
    void shutdown() override;

    RID create_stream( const AudioClip& p_clip );
    void play_sound( RID stream_rid );

    void destroy_resource( RID handle );

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace nc
