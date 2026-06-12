#pragma once

#include <memory>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>

#include <modules/utils/Logger.h>

namespace ncore {

/**
 * @brief Service container for engine subsystems
 *
 * This is a type-safe service locator that stores engine subsystems
 * (rendering, audio, input, etc.) and makes them available to systems.
 *
 * Usage:
 *   // Register services during engine init
 *   services.add<AudioManager>(audioManager);
 *   services.add<Viewport2D>(viewport);
 *
 *   // Access in systems
 *   auto &audio = world.get_services().get<AudioManager>();
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
    void add(T &service) {
        service_reg[std::type_index(typeid(T))] = &service;
    }

    /**
     * @brief Get a registered service
     * @tparam T Service type
     * @return Reference to the service
     * @throws std::runtime_error if service not found
     */
    template<typename T>
    T &get() {
        auto it = service_reg.find(std::type_index(typeid(T)));
        if (it == service_reg.end()) {
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
    bool has() const {
        return service_reg.find(std::type_index(typeid(T))) != service_reg.end();
    }

    /**
     * @brief Try to get a service, returns nullptr if not found
     * @tparam T Service type
     * @return Pointer to service or nullptr
     */
    template<typename T>
    T *try_get() {
        auto it = service_reg.find(std::type_index(typeid(T)));
        return it != service_reg.end() ? static_cast<T *>(it->second) : nullptr;
    }

private:
    std::unordered_map<std::type_index, void *> service_reg;
};

} // namespace ncore
