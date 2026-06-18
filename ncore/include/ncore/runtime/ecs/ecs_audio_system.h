#pragma once

#include <queue>

#include <ncore/kernel/resource.h>
#include <ncore/modules/audio/audio_service.h>
#include <ncore/modules/ecs/ecs_system.h>

namespace ncore {

class EcsWorld;
class AssetManager;

struct AudioEvent {
    RID sound;
    float volume = 1.0f;
    bool loop = false;
};

class EcsAudioSystem : public EcsSystem {
    NCLASS(EcsAudioSystem, EcsSystem)

public:
    EcsAudioSystem() { set_priority(50); }

    void on_init(EcsWorld &world) override;
    void on_variable_update(EcsWorld &world, double delta) override;

    void play_sound(const RID sound, float volume = 1.0f) { audio_queue.push({sound, volume, false}); }
    void play_sound_loop(const RID sound, float volume = 1.0f) { audio_queue.push({sound, volume, true}); }
    void play_sound(std::string_view path, float volume = 1.0f);

private:
    IAudioService *audio_mgr = nullptr;
    AssetManager *resources = nullptr;
    std::queue<AudioEvent> audio_queue;
};

} // namespace ncore
