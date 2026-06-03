#pragma once
#pragma once

#include <queue>
#include <string>

#include <AudioManager.h>
#include <Services.h>
#include <System.h>

namespace Aeon {

class World;

/**
 * @brief Event for requesting audio playback
 */
struct AudioEvent {
    std::string soundPath;
    float volume = 1.0f;
    bool loop = false;
};

/**
 * @brief Audio system that manages sound playback
 *
 * This system processes queued audio events during the variable update phase.
 * Components can request audio playback by getting a reference to this system
 * via world.GetSystem<AudioSystem>() and calling PlaySound().
 *
 * This decouples audio from immediate execution, allowing for priority
 * management, spatial audio, and other effects.
 *
 * Priority: 50 (medium priority, runs after gameplay but before rendering)
 */
class AudioSystem : public System {
public:
    AudioSystem() {
        SetPriority(50); // Run audio at medium priority
    }

    void OnVariableUpdate(World &world, double delta) override;

    /**
     * @brief Queue a sound for playback
     * @param path Path to the audio file
     * @param volume Volume multiplier (0.0 to 1.0)
     */
    void PlaySound(const std::string &path, float volume = 1.0f) { m_audioQueue.push({path, volume, false}); }

    /**
     * @brief Queue a looping sound for playback
     * @param path Path to the audio file
     * @param volume Volume multiplier (0.0 to 1.0)
     */
    void PlayLoopingSound(const std::string &path, float volume = 1.0f) { m_audioQueue.push({path, volume, true}); }

private:
    std::queue<AudioEvent> m_audioQueue;
};

} // namespace Aeon
