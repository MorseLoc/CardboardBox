#pragma once
#include "world/Geometry.h"
#include "world/TableLayout.h"
#include <array>
#include <string_view>
#include <vector>

namespace fl {
enum class Shape { Box, Sphere, Pawn };
struct RoomObject {
    Shape shape;
    Vec3 position, size, color;
    float yaw = 0.0F;
};
struct Seat {
    Vec2 position;
    float yaw;
    bool occupied = false;
};
class Room {
public:
    Room();
    [[nodiscard]] Vec2 move(Vec2 start, Vec2 delta, float radius) const;
    [[nodiscard]] bool blocked(Vec2 position, float radius) const;
    [[nodiscard]] Vec3 clipCamera(Vec3 target, Vec3 desired) const;
    [[nodiscard]] int nearestFreeSeat(Vec2 position) const;
    [[nodiscard]] std::string_view nearbyStation(Vec2 position) const;
    [[nodiscard]] const std::vector<RoomObject>& objects() const { return objects_; }
    [[nodiscard]] const std::array<Seat, 2>& seats() const { return seats_; }
    // Preview cards demonstrate size only; Phase 2 supplies real card state.
    [[nodiscard]] const std::vector<RoomObject>& displayCards() const { return capacityPreview_ ? capacityCards_ : sampleCards_; }
    [[nodiscard]] bool capacityPreviewEnabled() const { return capacityPreview_; }
    void toggleCapacityPreview() { capacityPreview_=!capacityPreview_; }
    bool claimSeat(int index);
    void releaseSeat(int index);
private:
    void box(Vec3 position, Vec3 size, Vec3 color, bool solid = false);
    void sphere(Vec3 position, Vec3 size, Vec3 color);
    Bounds2 bounds_{-layout::roomHalfWidth+0.2F,-layout::roomHalfDepth+0.2F,
                     layout::roomHalfWidth-0.2F,layout::roomHalfDepth-0.2F};
    std::vector<RoomObject> objects_;
    std::vector<RoomObject> sampleCards_,capacityCards_;
    bool capacityPreview_=false;
    std::vector<Bounds2> obstacles_;
    std::vector<Bounds3> cameraObstacles_;
    std::array<Seat, 2> seats_{{{{0,layout::seatZ},3.14159265F}, {{0,-layout::seatZ},0}}};
};
}
