#include "editor.h"

#include <SDL3/SDL.h>
#include <cstdlib>

Editor::Editor() {}

int Editor::init() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Initialization failed: %s", SDL_GetError());
        return EXIT_FAILURE;
    };

    SDL_Window *window =
        SDL_CreateWindow("My Funny Window", 800, 600, SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    return 0;
}

Editor::~Editor() { SDL_Quit(); }

void Editor::loop() {
    bool quit = false;
    SDL_Event evt;
    while (!quit) {
        // SDL Events
        while (SDL_PollEvent(&evt)) {
            switch (evt.type) {

            case SDL_EVENT_QUIT:
                quit = true;
                break;

            case SDL_EVENT_KEY_DOWN:
                if (evt.key.key == SDLK_ESCAPE) {
                    quit = true;
                    break;
                }
            }
        }

        // App logic
    }
}
