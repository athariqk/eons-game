#pragma once

#include <ncore/kernel/resource.h>
#include <ncore/modules/assets/asset.h>
#include <ncore/modules/service.h>

namespace ncore {

struct AudioClip;

class IAudioService : public IService {
    NCLASS( IAudioService, IService )

public:
    virtual void play_sound( const AudioClip* p_sound ) = 0;
};

} // namespace ncore
