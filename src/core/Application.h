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
    SDL_Window* window_ = nullptr;
    SDL_GLContext context_ = nullptr;
    bool sdlStarted_ = false;
    bool running_ = true;
    bool paused_ = false;
    Room room_;
    Player player_;
    Camera camera_;
    Renderer renderer_;
};
}
