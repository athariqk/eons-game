#include <ncore/modules/ecs/ecs_system.h>

#include <flecs.h>
#include <flecs/addons/pipeline.h>
#include <flecs/addons/system.h>

#include <ncore/modules/ecs/ecs_world.h>
#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>

namespace ncore {

//------------------------------------------------------------------------------
// EcsIter
//------------------------------------------------------------------------------

EcsIter::EcsIter(void *iter) : it_(iter) {}

void EcsIter::set_iter_(void *iter) { it_ = iter; }

double EcsIter::delta_time() const {
    auto *it = static_cast<ecs_iter_t *>(it_);
    return static_cast<double>(it->delta_time);
}

float EcsIter::delta_system_time() const {
    auto *it = static_cast<ecs_iter_t *>(it_);
    return it->delta_system_time;
}

int32_t EcsIter::count() const {
    auto *it = static_cast<ecs_iter_t *>(it_);
    return static_cast<int32_t>(it->count);
}

EcsEntityId EcsIter::entity(int32_t row) const {
    auto *it = static_cast<ecs_iter_t *>(it_);
    return static_cast<EcsEntityId>(it->entities[row]);
}

EcsWorld &EcsIter::world() const {
    auto *it = static_cast<ecs_iter_t *>(it_);
    return *static_cast<EcsWorld *>(ecs_get_binding_ctx(it->world));
}

void *EcsIter::field_(int32_t column, size_t size, size_t /*alignment*/) const {
    auto *it = static_cast<ecs_iter_t *>(it_);
    // ecs_field_w_size uses 0-based indices; public API is 1-based
    return ecs_field_w_size(it, size, static_cast<int8_t>(column - 1));
}

//------------------------------------------------------------------------------
// EcsSystemBuilder
//------------------------------------------------------------------------------

struct EcsSystemBuilder::Impl {
    EcsWorld &world;
    std::string name;
    EcsSystemPhase phase = EcsSystemPhase::Update;
    int32_t order = 0;
    std::vector<ecs_term_t> terms;
    bool built = false;

    Impl(EcsWorld &world, std::string name) : world(world), name(std::move(name)) {}
};

EcsSystemBuilder::EcsSystemBuilder(EcsWorld &world, std::string name) :
    pImpl(std::make_unique<Impl>(world, std::move(name))) {}

EcsSystemBuilder::~EcsSystemBuilder() {
    if (!pImpl->built) {
        NC_LOG_WARN_C(log::ECS, "EcsSystemBuilder '{}' discarded without build", pImpl->name);
    }
}

void EcsSystemBuilder::add_term_impl(const rfl::TypeInfo *type, uint8_t inout) {
    NC_ASSERT(type, "reflected component type missing");
    uint64_t comp_id = pImpl->world.register_component_type(type);

    ecs_term_t term{};
    term.id = comp_id;
    term.inout = (inout == 0) ? EcsInOutDefault : EcsIn;
    pImpl->terms.push_back(term);
}

EcsSystemBuilder &EcsSystemBuilder::in(EcsSystemPhase phase) {
    pImpl->phase = phase;
    return *this;
}

EcsSystemBuilder &EcsSystemBuilder::order(int32_t priority) {
    pImpl->order = priority;
    return *this;
}

namespace {

ecs_entity_t map_phase(EcsSystemPhase p) {
    switch (p) {
        case EcsSystemPhase::Init:
            return EcsOnStart;
        case EcsSystemPhase::PreUpdate:
            return EcsPreUpdate;
        case EcsSystemPhase::Update:
            return EcsOnUpdate;
        case EcsSystemPhase::PostUpdate:
            return EcsPostUpdate;
        case EcsSystemPhase::FixedUpdate:
            return EcsOnUpdate; // TODO: custom pipeline
        default:
            NC_ASSERT(false, "unknown system phase");
            return 0;
    }
}

} // namespace

EcsEntityId EcsSystemBuilder::iter(SystemCallback callback) {
    NC_ASSERT(callback, "system callback is null");

    auto *world = static_cast<ecs_world_t *>(pImpl->world.ecs_world_handle_());

    // assemble query descriptor
    ecs_query_desc_t qdesc{};
    size_t term_count = pImpl->terms.size();
    NC_ASSERT(term_count <= FLECS_TERM_COUNT_MAX, std::format("too many query terms ({})", term_count).c_str());
    memcpy(qdesc.terms, pImpl->terms.data(), term_count * sizeof(ecs_term_t));

    // build system descriptor
    ecs_entity_desc_t sys_ent_desc{.name = pImpl->name.c_str()};
    ecs_entity_t sys_ent = ecs_entity_init(world, &sys_ent_desc);
    ecs_system_desc_t sdesc{};
    sdesc.entity = sys_ent;
    sdesc.query = qdesc;
    sdesc.phase = map_phase(pImpl->phase);
    sdesc.callback = [](ecs_iter_t *it) {
        auto fn = static_cast<SystemCallback>(it->param);
        EcsIter wrap(it);
        fn(wrap);
    };
    sdesc.ctx = reinterpret_cast<void *>(callback);

    ecs_entity_t id = ecs_system_init(world, &sdesc);
    NC_ASSERT(id != 0, std::format("failed to register ECS system '{}'", pImpl->name).c_str());

    // TODO: apply ordering once custom pipelines are implemented
    (void) pImpl->order;

    pImpl->built = true;
    NC_LOG_TRACE_C(log::ECS, "registered system '{}' (entity {})", pImpl->name, id);
    return static_cast<EcsEntityId>(id);
}

} // namespace ncore
