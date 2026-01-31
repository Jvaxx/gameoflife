#ifndef MAIN_H
#include "maths.h"
#include <SDL3/SDL_render.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

struct Game_state {
    uint64_t last_tick;
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
    // Une grille de game of life.
    int w{20};
    int h{20};
    float tile_size{2.0}; // en mètres
    Vector2 origin{0, 0};
    std::vector<int> tiles{};

    Grid() {
        tiles.resize(w * h);
    }
    Grid(int width, int height)
        : w{width}, h{height} {
        tiles.resize(width * height);
    }

    int &get(int x, int y) {
        assert((x >= 0 && x < w && y >= 0 && y < h) && "Pb de dimensions");
        return tiles[x + w * y];
    }
    void set(int x, int y, int value) {
        assert((x >= 0 && x < w && y >= 0 && y < h) && "Pb de dimensions");
        tiles[x + w * y] = value;
    }
};

struct View {
    // Le view-port, distances et coordonnées en mètres. Y va vers le bas.
    Pixel_buffer *buffer;
    float pix_per_m{50};  // Le zoom
    float angle{0};       // En radians
    Vector2 origin{0, 0}; // En haut à gauche, en mètres
};

#define MAIN_H
#endif // !MAIN_H
