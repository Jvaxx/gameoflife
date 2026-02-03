#include "main.h"
#include "graphics.h"
#include "logic.h"
#include "maths.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

static SDL_Window *game_window = std::nullptr_t();
static SDL_Renderer *game_renderer = std::nullptr_t();
static Pixel_buffer *main_buffer = new Pixel_buffer{};
static View *game_view = new View{main_buffer};
static Grid *game_grid = new Grid(15, 15);

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Init video en échec.\n";
        return SDL_APP_FAILURE;
    }
    if (!SDL_CreateWindowAndRenderer("Ma fenêtre.",
                                     1440, 810,
                                     SDL_WINDOW_RESIZABLE,
                                     &game_window, &game_renderer)) {
        std::cerr << "Init renderer en échec.\n";
        return SDL_APP_FAILURE;
    }
    // SDL_SetRenderLogicalPresentation(game_renderer,
    //                                  1440, 810,
    //                                  SDL_LOGICAL_PRESENTATION_LETTERBOX);
    Game_state *state = new Game_state{};
    state->last_tick = SDL_GetTicks();
    *appstate = state;
    main_buffer->resize(game_renderer, 1440, 810);
    game_view->origin.x = 0;
    game_view->origin.y = 0;
    game_view->pix_per_m = 20;
    game_view->theta = 0.2;
    game_grid->origin.x = 0;
    game_grid->origin.y = 0;
    game_grid->tile_size = 1;

    game_grid->set(1, 10, 1);
    game_grid->set(2, 9, 1);
    game_grid->set(2, 8, 1);
    game_grid->set(1, 8, 1);
    game_grid->set(0, 8, 1);

    std::cout << "App initilisée.\n";
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    Game_state *state = static_cast<Game_state *>(appstate);
    uint64_t tick{SDL_GetTicks()};
    process_input(state->input, *game_view, *game_grid);
    if (tick - state->last_tick > 100) {
        main_buffer->clear_pixel(0xFFFF00FF);
        fill_grid(game_view, game_grid, game_renderer);
        draw_grid(game_view, game_grid, game_renderer);
        update_grid(*game_grid);
        main_buffer->update(game_renderer);
        state->last_tick += 100;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    Game_state *state = reinterpret_cast<Game_state *>(appstate);
    switch (event->type) {
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        std::string button{"?"};
        switch (event->button.button) {
        case SDL_BUTTON_LEFT:
            state->input.mouse_l.press = (event->button.down) ? 1 : state->input.mouse_l.press;
            state->input.mouse_l.x = event->button.x;
            state->input.mouse_l.y = event->button.y;
            break;
        case SDL_BUTTON_RIGHT:
            state->input.mouse_r.press = (event->button.down) ? 1 : state->input.mouse_r.press;
            state->input.mouse_r.x = event->button.x;
            state->input.mouse_r.y = event->button.y;
            break;
        case SDL_BUTTON_MIDDLE:
            break;
        case SDL_BUTTON_X1:
            break;
        case SDL_BUTTON_X2:
            break;
        default:
            break;
        }
        break;
    }

    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        switch (event->key.scancode) {
        case SDL_SCANCODE_H:
            state->input.mv_l.press = (event->key.down) ? 1 : state->input.mv_l.press;
            break;
        case SDL_SCANCODE_J:
            // NOTE: 4097 pour le modifieur shift, je n'arrive pas à faire fonctionner les flags SDL.
            if (event->key.mod == 4097) {
                state->input.rot_pos.press = (event->key.down) ? 1 : state->input.rot_pos.press;
            } else {
                state->input.mv_d.press = (event->key.down) ? 1 : state->input.mv_d.press;
            }
            break;
        case SDL_SCANCODE_K:
            if (event->key.mod == 4097) {
                state->input.rot_neg.press = (event->key.down) ? 1 : state->input.rot_neg.press;
            } else {
                state->input.mv_u.press = (event->key.down) ? 1 : state->input.mv_u.press;
            }
            break;
        case SDL_SCANCODE_L:
            state->input.mv_r.press = (event->key.down) ? 1 : state->input.mv_r.press;
            break;
        default:
            break;
        }
        // std::cout << "scancode:" << event->key.scancode << ", mod:" << event->key.mod
        //           << ", down:" << event->key.down << ", repeat:" << event->key.repeat << '\n';
        break;
    }

    case SDL_EVENT_MOUSE_WHEEL:
        state->input.wheel.scroll = event->wheel.y;
        // std::cout << "x:" << event->wheel.y << ", int_x:" << event->wheel.integer_x << ", int_y" << event->wheel.integer_y << "\n";
        break;

    case SDL_EVENT_WINDOW_RESIZED:
        std::cout << "Resized: " << event->window.data1 << ", " << event->window.data2 << '\n';
        main_buffer->resize(game_renderer, event->window.data1, event->window.data2);
        break;
    case SDL_EVENT_QUIT:
        std::cout << "On quitte avec succès.\n";
        return SDL_APP_SUCCESS;

    default:
        return SDL_APP_CONTINUE;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    std::cout << "On quitte.\n";
}
