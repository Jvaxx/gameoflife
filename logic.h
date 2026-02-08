#include <SDL3/SDL_rect.h>
#include <iostream>
#ifndef LOGIC_H
#include "maths.h"
#include <SDL3/SDL_render.h>
#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>
struct Mouse_button {
    bool press{};
    float x;
    float y;
};

struct Key_input {
    bool press{};
};

struct Mouse_wheel {
    float scroll{};
};

struct Game_input {
    Mouse_button mouse_l{};
    Mouse_button mouse_r{};
    Mouse_wheel wheel{};
    Key_input mv_u{};
    Key_input mv_d{};
    Key_input mv_l{};
    Key_input mv_r{};
    Key_input rot_pos{};
    Key_input rot_neg{};
    Key_input pause{};
    Key_input slow_down{};
    Key_input speed_up{};
};

struct Game_state {
    uint64_t last_tick;
    uint64_t last_frame;
    uint64_t last_input_proc;
    uint64_t process_time{0};
    uint64_t frames_since_log{0};
    uint64_t frame_time{0};
    uint64_t draw_grid_time{0};
    uint64_t fill_tiles_time{0};
    uint64_t draw_tiles_time_internal{0};
    uint64_t draw_tiles_count_internal{0};
    uint64_t draw_poly_time{0};
    uint64_t draw_poly_since_log{0};
    uint64_t clr_px_time{0};
    uint64_t render_menu_time{0};
    uint64_t buff_updt_time{0};
    uint64_t ticks_since_log{0};
    uint64_t last_log;
    float mspt{100};
    bool playing{};
    Game_input input;
};

struct Texture_deleter {
    void operator()(SDL_Texture *t) {
        if (t)
            SDL_DestroyTexture(t);
    }
};

struct View_buffer {
    // Buffer de pixels d'une view (activité princpale, menu, etc...)
    std::unique_ptr<SDL_Texture, Texture_deleter> texture;
    std::vector<uint32_t> pixels{};
    int pitch{};
    bool active{true};
    bool waiting_update{true};
    SDL_FRect fdest{0, 0};

    View_buffer() = default;

    void resize(SDL_Renderer *renderer, int new_width, int new_height) {
        fdest.w = new_width;
        fdest.h = new_height;
        pixels.resize(fdest.w * fdest.h);
        SDL_Texture *new_texture = SDL_CreateTexture(renderer,
                                                     SDL_PIXELFORMAT_ARGB8888,
                                                     SDL_TEXTUREACCESS_STREAMING,
                                                     fdest.w, fdest.h);
        texture.reset(new_texture);
        pitch = new_width * sizeof(uint32_t);
        waiting_update = true;
    }

    void upload_if_needed() {
        if (waiting_update && texture) {
            SDL_UpdateTexture(texture.get(), nullptr, pixels.data(), pitch);
            waiting_update = false;
        }
    }

    void render(SDL_Renderer *renderer) {
        if (active && texture) {
            SDL_RenderTexture(renderer, texture.get(), nullptr,
                              &fdest);
        }
    }

    void clear_pixel(uint32_t color) {
        pixels.assign(fdest.w * fdest.h, color);
    }
};

struct Screen_buffer {
    // Buffer de pixels final (c'est l'écran). Contient toutes les views.
    int w{};
    int h{};
    View_buffer main_view{};
    View_buffer menu_view{};

    Screen_buffer() = default;

    void resize(SDL_Renderer *renderer, int new_width, int new_height) {
        w = new_width;
        h = new_height;
        main_view.resize(renderer, w / 2, h / 3);
        main_view.fdest.x = 20;
        main_view.fdest.y = 10;
        menu_view.resize(renderer, w / 2, h / 6);
        menu_view.fdest.x = 10;
        menu_view.fdest.y = 400;
    }

    void render(SDL_Renderer *renderer) {
        main_view.upload_if_needed();
        menu_view.upload_if_needed();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        main_view.render(renderer);
        menu_view.render(renderer);

        SDL_RenderPresent(renderer);
    }
};

struct Grid {
    // Une grille de game of life. (Row-major)
    int w{20};
    int h{20};
    float tile_size{1.0}; // en mètres
    Vector2 origin{0, 0};
    std::vector<int> tiles1{}, tiles2{};
    int *current, *next;

    Grid() {
        tiles1.resize((w + 2) * (h + 2));
        tiles2.resize((w + 2) * (h + 2));
        current = tiles1.data();
        next = tiles2.data();
    }
    Grid(int width, int height)
        : w{width}, h{height} {
        tiles1.resize((width + 2) * (height + 2));
        tiles2.resize((width + 2) * (height + 2));
        current = tiles1.data();
        next = tiles2.data();
    }

    int &get(int x, int y) {
        // std::cout << "get avec: " << x << " " << y << '\n';
        assert((x >= 0 && x < w && y >= 0 && y < h) && "Pb de dimensions");
        return current[(x + 1) + (w + 2) * (y + 1)];
    }
    void set(int x, int y, int value) {
        // std::cout << "set avec: " << x << " " << y << " " << value << '\n';
        assert((x >= 0 && x < w && y >= 0 && y < h) && "Pb de dimensions");
        current[(x + 1) + (w + 2) * (y + 1)] = value;
    }
};

struct World_View {
    // Le view-port, distances et coordonnées en mètres. Y va vers le bas.
    View_buffer *buffer;
    float pix_per_m{50};  // Le zoom
    float theta{0};       // En radians
    Vector2 origin{0, 0}; // Le milieu du view-port, en mètres
};

void fill_grid(World_View *view, Grid *grid, Game_state *state);
void fill_grid2(World_View *view, Grid *grid, Game_state *state);
void draw_grid(World_View *view, Grid *grid);
void update_grid(Grid &grid);
Vector2 px_to_tile(World_View &view, Grid &grid, Vector2 &in);

// NOTE: temp pour test, mais c'est des fonction internes
bool tile_clic(World_View &view, Grid &grid, Vector2 &in);
void process_input(Game_state *state, World_View &view, Grid &grid);
void randomize_grid(Grid &grid, float proba);
#define LOGIC_H
#endif // !LOGIC_H
