#include <ncore/modules/ecs/ecs_query.h>

#include <flecs.h>

#include <ncore/modules/ecs/ecs_world.h>
#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>

namespace ncore {

//------------------------------------------------------------------------------
// EcsQuery
//------------------------------------------------------------------------------

EcsQuery::EcsQuery(EcsWorld *world_ref, void *world_handle, void *query_handle) :
    world_ref_(world_ref), world_(world_handle), query_(query_handle) {}

EcsQuery::Iterator EcsQuery::begin() { return Iterator(world_ref_, world_, query_); }

//------------------------------------------------------------------------------
// EcsQuery::Iterator
//------------------------------------------------------------------------------

EcsQuery::Iterator::Iterator(EcsWorld *, void *world, void *query) : world_(world), query_(query) {
    auto *w = static_cast<ecs_world_t *>(world_);
    auto *q = static_cast<ecs_query_t *>(query_);

    iter_ = new ecs_iter_t;
    *static_cast<ecs_iter_t *>(iter_) = ecs_query_iter(w, q);

    // advance to first table
    done_ = !ecs_query_next(static_cast<ecs_iter_t *>(iter_));
    if (!done_) {
        wrapper_.set_iter_(iter_);
    }
}

EcsQuery::Iterator::~Iterator() { delete static_cast<ecs_iter_t *>(iter_); }

EcsQuery::Iterator::Iterator(Iterator &&other) noexcept :
    world_(other.world_), query_(other.query_), iter_(other.iter_), wrapper_(std::move(other.wrapper_)),
    done_(other.done_) {
    other.iter_ = nullptr;
    other.done_ = true;
}

EcsQuery::Iterator &EcsQuery::Iterator::operator=(Iterator &&other) noexcept {
    if (this != &other) {
        delete static_cast<ecs_iter_t *>(iter_);
        world_ = other.world_;
        query_ = other.query_;
        iter_ = other.iter_;
        wrapper_ = std::move(other.wrapper_);
        done_ = other.done_;
        other.iter_ = nullptr;
        other.done_ = true;
    }
    return *this;
}

bool EcsQuery::Iterator::operator!=(std::nullptr_t) const { return !done_; }

EcsQuery::Iterator &EcsQuery::Iterator::operator++() {
    done_ = !ecs_query_next(static_cast<ecs_iter_t *>(iter_));
    if (!done_) {
        wrapper_.set_iter_(iter_);
    }
    return *this;
}

EcsIter &EcsQuery::Iterator::operator*() { return wrapper_; }

//------------------------------------------------------------------------------
// EcsQueryBuilder
//------------------------------------------------------------------------------

struct EcsQueryBuilder::Impl {
    EcsWorld &world;
    std::string name;
    std::string expr;
    std::vector<ecs_term_t> terms;
    bool built = false;

    Impl(EcsWorld &world, std::string name) : world(world), name(std::move(name)) {}
};

EcsQueryBuilder::EcsQueryBuilder(EcsWorld &world, std::string name) :
    pImpl(std::make_unique<Impl>(world, std::move(name))) {}

EcsQueryBuilder::~EcsQueryBuilder() {
    if (!pImpl->built) {
        NC_LOG_WARN_C(log::ECS, "EcsQueryBuilder '{}' discarded without build", pImpl->name);
    }
}

void EcsQueryBuilder::add_term_impl(const rfl::TypeInfo *type, uint8_t inout) {
    NC_ASSERT(type, "component type not registered in rfl::Registry");
    EcsComponentId comp_id = pImpl->world.register_component_type(type);

    ecs_term_t term{};
    term.id = comp_id;
    term.inout = (inout == 0) ? EcsInOutDefault : EcsIn;
    pImpl->terms.push_back(term);
}

EcsQuery EcsQueryBuilder::build() {
    size_t term_count = pImpl->terms.size();
    NC_ASSERT(term_count > 0,
              std::format("query '{}' has no terms! use .with<T>() or .read<T>() before .build()", pImpl->name).data());
    NC_ASSERT(term_count <= FLECS_TERM_COUNT_MAX, std::format("too many query terms (%zu)", term_count).data());

    ecs_query_desc_t qdesc{};
    memcpy(qdesc.terms, pImpl->terms.data(), term_count * sizeof(ecs_term_t));

    return pImpl->world.create_query_(pImpl->name, &qdesc);
}

} // namespace ncore
