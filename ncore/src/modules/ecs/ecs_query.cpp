#include <ncore/modules/ecs/ecs_query.h>

#include <flecs.h>

#include <ncore/modules/ecs/ecs_world.h>
#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>

#include "flecs_helpers.h"

namespace ncore {

//------------------------------------------------------------------------------
// EcsQuery
//------------------------------------------------------------------------------

EcsQuery::EcsQuery(EcsWorld *world_ref, void *world_handle, void *query_handle) :
    world_ref_(world_ref), world_(world_handle), query_(query_handle) {}

EcsQuery::Iterator EcsQuery::begin() { return Iterator(world_ref_, world_, query_); }

//------------------------------------------------------------------------------
// EcsIter
//------------------------------------------------------------------------------

EcsIter::EcsIter(void *iter) : it_(iter) {}

void EcsIter::set_iter_(void *iter) { it_ = iter; }

double EcsIter::delta_time() const {
    auto *it = static_cast<ecs_iter_t *>(it_);
    return static_cast<double>(it->delta_time);
}

float EcsIter::delta_time_internal() const {
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

void *EcsIter::get_component_(int32_t column, size_t size, size_t alignment) const {
    (void) alignment;
    auto *it = static_cast<ecs_iter_t *>(it_);
    return ecs_field_w_size(it, size, static_cast<int8_t>(column));
}

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

struct EcsQueryBuilder::Impl : public detail::FlecsQueryBuilder {
    // inherit constructor so we can instantiate this in EcsQueryBuilder
    using FlecsQueryBuilder::FlecsQueryBuilder;
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

void EcsQueryBuilder::add_term_pair_impl(const rfl::TypeInfo *first_type, const rfl::TypeInfo *sec_type,
                                         uint8_t inout) {
    NC_ASSERT(first_type && sec_type, "pair component types not registered");
    EcsComponentId first_id = pImpl->world.register_component_type(first_type);
    EcsComponentId second_id = pImpl->world.register_component_type(sec_type);

    ecs_term_t term{};
    term.id = ecs_make_pair(first_id, second_id);
    term.inout = (inout == 0) ? EcsInOutDefault : EcsIn;
    pImpl->terms.push_back(term);
}

EcsQueryBuilder &EcsQueryBuilder::all() {
    ecs_term_t term{};
    term.id = EcsWildcard;
    term.inout = EcsInOutDefault;
    pImpl->terms.push_back(term);
    return *this;
}

EcsQueryBuilder &EcsQueryBuilder::all_read() {
    ecs_term_t term{};
    term.id = EcsWildcard;
    term.inout = EcsIn;
    pImpl->terms.push_back(term);
    return *this;
}

EcsQueryBuilder &EcsQueryBuilder::expr(std::string_view dsl) {
    pImpl->expr = dsl;
    return *this;
}

const std::string &EcsQueryBuilder::name() const { return pImpl->name; }

EcsQuery EcsQueryBuilder::build() {
    size_t term_count = pImpl->terms.size();
    NC_ASSERT(term_count > 0,
        std::format("query '{}' has no terms! use .with<T>() or .read<T>() before .build()", pImpl->name).c_str());
    NC_ASSERT(term_count <= FLECS_TERM_COUNT_MAX, std::format("too many query terms ({})", term_count).c_str());

    ecs_query_desc_t qdesc = pImpl->get_as_descriptor();
    EcsQuery result = pImpl->world.create_query_(pImpl->name, &qdesc);
    pImpl->built = true;
    return result;
}

} // namespace ncore
