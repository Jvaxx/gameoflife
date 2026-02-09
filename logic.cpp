#include "logic.h"
#include "graphics.h"
#include <SDL3/SDL_timer.h>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

bool get_bounding_box(World_view *view, Grid *grid, int *c_min, int *c_max, int *r_min, int *r_max) {
    float sin_t = std::abs(std::sin(view->theta));
    float cos_t = std::abs(std::cos(view->theta));
    float w = view->buff->rec.w / (view->pix_per_m * 2);
    float h = view->buff->rec.h / (view->pix_per_m * 2);

    float Rx = w * cos_t + h * sin_t;
    float Ry = w * sin_t + h * cos_t;

    int col_min = static_cast<int>(std::floor((view->origin.x - Rx) / grid->tile_size));
    int row_min = static_cast<int>(std::floor((view->origin.y - Ry) / grid->tile_size));
    int col_max = static_cast<int>(std::ceil((view->origin.x + Rx) / grid->tile_size));
    int row_max = static_cast<int>(std::ceil((view->origin.y + Ry) / grid->tile_size));
    if (col_min >= grid->w || row_min >= grid->h || col_max < 0 || row_max < 0)
        return false;

    *c_min = (col_min < 0) ? 0 : col_min;
    *r_min = (row_min < 0) ? 0 : row_min;
    *c_max = (col_max > grid->w) ? grid->w : col_max;
    *r_max = (row_max > grid->h) ? grid->h : row_max;
    return true;
}

void fill_grid(World_view *view, Grid *grid, Game_state *state) {
    // NOTE: Claqué au sol. Voir fill_grid2. (complexité O(n^2))
    uint64_t fill_start_time{SDL_GetTicks()};
    // calcul des limites d'affichage (en coordonnées réelles)
    int col_min, raw_min, col_max, raw_max;
    if (!get_bounding_box(view, grid, &col_min, &col_max, &raw_min, &raw_max))
        return; // complètement en dehors de l'écran

    const View_Matrix m3 = Maths::mat_w_to_scr(view->origin, view->theta,
                                               view->pix_per_m,
                                               view->buff->rec.w,
                                               view->buff->rec.h);

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
                uint64_t start_draw_poly{SDL_GetTicks()};
                Graphics::draw_polygon(view->buff, tile, 0xFF0000FF);
                state->draw_poly_time += SDL_GetTicks() - start_draw_poly;
            } else {
                uint64_t start_draw_poly{SDL_GetTicks()};
                Graphics::draw_polygon(view->buff, tile, 0xFFFFFFFF);
                state->draw_poly_time += SDL_GetTicks() - start_draw_poly;
            }
            ++state->draw_poly_since_log;
        }
    }
    state->draw_tiles_time_internal += SDL_GetTicks() - fill_start_time;
}

