#pragma once

#include <format>
#include <memory>
#include <ranges>
#include <vector>

#include <ncore/kernel/collection.h>
#include <ncore/kernel/types.h>
#include <ncore/modules/module.h>
#include <ncore/utils/assert.h>

namespace nc {

/**
 * @brief ModuleRegistry implements the service locator pattern for
 * managing "singleton" IModule classes.
 */
class NCORE_API ModuleRegistry {
public:
    ModuleRegistry()                                   = default;
    ModuleRegistry( const ModuleRegistry& )            = delete;
    ModuleRegistry& operator=( const ModuleRegistry& ) = delete;

    /**
     * @brief Resolves the concrete module object of the given type.
     *
     * Type argument can be either the exact type or any types in
     * IModule's inheritance hierarchy.
     *
     * TODO: add docs explaining implementation details and caveats
     *
     * TODO: cache resolved types?
     *
     * @return The **first** matching instance if multiple modules are
     * found, or nullptr if no matching module is found.
     */
    template<std::derived_from<IModule> T>
    T* resolve()
    {
        rfl::TypeId target = rfl::Registry::get_type_id<T>();

        for (auto& [id, svc] : modules) {
            if (id == target)
                return static_cast<T*>( svc.get() );
        }

        for (auto& [id, svc] : modules) {
            if (svc->is_a( target ))
                return static_cast<T*>( svc.get() );
        }

        auto class_name = rfl::Registry::get<T>().name;
        NC_ASSERT_RETVAL( false, nullptr, std::format( "Module '{}' could not be resolved", class_name ).c_str() );
    }

    /**
     * @brief Registers a module instance of the given type with
     * the provided constructor arguments.
     */
    template<std::derived_from<IModule> T, typename... Args>
    T* provide( Args&&... args )
    {
        rfl::TypeId id = rfl::Registry::get_type_id<T>();

        for (auto& [existing_id, svc] : modules)
            NC_ASSERT( existing_id != id, "Module already registered" );

        auto instance = std::make_unique<T>( std::forward<Args>( args )... );
        T* ptr        = instance.get();
        modules.emplace_back( id, std::move( instance ) );
        return ptr;
    }

    IModule* resolve_by_name( std::string_view name )
    {
        for (auto& [id, svc] : modules)
            if (svc->get_class_name() == name)
                return svc.get();
        NC_ASSERT_RETVAL( false, nullptr, std::format( "Module '{}' could not be resolved", name ).c_str() );
    }

    Error init_all()
    {
        for (auto& [_, module] : modules) {
            NC_LOG_DEBUG( "Initializing module: {}", module->get_class_name() );
            if (module->init() != Error::OK)
                return Error::FAIL;
        }
        return Error::OK;
    }

    /**
     * @brief Finalizes all registered modules.
     */
    void cleanup_all()
    {
        for (auto& [_, module] : modules) {
            NC_LOG_DEBUG( "Finalizing module: {}", module->get_class_name() );
            module->finalize();
        }
    }

    /**
     * @brief Clears all registered modules from the registry.
     */
    void clear()
    {
        modules.clear();
    }

    auto view()
    {
        return std::views::transform( modules, []( auto& entry ) -> IModule* { return entry.second.get(); } );
    }

private:
    using ModuleEntry = std::pair<rfl::TypeId, std::unique_ptr<IModule>>;
    Vector<ModuleEntry> modules;
};

} // namespace nc
