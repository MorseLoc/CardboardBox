#pragma once
#include "graphics/Camera.h"
#include "graphics/Renderer.h"
#include "world/Player.h"
#include <SDL3/SDL.h>
#include "networking/NetworkRoom.h"
#include "networking/PawnSmoother.h"

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
    void openConnectionMenu();
    void showClub(ClubPage page);
    bool clubEvent(const SDL_Event& event);
    bool connectionEvent(const SDL_Event& event);
    void updateNetwork(float dt,net::MovementInput input);
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
    std::optional<Tabletop> localTable_;
    TabletopController controls_{table_};
    ClubUi club_;
    Room room_;
    Player player_;
    Player remote_;
    net::NetworkRoom network_;
    ConnectionView connection_;
    bool haveRemote_=false;
    std::array<net::PawnSmoother,2> smoothers_;
    Camera camera_;
    Renderer renderer_;
};
}
