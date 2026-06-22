#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <ncore/kernel/types.h>
#include <ncore/modules/ecs/ecs_entity.h>

namespace ncore {

class EcsWorld;

/**
 * @brief Defines which pipeline phase a system runs in.
 */
enum class EcsSystemPhase {
    Init,
    PreUpdate,
    FixedUpdate,
    Update,
    PostUpdate,
};

class EcsIter;

/**
 * @brief A system callback is a stateless function receiving the
 * current iteration state.
 */
using SystemCallback = void (*)(EcsIter &);

class EcsIter {
public:
    double delta_time() const;
    float delta_system_time() const;
    int32_t count() const;
    EcsEntityId entity(int32_t row) const;
    EcsWorld &world() const;

    /**
     * @brief Typed column access. Column indices are 1-based in the order
     * terms were added to the builder.
     */
    template<typename T>
    T *field(int32_t column) {
        auto &info = rfl::Registry::get<T>();
        return static_cast<T *>(field_(column, info.size, info.alignment));
    }

    /**
     * @brief Gets a single element from a typed column at a given row.
     */
    template<typename T>
    T &get(int32_t column, int32_t row) {
        return field<T>(column)[row];
    }

private:
    friend class EcsSystemBuilder;
    friend class EcsQuery;

    // iterator impl details
    explicit EcsIter(void *iter);
    void set_iter_(void *iter);
    void *field_(int32_t column, size_t size, size_t alignment) const;
    void *it_ = nullptr;
};

/**
 * @brief EcsSystemBuilder is fluent API for registering ECS systems
 */
class EcsSystemBuilder {
public:
    EcsSystemBuilder(EcsWorld &world, std::string name);
    ~EcsSystemBuilder();

    /**
     * @brief Add read-write component terms.
     */
    template<typename... Comps>
    EcsSystemBuilder &with() {
        (add_term_<Comps>(0), ...);
        return *this;
    }

    /**
     * @brief Add read-only component terms.
     */
    template<typename... Comps>
    EcsSystemBuilder &read() {
        (add_term_<Comps>(1), ...);
        return *this;
    }

    EcsSystemBuilder &in(EcsSystemPhase phase);
    EcsSystemBuilder &order(int32_t priority);

    /**
     * @brief Finalise and register the system with the given callback.
     * @return The entity handle of the created system.
     */
    EcsEntityId iter(SystemCallback callback);

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
