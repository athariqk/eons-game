#include <modules/ecs/ecs_world.h>

#include <cstdint>
#include <memory>

// todo: move to cmake config
#include <flecs.h>
#include <flecs/addons/cpp/flecs.hpp>

#include <modules/ecs/ecs_entity.h>
#include <utils/macro.h>

namespace ncore {

struct EcsWorld::Impl {
    ecs_world_t *world;
    std::unordered_map<rfl::TypeId, ecs_id_t> comp_id_map;
    ecs_id_t comp_id_counter;
};

EcsWorld::EcsWorld() : pimpl(std::make_unique<Impl>()) { pimpl->world = ecs_init(); }

EcsWorld::~EcsWorld() {
    ecs_fini(pimpl->world);
    // TODO: handle failure
}

//------------------------------------------------------------------------------

EcsEntity EcsWorld::create_entity() {
    ecs_entity_t f_ent = ecs_entity_init(pimpl->world, nullptr);
    EcsEntity ent = static_cast<EcsEntity>(f_ent); // yes, we can just cast this
    NC_LOG_TRACE_C(log::ECS, "created entity: {}", ent);
    return ent;
}

void EcsWorld::destroy(EcsEntity entity) {
    ecs_delete(pimpl->world, entity);
    NC_LOG_TRACE_C(log::ECS, "destroyed entity: {}", entity);
}

void EcsWorld::component_set(EcsEntity eid, rfl::TypeId tid, const void *data, size_t sz) {
    // TODO: register component type if not already

    if (eid == INVALID_ENTITY_ID) {
        // is a component type
        ecs_type_info_t type_info{.size = static_cast<ecs_size_t>(sz)};
        ecs_component_desc_t desc{.entity = eid, .type = type_info};
        ecs_component_init(pimpl->world, &desc);
    }

    auto it = pimpl->comp_id_map.find(tid);
    if (it == pimpl->comp_id_map.end()) {
        ecs_id_t bump_id = pimpl->comp_id_counter++;
        pimpl->comp_id_map[tid] = bump_id;
        it->second = bump_id;
    }
    ecs_set_id(pimpl->world, eid, it->second, sz, const_cast<void *>(data));
}

void *EcsWorld::component_get(EcsEntity eid, rfl::TypeId tid) const {
    return const_cast<void *>(ecs_get_id(pimpl->world, eid, static_cast<ecs_id_t>(tid.value)));
}

bool EcsWorld::component_has(EcsEntity eid, rfl::TypeId tid) const {
    return ecs_has_id(pimpl->world, eid, static_cast<ecs_id_t>(tid.value)) != 0;
}

//------------------------------------------------------------------------------

void EcsWorld::system_add(rfl::TypeId tid, const void *data) {
    NC_LOG_TRACE_C(log::ECS, "added system '{}' to world", rfl::Registry::get_class_name(tid));
}

void *EcsWorld::system_get(rfl::TypeId tid) const { return nullptr; }

//------------------------------------------------------------------------------

void EcsWorld::on_init() {}

void EcsWorld::on_fixed_update(double p_delta, uint64_t ticks) {}

void EcsWorld::on_variable_update(double p_delta) {}

void EcsWorld::on_post_update(double p_delta) {}

void EcsWorld::on_finish() {}

} // namespace ncore
