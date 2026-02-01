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

float pi = std::numbers::pi_v<float>;

static SDL_Window *game_window = std::nullptr_t();
static SDL_Renderer *game_renderer = std::nullptr_t();
static Pixel_buffer *main_buffer = new Pixel_buffer{};
static View *game_view = new View{main_buffer};
static Grid *game_grid = new Grid(5, 5);

bool get_bounding_box(View *view, Grid *grid, int *c_min, int *c_max, int *r_min, int *r_max) {
    // NOTE: Full heuristique, pas sûr des cas limite, et pas opti. (voir bounding box "AABB")
    float sqr = std::sqrt(std::pow(view->buffer->height, 2) + std::pow(view->buffer->width, 2)) / 2;
    float itan = std::atan2(view->buffer->height, view->buffer->width);
    float x_max = sqr * std::cos(-itan + std::remainder(view->theta, pi / 2)) + view->origin.x;
    float y_max = sqr * std::sin(itan + std::remainder(view->theta, pi / 2)) + view->origin.y;
    float x_min = -sqr * std::cos(-itan + std::remainder(view->theta, pi / 2)) + view->origin.x;
    float y_min = -sqr * std::sin(itan + std::remainder(view->theta, pi / 2)) + view->origin.y;

    // limites d'affichage théorique de la grille
    int col_min = static_cast<int>(x_min / grid->tile_size - grid->origin.x);
    int raw_min = static_cast<int>(y_min / grid->tile_size - grid->origin.y);
    int col_max = static_cast<int>(x_max / grid->tile_size - grid->origin.x + 0.9f);
    int raw_max = static_cast<int>(y_max / grid->tile_size - grid->origin.y + 0.9f);
    if (col_min >= grid->w || raw_min >= grid->h || col_max < 0 || raw_max < 0)
        return false; // complètement en dehors de l'écran

    // limites de la grilles réelles
    *c_min = (col_min < 0) ? 0 : col_min;
    *r_min = (raw_min < 0) ? 0 : raw_min;
    *c_max = (col_max > grid->w) ? grid->w : col_max;
    *r_max = (raw_max > grid->h) ? grid->h : raw_max;
    return true;
}

void fill_grid(View *view, Grid *grid, SDL_Renderer *renderer) {
    // calcul des limites d'affichage (en coordonnées réelles)
    int col_min, raw_min, col_max, raw_max;
    if (!get_bounding_box(view, grid, &col_min, &col_max, &raw_min, &raw_max))
        return; // complètement en dehors de l'écran

    const View_Matrix m3 = Maths::create_view_mat(view->origin, view->theta,
                                                  view->pix_per_m,
                                                  view->buffer->width,
                                                  view->buffer->height);

    for (int Y{raw_min}; Y < raw_max; ++Y) {
        for (int X{col_min}; X < col_max; ++X) {
            std::vector<Vector2> tile{
                {X * grid->tile_size + grid->origin.x, Y * grid->tile_size + grid->origin.y},
                {(X + 1) * grid->tile_size + grid->origin.x, Y * grid->tile_size + grid->origin.y},
                {(X + 1) * grid->tile_size + grid->origin.x, (Y + 1) * grid->tile_size + grid->origin.y},
                {X * grid->tile_size + grid->origin.x, (Y + 1) * grid->tile_size + grid->origin.y},
            };

            tile = Maths::world_to_scr(tile, m3);
            if (grid->get(X, Y)) {
                Graphics::draw_polygon(view->buffer, tile, 0xFF0000FF);
            } else {
                Graphics::draw_polygon(view->buffer, tile, 0xFFFFFFFF);
            }
        }
    }
}

void draw_grid(View *view, Grid *grid, SDL_Renderer *renderer) {
    // calcul des limites d'affichage (en coordonnées réelles)
    int col_min, raw_min, col_max, raw_max;
    if (!get_bounding_box(view, grid, &col_min, &col_max, &raw_min, &raw_max))
        return; // complètement en dehors de l'écran

    const View_Matrix m3 = Maths::create_view_mat(view->origin, view->theta,
                                                  view->pix_per_m,
                                                  view->buffer->width,
                                                  view->buffer->height);
    // lignes verticales
    for (int X{col_min}; X <= col_max; ++X) {
        Vector2 p1 = {X * grid->tile_size + grid->origin.x, raw_min * grid->tile_size + grid->origin.y};
        Vector2 p2 = {X * grid->tile_size + grid->origin.x, (raw_max + 0) * grid->tile_size + grid->origin.y};
        p1 = Maths::world_to_scr(p1, m3);
        p2 = Maths::world_to_scr(p2, m3);
        Vector2_int p1_int = Maths::pt_float_to_int(p1);
        Vector2_int p2_int = Maths::pt_float_to_int(p2);
        Graphics::draw_line_bresenham(view->buffer, p1_int, p2_int, 0xFF00FF00);
    }

    // lignes horizontales
    for (int Y{raw_min}; Y <= raw_max; ++Y) {
        Vector2 p1 = {col_min * grid->tile_size + grid->origin.x, Y * grid->tile_size + grid->origin.y};
        Vector2 p2 = {(col_max + 0) * grid->tile_size + grid->origin.x, Y * grid->tile_size + grid->origin.y};
        p1 = Maths::world_to_scr(p1, m3);
        p2 = Maths::world_to_scr(p2, m3);
        Vector2_int p1_int = Maths::pt_float_to_int(p1);
        Vector2_int p2_int = Maths::pt_float_to_int(p2);
        Graphics::draw_line_bresenham(view->buffer, p1_int, p2_int, 0xFF00FF00);
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
    game_view->origin.x = 3;
    game_view->origin.y = 3;
    game_view->pix_per_m = 30;
    game_grid->origin.x = -3;
    game_grid->origin.y = -3;
    game_view->theta = 0.4f;

    std::cout << "App initilisée.\n";
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    Game_state *state = static_cast<Game_state *>(appstate);
    uint64_t tick{SDL_GetTicks()};
    if (tick - state->last_tick > 50) {
        main_buffer->clear_pixel(0xFFFF00FF);
        fill_grid(game_view, game_grid, game_renderer);
        draw_grid(game_view, game_grid, game_renderer);
        // game_view->origin.x -= 0.1f;
        game_view->theta += 0.001;
        main_buffer->update(game_renderer);
        state->last_tick += 50;
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
