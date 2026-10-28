#pragma once
#include "world/Room.h"
#include <glm/glm.hpp>

namespace fl {
class Camera {
public:
    void orbit(float horizontal, float vertical);
    void pan(float horizontal, float vertical, int seat);
    void zoom(float steps, bool seated);
    void reset();
    void update(float dt, Vec2 player, int seat, const Room& room, bool snap = false);
    [[nodiscard]] float yaw() const { return yaw_; }
    [[nodiscard]] glm::mat4 viewProjection(float aspect) const;
private:
    float yaw_ = 0.0F;
    float pitch_ = 0.55F;
    float distance_ = 4.8F;
    float tableDistance_ = layout::tableCameraDistance;
    glm::vec2 tablePan_{0.0F};
    glm::vec3 eye_{0.0F}, target_{0.0F};
    int lastSeat_ = -1;
};
}
