#pragma once
#include "graphics/Camera.h"
#include "graphics/Renderer.h"
#include "world/Player.h"
#include <SDL3/SDL.h>

namespace fl {
class Application {
public:
    Application() = default;
    ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    int run();
private:
    void initialize();
    void processEvents();
    void pointer(float logicalX,float logicalY);
    std::optional<Ray> pointerRay() const;
    SDL_Window* window_ = nullptr;
    SDL_GLContext context_ = nullptr;
    bool sdlStarted_ = false;
    bool running_ = true;
    bool paused_ = false;
    float mouseX_=0,mouseY_=0;
    int drawableWidth_=1280,drawableHeight_=720;
    Tabletop table_;
    TabletopController controls_{table_};
    Room room_;
    Player player_;
    Camera camera_;
    Renderer renderer_;
};
}
