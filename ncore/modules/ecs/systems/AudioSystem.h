#pragma once
#pragma once

#include <queue>
#include <string>

#include <modules/Services.h>
#include <modules/audio/AudioManager.h>
#include <modules/ecs/System.h>
#include <modules/resources/ResourceHandle.h>

namespace ncore {

class World;
class ResourceManager;

/**
 * @brief Event for requesting audio playback
 */
struct AudioEvent {
    ResourceHandle res_hnd;
    float volume = 1.0f;
    bool loop = false;
};

/**
 * @brief Audio system that manages sound playback
 *
 * This system processes queued audio events during the variable update phase.
 * Components can request audio playback by getting a reference to this system
 * via world.get_system<AudioSystem>() and calling PlaySound().
 *
 * This decouples audio from immediate execution, allowing for priority
 * management, spatial audio, and other effects.
 *
 * Priority: 50 (medium priority, runs after gameplay but before rendering)
 */
class AudioSystem : public System {
public:
    AudioSystem() {
        set_priority(50); // Run audio at medium priority
    }

    bool on_init(World &world) override;
    void on_variable_update(World &world, double delta) override;

    /**
     * @brief Queue a sound for playback
     * @param path Path to the audio file
     * @param volume Volume multiplier (0.0 to 1.0)
     */
    void play_sound(const ResourceHandle res, float volume = 1.0f) { audio_queue.push({res, volume, false}); }

    /**
     * @brief Queue a looping sound for playback
     * @param path Path to the audio file
     * @param volume Volume multiplier (0.0 to 1.0)
     */
    void play_sound_loop(const ResourceHandle res, float volume = 1.0f) { audio_queue.push({res, volume, true}); }

    void play_sound(std::string_view path, float volume = 1.0f);

private:
    ResourceManager *res_mgr = nullptr;
    std::queue<AudioEvent> audio_queue;
};

} // namespace ncore
