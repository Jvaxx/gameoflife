#include "logic.h"
#include "graphics.h"
#include <cmath>
bool get_bounding_box(View *view, Grid *grid, int *c_min, int *c_max, int *r_min, int *r_max) {
    // NOTE: Full heuristique, pas sûr des cas limite, et pas opti. (voir bounding box "AABB")
    // BUG: Sur des angles proches de pi/2, problème de bounding box.
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

Vector2 px_to_tile(View &view, Grid &grid, Vector2 &in) {
    // Transforme les coordonnées Ecran (px) vers coordonnées dans le repère de la grille.
    // Peut-être out of bounds sans problème.
    View_Matrix m = Maths::mat_scr_to_w(view.origin, view.theta, view.pix_per_m,
                                        view.buffer->width, view.buffer->height);
    Vector2 real{Maths::transformed(in, m)};
    real += grid.origin;
    return {real.x / grid.tile_size, real.y / grid.tile_size};
}

bool tile_clic(View &view, Grid &grid, Vector2 in, int value) {
    // Allume la case cliquée (prend les coordonnées pixel brutes de l'écran) si la case est valide.
    // Ne fait rien sinon. Renvoie true si succès, false si échec.
    Vector2 px{px_to_tile(view, grid, in)};
    if (px.x < 0 || px.x > grid.w || px.y < 0 || px.y > grid.h) {
        return false;
    }
    // std::cout << "set avec value:" << value << " en (x,y):" << static_cast<int>(px.x) << ", " << static_cast<int>(px.y) << "\n";
    grid.set(static_cast<int>(px.x), static_cast<int>(px.y), value);
    return true;
}

void process_input(Game_input &input, View &view, Grid &grid) {
    // TODO: Améliorer ça?
    const float MOVE_UNIT{0.2f};                   // Constante temporaire, à voir plus tard.
    const float ZOOM_UNIT{0.05f * view.pix_per_m}; // Constante temporaire, à voir plus tard.
    const float ROT_UNIT{0.02f};                   // Constante temporaire, à voir plus tard.
    if (input.mv_l.press) {
        // move left
        view.origin += Maths::transformed({-MOVE_UNIT, 0},
                                          {std::cos(view.theta), -std::sin(view.theta), 0,
                                           std::sin(view.theta), std::cos(view.theta), 0});
        input.mv_l.press = 0;
    }
    if (input.mv_r.press) {
        // move left
        view.origin += Maths::transformed({+MOVE_UNIT, 0},
                                          {std::cos(view.theta), -std::sin(view.theta), 0,
                                           std::sin(view.theta), std::cos(view.theta), 0});
        input.mv_r.press = 0;
    }
    if (input.mv_u.press) {
        // move left
        view.origin += Maths::transformed({0, +MOVE_UNIT},
                                          {std::cos(view.theta), -std::sin(view.theta), 0,
                                           std::sin(view.theta), std::cos(view.theta), 0});
        input.mv_u.press = 0;
    }
    if (input.mv_d.press) {
        // move left
        view.origin += Maths::transformed({0, -MOVE_UNIT},
                                          {std::cos(view.theta), -std::sin(view.theta), 0,
                                           std::sin(view.theta), std::cos(view.theta), 0});
        input.mv_d.press = 0;
    }
    if (input.wheel.scroll) {
        // zoom
        view.pix_per_m += ZOOM_UNIT * input.wheel.scroll;
        input.wheel.scroll = 0;
    }
    if (input.rot_pos.press) {
        // rotation positive du viewport
        view.theta += ROT_UNIT;
        input.rot_pos.press = 0;
    }
    if (input.rot_neg.press) {
        // rotation negative du viewport
        view.theta -= ROT_UNIT;
        input.rot_neg.press = 0;
    }
    if (input.mouse_l.press) {
        // allumer une case
        tile_clic(view, grid, {input.mouse_l.x, input.mouse_l.y}, 1);
        input.mouse_l.press = 0;
    }
    if (input.mouse_r.press) {
        // éteindre une case
        tile_clic(view, grid, {input.mouse_r.x, input.mouse_r.y}, 0);
        input.mouse_r.press = 0;
    }
}
