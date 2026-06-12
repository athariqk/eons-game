#include "AudioSystem.h"

#include <modules/MainLoop.h>
#include <modules/World.h>
#include <modules/resources/ResourceManager.h>

namespace ncore {

bool AudioSystem::on_init(World &world) {
    res_mgr = world.get_main_loop().get_services().try_get<ResourceManager>();
    if (!res_mgr)
        return false;
    return true;
}

void AudioSystem::on_variable_update(World &world, double delta) {
    auto audio = world.get_main_loop().get_services().try_get<AudioManager>();
    if (!audio)
        return;

    // Process queued audio events
    while (!audio_queue.empty()) {
        auto &event = audio_queue.front();

        // TODO: Apply volume, looping, spatial audio, etc.
        auto clip = res_mgr->access<AudioClip>(event.res_hnd);
        audio->play_wav(clip);

        audio_queue.pop();
    }

    // TODO: Update spatial audio based on entity positions
    // TODO: Handle audio fading, ducking, etc.
}

void AudioSystem::play_sound(std::string_view path, float volume) {
    auto res = res_mgr->get<AudioClip>(std::string(path));
    play_sound(res, volume);
}

} // namespace ncore
