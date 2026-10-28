#include "world/Geometry.h"

namespace fl {
bool circleBlocked(Vec2 p, float r, Bounds2 room, std::span<const Bounds2> obstacles) {
    if (p.x - r < room.minX || p.x + r > room.maxX ||
        p.z - r < room.minZ || p.z + r > room.maxZ) return true;
    for (const auto& b : obstacles) {
        const float dx = p.x - std::clamp(p.x, b.minX, b.maxX);
        const float dz = p.z - std::clamp(p.z, b.minZ, b.maxZ);
        if (dx * dx + dz * dz < r * r) return true;
    }
    return false;
}
Vec2 moveCircle(Vec2 p, Vec2 delta, float radius, Bounds2 room, std::span<const Bounds2> obstacles) {
    // Small steps prevent a fast-moving pawn from skipping a thin obstacle.
    // Try X and Z separately so a blocked axis still permits sliding.
    const int steps = std::max(1, static_cast<int>(std::ceil(
        std::max(std::abs(delta.x), std::abs(delta.z)) / 0.05F)));
    const float dx = delta.x / static_cast<float>(steps);
    const float dz = delta.z / static_cast<float>(steps);
    for (int i = 0; i < steps; ++i) {
        Vec2 candidate{p.x + dx, p.z};
        if (!circleBlocked(candidate, radius, room, obstacles)) p.x = candidate.x;
        candidate = {p.x, p.z + dz};
        if (!circleBlocked(candidate, radius, room, obstacles)) p.z = candidate.z;
    }
    return p;
}
float segmentHit(Vec3 from, Vec3 to, Bounds3 b, float padding) {
    b.min = {b.min.x-padding, b.min.y-padding, b.min.z-padding};
    b.max = {b.max.x+padding, b.max.y+padding, b.max.z+padding};
    // The table camera looks from a target just above the tabletop. Ignore
    // boxes containing the target so seating never collapses the camera boom.
    if (from.x >= b.min.x && from.x <= b.max.x && from.y >= b.min.y &&
        from.y <= b.max.y && from.z >= b.min.z && from.z <= b.max.z) return 1.0F;
    float enter = 0.0F, leave = 1.0F;
    const float origin[] = {from.x, from.y, from.z};
    const float delta[] = {to.x-from.x, to.y-from.y, to.z-from.z};
    const float lower[] = {b.min.x, b.min.y, b.min.z};
    const float upper[] = {b.max.x, b.max.y, b.max.z};
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(delta[axis]) < 0.00001F) {
            if (origin[axis] < lower[axis] || origin[axis] > upper[axis]) return 1.0F;
        } else {
            const float a = (lower[axis]-origin[axis]) / delta[axis];
            const float c = (upper[axis]-origin[axis]) / delta[axis];
            enter = std::max(enter, std::min(a, c));
            leave = std::min(leave, std::max(a, c));
            if (enter > leave) return 1.0F;
        }
    }
    return enter;
}
}
