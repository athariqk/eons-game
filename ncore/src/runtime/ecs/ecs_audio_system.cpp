#include <runtime/ecs/ecs_audio_system.h>

#include <modules/assets/asset_manager.h>
#include <modules/audio/audio_clip.h>
#include <ncore/runtime/ecs_world.h>
#include <ncore/runtime/service_locator.h>

namespace ncore {

void EcsAudioSystem::on_init(EcsWorld &world) {
    resources = world.get_services().resolve<AssetManager>();
    audio_mgr = world.get_services().resolve<IAudioService>();
}

void EcsAudioSystem::on_variable_update(EcsWorld &world, double delta) {
    while (!audio_queue.empty()) {
        auto &event = audio_queue.front();
        auto clip = resources->get<AudioClip>(event.sound);
        audio_mgr->play_sound(clip);
        audio_queue.pop();
    }
}

void EcsAudioSystem::play_sound(std::string_view path, float volume) {
    auto res = resources->load<AudioClip>(path);
    play_sound(res, volume);
}

} // namespace ncore
