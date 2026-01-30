#ifndef GRAPHICS_H
#include "main.h"
#include "maths.h"
#include <cstdint>

namespace Graphics {
void draw_line_bresenham(Pixel_buffer *buffer, Vector2 p1, Vector2 p2, uint32_t color);
// inline void draw_line_bresenham(Pixel_buffer *buffer, Vector2_int p1, Vector2_int p2, uint32_t color) {
//     draw_line_bresenham(buffer, p1, p2, color);
// }
// void draw_triangle_horizontal(Pixel_buffer *buffer, vector2 p1, vector2 p2, vector2 p3, uint32_t color);
void draw_triangle(Pixel_buffer *buffer, Vector2 p1, Vector2 p2, Vector2 p3, uint32_t color);
void draw_polygon(Pixel_buffer *buffer, std::vector<Vector2_int> points, uint32_t color);
} // namespace Graphics

#define GRAPHICS_H
#endif // !GRAPHICS_H
