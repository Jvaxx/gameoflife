#include "main.h"
#include "maths.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <ostream>
#include <ranges>
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
    int y_max; // Y où l'arête s'arrête
    int y_start;
    float x;     // X courant, on itère dessus.
    float dx_dy; // dx/dy

    bool operator<(const Edge &other) const {
        return y_start < other.y_start;
    }
};

void draw_polygon(Pixel_buffer *buffer, const std::span<Vector2> &pts, uint32_t color) {
    // WARNING: C'est le goulot d'étranglement pour le rendu. Pour des polygones convexes, voir
    // rasterizer spécilisé pour. (voire même spécialisé pour quadrilatères convexes)
    if (pts.size() < 3)
        return;

    std::vector<Edge> all_edges;
    all_edges.reserve(pts.size());
    int poly_max_y{};
    int poly_min_y{INT_MAX};

    // énumaration des arrêtes
    for (size_t i{}; i < pts.size(); ++i) {
        Vector2 p1 = pts[i];
        Vector2 p2 = pts[(i + 1) % pts.size()];
        if (static_cast<int>(p1.y) == static_cast<int>(p2.y))
            continue; // sur le même y: pas d'intersection
        if (p1.y > p2.y)
            std::swap(p1, p2);
        int y_sart = static_cast<int>(p1.y);
        int y_end = static_cast<int>(p2.y);
        if (y_sart >= buffer->height || y_end <= 0)
            continue;

        Edge edge;
        edge.dx_dy = static_cast<float>(p2.x - p1.x) / (p2.y - p1.y);
        edge.y_max = std::min(y_end, buffer->height);
        edge.x = static_cast<float>(p1.x);
        edge.y_start = y_sart;

        if (edge.y_start < 0) {
            edge.x += edge.dx_dy * (0 - edge.y_start); // on ramène à y = 0
            edge.y_start = 0;
        }
        all_edges.push_back(edge);
        poly_max_y = std::max(poly_max_y, edge.y_max);
        poly_min_y = std::min(poly_min_y, edge.y_start);
    }

    if (all_edges.empty())
        return;

    // tri inversé des arrêtes par leur y_start
    std::sort(all_edges.begin(), all_edges.end(), [](const Edge &a, const Edge &b) {
        return a.y_start > b.y_start;
    });

    std::vector<Edge> active_edges;
    active_edges.reserve(pts.size());
    // scanline
    int start_y = std::max(0, poly_min_y);
    int max_scan_y = std::min(buffer->height, poly_max_y);
    for (int y{start_y}; y < max_scan_y; ++y) {
        // chargement des active_edges
        while (!all_edges.empty() && (all_edges.back().y_start == y)) {
            active_edges.push_back(all_edges.back());
            all_edges.pop_back();
        }

        // on garde uniquement les arrêtes dont y_max > yactuel
        size_t write_idx = 0;
        for (size_t read_idx = 0; read_idx < active_edges.size(); ++read_idx) {
            if (active_edges[read_idx].y_max > y) {
                active_edges[write_idx++] = active_edges[read_idx];
            }
        }
        active_edges.resize(write_idx);

        if (active_edges.empty())
            continue;

        // tri par insertion sur x
        for (size_t i = 1; i < active_edges.size(); ++i) {
            Edge key = active_edges[i];
            int j = i - 1;
            while (j >= 0 && active_edges[j].x > key.x) {
                active_edges[j + 1] = active_edges[j];
                --j;
            }
            active_edges[j + 1] = key;
        }

        // rasterize
        uint32_t *pixel_ptr = reinterpret_cast<uint32_t *>(buffer->pixels.data()) + y * buffer->width;
        for (size_t i = 0; i + 1 < active_edges.size(); i += 2) {
            int x_start = static_cast<int>(std::ceil(active_edges[i].x));
            int x_end = static_cast<int>(std::ceil(active_edges[i + 1].x));

            if (x_start < 0)
                x_start = 0;
            if (x_end > buffer->width)
                x_end = buffer->width;

            if (x_end > x_start) {
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
