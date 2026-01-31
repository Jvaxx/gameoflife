#include "main.h"
#include "graphics.h"
#include "maths.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
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
static Grid *game_grid = new Grid(5, 5);

void draw_grid_opti(View *view, Grid *grid, SDL_Renderer *renderer) {
    uint32_t grid_color = 0xFF00FF00; // Vert
    int screen_w = view->buffer->width;
    int screen_h = view->buffer->height;

    float grid_world_w = grid->w * grid->tile_size;
    float grid_world_h = grid->h * grid->tile_size;

    int grid_screen_x0 = static_cast<int>((0.0f - view->origin.x) * view->pix_per_m);
    int grid_screen_y0 = static_cast<int>((0.0f - view->origin.y) * view->pix_per_m);
    int grid_screen_x1 = static_cast<int>((grid_world_w - view->origin.x) * view->pix_per_m);
    int grid_screen_y1 = static_cast<int>((grid_world_h - view->origin.y) * view->pix_per_m);

    int v_line_y_start = std::clamp(grid_screen_y0, 0, screen_h);
    int v_line_y_end = std::clamp(grid_screen_y1, 0, screen_h);

    int h_line_x_start = std::clamp(grid_screen_x0, 0, screen_w);
    int h_line_x_end = std::clamp(grid_screen_x1, 0, screen_w);

    if (v_line_y_start > v_line_y_end || h_line_x_start > h_line_x_end)
        return;

    float world_left = view->origin.x;
    float world_right = view->origin.x + (screen_w / view->pix_per_m);

    float world_top = view->origin.y;
    float world_bottom = view->origin.y + (screen_h / view->pix_per_m);

    int col_min = std::clamp(static_cast<int>(std::floor(world_left / grid->tile_size)), 0, grid->w);
    int col_max = std::clamp(static_cast<int>(std::ceil(world_right / grid->tile_size)), 0, grid->w);
    int row_min = std::clamp(static_cast<int>(std::floor(world_top / grid->tile_size)), 0, grid->h);
    int row_max = std::clamp(static_cast<int>(std::ceil(world_bottom / grid->tile_size)), 0, grid->h);

    // lignes verticales
    for (int col = col_min; col <= col_max; ++col) {
        // Coordonnée X de la ligne verticale
        int screen_x = static_cast<int>((col * grid->tile_size - view->origin.x) * view->pix_per_m);

        if (screen_x >= 0 && screen_x < screen_w) {
            Graphics::draw_line_vertical(view->buffer, screen_x, v_line_y_start, v_line_y_end, grid_color);
        }
    }

    // lignes horizontales
    for (int row = row_min; row <= row_max; ++row) {
        // Coordonnée Y de la ligne horizontale
        int screen_y = static_cast<int>((row * grid->tile_size - view->origin.y) * view->pix_per_m);

        if (screen_y >= 0 && screen_y < screen_h) {
            Graphics::draw_line_horizontal(view->buffer, screen_y, h_line_x_start, h_line_x_end, grid_color);
        }
    }
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Init video en échec.\n";
        return SDL_APP_FAILURE;
    }
    if (!SDL_CreateWindowAndRenderer("Ma fenêtre.",
                                     600, 600,
                                     SDL_WINDOW_RESIZABLE,
                                     &game_window, &game_renderer)) {
        std::cerr << "Init renderer en échec.\n";
        return SDL_APP_FAILURE;
    }
    Game_state *state = new Game_state{};
    state->last_tick = SDL_GetTicks();
    *appstate = state;

    main_buffer->resize(game_renderer, 600, 600);

    std::cout << "App initilisée.\n";
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    Game_state *state = static_cast<Game_state *>(appstate);
    uint64_t tick{SDL_GetTicks()};
    if (tick - state->last_tick > 500) {
        main_buffer->clear_pixel(0xFFFF00FF);
        draw_grid_opti(game_view, game_grid, game_renderer);
        game_view->origin.x -= 0.1f;
        // std::vector<Vector2> poly{
        //     {-20, 100},
        //     {100, 200},
        //     {60, 150},
        //     {50, 0}};
        // Graphics::draw_polygon(main_buffer, poly, 0xFF00FF00);
        main_buffer->update(game_renderer);
        state->last_tick += 500;
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
