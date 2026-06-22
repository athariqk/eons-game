#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <ncore/kernel/types.h>
#include <ncore/modules/ecs/ecs_entity.h>
#include <ncore/modules/ecs/ecs_system.h>

namespace ncore {

class EcsWorld;
class EcsQueryBuilder;

/**
 * @brief A lightweight, non-owning handle to a flecs query owned by the world.
 *
 * Queries are created through EcsQueryBuilder and cached inside EcsWorld.
 * Supports range-for iteration.
 */
class EcsQuery {
public:
    class Iterator;

    EcsQuery() = default;

    Iterator begin();
    static std::nullptr_t end() { return nullptr; }

private:
    friend class EcsWorld;
    friend class EcsQueryBuilder;

    EcsQuery(EcsWorld *world_ref, void *world_handle, void *query_handle);

    // internal impl details
    EcsWorld *world_ref_ = nullptr;
    void *world_ = nullptr;
    void *query_ = nullptr;
};

//------------------------------------------------------------------------------

class EcsQuery::Iterator {
public:
    ~Iterator();

    Iterator(Iterator &&) noexcept;
    Iterator &operator=(Iterator &&) noexcept;
    Iterator(const Iterator &) = delete;
    Iterator &operator=(const Iterator &) = delete;

    bool operator!=(std::nullptr_t) const;
    Iterator &operator++();
    EcsIter &operator*();

private:
    friend class EcsQuery;
    Iterator(EcsWorld *world_ref, void *world, void *query);

    // internal impl details
    void *world_ = nullptr;
    void *query_ = nullptr;
    void *iter_ = nullptr;
    EcsIter wrapper_{nullptr};
    bool done_ = true;
};

//------------------------------------------------------------------------------

class EcsQueryBuilder {
public:
    EcsQueryBuilder(EcsWorld &world, std::string name);
    ~EcsQueryBuilder();

    template<typename... Comps>
    EcsQueryBuilder &with() {
        (add_term_<Comps>(0), ...);
        return *this;
    }

    template<typename... Comps>
    EcsQueryBuilder &read() {
        (add_term_<Comps>(1), ...);
        return *this;
    }

    /**
     * @brief Finalise and build the query. The query is owned by the world;
     * the returned EcsQuery is a lightweight handle.
     */
    EcsQuery build();

private:
    template<typename T>
    void add_term_(uint8_t inout) {
        add_term_impl(rfl::Registry::find<T>(), inout);
    }
    void add_term_impl(const rfl::TypeInfo *type, uint8_t inout);

    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace ncore
