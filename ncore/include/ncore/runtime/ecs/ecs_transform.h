#pragma once

#include <format>

#include <ncore/kernel/types.h>
#include <ncore/kernel/Structures.h>

namespace ncore {

struct EcsTransform {
    EcsTransform() = default;
    EcsTransform(const Vec2 &pos) : position(pos) {}
    EcsTransform(const Vec2 &pos, float rot) : position(pos), angle(rot) {}
    EcsTransform(const Vec2 &pos, float rot, const Vec2 &dim) : position(pos), angle(rot), dimension(dim) {}

    Vec2 position{0.0f, 0.0f};
    float angle = 0.0f;
    Vec2 dimension{0.0f, 0.0f};
    float scale = 1.0f;

    std::string to_string() const {
        return std::format("EcsTransform<position={}, angle={}, dimension={}, scale={}>", position.to_string(),
                           angle, dimension.to_string(), scale);
    }

    NC_BIND(EcsTransform,
        NC_F(EcsTransform, position)
        NC_F(EcsTransform, angle)
        NC_F(EcsTransform, dimension)
        NC_F(EcsTransform, scale)
    );
};

} // namespace ncore
