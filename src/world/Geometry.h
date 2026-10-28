#pragma once
#include <algorithm>
#include <cmath>
#include <span>

namespace fl {
// World coordinates are deliberately independent of SDL, OpenGL, and GLM.
// Y is height; movement and floor collisions use the X/Z plane.
struct Vec2 { float x = 0.0F, z = 0.0F; };
struct Vec3 { float x = 0.0F, y = 0.0F, z = 0.0F; };
struct Bounds2 { float minX, minZ, maxX, maxZ; };
struct Bounds3 { Vec3 min, max; };
inline float distanceSquared(Vec2 a, Vec2 b) {
    const float x = a.x - b.x, z = a.z - b.z;
    return x * x + z * z;
}
bool circleBlocked(Vec2 position, float radius, Bounds2 room, std::span<const Bounds2> obstacles);
Vec2 moveCircle(Vec2 start, Vec2 delta, float radius, Bounds2 room, std::span<const Bounds2> obstacles);
// Returns the earliest segment hit in [0, 1], or 1 when unobstructed.
float segmentHit(Vec3 from, Vec3 to, Bounds3 box, float padding);
}
