#pragma once

#include <modules/assets/asset.h>
#include <ncore/kernel/resource.h>
#include <ncore/kernel/service.h>

namespace ncore {

struct AudioClip;

class IAudioService : public IService {
    NCLASS(IAudioService, IService)

public:
    virtual void play_sound(const AudioClip *p_sound) = 0;
};

} // namespace ncore
