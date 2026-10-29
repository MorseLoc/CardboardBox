#pragma once
#include "world/Room.h"

namespace fl {
class Player {
public:
    static constexpr float radius=0.36F;
    void update(float sideways, float forward, bool run, float cameraYaw, float dt, const Room& room);
    bool interact(Room& room);
    bool stand(Room& room);
    bool sitAt(Room& room,int index);
    // Presentation of an already validated host state. Never sent as a movement request.
    void applyNetworkState(Vec2 position,float yaw,int seat) {position_=position;yaw_=yaw;seat_=seat;}
    [[nodiscard]] Vec2 position() const { return position_; }
    [[nodiscard]] float yaw() const { return yaw_; }
    [[nodiscard]] int seat() const { return seat_; }
    [[nodiscard]] bool seated() const { return seat_>=0; }
private:
    Vec2 position_{0,layout::spawnZ};
    Vec2 returnPosition_{};
    float yaw_=3.14159265F;
    int seat_=-1;
};
}
