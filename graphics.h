#include <span>
#ifndef GRAPHICS_H
#include "main.h"
#include "maths.h"
#include <cstdint>

namespace Graphics {
void draw_line_bresenham(Pixel_buffer *buffer, Vector2_int p1, Vector2_int p2, uint32_t color);
void draw_polygon(Pixel_buffer *buffer, const std::span<Vector2> &pts, uint32_t color);
void draw_convex_quad(Pixel_buffer *buffer, const std::span<Vector2> &pts, uint32_t color);
void draw_line_horizontal(Pixel_buffer *buffer, int y, int x1, int x2, uint32_t color);
void draw_line_vertical(Pixel_buffer *buffer, int x, int y1, int y2, uint32_t color);
} // namespace Graphics

#define GRAPHICS_H
#endif // !GRAPHICS_H
