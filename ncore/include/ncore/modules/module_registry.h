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
     * IModule's inheritance hierarchy. Resolved modules are cached
     * to avoid future RTTI hits.
     *
     * TODO: add docs explaining implementation details and caveats
     *
     * @return The **first** matching instance if multiple modules are
     * found, or nullptr if no matching module is found.
     */
    template<std::derived_from<IModule> T>
    T* resolve()
    {
        rtti::TypeId target = rtti::TypeRegistry::get_type_id<T>();

        auto it = cache_by_id.find( target );
        if (it != cache_by_id.end()) {
            return static_cast<T*>( it->second );
        }

        for (auto& [id, m] : modules) {
            if (id == target) {
                cache_by_id[target] = m.get();
                return static_cast<T*>( m.get() );
            }
        }

        for (auto& [id, m] : modules) {
            if (m->is_a( target )) {
                cache_by_id[target] = m.get();
                return static_cast<T*>( m.get() );
            }
        }

        auto class_name = rtti::TypeRegistry::get<T>().name;
        NC_ASSERT_RETVAL( false, nullptr, std::format( "Module '{}' could not be resolved", class_name ).c_str() );
    }

    /**
     * @brief Registers a module instance of the given type with
     * the provided constructor arguments.
     *
     * NOTE: modules are ordered by insertion. This is an important
     * property for cross-module dependency and in-order initialization
     * if using init_all().
     */
    template<std::derived_from<IModule> T, typename... Args>
    T* provide( Args&&... args )
    {
        cache_by_id.clear();
        cache_by_name.clear();

        rtti::TypeId id = rtti::TypeRegistry::get_type_id<T>();

        for (auto& [existing_id, m] : modules)
            NC_ASSERT( existing_id != id, "Module already registered" );

        auto instance = std::make_unique<T>( std::forward<Args>( args )... );
        T* ptr        = instance.get();
        modules.emplace_back( id, std::move( instance ) );
        return ptr;
    }

    IModule* resolve_by_name( std::string_view name )
    {
        auto it = cache_by_name.find( name );
        if (it != cache_by_name.end()) {
            return it->second;
        }

        for (auto& [id, m] : modules) {
            if (m->get_class_name() == name) {
                cache_by_name[name] = m.get();
                return m.get();
            }
        }

        NC_ASSERT_RETVAL( false, nullptr, std::format( "Module '{}' could not be resolved", name ).c_str() );
    }

    Error init_all( ConfFile& cfg_file )
    {
        for (auto& [_, m] : modules) {
            NC_LOG_DEBUG( "Initializing module: {}", m->get_class_name() );
            if (m->init( cfg_file ) != Error::OK) {
                NC_LOG_ERROR( "{} - module init FAIL", m->get_class_name() );
                return Error::FAIL;
            }
        }
        return Error::OK;
    }

    /**
     * @brief Finalizes all registered modules.
     */
    void cleanup_all()
    {
        for (auto& [_, m] : modules) {
            NC_LOG_DEBUG( "Finalizing module: {}", m->get_class_name() );
            m->finalize();
        }
    }

    /**
     * @brief Clears all registered modules from the registry.
     */
    void clear()
    {
        modules.clear();
        cache_by_id.clear();
        cache_by_name.clear();
    }

    auto view()
    {
        return std::views::transform( modules, []( auto& entry ) -> IModule* { return entry.second.get(); } );
    }

private:
    using ModuleEntry = std::pair<rtti::TypeId, std::unique_ptr<IModule>>;
    Vector<ModuleEntry> modules;
    std::unordered_map<rtti::TypeId, IModule*> cache_by_id;
    std::unordered_map<std::string_view, IModule*> cache_by_name;
};

} // namespace nc
