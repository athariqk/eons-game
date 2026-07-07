#pragma once

#include <ncore/kernel/structures.h>
#include <ncore/kernel/types.h>

namespace nc {

struct NCORE_API EcsTransform {
    EcsTransform() = default;
    EcsTransform( const Vec2& pos ) : position( pos ) {}
    EcsTransform( const Vec2& pos, float rot ) : position( pos ), angle( rot ) {}
    EcsTransform( const Vec2& pos, float rot, const Vec2& dim ) : position( pos ), angle( rot ), dimension( dim ) {}

    Vec2 position{ 0.0f, 0.0f };
    float angle = 0.0f;
    Vec2 dimension{ 0.0f, 0.0f };
    float scale = 1.0f;

    // TODO: remove this to use reflection instead
    std::string to_string() const
    {
        return std::format(
            "EcsTransform<position={}, angle={}, dimension={}, scale={}>", position.to_string(), angle,
            dimension.to_string(), scale
        );
    }

    NSTRUCT(
        EcsTransform, NC_F( EcsTransform, position ) NC_F( EcsTransform, angle ) NC_F( EcsTransform, dimension )
                          NC_F( EcsTransform, scale )
    )
};

} // namespace nc
