#pragma once
#include "networking/RoomProtocol.h"
#include <algorithm>
#include <cmath>
#include <deque>
namespace fl::net {
// Display-only interpolation with 75 ms of snapshots. Authority stays untouched.
// Seats, reconnects and teleports snap rather than blend through furniture.
class PawnSmoother {
public:
    void reset() {samples_.clear();}
    PawnState update(const PawnState& target,std::uint64_t tick,float dt) {
        const double time=static_cast<double>(tick)/60.0;
        if(samples_.empty() || target.seat!=samples_.back().pawn.seat ||
           target.connected!=samples_.back().pawn.connected || time<samples_.back().time ||
           distanceSquared(target.position,samples_.back().pawn.position)>4) {
            samples_.clear();samples_.push_back({time,target});playback_=time-0.075;return target;
        }
        if(time>samples_.back().time) samples_.push_back({time,target});
        playback_=std::clamp(playback_+static_cast<double>(std::clamp(dt,0.0F,0.1F)),time-0.2,time);
        while(samples_.size()>2 && samples_[1].time<=playback_) samples_.pop_front();
        while(samples_.size()>32) samples_.pop_front();
        auto out=samples_.front().pawn;
        if(samples_.size()>1) {
            const auto& a=samples_[0];const auto& b=samples_[1];
            const float t=static_cast<float>(std::clamp((playback_-a.time)/(b.time-a.time),0.0,1.0));
            out.position={a.pawn.position.x+(b.pawn.position.x-a.pawn.position.x)*t,a.pawn.position.z+(b.pawn.position.z-a.pawn.position.z)*t};
            out.yaw=a.pawn.yaw+std::remainder(b.pawn.yaw-a.pawn.yaw,6.283185307F)*t;
        }
        return out;
    }
private:
    struct Sample {double time;PawnState pawn;};
    std::deque<Sample> samples_;
    double playback_=0;
};
}
