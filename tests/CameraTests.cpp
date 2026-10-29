#include "graphics/Camera.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char* message) {if(!ok) throw std::runtime_error(message);}
void roundTrip(fl::Camera& camera,const fl::Room& room,int seat,float aspect) {
    camera.update(0,{0,fl::layout::approachZ},seat,room,true);
    for(float x:{-2.7F,0.0F,2.7F}) for(float z:{-2.0F,0.0F,2.0F}) {
        const auto clip=camera.viewProjection(aspect)*glm::vec4{x,1.36F,z,1};
        const float sx=(clip.x/clip.w+1)/2,sy=(1-clip.y/clip.w)/2;
        const auto ray=camera.screenRay(sx,sy,aspect);const auto point=fl::onPlane(ray,1.36F);
        check(point && std::abs(point->x-x)<0.002F && std::abs(point->z-z)<0.002F,"Camera screen/ray/table round trip failed");
    }
}
}
int main() {
    try {
        fl::Room room;fl::Camera camera;
        for(int seat:{0,1}) for(float aspect:{1.6F,1280.0F/720.0F,2.37F}) {
            camera.reset();roundTrip(camera,room,seat,aspect);
            camera.zoom(3,true);camera.pan(0.7F,-0.4F,seat);roundTrip(camera,room,seat,aspect);
        }
        std::cout<<"PASS: actual GLM camera projection/ray round trips from both seats, three aspects and panned/zoomed views\n";
        return EXIT_SUCCESS;
    }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return EXIT_FAILURE;}
}
