#include <ncore/modules/ecs/ecs_world.h>

// todo: move to cmake config
#include <flecs.h>
#include <flecs/addons/pipeline.h>
#include <flecs/addons/system.h>

#include <ncore/modules/ecs/ecs_entity.h>
#include <ncore/modules/events/event_bus.h>
#include <ncore/modules/service_locator.h>
#include <ncore/utils/log.h>

namespace ncore {

struct EcsWorld::Impl {
    ecs_world_t *world = nullptr;
    std::unordered_map<const rfl::TypeInfo *, EcsComponentId> comp_id_map;
    std::unordered_map<std::string, ecs_query_t *> query_cache;
};

EcsWorld::EcsWorld(ServiceLocator &services) : IGameWorld(services), pImpl(std::make_unique<Impl>()) {
    pImpl->world = ecs_init();
    ecs_set_binding_ctx(pImpl->world, this, nullptr);
}

EcsWorld::~EcsWorld() {}

//------------------------------------------------------------------------------

void EcsWorld::on_init() {
    auto event_bus = get_services().resolve<EventBus>();

    event_bus->subscribe<WindowCloseEvent>([this](WindowCloseEvent &) {
        wants_to_quit = true;
        NC_LOG_TRACE("window close event received, requesting exit...");
    });

    event_bus->subscribe<WindowResizeEvent>(
        [this](WindowResizeEvent &e) { NC_LOG_TRACE("window resolution changed: {}x{}", e.width, e.height); });
}

bool EcsWorld::on_fixed_update(double p_delta) {
    // Fixed timestep — TODO: run fixed-update pipeline
    return wants_to_quit;
}

bool EcsWorld::on_variable_update(double p_delta) {
    ecs_progress(pImpl->world, static_cast<float>(p_delta));
    return wants_to_quit;
}

void EcsWorld::on_finish() {
    ecs_fini(pImpl->world);
    NC_LOG_TRACE_C(log::ECS, "world finished");
}

//------------------------------------------------------------------------------
// Entities
//------------------------------------------------------------------------------

EcsEntityId EcsWorld::create_entity(std::string name) {
    ecs_entity_desc_t desc{.name = name.c_str()};
    ecs_entity_t f_ent = ecs_entity_init(pImpl->world, &desc);
    return static_cast<EcsEntityId>(f_ent);
}

std::span<EcsEntityId> EcsWorld::get_entities() const {
    ecs_entities_t ents = ecs_get_entities(pImpl->world);
    return std::span<EcsEntityId>(const_cast<EcsEntityId *>(ents.ids), ents.alive_count);
}

void EcsWorld::destroy(EcsEntityId entity) {
    ecs_delete(pImpl->world, entity);
    NC_LOG_TRACE_C(log::ECS, "destroyed entity: {}", entity);
}

//------------------------------------------------------------------------------
// Components
//------------------------------------------------------------------------------

EcsEntityId EcsWorld::set_component_(EcsEntityId eid, const rfl::TypeInfo *type, const void *data, size_t sz) {
    // IF entity ID is invalid, we can use the component ID as entity ID,
    // meaning it is a singleton entity
    NC_LOG_TRACE_C(log::ECS, "binding data of size {} on entity {}", sz, eid);
    EcsComponentId comp_id = register_component_type(type);
    if (eid == INVALID_ENTITY_ID) {
        eid = comp_id; // singleton
    }
    ecs_set_id(pImpl->world, eid, comp_id, sz, const_cast<void *>(data));
    return eid;
}

void *EcsWorld::get_component_(EcsComponentId eid, const rfl::TypeInfo *type) const {
    auto it = pImpl->comp_id_map.find(type);
    if (it == pImpl->comp_id_map.end())
        return nullptr;
    return const_cast<void *>(ecs_get_id(pImpl->world, eid, it->second));
}

bool EcsWorld::has_component_(EcsComponentId eid, const rfl::TypeInfo *type) const {
    auto it = pImpl->comp_id_map.find(type);
    if (it == pImpl->comp_id_map.end())
        return false;
    return ecs_has_id(pImpl->world, eid, it->second) != 0;
}

//------------------------------------------------------------------------------
// Systems & queries
//------------------------------------------------------------------------------

EcsSystemBuilder EcsWorld::system(std::string_view name) { return EcsSystemBuilder(*this, std::string(name)); }

EcsQueryBuilder EcsWorld::query(std::string_view name) { return EcsQueryBuilder(*this, std::string(name)); }

EcsComponentId EcsWorld::register_component_type(const rfl::TypeInfo *type) {
    auto it = pImpl->comp_id_map.find(type);
    if (it != pImpl->comp_id_map.end())
        return static_cast<EcsComponentId>(it->second); // returns existing

    // auto-register
    ecs_type_info_t type_info{.size = static_cast<ecs_size_t>(type->size),
                              .alignment = static_cast<ecs_size_t>(type->alignment),
                              .name = type->name};
    ecs_component_desc_t desc{.entity = 0, .type = type_info};
    auto comp_id = ecs_component_init(pImpl->world, &desc);
    NC_ASSERT(comp_id != 0, std::format("failed to auto-register component '{}'", type->name).data());
    pImpl->comp_id_map[type] = comp_id;
    return static_cast<EcsComponentId>(comp_id);
}

void *EcsWorld::ecs_world_handle_() const { return pImpl->world; }

EcsQuery EcsWorld::create_query_(const std::string &name, void *qdesc_ptr) {
    auto *qdesc = static_cast<ecs_query_desc_t *>(qdesc_ptr);

    auto &cached = pImpl->query_cache[name];
    if (cached) {
        NC_LOG_TRACE_C(log::ECS, "reusing cached query '{}'", name);
        return EcsQuery(this, pImpl->world, cached);
    }

    ecs_query_t *q = ecs_query_init(pImpl->world, qdesc);
    NC_ASSERT(q, std::format("failed to create query '{}'", name).data());
    cached = q;
    NC_LOG_TRACE_C(log::ECS, "created query '{}'", name);
    return EcsQuery(this, pImpl->world, q);
}

} // namespace ncore
