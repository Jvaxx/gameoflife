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
void draw_line_bresenham(View_buffer *buffer, Vector2_int p1, Vector2_int p2, uint32_t color) {
    using namespace Maths;
    if (cohen_sutherland_frame(&p1, &p2, buffer->rec.w, buffer->rec.h)) {
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
            buffer->pixels.data()[x0 + static_cast<int>(buffer->rec.w) * y0] = color;
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

void draw_polygon(View_buffer *buffer, const std::span<Vector2> &pts, uint32_t color) {
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
        if (y_sart >= buffer->rec.h || y_end <= 0)
            continue;

        Edge edge;
        edge.dx_dy = static_cast<float>(p2.x - p1.x) / (p2.y - p1.y);
        edge.y_max = std::min(y_end, static_cast<int>(buffer->rec.h));
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
    int max_scan_y = std::min(static_cast<int>(buffer->rec.h), poly_max_y);
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
        uint32_t *pixel_ptr = reinterpret_cast<uint32_t *>(buffer->pixels.data()) + y * static_cast<int>(buffer->rec.w);
        for (size_t i = 0; i + 1 < active_edges.size(); i += 2) {
            int x_start = static_cast<int>(std::ceil(active_edges[i].x));
            int x_end = static_cast<int>(std::ceil(active_edges[i + 1].x));

            if (x_start < 0)
                x_start = 0;
            if (x_end > buffer->rec.w)
                x_end = buffer->rec.w;

            if (x_end > x_start) {
                std::fill_n(pixel_ptr + x_start, x_end - x_start, color);
            }
        }

        for (auto &edge : active_edges) {
            edge.x += edge.dx_dy;
        }
    }
}

// Structure légère pour maintenir l'état d'un côté du quad
struct EdgeWalker {
    float x;       // Position X courante
    float dx_dy;   // Pente (inverse slope)
    int y_current; // Y courant
    int y_end;     // Y cible pour ce segment
    int p_index;   // Index du sommet de départ actuel
    int direction; // +1 ou -1 (sens de parcours dans le tableau de points)

    // Vérifie si le walker a fini son segment et doit passer au suivant
    bool is_finished() const { return y_current >= y_end; }
};

void draw_convex_quad(View_buffer *buffer, const std::span<Vector2> &pts, uint32_t color) {
    // NOTE: Fonction générée.

    // Optimisation : On suppose pts.size() == 4
    if (pts.size() != 4)
        return;

    // 1. Trouver le sommet le plus haut (Top) et le plus bas (Bottom)
    int top_idx = 0;
    int bot_idx = 0;
    for (int i = 1; i < 4; ++i) {
        if (pts[i].y < pts[top_idx].y)
            top_idx = i;
        if (pts[i].y > pts[bot_idx].y)
            bot_idx = i;
    }

    int y_min = static_cast<int>(pts[top_idx].y);
    int y_max = static_cast<int>(pts[bot_idx].y);

    // Culling vertical rapide
    if (y_min >= buffer->rec.h || y_max <= 0 || y_min == y_max)
        return;

    // Helper pour initialiser/mettre à jour un walker
    auto setup_walker_segment = [&](EdgeWalker &w) {
        // On récupère les points : P_start -> P_target
        // L'index cible dépend de la direction (+1 ou -1 modulo 4)
        int next_idx = (w.p_index + w.direction + 4) % 4;

        const Vector2 &p1 = pts[w.p_index];
        const Vector2 &p2 = pts[next_idx];

        float dy = p2.y - p1.y;
        w.y_end = static_cast<int>(p2.y);

        // Si segment horizontal ou problème, dx_dy = 0
        if (std::abs(dy) < 0.001f) {
            w.dx_dy = 0;
        } else {
            w.dx_dy = (p2.x - p1.x) / dy;
        }

        // Initialisation de X et Y
        w.x = p1.x;
        w.y_current = static_cast<int>(p1.y);

        // Pre-stepping (Clipping Top)
        // Si le segment commence hors écran (en haut), on avance X
        if (w.y_current < 0) {
            w.x += w.dx_dy * (0 - w.y_current);
            w.y_current = 0;
        }

        // On prépare l'index pour le prochain saut
        w.p_index = next_idx;
    };

    // 2. Initialiser les deux walkers partant du sommet Top
    // Walker 1 part dans le sens horaire (+1), Walker 2 anti-horaire (-1)
    EdgeWalker left_w, right_w;

    left_w.p_index = top_idx;
    left_w.direction = -1; // Vers "l'arrière" du tableau
    setup_walker_segment(left_w);

    right_w.p_index = top_idx;
    right_w.direction = 1; // Vers "l'avant" du tableau
    setup_walker_segment(right_w);

    // 3. Boucle de rendu unique (Single Pass)
    int start_y = std::max(0, y_min);
    int end_y = std::min(static_cast<int>(buffer->rec.h), y_max);

    uint32_t *row_ptr = reinterpret_cast<uint32_t *>(buffer->pixels.data()) + start_y * static_cast<int>(buffer->rec.w);

    for (int y = start_y; y < end_y; ++y) {
        // A. Gestion des changements de segment (le "coude" du quad)
        // Si un walker a atteint la fin de son segment, on passe au suivant
        // Note: on utilise while pour gérer les segments très courts/horizontaux
        while (left_w.is_finished() && left_w.p_index != bot_idx) {
            setup_walker_segment(left_w);
            // Recalage précis si le segment suivant commençait un peu avant/après Y actuel
            // (nécessaire à cause des arrondis float -> int)
            float diff = static_cast<float>(y - left_w.y_current);
            if (diff > 0)
                left_w.x += left_w.dx_dy * diff;
        }
        while (right_w.is_finished() && right_w.p_index != bot_idx) {
            setup_walker_segment(right_w);
            float diff = static_cast<float>(y - right_w.y_current);
            if (diff > 0)
                right_w.x += right_w.dx_dy * diff;
        }

        // B. Rendu de la ligne
        // On ne sait pas qui est gauche/droite à cause du winding order,
        // donc on trie simplement x1 et x2.
        int x1 = static_cast<int>(left_w.x);
        int x2 = static_cast<int>(right_w.x);
        if (x1 > x2)
            std::swap(x1, x2);

        // Clipping Horizontal
        x1 = std::max(0, x1);
        x2 = std::min(static_cast<int>(buffer->rec.w), x2);

        if (x2 > x1) {
            std::fill_n(row_ptr + x1, x2 - x1, color);
        }

        // C. Avancement
        left_w.x += left_w.dx_dy;
        right_w.x += right_w.dx_dy;

        // Mise à jour de y_current pour la logique de changement de segment
        left_w.y_current++;
        right_w.y_current++;

        row_ptr += static_cast<int>(buffer->rec.w);
    }
}

void draw_line_horizontal(View_buffer *buffer, int y, int x1, int x2, uint32_t color) {
    if (x2 < x1)
        std::swap(x1, x2);
    if (x2 < 0 || x1 >= buffer->rec.w || y < 0 || y >= buffer->rec.h)
        return;
    x1 = (x1 < 0) ? 0 : x1;
    x2 = (x2 >= buffer->rec.w) ? buffer->rec.w - 1 : x2;
    uint32_t *pix_ptr = reinterpret_cast<uint32_t *>(buffer->pixels.data()) + y * static_cast<int>(buffer->rec.w);
    for (; x1 <= x2; ++x1) {
        pix_ptr[x1] = color;
    }
}

void draw_line_vertical(View_buffer *buffer, int x, int y1, int y2, uint32_t color) {
    if (y2 < y1)
        std::swap(y1, y2);
    if (x < 0 || x >= buffer->rec.w || y2 < 0 || y1 >= buffer->rec.h)
        return;
    y1 = (y1 < 0) ? 0 : y1;
    y2 = (y2 >= buffer->rec.h) ? buffer->rec.h - 1 : y2;
    uint32_t *pix_ptr = reinterpret_cast<uint32_t *>(buffer->pixels.data()) + y1 * static_cast<int>(buffer->rec.w) + x;
    for (; y1 <= y2; ++y1) {
        *pix_ptr = color;
        pix_ptr += static_cast<int>(buffer->rec.w);
    }
}

} // namespace Graphics
