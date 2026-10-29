#include "world/Player.h"
#include "graphics/BitmapFont.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

namespace {
void check(bool condition,const std::string& message) {
    // Do not use assert: these checks must still execute in Release builds.
    if (!condition) throw std::runtime_error(message);
}
bool near(float a,float b,float epsilon=0.002F) { return std::abs(a-b)<epsilon; }
void walkTo(fl::Player& player,const fl::Room& room,fl::Vec2 target) {
    for (int i=0;i<2000 && fl::distanceSquared(player.position(),target)>0.004F;++i) {
        const auto p=player.position();
        player.update(target.x-p.x,p.z-target.z,false,0,0.016F,room);
        check(!room.blocked(player.position(),fl::Player::radius),"Walking path entered a solid");
    }
    check(fl::distanceSquared(player.position(),target)<0.004F,"Could not reach walking waypoint");
}
void collisionTests() {
    const fl::Bounds2 bounds{-10,-10,10,10};
    const fl::Bounds2 wall{-0.02F,-2,0.02F,2};
    const std::span<const fl::Bounds2> obstacles{&wall,1};
    auto p=fl::moveCircle({-3,0},{6,0},0.3F,bounds,obstacles);
    check(p.x < -0.30F && p.x > -0.38F,"Fast motion tunneled through thin wall");
    p=fl::moveCircle({-0.35F,-1},{0.8F,1},0.3F,bounds,obstacles);
    check(p.x < -0.30F && p.z > -0.05F,"Wall sliding failed");
    p=fl::moveCircle({9,9},{4,4},0.3F,bounds,{});
    check(p.x<=9.7F && p.z<=9.7F,"Room boundary failed");
    const fl::Bounds2 block{0,0,1,1};
    check(!fl::circleBlocked({-0.25F,-0.25F},0.30F,bounds,{&block,1}),"Rounded corner falsely blocked");
    check(fl::circleBlocked({-0.20F,-0.20F},0.30F,bounds,{&block,1}),"Corner overlap missed");
    const fl::Bounds3 box{{1,-1,-1},{2,1,1}};
    check(near(fl::segmentHit({0,0,0},{4,0,0},box,0),0.25F),"Camera ray entry failed");
    check(near(fl::segmentHit({0,2,0},{4,2,0},box,0),1),"Parallel camera ray should miss");
    check(fl::segmentHit({0,1.1F,0},{4,1.1F,0},box,0.2F)<1,"Camera padding missed edge");
}
void playerAndSeatTests() {
    fl::Room room;
    fl::Player a,b;
    check(!room.blocked(a.position(),fl::Player::radius),"Spawn position is blocked");
    const auto start=a.position();
    a.update(0,1,false,0,0.1F,room);
    b.update(1,1,false,0,0.1F,room);
    const float straight=std::sqrt(fl::distanceSquared(start,a.position()));
    const float diagonal=std::sqrt(fl::distanceSquared(start,b.position()));
    check(near(straight,diagonal),"Diagonal movement is faster");
    fl::Player runner;
    runner.update(0,1,true,0,0.1F,room);
    check(fl::distanceSquared(start,runner.position())>fl::distanceSquared(start,a.position()),"Run did not increase speed");
    fl::Player turned;
    turned.update(0,1,false,1.570796326F,0.1F,room);
    check(turned.position().x < -0.25F && near(turned.position().z,start.z),"Camera-relative forward is wrong");
    check(!a.interact(room),"Distant player should not sit");
    walkTo(a,room,{0,fl::layout::approachZ});
    const auto standing=a.position();
    check(a.interact(room) && a.seat()==0,"South chair entry failed");
    check(room.seats()[0].occupied,"Seat ownership was not recorded");
    const auto sitting=a.position();
    a.update(1,1,true,0,1.0F,room);
    check(fl::distanceSquared(sitting,a.position())==0,"Seated player can walk");
    walkTo(b,room,{0,fl::layout::approachZ});
    check(!b.interact(room),"A second player claimed the occupied chair");
    check(a.stand(room),"Standing failed");
    check(!room.seats()[0].occupied && fl::distanceSquared(a.position(),standing)<0.000001F,"Seat release/return position failed");
    walkTo(a,room,{5.0F,fl::layout::approachZ});
    walkTo(a,room,{5.0F,-fl::layout::approachZ});
    walkTo(a,room,{0,-fl::layout::approachZ});
    check(a.interact(room) && a.seat()==1,"North chair entry failed");
    check(a.interact(room) && !a.seated(),"E-to-stand failed");
    check(!room.claimSeat(-1) && !room.claimSeat(2),"Invalid seat IDs accepted");
    // Repeated random runs exercise room/furniture edges and corners.
    std::mt19937 rng{42};
    std::uniform_real_distribution<float> direction{-1,1};
    for (int i=0;i<15000;++i) {
        a.update(direction(rng),direction(rng),true,direction(rng)*3.14F,0.05F,room);
        check(!room.blocked(a.position(),fl::Player::radius),"Random movement penetrated room/furniture");
    }
    const auto clipped=room.clipCamera({0,1,8},{0,3,14});
    check(clipped.z<fl::layout::roomHalfDepth-0.2F,"Third-person camera crossed south wall");
}
void tableCapacityTests() {
    fl::Room room;
    check(room.displayCards().size()==2,"Normal preview should contain two samples");
    room.toggleCapacityPreview();
    const auto& cards=room.displayCards();
    check(cards.size()==200,"Capacity preview must contain 20 x 10 cards");
    std::vector<float> xs,zs;
    for (std::size_t i=0;i<cards.size();++i) {
        const auto& c=cards[i];
        check(std::abs(c.position.x)+c.size.x/2 < fl::layout::feltWidth/2,"Card extends past felt side");
        check(std::abs(c.position.z)+c.size.z/2 < fl::layout::feltDepth/2,"Card extends past felt end");
        check(near(c.size.x,fl::layout::cardWidth) && near(c.size.z,fl::layout::cardDepth),"Inconsistent card size");
        if (std::find(xs.begin(),xs.end(),c.position.x)==xs.end()) xs.push_back(c.position.x);
        if (std::find(zs.begin(),zs.end(),c.position.z)==zs.end()) zs.push_back(c.position.z);
        for (std::size_t j=0;j<i;++j) {
            const auto& b=cards[j];
            const bool separatedX=std::abs(c.position.x-b.position.x)>(c.size.x+b.size.x)/2;
            const bool separatedZ=std::abs(c.position.z-b.position.z)>(c.size.z+b.size.z)/2;
            check(separatedX || separatedZ,"Capacity cards overlap");
        }
    }
    check(xs.size()==20 && zs.size()==10,"Preview does not have exactly 20 columns and 10 rows");
    room.toggleCapacityPreview();
    check(room.displayCards().size()==2,"Capacity preview did not restore samples");
    // Project the full felt corners using the overview configuration and room
    // clipping. Both seat orientations must fit between the top/bottom HUDs.
    for (float sign : {-1.0F,1.0F}) {
        const float pitch=fl::layout::tableCameraPitch;
        const float distance=fl::layout::tableCameraDistance;
        const fl::Vec3 target{0,1.42F,0};
        const auto eye=room.clipCamera(target,{0,target.y+distance*std::sin(pitch),sign*distance*std::cos(pitch)});
        const float actualDistance=std::sqrt((eye.y-target.y)*(eye.y-target.y)+eye.z*eye.z);
        for (float x : {-fl::layout::feltWidth/2,fl::layout::feltWidth/2})
            for (float z : {-fl::layout::feltDepth/2,fl::layout::feltDepth/2}) {
                const float dy=1.35F-target.y;
                const float depth=actualDistance-dy*std::sin(pitch)-z*sign*std::cos(pitch);
                const float screenY=(dy*std::cos(pitch)-z*sign*std::sin(pitch))/(depth*std::tan(0.47996554F));
                const float screenX=x/(depth*std::tan(0.47996554F)*1.6F);
                check(depth>0 && std::abs(screenX)<0.9F && screenY>-0.44F && screenY<0.63F,"Default overview crops the felt or hides it under HUD");
            }
    }
}
void fontTests() {
    std::vector<fl::RenderVertex> vertices;
    fl::appendText(vertices,"PRESS E TO SIT\nWASD / SHIFT",0,0,2);
    check(!vertices.empty() && vertices.size()%6==0,"Font did not emit complete pixel quads");
    for (const auto& v:vertices) check(std::isfinite(v.x) && std::isfinite(v.y),"Invalid HUD vertex");
}
}
int main() {
    try {
        collisionTests(); playerAndSeatTests(); tableCapacityTests(); fontTests();
        std::cout<<"PASS: collision, no tunneling, wall sliding, boundaries, camera rays, movement, both seats, seat exclusivity, safe standing, 15000 movement steps, 200-card layout with no overlap, both-seat overview fit, HUD geometry\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n'; return EXIT_FAILURE;
    }
}
