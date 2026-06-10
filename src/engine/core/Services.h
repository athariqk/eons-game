#pragma once

#include <memory>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>

#include <Logger.h>

namespace Aeon {

/**
 * @brief Service container for engine subsystems
 * 
 * This is a type-safe service locator that stores engine subsystems
 * (rendering, audio, input, etc.) and makes them available to systems.
 * 
 * Usage:
 *   // Register services during engine init
 *   services.Register<AudioManager>(audioManager);
 *   services.Register<Viewport2D>(viewport);
 *   
 *   // Access in systems
 *   auto &audio = world.GetServices().Get<AudioManager>();
 *   audio.PlaySound("boom.wav");
 */
class Services {
public:
    /**
     * @brief Register a service by reference
     * @tparam T Service type
     * @param service Reference to the service instance
     */
    template<typename T>
    void Register(T &service) {
        LOG_TRACE(Log::Engine, "Registering service: {}", typeid(T).name());
        m_services[std::type_index(typeid(T))] = &service;
    }

    /**
     * @brief Get a registered service
     * @tparam T Service type
     * @return Reference to the service
     * @throws std::runtime_error if service not found
     */
    template<typename T>
    T &Get() {
        auto it = m_services.find(std::type_index(typeid(T)));
        if (it == m_services.end()) {
            throw std::runtime_error("Service not registered");
        }
        return *static_cast<T *>(it->second);
    }

    /**
     * @brief Check if a service is registered
     * @tparam T Service type
     * @return true if service exists
     */
    template<typename T>
    bool Has() const {
        return m_services.find(std::type_index(typeid(T))) != m_services.end();
    }

    /**
     * @brief Try to get a service, returns nullptr if not found
     * @tparam T Service type
     * @return Pointer to service or nullptr
     */
    template<typename T>
    T *TryGet() {
        auto it = m_services.find(std::type_index(typeid(T)));
        return it != m_services.end() ? static_cast<T *>(it->second) : nullptr;
    }

private:
    std::unordered_map<std::type_index, void *> m_services;
};

} // namespace Aeon
