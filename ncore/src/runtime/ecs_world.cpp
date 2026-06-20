#include <ncore/runtime/ecs_world.h>

// todo: move to cmake config
#include <flecs.h>

#include <ncore/modules/events/event_bus.h>
#include <ncore/runtime/service_locator.h>

#include <ncore/runtime/ecs/ecs_entity.h>
#include <ncore/utils/log.h>

namespace ncore {

struct EcsWorld::Impl {
    ecs_world_t *world;
    std::unordered_map<const rfl::TypeInfo *, ecs_id_t> comp_id_map;
    ecs_id_t comp_id_counter;
};

EcsWorld::EcsWorld(EventBus &event_bus, ServiceLocator &services) :
    pimpl(std::make_unique<Impl>()), services(services), event_bus(event_bus) {
    pimpl->world = ecs_init();
}

EcsWorld::~EcsWorld() {
    ecs_fini(pimpl->world);
    // TODO: handle failure
}

//------------------------------------------------------------------------------

void EcsWorld::on_init() {
    // Initialization logic here
}

void EcsWorld::on_fixed_update(double p_delta, uint64_t ticks) {
    // Fixed update logic here
}

void EcsWorld::on_variable_update(double p_delta) {
    // Variable update logic here
}

void EcsWorld::on_post_update(double p_delta) {
    // Post update logic here
}

void EcsWorld::on_finish() {
    // Finish logic here
}

//------------------------------------------------------------------------------

EcsEntity EcsWorld::create_entity() {
    ecs_entity_desc_t desc{.name = "Test"};
    ecs_entity_t f_ent = ecs_entity_init(pimpl->world, &desc);
    EcsEntity ent = static_cast<EcsEntity>(f_ent); // yes, we can just cast this
    NC_LOG_TRACE_C(log::ECS, "created entity: {}", ent);
    return ent;
}

void EcsWorld::destroy(EcsEntity entity) {
    ecs_delete(pimpl->world, entity);
    NC_LOG_TRACE_C(log::ECS, "destroyed entity: {}", entity);
}

void EcsWorld::component_set(EcsEntity eid, const rfl::TypeInfo *type, const void *data, size_t sz) {
    // TODO: register component type if not already

    NC_LOG_TRACE_C(log::ECS, "binding data of size {} on entity {}", sz, eid);

    auto it = pimpl->comp_id_map.find(type);
    if (it == pimpl->comp_id_map.end()) {
        ecs_id_t bump_id = pimpl->comp_id_counter++;
        ecs_type_info_t type_info{.size = static_cast<ecs_size_t>(sz),
                                  .alignment = static_cast<ecs_size_t>(type->alignment)};
        ecs_component_desc_t desc{.entity = eid, .type = type_info};
        auto init = ecs_component_init(pimpl->world, &desc);
        NC_ASSERT(init == 0, "failed to register component");
        pimpl->comp_id_map[type] = bump_id;
    }

    ecs_id_t comp_id = pimpl->comp_id_map[type];
    if (eid == INVALID_ENTITY_ID) {
        // means this is a singleton component
        eid = comp_id;
    }
    ecs_set_id(pimpl->world, eid, comp_id, sz, const_cast<void *>(data));
}

void *EcsWorld::component_get(EcsEntity eid, const rfl::TypeInfo *type) const {
    auto it = pimpl->comp_id_map.find(type);
    if (it == pimpl->comp_id_map.end()) {
        return nullptr;
    }
    return const_cast<void *>(ecs_get_id(pimpl->world, eid, it->second));
}

bool EcsWorld::component_has(EcsEntity eid, const rfl::TypeInfo *type) const {
    auto it = pimpl->comp_id_map.find(type);
    if (it == pimpl->comp_id_map.end()) {
        return false;
    }
    return ecs_has_id(pimpl->world, eid, it->second) != 0;
}

//------------------------------------------------------------------------------

void EcsWorld::system_add(const rfl::TypeInfo *type, const void *data) {
    NC_LOG_TRACE_C(log::ECS, "added system '{}' to world", rfl::Registry::get_type_name(type->id));
}

void *EcsWorld::system_get(const rfl::TypeInfo *type) const { return nullptr; }

} // namespace ncore
