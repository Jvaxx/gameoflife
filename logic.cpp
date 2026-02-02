#include "logic.h"
#include "graphics.h"
#include <cmath>
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

    const View_Matrix m3 = Maths::mat_w_to_scr(view->origin, view->theta,
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

            tile = Maths::transformed(tile, m3);
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

    const View_Matrix m3 = Maths::mat_w_to_scr(view->origin, view->theta,
                                               view->pix_per_m,
                                               view->buffer->width,
                                               view->buffer->height);
    // lignes verticales
    for (int X{col_min}; X <= col_max; ++X) {
        Vector2 p1 = {X * grid->tile_size + grid->origin.x, raw_min * grid->tile_size + grid->origin.y};
        Vector2 p2 = {X * grid->tile_size + grid->origin.x, (raw_max + 0) * grid->tile_size + grid->origin.y};
        p1 = Maths::transformed(p1, m3);
        p2 = Maths::transformed(p2, m3);
        Vector2_int p1_int = Maths::pt_float_to_int(p1);
        Vector2_int p2_int = Maths::pt_float_to_int(p2);
        Graphics::draw_line_bresenham(view->buffer, p1_int, p2_int, 0xFF00FF00);
    }

    // lignes horizontales
    for (int Y{raw_min}; Y <= raw_max; ++Y) {
        Vector2 p1 = {col_min * grid->tile_size + grid->origin.x, Y * grid->tile_size + grid->origin.y};
        Vector2 p2 = {(col_max + 0) * grid->tile_size + grid->origin.x, Y * grid->tile_size + grid->origin.y};
        p1 = Maths::transformed(p1, m3);
        p2 = Maths::transformed(p2, m3);
        Vector2_int p1_int = Maths::pt_float_to_int(p1);
        Vector2_int p2_int = Maths::pt_float_to_int(p2);
        Graphics::draw_line_bresenham(view->buffer, p1_int, p2_int, 0xFF00FF00);
    }
}

void update_grid(Grid &grid) {
    Grid old_grid{grid};
    for (int row{}; row < grid.h; ++row) {
        for (int col{}; col < grid.w; ++col) {
            int sum{};
            for (int x{-1}; x < 2; ++x) {
                for (int y{-1}; y < 2; ++y) {
                    if (col + x >= 0 && col + x < grid.w && row + y >= 0 && row + y < grid.h && (x != 0 || y != 0))
                        sum += (old_grid.get(col + x, row + y)) ? 1 : 0;
                }
            }
            switch (sum) {
            case 3:
                grid.set(col, row, 1);
                break;

            case 2:
                grid.set(col, row, grid.get(col, row));
                break;

            default:
                grid.set(col, row, 0);
            }
        }
    }
}
