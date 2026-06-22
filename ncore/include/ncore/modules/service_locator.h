#pragma once

#include <format>
#include <ranges>
#include <unordered_map>

#include <ncore/kernel/types.h>
#include <ncore/modules/service.h>
#include <ncore/utils/assert.h>

namespace ncore {

/**
 * @brief Implements the service locator pattern for managing
 * global services of abstracted types.
 */
class ServiceLocator {
public:
    ServiceLocator(ServiceLocator const &) = delete;
    void operator=(ServiceLocator const &) = delete;

    static ServiceLocator &get_instance() {
        static ServiceLocator instance;
        return instance;
    }

    /**
     * @brief Resolves the concrete service object of the given type.
     *
     * Type argument can be either the exact type or any types in
     * IService's inheritance hierarchy.
     *
     * TODO: add docs explaining implementation details and caveats
     *
     * @return The **first** matching instance if multiple services are
     * found, or nullptr if no matching service is found.
     */
    template<std::derived_from<IService> T>
    static T *resolve() {
        auto target = rfl::Registry::get_type_id<T>();

        auto it = get_instance().services.find(target);
        if (it != get_instance().services.end())
            return static_cast<T *>(it->second.get());

        for (auto &[key, svc]: get_instance().services) {
            if (svc->is_a(target))
                return static_cast<T *>(svc.get());
        }

        auto class_name = rfl::Registry::get<T>().name;
        NC_ASSERT_RETVAL(false, nullptr, std::format("Service '{}' could not be resolved", class_name).c_str());
    }

    /**
     * @brief Registers a service instance of the given type with
     * the provided constructor arguments.
     */
    template<std::derived_from<IService> T, typename... Args>
    static T *provide(Args &&...args) {
        NC_ASSERT(!get_instance().services.contains(rfl::Registry::get_type_id<T>()), "Service already registered");
        auto instance = std::make_shared<T>(std::forward<Args>(args)...);
        get_instance().services[rfl::Registry::get_type_id<T>()] = instance;
        return instance.get();
    }

    static IService &resolve_by_name(std::string_view name) {
        for (auto &[key, svc]: get_instance().services)
            if (svc->get_class_name() == name)
                return *svc;
        NC_ASSERT(false, std::format("Service '{}' could not be resolved", name).c_str());
    }

    static Error init_all() {
        for (auto &[key, svc]: get_instance().services) {
            if (svc->init() != Error::OK)
                return Error::FAIL;
        }
        return Error::OK;
    }

    static void cleanup_all() {
        for (auto &[key, svc]: get_instance().services)
            svc->finalize();
    }

    static auto view() { return std::views::values(get_instance().services); }

private:
    ServiceLocator() = default;

    // TODO: replace shared_ptr with something else...
    std::unordered_map<rfl::TypeId, std::shared_ptr<IService>> services;
};

} // namespace ncore
