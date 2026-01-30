#include "main.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>
#include <iostream>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    Game_state *state = new Game_state{};
    state->last_tick = SDL_GetTicks();
    *appstate = state;
    std::cout << "App initilisée.\n";
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    Game_state *state = static_cast<Game_state *>(appstate);
    uint64_t tick{SDL_GetTicks()};
    if (tick - state->last_tick > 1000) {
        std::cout << "Iteréation.\n";
        state->last_tick += 1000;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        std::cout << "On quitte avec succès.\n";
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    std::cout << "On quitte.\n";
}
