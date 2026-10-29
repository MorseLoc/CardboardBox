#include "core/Application.h"
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>
#include <memory>
#include <filesystem>

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
    requireSdl(SDL_SetAppMetadata("FemboyLover.exe", "0.2.2", "private.femboylover.clubhouse"), "Set metadata");
    requireSdl(SDL_Init(SDL_INIT_VIDEO), "Initialize SDL3");
    sdlStarted_ = true;
    requireSdl(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3), "Set GL major");
    requireSdl(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3), "Set GL minor");
    requireSdl(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE), "Set core profile");
    requireSdl(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1), "Set double buffer");
    requireSdl(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24), "Set depth buffer");
    window_ = SDL_CreateWindow("FemboyLover.exe | PHASE 2 | Twenty wide / ten deep / tap that",
                               1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window_) throw std::runtime_error(std::string{"Create window: "} + SDL_GetError());
    requireSdl(SDL_SetWindowMinimumSize(window_, 960, 600), "Set minimum window size");
    context_ = SDL_GL_CreateContext(window_);
    if (!context_) throw std::runtime_error(std::string{"OpenGL 3.3 context: "} + SDL_GetError());
    requireSdl(SDL_GL_MakeCurrent(window_, context_), "Make OpenGL context current");
    if (!SDL_GL_SetSwapInterval(1)) SDL_Log("VSync unavailable: %s", SDL_GetError());
    SDL_Log("OpenGL: %s | Renderer: %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)),
            reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    std::unique_ptr<char,decltype(&SDL_free)> pref{SDL_GetPrefPath("FemboyLover","Clubhouse"),SDL_free};
    if(!pref) throw std::runtime_error(std::string{"Card cache path: "}+SDL_GetError());
    const std::string_view utf8Pref{pref.get()};
    const std::filesystem::path prefPath{std::u8string(utf8Pref.begin(),utf8Pref.end())};
    renderer_.initialize(table_,prefPath/"cards");
    SDL_Log("Card cache root: %s",pref.get());
    camera_.update(0.0F, player_.position(), player_.seat(), room_, true);
    SDL_Log("WASD: move | Shift: run | RMB-drag: camera / table pan | wheel: zoom | E: sit/stand | Esc: stand/pause | Q: quit while paused");
}
void Application::pointer(float x,float y) {
    int w=1,h=1;
    requireSdl(SDL_GetWindowSize(window_,&w,&h),"Read window size");
    requireSdl(SDL_GetWindowSizeInPixels(window_,&drawableWidth_,&drawableHeight_),"Read drawable size");
    mouseX_=x*static_cast<float>(drawableWidth_)/static_cast<float>(std::max(1,w));
    mouseY_=y*static_cast<float>(drawableHeight_)/static_cast<float>(std::max(1,h));
    controls_.setViewport(static_cast<float>(drawableWidth_),static_cast<float>(drawableHeight_));
}
std::optional<Ray> Application::pointerRay() const {
    if(!player_.seated() || drawableWidth_<=0 || drawableHeight_<=0) return {};
    return camera_.screenRay(mouseX_/static_cast<float>(drawableWidth_),mouseY_/static_cast<float>(drawableHeight_),
                             static_cast<float>(drawableWidth_)/static_cast<float>(drawableHeight_));
}
void Application::processEvents() {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        if(event.type==SDL_EVENT_QUIT || event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED) running_=false;
        if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST) {controls_.cancel();paused_=true;}
        if(event.type==SDL_EVENT_KEY_DOWN && !event.key.repeat) {
            const bool shift=(SDL_GetModState() & SDL_KMOD_SHIFT)!=0;
            switch(event.key.key) {
            case SDLK_ESCAPE:
                if(paused_) paused_=false;
                else if(controls_.escape()) {}
                else if(player_.seated()) {player_.stand(room_);controls_.setViewer(-1);}
                else paused_=true;
                break;
            case SDLK_Q: if(paused_) running_=false;break;
            case SDLK_E:
                if(!paused_ && !table_.dragging()) {controls_.cancel();player_.interact(room_);controls_.setViewer(player_.seat());}
                break;
            case SDLK_G: if(!paused_) {controls_.cancel();room_.toggleCapacityPreview();}break;
            case SDLK_HOME: if(!paused_) camera_.reset();break;
            default: break;
            }
            if(!paused_ && player_.seated() && !room_.capacityPreviewEnabled()) {
                switch(event.key.key) {
                case SDLK_T: controls_.action(TableAction::Tap);break;
                case SDLK_F: controls_.action(TableAction::Flip);break;
                case SDLK_R: controls_.action(shift?TableAction::RotateLeft:TableAction::RotateRight);break;
                case SDLK_D: controls_.action(shift?TableAction::DrawSeven:TableAction::Draw);break;
                case SDLK_S: controls_.action(TableAction::Shuffle);break;
                case SDLK_H: controls_.action(TableAction::Hand);break;
                case SDLK_B: controls_.action(TableAction::Group);break;
                case SDLK_X: controls_.action(TableAction::Split);break;
                case SDLK_L: controls_.action(shift?TableAction::Bottom:TableAction::Top);break;
                case SDLK_C: controls_.action(TableAction::Center);break;
                default: break;
                }
            }
        }
        if(paused_) continue;
        const bool active=player_.seated() && !room_.capacityPreviewEnabled();
        if(event.type==SDL_EVENT_MOUSE_MOTION) {
            pointer(event.motion.x,event.motion.y);
            if(active) controls_.pointerMove(mouseX_,mouseY_,pointerRay());
            const bool panButtons=(event.motion.state & (SDL_BUTTON_RMASK|SDL_BUTTON_MMASK))!=0;
            if(panButtons && !table_.dragging() && !controls_.inspectionOpen() && !controls_.menuOpen()) {
                if(player_.seated()) camera_.pan(-event.motion.xrel*0.006F,-event.motion.yrel*0.006F,player_.seat());
                else camera_.orbit(-event.motion.xrel*0.006F,event.motion.yrel*0.006F);
            }
        }
        if(event.type==SDL_EVENT_MOUSE_BUTTON_DOWN && active) {
            pointer(event.button.x,event.button.y);const auto ray=pointerRay();
            if(event.button.button==SDL_BUTTON_LEFT)
                controls_.leftDown(mouseX_,mouseY_,ray,(SDL_GetModState()&SDL_KMOD_CTRL)!=0,event.button.clicks>=2);
            if(event.button.button==SDL_BUTTON_RIGHT)
                controls_.rightDown(mouseX_,mouseY_,ray,(SDL_GetModState()&SDL_KMOD_SHIFT)!=0);
        }
        if(event.type==SDL_EVENT_MOUSE_BUTTON_UP && active && event.button.button==SDL_BUTTON_LEFT) {
            pointer(event.button.x,event.button.y);
            controls_.leftUp(mouseX_,mouseY_,pointerRay(),(SDL_GetModState()&SDL_KMOD_SHIFT)!=0);
        }
        if(event.type==SDL_EVENT_MOUSE_WHEEL) {
            float x=0,y=0;SDL_GetMouseState(&x,&y);pointer(x,y);
            const float amount=event.wheel.direction==SDL_MOUSEWHEEL_FLIPPED ? -event.wheel.y:event.wheel.y;
            if(!(active && controls_.wheel(amount,mouseX_,mouseY_)) && !table_.dragging()) camera_.zoom(amount,player_.seated());
        }
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
        controls_.setViewport(static_cast<float>(width),static_cast<float>(height));
        renderer_.draw(width,height,camera_.viewProjection(static_cast<float>(width)/static_cast<float>(height)),room_,player_,paused_,table_,controls_);
        requireSdl(SDL_GL_SwapWindow(window_), "Swap window buffers");
        SDL_Delay(paused_ ? 16 : 1); // Throttle pause and drivers that refuse VSync.
    }
    return 0;
}
}
