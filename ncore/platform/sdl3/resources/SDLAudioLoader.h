#pragma once

#include <modules/resources/IResourceLoader.h>

namespace ncore {

// Can only load WAV of a certain format for now
struct SDLAudioLoader : public IResourceLoader<AudioClip> {
public:
    AudioClip load_from_disk(std::string location) override;
};

} // namespace ncore
