#include "graphics/Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

namespace fl {
void Camera::orbit(float horizontal, float vertical) {
    yaw_ = std::remainder(yaw_ + horizontal, glm::two_pi<float>());
    pitch_ = std::clamp(pitch_ + vertical, 0.20F, 1.05F);
}
void Camera::pan(float horizontal, float vertical, int seat) {
    const float side = seat == 1 ? -1.0F : 1.0F;
    tablePan_.x = std::clamp(tablePan_.x + horizontal * side, -2.8F, 2.8F);
    tablePan_.y = std::clamp(tablePan_.y + vertical * side, -2.1F, 2.1F);
}
void Camera::zoom(float steps, bool seated) {
    if (seated) tableDistance_ = std::clamp(tableDistance_ - steps * 0.80F, 3.8F, 20.0F);
    else distance_ = std::clamp(distance_ - steps * 0.35F, 2.8F, 7.0F);
}
void Camera::reset() {
    yaw_ = 0.0F; pitch_ = 0.55F; distance_ = 4.8F;
    tableDistance_ = layout::tableCameraDistance; tablePan_ = glm::vec2{0};
}
void Camera::update(float dt, Vec2 player, int seat, const Room& room, bool snap) {
    const bool seated = seat >= 0;
    if (seat != lastSeat_) { tablePan_ = glm::vec2{0}; lastSeat_ = seat; }
    const glm::vec3 desiredTarget = seated
        ? glm::vec3{tablePan_.x, 1.42F, tablePan_.y}
        : glm::vec3{player.x, 1.05F, player.z};
    const float pitch = seated ? layout::tableCameraPitch : pitch_;
    const float yaw = seated ? (seat == 1 ? glm::pi<float>() : 0.0F) : yaw_;
    const float distance = seated ? tableDistance_ : distance_;
    const glm::vec3 offset{distance*std::cos(pitch)*std::sin(yaw),
                           distance*std::sin(pitch),
                           distance*std::cos(pitch)*std::cos(yaw)};
    const glm::vec3 wantedEye = desiredTarget + offset;
    const float blend = snap ? 1.0F : 1.0F-std::exp(-9.0F*dt);
    target_ = glm::mix(target_, desiredTarget, blend);
    eye_ = glm::mix(eye_, wantedEye, blend);
    const auto clipped = room.clipCamera({target_.x,target_.y,target_.z},{eye_.x,eye_.y,eye_.z});
    eye_ = {clipped.x,clipped.y,clipped.z};
    if (glm::length(eye_-target_) < 0.05F) eye_ = target_ + glm::vec3{0,0.05F,0.05F};
}
Ray Camera::screenRay(float normalizedX,float normalizedY,float aspect) const {
    const auto inverse=glm::inverse(viewProjection(aspect));
    auto nearPoint=inverse*glm::vec4{normalizedX*2-1,1-normalizedY*2,-1,1};
    auto farPoint=inverse*glm::vec4{normalizedX*2-1,1-normalizedY*2,1,1};
    nearPoint/=nearPoint.w;farPoint/=farPoint.w;
    const auto direction=glm::normalize(glm::vec3{farPoint-nearPoint});
    return {{nearPoint.x,nearPoint.y,nearPoint.z},{direction.x,direction.y,direction.z}};
}
glm::mat4 Camera::viewProjection(float aspect) const {
    return glm::perspective(glm::radians(55.0F), aspect, 0.06F, 100.0F)
         * glm::lookAt(eye_, target_, glm::vec3{0,1,0});
}
}
