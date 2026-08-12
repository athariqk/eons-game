#pragma once

#include <format>
#include <memory>
#include <ranges>
#include <vector>

#include <ncore/core/collection.h>
#include <ncore/core/types.h>
#include <ncore/services/service.h>
#include <ncore/utils/assert.h>

namespace nc {

/**
 * @brief ServiceRegistry implements the service locator pattern for
 * managing "singleton" IService classes.
 */
class NCAPI ServiceRegistry {
public:
    ServiceRegistry()                                    = default;
    ServiceRegistry( const ServiceRegistry& )            = delete;
    ServiceRegistry& operator=( const ServiceRegistry& ) = delete;

    /**
     * @brief Resolves the concrete service object of the given type.
     *
     * Type argument can be either the exact type or any types in
     * IService's inheritance hierarchy. Resolved services are cached
     * to avoid future RTTI hits.
     *
     * TODO: add docs explaining implementation details and caveats
     *
     * @return The **first** matching instance if multiple services are
     * found, or nullptr if no matching service is found.
     */
    template<std::derived_from<IService> T>
    T* resolve()
    {
        rtti::TypeId target = rtti::TypeRegistry::get_type_id<T>();

        auto it = cache_by_id.find( target );
        if (it != cache_by_id.end()) {
            return static_cast<T*>( it->second );
        }

        for (auto& [id, s] : services) {
            if (id == target) {
                cache_by_id[target] = s.get();
                return static_cast<T*>( s.get() );
            }
        }

        for (auto& [id, s] : services) {
            if (s->is_a( target )) {
                cache_by_id[target] = s.get();
                return static_cast<T*>( s.get() );
            }
        }

        auto class_name = rtti::TypeRegistry::get<T>().name;
        NC_FAIL_MSG_RETVAL( false, nullptr, std::format( "Service '{}' could not be resolved", class_name ).c_str() );
    }

    /**
     * @brief Registers a service instance of the given type with
     * the provided constructor arguments.
     *
     * NOTE: services are ordered by insertion. This is an important
     * property for cross-service dependency and in-order initialization
     * if using init_all().
     */
    template<std::derived_from<IService> T, typename... Args>
    T* provide( Args&&... args )
    {
        cache_by_id.clear();
        cache_by_name.clear();

        rtti::TypeId id = rtti::TypeRegistry::get_type_id<T>();

        for (auto& [existing_id, s] : services)
            NC_ASSERT( existing_id != id, "Service already registered" );

        auto instance = std::make_unique<T>( std::forward<Args>( args )... );
        T* ptr        = instance.get();
        services.emplace_back( id, std::move( instance ) );
        return ptr;
    }

    IService* resolve_by_name( std::string_view name )
    {
        auto it = cache_by_name.find( name );
        if (it != cache_by_name.end()) {
            return it->second;
        }

        for (auto& [id, s] : services) {
            if (s->get_class_name() == name) {
                cache_by_name[name] = s.get();
                return s.get();
            }
        }

        NC_FAIL_MSG_RETVAL( false, nullptr, std::format( "Service '{}' could not be resolved", name ).c_str() );
    }

    Error init_all( ConfFile& cfg_file )
    {
        for (auto& [_, s] : services) {
            NC_LOG_DEBUG( "Initializing service: {}", s->get_class_name() );
            if (s->init( cfg_file ) != Error::OK) {
                NC_LOG_ERROR( "{} - service init FAIL", s->get_class_name() );
                return Error::FAIL;
            }
        }
        return Error::OK;
    }

    /**
     * @brief Finalizes all registered services.
     */
    void cleanup_all()
    {
        for (auto& [_, s] : services) {
            NC_LOG_DEBUG( "Finalizing service: {}", s->get_class_name() );
            s->shutdown();
        }
    }

    /**
     * @brief Clears all registered services from the registry.
     */
    void clear()
    {
        services.clear();
        cache_by_id.clear();
        cache_by_name.clear();
    }

    auto view()
    {
        return std::views::transform( services, []( auto& entry ) -> IService* { return entry.second.get(); } );
    }

private:
    using ServiceEntry = std::pair<rtti::TypeId, std::unique_ptr<IService>>;
    DynamicArray<ServiceEntry> services;
    std::unordered_map<rtti::TypeId, IService*> cache_by_id;
    std::unordered_map<std::string_view, IService*> cache_by_name;
};

} // namespace nc
