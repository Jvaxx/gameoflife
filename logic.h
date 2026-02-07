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
    uint64_t draw_tiles_time{0};
    uint64_t draw_tiles_time_internal{0};
    uint64_t draw_tiles_count_internal{0};
    uint64_t draw_poly_time{0};
    uint64_t draw_poly_since_log{0};
    uint64_t clr_px_time{0};
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

struct Pixel_buffer {
    // L'écran.
    std::unique_ptr<SDL_Texture, Texture_deleter> texture;
    std::vector<uint32_t> pixels{};
    int width{};
    int height{};
    int pitch{};
    int bytes_per_pixel{4};

    Pixel_buffer() = default;

    void resize(SDL_Renderer *renderer, int new_width, int new_height) {
        width = new_width;
        height = new_height;
        pixels.resize(width * height);
        SDL_Texture *new_texture = SDL_CreateTexture(renderer,
                                                     SDL_PIXELFORMAT_ARGB8888,
                                                     SDL_TEXTUREACCESS_STREAMING,
                                                     width, height);
        texture.reset(new_texture);
        pitch = new_width * sizeof(uint32_t);
    }

    void update(SDL_Renderer *renderer) {
        SDL_UpdateTexture(texture.get(), nullptr, pixels.data(), pitch);
        SDL_RenderTexture(renderer, texture.get(), nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    void set_pixel(int x, int y, uint32_t pixel) {
        assert(x >= 0 && x < width && y >= 0 && y < height && "Problèmes de dimensions.");
        pixels[width * y + x] = pixel;
    }

    void clear_pixel(uint32_t color) {
        pixels.assign(width * height, color);
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

struct View {
    // Le view-port, distances et coordonnées en mètres. Y va vers le bas.
    Pixel_buffer *buffer;
    float pix_per_m{50};  // Le zoom
    float theta{0};       // En radians
    Vector2 origin{0, 0}; // Le milieu du view-port, en mètres
};

void fill_grid(View *view, Grid *grid, SDL_Renderer *renderer, Game_state *state);
void fill_grid2(View *view, Grid *grid, SDL_Renderer *renderer, Game_state *state);
void draw_grid(View *view, Grid *grid, SDL_Renderer *renderer);
void update_grid(Grid &grid);
Vector2 px_to_tile(View &view, Grid &grid, Vector2 &in);

// NOTE: temp pour test, mais c'est des fonction internes
bool tile_clic(View &view, Grid &grid, Vector2 &in);
void process_input(Game_state *state, View &view, Grid &grid);
void randomize_grid(Grid &grid, float proba);
#define LOGIC_H
#endif // !LOGIC_H
