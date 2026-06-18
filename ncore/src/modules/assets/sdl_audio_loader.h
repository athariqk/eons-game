#pragma once

#include <modules/assets/asset_loader.h>

#include <modules/audio/audio_clip.h>

namespace ncore {

// Can only load WAV of a certain format for now
struct SDLAudioLoader : public IAssetLoader<AudioClip> {
public:
    std::unique_ptr<AudioClip> load(const std::string_view path) override;
};

} // namespace ncore
