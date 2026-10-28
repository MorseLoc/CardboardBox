#include "core/Application.h"
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <string>

namespace fl {
namespace {
void requireSdl(bool success, const char* action) {
    if (!success) throw std::runtime_error(std::string{action} + ": " + SDL_GetError());
}
}
Application::~Application() {
    // GPU resources must be released while their context still exists.
    renderer_.shutdown();
    if (context_) SDL_GL_DestroyContext(context_);
    if (window_) SDL_DestroyWindow(window_);
    if (sdlStarted_) SDL_Quit();
}
void Application::initialize() {
    SDL_SetMainReady(); // We supply a normal C++ main instead of SDL's wrapper.
    requireSdl(SDL_SetAppMetadata("FemboyLover.exe", "0.1.1", "private.femboylover.clubhouse"), "Set metadata");
    requireSdl(SDL_Init(SDL_INIT_VIDEO), "Initialize SDL3");
    sdlStarted_ = true;
    requireSdl(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3), "Set GL major");
    requireSdl(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3), "Set GL minor");
    requireSdl(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE), "Set core profile");
    requireSdl(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1), "Set double buffer");
    requireSdl(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24), "Set depth buffer");
    window_ = SDL_CreateWindow("FemboyLover.exe | PHASE 1.1 | Giant table / tiny cards",
                               1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window_) throw std::runtime_error(std::string{"Create window: "} + SDL_GetError());
    requireSdl(SDL_SetWindowMinimumSize(window_, 960, 600), "Set minimum window size");
    context_ = SDL_GL_CreateContext(window_);
    if (!context_) throw std::runtime_error(std::string{"OpenGL 3.3 context: "} + SDL_GetError());
    requireSdl(SDL_GL_MakeCurrent(window_, context_), "Make OpenGL context current");
    if (!SDL_GL_SetSwapInterval(1)) SDL_Log("VSync unavailable: %s", SDL_GetError());
    SDL_Log("OpenGL: %s | Renderer: %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)),
            reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    renderer_.initialize();
    camera_.update(0.0F, player_.position(), player_.seat(), room_, true);
    SDL_Log("WASD: move | Shift: run | RMB-drag: camera / table pan | wheel: zoom | E: sit/stand | Esc: stand/pause | Q: quit while paused");
}
void Application::processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) running_ = false;
        if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) paused_ = true;
        if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
            switch (event.key.key) {
            case SDLK_ESCAPE:
                if (paused_) paused_ = false;
                else if (player_.seated()) player_.stand(room_);
                else paused_ = true;
                break;
            case SDLK_Q: if (paused_) running_ = false; break;
            case SDLK_E: if (!paused_) player_.interact(room_); break;
            case SDLK_G: if (!paused_) room_.toggleCapacityPreview(); break;
            case SDLK_HOME: if (!paused_) camera_.reset(); break;
            default: break;
            }
        }
        if (paused_) continue;
        if (event.type == SDL_EVENT_MOUSE_MOTION && (event.motion.state & SDL_BUTTON_RMASK)) {
            if (player_.seated()) camera_.pan(-event.motion.xrel*0.006F, -event.motion.yrel*0.006F, player_.seat());
            else camera_.orbit(-event.motion.xrel*0.006F, event.motion.yrel*0.006F);
        }
        if (event.type == SDL_EVENT_MOUSE_WHEEL)
            camera_.zoom(event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -event.wheel.y : event.wheel.y, player_.seated());
    }
}
int Application::run() {
    initialize();
    using Clock = std::chrono::steady_clock;
    auto previous = Clock::now();
    while (running_) {
        const auto now = Clock::now();
        const float dt = std::clamp(std::chrono::duration<float>(now-previous).count(), 0.0F, 0.05F);
        previous = now;
        processEvents();
        if (!running_) break;
        const bool focused = (SDL_GetWindowFlags(window_) & SDL_WINDOW_INPUT_FOCUS) != 0;
        const bool* keys = SDL_GetKeyboardState(nullptr);
        if (!paused_ && focused) {
            const float sideways = static_cast<float>(keys[SDL_SCANCODE_D])-static_cast<float>(keys[SDL_SCANCODE_A]);
            const float forward = static_cast<float>(keys[SDL_SCANCODE_W])-static_cast<float>(keys[SDL_SCANCODE_S]);
            player_.update(sideways,forward,keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT],camera_.yaw(),dt,room_);
            if (!player_.seated()) {
                const float horizontal = static_cast<float>(keys[SDL_SCANCODE_RIGHT])-static_cast<float>(keys[SDL_SCANCODE_LEFT]);
                const float vertical = static_cast<float>(keys[SDL_SCANCODE_UP])-static_cast<float>(keys[SDL_SCANCODE_DOWN]);
                camera_.orbit(horizontal*dt,vertical*dt);
            }
        }
        if (!paused_) camera_.update(dt,player_.position(),player_.seat(),room_);
        int width = 0, height = 0;
        requireSdl(SDL_GetWindowSizeInPixels(window_, &width, &height), "Read drawable size");
        if ((SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED) || width<=0 || height<=0) {
            SDL_Delay(30);
            continue;
        }
        renderer_.draw(width,height,camera_.viewProjection(static_cast<float>(width)/static_cast<float>(height)),room_,player_,paused_);
        requireSdl(SDL_GL_SwapWindow(window_), "Swap window buffers");
        SDL_Delay(paused_ ? 16 : 1); // Throttle pause and drivers that refuse VSync.
    }
    return 0;
}
}