void fill_grid2(World_view *view, Grid *grid, Game_state *state) {
    // Itère ligne par ligne pour draw plusieurs tiles à la fois quand possible pour
    // réduire le nombre d'appel à draw_polygon
    // Complexité: O(nLignesAffichées) + nVivantSurLaLigneEtNonContigu

    // calcul des limites d'affichage (en coordonnées réelles)
    int col_min, raw_min, col_max, raw_max;
    if (!get_bounding_box(view, grid, &col_min, &col_max, &raw_min, &raw_max))
        return; // complètement en dehors de l'écran

    const View_Matrix m3 = Maths::mat_w_to_scr(view->origin, view->theta,
                                               view->pix_per_m,
                                               view->buff->rec.w,
                                               view->buff->rec.h);
    Vector2 dx_screen{
        grid->tile_size * m3.m00,
        grid->tile_size * m3.m10};

    // clear le fond pour éviter de déssiner les tuiles vides
    std::vector<Vector2> background{
        {col_min * grid->tile_size + grid->origin.x, raw_min * grid->tile_size + grid->origin.y},
        {col_max * grid->tile_size + grid->origin.x, raw_min * grid->tile_size + grid->origin.y},
        {col_max * grid->tile_size + grid->origin.x, raw_max * grid->tile_size + grid->origin.y},
        {col_min * grid->tile_size + grid->origin.x, raw_max * grid->tile_size + grid->origin.y},
    };
    background = Maths::transformed(background, m3);
    Graphics::draw_convex_quad(view->buff, background, 0xFFFFFFFF);
    state->draw_poly_since_log += 1;

    std::array<Vector2, 4> poly_buffer;
    for (int Y{raw_min}; Y < raw_max; ++Y) {
        Vector2 row_p00(col_min * grid->tile_size + grid->origin.x, Y * grid->tile_size + grid->origin.y);
        Vector2 row_p01(col_min * grid->tile_size + grid->origin.x, (Y + 1) * grid->tile_size + grid->origin.y);
        row_p00 = Maths::transformed(row_p00, m3);
        row_p01 = Maths::transformed(row_p01, m3);
        int Xtmp{col_min};
        while (Xtmp < col_max) {
            while (Xtmp < col_max && !grid->current[(Xtmp + 1) + (grid->w + 2) * (Y + 1)]) {
                ++Xtmp;
            }
            if (Xtmp >= col_max)
                break;

            // On est sur une case allumée, on cherche sa fin
            int start_X{Xtmp};
            while (Xtmp < col_max && grid->current[(Xtmp + 1) + (grid->w + 2) * (Y + 1)]) {
                Xtmp++;
            }
            int end_X{Xtmp};

            float start = static_cast<float>(start_X - col_min);
            float end = static_cast<float>(end_X - col_min);
            poly_buffer[0] = row_p00 + start * dx_screen;
            poly_buffer[1] = row_p00 + end * dx_screen;
            poly_buffer[2] = row_p01 + end * dx_screen;
            poly_buffer[3] = row_p01 + start * dx_screen;
            uint64_t time_start = SDL_GetPerformanceCounter();
            Graphics::draw_convex_quad(view->buff, poly_buffer, 0xFF0000FF);
            state->draw_tiles_time_internal += SDL_GetPerformanceCounter() - time_start;
            state->draw_tiles_count_internal++;
            state->draw_poly_since_log += 1;
        }
    }
}

void draw_grid(World_view *view, Grid *grid) {
    // calcul des limites d'affichage (en coordonnées réelles)
    if (grid->tile_size * view->pix_per_m < 5)
        return; // zoom trop faible, inutile d'afficher la grille
    int col_min, raw_min, col_max, raw_max;
    if (!get_bounding_box(view, grid, &col_min, &col_max, &raw_min, &raw_max))
        return; // complètement en dehors de l'écran

    const View_Matrix m3 = Maths::mat_w_to_scr(view->origin, view->theta,
                                               view->pix_per_m,
                                               view->buff->rec.w,
                                               view->buff->rec.h);
    // lignes verticales
    for (int X{col_min}; X <= col_max; ++X) {
        Vector2 p1 = {X * grid->tile_size + grid->origin.x, raw_min * grid->tile_size + grid->origin.y};
        Vector2 p2 = {X * grid->tile_size + grid->origin.x, (raw_max + 0) * grid->tile_size + grid->origin.y};
        p1 = Maths::transformed(p1, m3);
        p2 = Maths::transformed(p2, m3);
        Vector2_int p1_int = Maths::pt_float_to_int(p1);
        Vector2_int p2_int = Maths::pt_float_to_int(p2);
        Graphics::draw_line_bresenham(view->buff, p1_int, p2_int, 0xFF00FF00);
    }

    // lignes horizontales
    for (int Y{raw_min}; Y <= raw_max; ++Y) {
        Vector2 p1 = {col_min * grid->tile_size + grid->origin.x, Y * grid->tile_size + grid->origin.y};
        Vector2 p2 = {(col_max + 0) * grid->tile_size + grid->origin.x, Y * grid->tile_size + grid->origin.y};
        p1 = Maths::transformed(p1, m3);
        p2 = Maths::transformed(p2, m3);
        Vector2_int p1_int = Maths::pt_float_to_int(p1);
        Vector2_int p2_int = Maths::pt_float_to_int(p2);
        Graphics::draw_line_bresenham(view->buff, p1_int, p2_int, 0xFF00FF00);
    }
}

void update_grid(Grid &grid) {
    const int stride = grid.w + 2;

    const int *row_top = grid.current;
    const int *row_mid = grid.current + stride;
    const int *row_bot = grid.current + 2 * stride;

    int *p_dest = grid.next + stride;

    for (int row{1}; row <= grid.h; ++row) {
        for (int col{1}; col <= grid.w; ++col) {
            int sum = row_top[col - 1] + row_top[col] + row_top[col + 1] +
                      row_mid[col - 1] + row_mid[col + 1] +
                      row_bot[col - 1] + row_bot[col] + row_bot[col + 1];

            p_dest[col] = (sum == 3) | ((sum == 2) & row_mid[col]);
        }
        row_top += stride;
        row_mid += stride;
        row_bot += stride;
        p_dest += stride;
    }
    std::swap(grid.current, grid.next);
}

