#include "AudioSystem.h"

#include <World.h>

namespace ncore {

void AudioSystem::on_variable_update(World &world, double delta) {
    auto *audio = world.get_main_loop().get_services().try_get<AudioManager>();
    if (!audio)
        return;

    // Process queued audio events
    while (!audio_queue.empty()) {
        auto &event = audio_queue.front();

        // TODO: Apply volume, looping, spatial audio, etc.
        audio->play_wav(event.sound_path.c_str());

        audio_queue.pop();
    }

    // TODO: Update spatial audio based on entity positions
    // TODO: Handle audio fading, ducking, etc.
}

} // namespace ncore

