#pragma once

#include <vector>

#include <flecs.h>

namespace nc {
class EcsWorld;
}

namespace nc::detail {

struct FlecsQueryBuilder {
    EcsWorld& world;
    std::string name;
    std::string expr;
    std::vector<ecs_term_t> terms;
    bool built = false;

    FlecsQueryBuilder( EcsWorld& w, std::string n ) : world( w ), name( std::move( n ) ) {}

    // Wraps our data into Flecs' C struct
    // TODO: use implicit conversion opr overloading
    ecs_query_desc_t get_as_descriptor() const
    {
        ecs_query_desc_t qdesc{};
        qdesc.cache_kind = EcsQueryCacheDefault;
        memcpy( qdesc.terms, terms.data(), terms.size() * sizeof( ecs_term_t ) );
        if (!expr.empty()) {
            qdesc.expr = expr.c_str();
        }
        return qdesc;
    }
};

inline ecs_entity_t map_phase( EcsSystemPhase p )
{
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
            NC_ASSERT( false, "Unknown system phase" );
            return 0;
    }
}

} // namespace nc::detail
