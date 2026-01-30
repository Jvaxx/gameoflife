#include "main.h"
#include "maths.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
namespace Graphics {
void draw_line_bresenham(Pixel_buffer *buffer, Vector2 p1, Vector2 p2, uint32_t color) {
    using namespace Maths;
    if (cohen_sutherland_frame(&p1, &p2, buffer->width, buffer->height)) {
        int x0{round_float_to_int(p1.x)};
        int y0{round_float_to_int(p1.y)};
        int x1{round_float_to_int(p2.x)};
        int y1{round_float_to_int(p2.y)};
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

        while (x0 < x1 && y0 < y1) {
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

void draw_triangle_horizontal(Pixel_buffer *buffer, Vector2 p1, Vector2 p2, Vector2 p3, uint32_t color) {
    using namespace Maths;
    Vector2_int start;
    Vector2_int end;
    Vector2_int other;
    point_float_to_int((p2.x > p1.x) ? p1 : p2, &start);
    point_float_to_int((p2.x > p1.x) ? p2 : p1, &end);
    point_float_to_int(p3, &other);
    int increment_start_x = (start.x > other.x) ? -1 : 1;
    int increment_end_x = (end.x > other.x) ? -1 : 1;
    int increment_y = (start.y > other.y) ? -1 : 1;

    int dx2 = abs(start.x - other.x);
    int dy = abs(start.y - other.y);
    int error2 = 0;

    int dx4 = abs(end.x - other.x);
    int error4 = 0;

    while (start.y != other.y) {
        if (0 <= start.y && start.y < buffer->height) {
            int start_draw_x = start.x;
            if (start_draw_x < 0) {
                start_draw_x = 0;
            } else if (start_draw_x >= buffer->width) {
                start_draw_x = buffer->width - 1;
            }
            int end_draw_x = end.x;
            if (end_draw_x < 0) {
                end_draw_x = 0;
            } else if (end_draw_x >= buffer->width) {
                end_draw_x = buffer->width - 1;
            }
            for (int X = start_draw_x;
                 X < end_draw_x;
                 ++X) {
                buffer->set_pixel(X, start.y, color);
            }
        }
        start.y += increment_y;
        end.y += increment_y;
        error2 += 2 * dx2;
        while (error2 > dy) {
            start.x += increment_start_x;
            error2 -= 2 * dy;
        }
        error4 += 2 * dx4;
        while (error4 > dy) {
            end.x += increment_end_x;
            error4 -= 2 * dy;
        }
    }
    return;
}

void draw_triangle(Pixel_buffer *buffer, Vector2 p1, Vector2 p2, Vector2 p3, uint32_t color) {
    // tri
    Vector2 *point1 = &p1;
    Vector2 *point2 = &p2;
    Vector2 *point3 = &p3;
    if (point1->y > point2->y) {
        Vector2 *inter = point2;
        point2 = point1;
        point1 = inter;
    }
    if (point2->y > point3->y) {
        Vector2 *inter = point3;
        point3 = point2;
        point2 = inter;
    }
    if (point1->y > point2->y) {
        Vector2 *inter = point2;
        point2 = point1;
        point1 = inter;
    }
    float x4 = ((point3->x - point1->x) * (point2->y - point1->y) /
                (point3->y - point1->y)) +
               point1->x;
    Vector2 point4 = {x4, point2->y};

    // on a point4.y = point2->y
    draw_triangle_horizontal(buffer, *point2, point4, *point3, color);
    // printf("x1: %f  x2: %f\n", point2->x, point4.x);
    draw_triangle_horizontal(buffer, *point2, point4, *point1, color);
    return;
}
} // namespace Graphics
