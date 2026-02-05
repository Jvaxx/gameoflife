#include "main.h"
#include "maths.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <ostream>
#include <span>
#include <utility>
#include <vector>
namespace Graphics {
void draw_line_bresenham(Pixel_buffer *buffer, Vector2_int p1, Vector2_int p2, uint32_t color) {
    using namespace Maths;
    if (cohen_sutherland_frame(&p1, &p2, buffer->width, buffer->height)) {
        int x0{p1.x};
        int y0{p1.y};
        int x1{p2.x};
        int y1{p2.y};
        int dx = (x1 > x0) ? 1 : -1;
        int dy = (y1 > y0) ? 1 : -1;

        bool x_main_axis = (std::abs(x1 - x0) > std::abs(y1 - y0));
        int *main_axis = (x_main_axis) ? &x0 : &y0;
        int *small_axis = (!x_main_axis) ? &x0 : &y0;
        int main_increment = (x_main_axis) ? dx : dy;
        int small_increment = (!x_main_axis) ? dx : dy;
        int main_vector = (x_main_axis) ? std::abs(x1 - x0) : std::abs(y1 - y0);
        int small_vector = (!x_main_axis) ? std::abs(x1 - x0) : std::abs(y1 - y0);
        int error = 0;

        for (int i{}; i <= main_vector; ++i) {
            buffer->set_pixel(x0, y0, color);
            *main_axis += main_increment;
            error += small_vector * 2;
            if (error > main_vector) {
                *small_axis += small_increment;
                error -= 2 * main_vector;
            }
        }
    }
}

struct Edge {
    int y_max;   // Y où l'arête s'arrête
    float x;     // X courant, on itère dessus.
    float dx_dy; // dx/dy

    bool operator<(const Edge &other) const {
        return x < other.x;
    }
};

void draw_polygon(Pixel_buffer *buffer, const std::span<Vector2> &pts, uint32_t color) {
    // WARNING: C'est le goulot d'étranglement pour le rendu. Pour des polygones convexes, voir
    // rasterizer spécilisé pour. (voire même spécialisé pour quadrilatères convexes)
    if (pts.empty())
        return;

    int min_y = pts[0].y, max_y = pts[0].y;
    int min_x = pts[0].x, max_x = pts[0].x;
    for (const auto &p : pts) {
        min_y = std::min(min_y, static_cast<int>(p.y));
        max_y = std::max(max_y, static_cast<int>(p.y));
        min_x = std::min(min_x, static_cast<int>(p.x));
        max_x = std::max(max_x, static_cast<int>(p.x));
    }
    if (max_y < 0 || min_y >= buffer->height || max_x < 0 || min_x >= buffer->width)
        return;

    // Tableau des arêtes
    std::vector<std::vector<Edge>> edges_starting_at(max_y - min_y + 1);
    for (size_t i{}; i < pts.size(); ++i) {
        Vector2 p1 = pts[i];
        Vector2 p2 = pts[(i + 1) % pts.size()];

        if (p1.y == p2.y)
            continue; // horizontal donc inutile
        if (p1.y > p2.y)
            std::swap(p1, p2);

        Edge edge;
        edge.y_max = p2.y;
        edge.x = static_cast<float>(p1.x);
        edge.dx_dy = static_cast<float>(p2.x - p1.x) / (p2.y - p1.y);

        edges_starting_at[p1.y - min_y].push_back(edge);
    }

    std::vector<Edge> active_edges;
    active_edges.reserve(pts.size());
    // scanline
    for (int y = min_y; y < max_y; ++y) {
        auto &new_edges = edges_starting_at[y - min_y];
        for (auto &e : new_edges) {
            auto pos = std::lower_bound(active_edges.begin(), active_edges.end(), e);
            active_edges.insert(pos, e);
        }

        size_t write_idx = 0;
        for (size_t read_idx = 0; read_idx < active_edges.size(); ++read_idx) {
            if (active_edges[read_idx].y_max > y) {
                active_edges[write_idx++] = active_edges[read_idx];
            }
        }
        active_edges.resize(write_idx);

        // Tri par insertion (plus rapide pour données presque triées)
        for (size_t i = 1; i < active_edges.size(); ++i) {
            Edge key = active_edges[i];
            int j = i - 1;
            while (j >= 0 && active_edges[j].x > key.x) {
                active_edges[j + 1] = active_edges[j];
                --j;
            }
            active_edges[j + 1] = key;
        }

        if (y >= 0 && y < buffer->height) {
            uint32_t *pixel_ptr = reinterpret_cast<uint32_t *>(buffer->pixels.data()) + y * buffer->width;
            for (size_t i = 0; i + 1 < active_edges.size(); i += 2) {
                int x_start = std::max(0, static_cast<int>(std::ceil(active_edges[i].x)));
                int x_end = std::min(buffer->width, static_cast<int>(std::ceil(active_edges[i + 1].x)));

                std::fill_n(pixel_ptr + x_start, x_end - x_start, color);
            }
        }

        for (auto &edge : active_edges) {
            edge.x += edge.dx_dy;
        }
    }
}

void draw_line_horizontal(Pixel_buffer *buffer, int y, int x1, int x2, uint32_t color) {
    if (x2 < x1)
        std::swap(x1, x2);
    if (x2 < 0 || x1 >= buffer->width || y < 0 || y >= buffer->height)
        return;
    x1 = (x1 < 0) ? 0 : x1;
    x2 = (x2 >= buffer->width) ? buffer->width - 1 : x2;
    uint32_t *pix_ptr = reinterpret_cast<uint32_t *>(buffer->pixels.data()) + y * buffer->width;
    for (; x1 <= x2; ++x1) {
        pix_ptr[x1] = color;
    }
}

void draw_line_vertical(Pixel_buffer *buffer, int x, int y1, int y2, uint32_t color) {
    if (y2 < y1)
        std::swap(y1, y2);
    if (x < 0 || x >= buffer->width || y2 < 0 || y1 >= buffer->height)
        return;
    y1 = (y1 < 0) ? 0 : y1;
    y2 = (y2 >= buffer->height) ? buffer->height - 1 : y2;
    uint32_t *pix_ptr = reinterpret_cast<uint32_t *>(buffer->pixels.data()) + y1 * buffer->width + x;
    for (; y1 <= y2; ++y1) {
        *pix_ptr = color;
        pix_ptr += buffer->width;
    }
}

} // namespace Graphics
