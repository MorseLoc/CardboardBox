#include "core/Application.h"
#include <SDL3/SDL.h>
#include <exception>
#include <cstdlib>

int main() {
    try {
        fl::Application app;
        return app.run();
    } catch (const std::exception& error) {
        SDL_Log("Startup/runtime failure: %s", error.what());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "FemboyLover.exe - launch failed", error.what(), nullptr);
        return EXIT_FAILURE;
    }
}
