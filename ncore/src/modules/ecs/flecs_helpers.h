#pragma once

#include <vector>

#include <flecs.h>

namespace ncore {
class EcsWorld;
}

namespace ncore::detail {

struct FlecsQueryBuilder {
    EcsWorld& world;
    std::string name;
    std::string expr;
    std::vector<ecs_term_t> terms;
    bool built = false;

    FlecsQueryBuilder(EcsWorld& w, std::string n) : world(w), name(std::move(n)) {}

    // Wraps our data into Flecs' C struct
    // TODO: use implicit conversion opr overloading
    ecs_query_desc_t get_as_descriptor() const
    {
        ecs_query_desc_t qdesc{.cache_kind = EcsQueryCacheDefault};
        memcpy(qdesc.terms, terms.data(), terms.size() * sizeof(ecs_term_t));
        if (!expr.empty()) {
            qdesc.expr = expr.c_str();
        }
        return qdesc;
    }
};

} // namespace ncore::detail