Vector2 px_to_tile(World_view &view, Grid &grid, Vector2 &in) {
    // Transforme les coordonnées Ecran (px) vers coordonnées dans le repère de la grille.
    // Peut-être out of bounds sans problème.
    View_Matrix m = Maths::mat_scr_to_w(view.origin, view.theta, view.pix_per_m,
                                        view.buff->rec.w, view.buff->rec.h);
    Vector2 real{Maths::transformed(in, m)};
    real += grid.origin;
    return {real.x / grid.tile_size, real.y / grid.tile_size};
}

bool tile_clic(World_view &view, Grid &grid, Vector2 in, int value) {
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

void process_input(Game_state *state, World_view &view, Grid &grid) {
    // std::cout << "    Processing input\n";
    uint64_t start_time = SDL_GetTicks();
    bool input_processed{false};
    // TODO: Améliorer ça?
    float dt = SDL_GetTicks() - state->last_input_proc;
    const float MOVE_UNIT{(0.12f / view.pix_per_m) * dt}; // Constante temporaire, à voir plus tard.
    const float ZOOM_UNIT{0.08f * view.pix_per_m};        // Constante temporaire, à voir plus tard.
    const float ROT_UNIT{0.001f * dt};                    // Constante temporaire, à voir plus tard.
    const float MSPT_MULT{0.1f};                          // Constante temporaire, à voir plus tard.
    if (state->input.mv_l.press) {
        // move left
        view.origin += Maths::transformed({-MOVE_UNIT, 0},
                                          {std::cos(view.theta), -std::sin(view.theta), 0,
                                           std::sin(view.theta), std::cos(view.theta), 0});
        input_processed = true;
    }
    if (state->input.mv_r.press) {
        // move left
        view.origin += Maths::transformed({+MOVE_UNIT, 0},
                                          {std::cos(view.theta), -std::sin(view.theta), 0,
                                           std::sin(view.theta), std::cos(view.theta), 0});
        input_processed = true;
    }
    if (state->input.mv_u.press) {
        // move left
        view.origin += Maths::transformed({0, +MOVE_UNIT},
                                          {std::cos(view.theta), -std::sin(view.theta), 0,
                                           std::sin(view.theta), std::cos(view.theta), 0});
        input_processed = true;
    }
    if (state->input.mv_d.press) {
        // move left
        view.origin += Maths::transformed({0, -MOVE_UNIT},
                                          {std::cos(view.theta), -std::sin(view.theta), 0,
                                           std::sin(view.theta), std::cos(view.theta), 0});
        input_processed = true;
    }
    if (state->input.wheel.scroll) {
        // zoom
        view.pix_per_m += ZOOM_UNIT * state->input.wheel.scroll;
        state->input.wheel.scroll = 0;
        input_processed = true;
    }
    if (state->input.rot_pos.press) {
        // rotation positive du viewport
        view.theta += ROT_UNIT;
        input_processed = true;
    }
    if (state->input.rot_neg.press) {
        // rotation negative du viewport
        view.theta -= ROT_UNIT;
        input_processed = true;
    }
    if (state->input.mouse_l.press) {
        // allumer une case
        tile_clic(view, grid, {state->input.mouse_l.x, state->input.mouse_l.y}, 1);
        input_processed = true;
    }
    if (state->input.mouse_r.press) {
        // éteindre une case
        tile_clic(view, grid, {state->input.mouse_r.x, state->input.mouse_r.y}, 0);
        input_processed = true;
    }
    if (state->input.pause.press) {
        // pause la simulation
        state->playing = !state->playing;
        state->input.pause.press = 0;
        input_processed = true;
    }
    if (state->input.speed_up.press) {
        // accélérer la simulation
        state->mspt -= state->mspt * MSPT_MULT;
        state->input.speed_up.press = 0;
        std::cout << "[tick/s]: " << 1000 / state->mspt << '\n';
        input_processed = true;
    }
    if (state->input.slow_down.press) {
        // ralentir la simulation
        state->mspt += state->mspt * MSPT_MULT;
        state->input.slow_down.press = 0;
        std::cout << "[tick/s]: " << 1000 / state->mspt << '\n';
        input_processed = true;
    }

    // Perf log
    if (input_processed && (SDL_GetTicks() - start_time) > 1) {
        std::cout << "[WARNING] Long input process time: " << start_time - SDL_GetTicks() << " ms\n";
    }

    state->last_input_proc = SDL_GetTicks();
}

void randomize_grid(Grid &grid, float proba) {
    // Rempli une grille tel que chaque case ait un proba "proba" comprise entre 0 et 1 d'être vivante.
    std::mt19937 mt(42);
    std::uniform_real_distribution alive{0.0f, 1.0f};
    for (int X{}; X < grid.w; ++X) {
        for (int Y{}; Y < grid.h; ++Y) {
            if (alive(mt) < proba) {
                grid.current[X + 1 + (Y + 1) * (grid.w + 2)] = 1;
            }
        }
    }
}

void render_menu(Menu_view *view) {
    // std::cout << "enter render_menu\n";
    view->buff->clear_pixel(0xFFFFFFFF);
    Vector2 p1, p2, p3, p4;
    for (int i = 0; i < view->buttons.size(); ++i) {
        p1 = {static_cast<float>(view->buttons[i].w + 10) * i + 20, 20};
        p2 = {static_cast<float>(view->buttons[i].w) * (i + 1) + 20, 20};
        p3 = {static_cast<float>(view->buttons[i].w) * (i + 1) + 20, static_cast<float>(view->buttons[i].h) + 20};
        p4 = {static_cast<float>(view->buttons[i].w + 10) * i + 20, static_cast<float>(view->buttons[i].h) + 20};
        std::vector<Vector2> corners{p1, p2, p3, p4};
        Graphics::draw_convex_quad(view->buff, corners, 0xFFFF0000);
    }
    view->buff->waiting_update = true;
}

void print_logs(Game_state *state) {
    std::cout << "[STATS] AVG Effective TPS: " << state->ticks_since_log / 2.000f << '\n';
    std::cout << "[STATS] AVG MSPT: " << static_cast<float>(state->process_time) * 1000 / (state->ticks_since_log * SDL_GetPerformanceFrequency()) << '\n';
    std::cout << "[STATS] AVG MSPF: " << static_cast<float>(state->frame_time) * 1000 / (state->frames_since_log * SDL_GetPerformanceFrequency()) << '\n';
    // std::cout << "[STATS] AVG MSPDrawGrid: " << static_cast<float>(state->draw_grid_time) * 1000 / (state->frames_since_log * SDL_GetPerformanceFrequency()) << '\n';
    // std::cout << "[STATS] AVG MSPDrawTiles: " << static_cast<float>(state->draw_tiles_time) * 1000 / (state->frames_since_log * SDL_GetPerformanceFrequency()) << '\n';
    // std::cout << "[STATS] AVG MSPDrawTilesInternal: " << static_cast<float>(state->draw_tiles_time_internal) * 1000 / (state->frames_since_log * SDL_GetPerformanceFrequency()) << '\n';
    // std::cout << "[STATS] AVG MSPDrawPoly (x10K): " << 10000 * static_cast<float>(state->draw_poly_time) / state->draw_poly_since_log << '\n';
    // std::cout << "[STATS] AVG MSPClrPx: " << static_cast<float>(state->clr_px_time) * 1000 / (state->frames_since_log * SDL_GetPerformanceFrequency()) << '\n';
    // std::cout << "[STATS] AVG MSPBuffUpdt: " << static_cast<float>(state->buff_updt_time) * 1000 / (state->frames_since_log * SDL_GetPerformanceFrequency()) << '\n';
    // std::cout << "[STATS] AVG Poly/F: " << static_cast<float>(state->draw_poly_since_log) / state->frames_since_log << '\n';
    std::cout << '\n';
    state->ticks_since_log = 0;
    state->frames_since_log = 0;
    state->draw_poly_since_log = 0;
    state->process_time = 0;
    state->frame_time = 0;
    state->draw_grid_time = 0;
    state->fill_tiles_time = 0;
    state->draw_tiles_time_internal = 0;
    state->draw_tiles_count_internal = 0;
    state->draw_poly_time = 0;
    state->clr_px_time = 0;
    state->render_menu_time = 0;
    state->buff_updt_time = 0;
    state->last_log = SDL_GetPerformanceCounter();
}
