#ifndef MAIN_H
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
};

#define MAIN_H
#endif // !MAIN_H
