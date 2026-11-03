#pragma once

#include <ncore/services/service.h>
#include <ncore/structs/rid.h>

#include "audio_desc.h"

namespace nc {

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

    RID create_stream( const AudioStreamDesc& p_desc );
    void play_sound( RID stream_rid );

    void destroy_rid( RID handle );

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace nc
