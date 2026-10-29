#include "world/Room.h"
#include <algorithm>

namespace fl {
void Room::box(Vec3 p, Vec3 s, Vec3 color, bool solid) {
    objects_.push_back({Shape::Box, p, s, color});
    if (solid) {
        const Bounds3 bounds{{p.x-s.x/2,p.y-s.y/2,p.z-s.z/2},
                             {p.x+s.x/2,p.y+s.y/2,p.z+s.z/2}};
        obstacles_.push_back({bounds.min.x,bounds.min.z,bounds.max.x,bounds.max.z});
        cameraObstacles_.push_back(bounds);
    }
}
void Room::sphere(Vec3 p, Vec3 s, Vec3 color) { objects_.push_back({Shape::Sphere,p,s,color}); }
Room::Room() {
    const Vec3 wood{0.43F,0.25F,0.17F}, darkWood{0.24F,0.14F,0.13F};
    // The larger room preserves walking space and station access around a
    // table that genuinely accommodates twenty by ten upright cards.
    box({0,-0.12F,0},{layout::roomHalfWidth*2,0.24F,layout::roomHalfDepth*2},{0.46F,0.32F,0.25F});
    for (int i=-13;i<=13;++i)
        box({static_cast<float>(i)*0.75F,0.002F,0},{0.018F,0.006F,19.6F},{0.29F,0.19F,0.18F});
    for (float z : {-layout::roomHalfDepth,layout::roomHalfDepth})
        box({0,2.2F,z},{20,4.4F,0.4F},{0.55F,0.36F,0.40F},true);
    for (float x : {-layout::roomHalfWidth,layout::roomHalfWidth})
        box({x,2.2F,0},{0.4F,4.4F,20},{0.48F,0.33F,0.38F},true);
    for (float z : {-9.77F,9.77F}) box({0,0.13F,z},{19.6F,0.26F,0.07F},darkWood);
    for (float x : {-9.77F,9.77F}) box({x,0.13F,0},{0.07F,0.26F,19.6F},darkWood);
    // A painted night window; geometry only, not glass or an outdoor world.
    box({0,2.65F,-9.76F},{2.8F,1.85F,0.07F},wood);
    box({0,2.65F,-9.70F},{2.55F,1.6F,0.06F},{0.17F,0.33F,0.45F});
    box({0,2.65F,-9.64F},{0.09F,1.65F,0.06F},wood);
    box({0,2.65F,-9.64F},{2.6F,0.09F,0.06F},wood);
    sphere({0.80F,3.05F,-9.58F},{0.30F,0.30F,0.04F},{0.95F,0.82F,0.56F});
    box({0,0.015F,0},{10.3F,0.025F,11.2F},{0.30F,0.25F,0.37F});
    // Table footprint intentionally blocks walking underneath it. Cards stay
    // small; navigation, seats, camera and room dimensions grow with the table.
    box({0,1.17F,0},{layout::tableWidth,0.30F,layout::tableDepth},wood,true);
    box({0,1.335F,0},{layout::feltWidth,0.03F,layout::feltDepth},{0.16F,0.38F,0.30F});
    for (float x : {-3.25F,3.25F}) for (float z : {-2.4F,2.4F})
        box({x,0.52F,z},{0.30F,1.04F,0.30F},darkWood);
    // Two visible chairs, centered on the short ends of the play area.
    for (const auto& seat : seats_) {
        const float z = seat.position.z, outward = z > 0 ? 1.0F : -1.0F;
        box({0,0.48F,z},{0.95F,0.18F,0.90F},wood,true);
        box({0,1.00F,z+outward*0.38F},{0.95F,1.05F,0.16F},wood,true);
        box({0,0.59F,z},{0.77F,0.07F,0.65F},{0.63F,0.34F,0.46F});
        for (float x : {-0.35F,0.35F}) for (float dz : {-0.30F,0.30F})
            box({x,0.22F,z+dz},{0.12F,0.44F,0.12F},darkWood);
    }
    // Station shells. These will open their real interfaces in later phases.
    box({-6.0F,0.66F,-8.3F},{3.3F,1.32F,1.2F},wood,true); // shop
    box({-6.0F,1.36F,-8.3F},{3.5F,0.12F,1.35F},darkWood);
    for (int i=0;i<5;++i) {
        const float x=-7.1F+static_cast<float>(i)*0.55F;
        box({x,1.60F,-8.35F},{0.36F,0.40F,0.16F},{0.75F,0.36F+static_cast<float>(i)*0.06F,0.60F});
    }
    // Five basic-land box placeholders beside the shop.
    for (int i=0;i<5;++i)
        box({-8.2F,0.17F+static_cast<float>(i)*0.31F,-8.3F},{0.48F,0.27F,0.8F},
            {0.35F+static_cast<float>(i)*0.08F,0.46F,0.36F},true);
    box({6.0F,0.67F,-8.3F},{3.2F,1.34F,1.2F},wood,true); // deck desk
    box({6.0F,1.38F,-8.3F},{3.4F,0.10F,1.4F},darkWood);
    for (int i=0;i<4;++i)
        box({5.1F+static_cast<float>(i)*0.54F,1.56F,-8.35F},{0.37F,0.27F,0.65F},
            {0.33F,0.47F,0.58F+static_cast<float>(i)*0.06F});
    box({-7.0F,0.43F,0.0F},{1.65F,0.86F,1.25F},{0.31F,0.44F,0.41F},true); // toy chest
    box({-7.0F,0.90F,0.0F},{1.78F,0.16F,1.35F},wood);
    box({-7.0F,0.52F,0.65F},{0.20F,0.26F,0.04F},{0.85F,0.64F,0.28F});
    box({7.0F,0.57F,0.0F},{1.5F,1.14F,0.9F},wood,true); // vanity
    box({7.0F,1.9F,0.0F},{1.4F,1.7F,0.18F},darkWood);
    box({7.0F,1.9F,0.12F},{1.15F,1.45F,0.05F},{0.58F,0.69F,0.72F});
    // Lamps provide warm-colored shapes; lighting remains stylized face shading.
    for (float x : {-8.0F,8.0F}) {
        box({x,0.90F,7.3F},{0.13F,1.8F,0.13F},darkWood,true);
        sphere({x,0.08F,7.3F},{0.65F,0.15F,0.65F},darkWood);
        sphere({x,1.96F,7.3F},{0.68F,0.62F,0.68F},{0.98F,0.76F,0.46F});
    }
    // Two sample shapes in normal view; G displays the same size in 200 slots.
    const Vec3 cardSize{layout::cardWidth,layout::cardThickness,layout::cardDepth};
    sampleCards_.push_back({Shape::Box,{-1.15F,layout::cardCenterY,0.25F},cardSize,{0.95F,0.84F,0.64F}});
    sampleCards_.push_back({Shape::Box,{1.15F,layout::cardCenterY,-0.25F},cardSize,{0.54F,0.59F,0.92F}});
    capacityCards_.reserve(static_cast<std::size_t>(layout::columns*layout::rows));
    for (int row=0;row<layout::rows;++row) for (int column=0;column<layout::columns;++column) {
        // Alternating five-by-five color groups make the dimensions countable.
        const bool alternate=((column/5)+(row/5))%2!=0;
        const Vec3 color=alternate ? Vec3{0.64F,0.60F,0.87F} : Vec3{0.91F,0.78F,0.58F};
        capacityCards_.push_back({Shape::Box,{layout::gridX(column),layout::cardCenterY,layout::gridZ(row)},cardSize,color});
    }

}
Vec2 Room::move(Vec2 p, Vec2 delta, float radius) const { return moveCircle(p,delta,radius,bounds_,obstacles_); }
bool Room::blocked(Vec2 p, float radius) const { return circleBlocked(p,radius,bounds_,obstacles_); }
Vec3 Room::clipCamera(Vec3 target, Vec3 desired) const {
    float t=1.0F;
    for (const auto& b : cameraObstacles_) t=std::min(t,segmentHit(target,desired,b,0.16F));
    t=std::max(0.0F,t-0.015F);
    return {target.x+(desired.x-target.x)*t,target.y+(desired.y-target.y)*t,target.z+(desired.z-target.z)*t};
}
int Room::nearestFreeSeat(Vec2 p) const {
    int best=-1;
    float nearest=1.40F*1.40F;
    for (int i=0;i<static_cast<int>(seats_.size());++i) {
        const auto& s=seats_[static_cast<std::size_t>(i)];
        const float d=distanceSquared(p,s.position);
        if (!s.occupied && d < nearest) { nearest=d; best=i; }
    }
    return best;
}
bool Room::claimSeat(int i) {
    if (i<0 || i>=static_cast<int>(seats_.size()) || seats_[static_cast<std::size_t>(i)].occupied) return false;
    seats_[static_cast<std::size_t>(i)].occupied=true; return true;
}
void Room::releaseSeat(int i) {
    if (i>=0 && i<static_cast<int>(seats_.size())) seats_[static_cast<std::size_t>(i)].occupied=false;
}
std::string_view Room::nearbyStation(Vec2 p) const {
    if (distanceSquared(p,{-6.0F,-6.7F})<3.2F) return "CARD SHOP / LAND BOXES - COMING LATER";
    if (distanceSquared(p,{6.0F,-6.7F})<3.2F) return "DECK BUILDER - COMING LATER";
    if (distanceSquared(p,{-7.0F,1.2F})<2.8F) return "TOY CHEST - COMING LATER";
    if (distanceSquared(p,{7.0F,1.3F})<2.8F) return "FACE PAINTER - COMING LATER";
    return {};
}
}
