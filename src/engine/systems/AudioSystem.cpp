#include "AudioSystem.h"

#include <World.h>

namespace Aeon {

void AudioSystem::OnVariableUpdate(World &world, double delta) {
    auto *audio = world.GetMainLoop().GetServices().TryGet<AudioManager>();
    if (!audio)
        return;

    // Process queued audio events
    while (!m_audioQueue.empty()) {
        auto &event = m_audioQueue.front();

        // TODO: Apply volume, looping, spatial audio, etc.
        audio->PlayWAV(event.soundPath.c_str());

        m_audioQueue.pop();
    }

    // TODO: Update spatial audio based on entity positions
    // TODO: Handle audio fading, ducking, etc.
}

} // namespace Aeon
