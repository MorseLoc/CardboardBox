#include "world/Player.h"
#include <cmath>

namespace fl {
void Player::update(float sideways,float forward,bool run,float cameraYaw,float dt,const Room& room) {
    if (seated() || dt<=0) return;
    const float length=std::sqrt(sideways*sideways+forward*forward);
    if (length<0.001F) return;
    sideways/=length; forward/=length; // Diagonal movement is not faster.
    const float dx=sideways*std::cos(cameraYaw)-forward*std::sin(cameraYaw);
    const float dz=-sideways*std::sin(cameraYaw)-forward*std::cos(cameraYaw);
    const float speed=run ? 4.8F : 2.7F;
    position_=room.move(position_,{dx*speed*dt,dz*speed*dt},radius);
    yaw_=std::atan2(dx,dz);
}
bool Player::interact(Room& room) {
    if (seated()) return stand(room);
    const int index=room.nearestFreeSeat(position_);
    if (!room.claimSeat(index)) return false;
    returnPosition_=position_;
    const auto& s=room.seats()[static_cast<std::size_t>(index)];
    position_=s.position; yaw_=s.yaw; seat_=index;
    return true;
}
bool Player::stand(Room& room) {
    if (!seated()) return false;
    // Returning to the last valid standing position avoids spawning inside the
    // chair. Static Phase 1 furniture cannot invalidate that saved position.
    if (room.blocked(returnPosition_,radius)) return false;
    room.releaseSeat(seat_); seat_=-1; position_=returnPosition_;
    return true;
}
}
