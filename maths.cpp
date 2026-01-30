#include "maths.h"
#include <cstdint>
namespace Maths {
int round_float_to_int(double f) {
    return static_cast<int>(f + 0.5f);
}

uint8_t get_sutherland_code(Vector2 p, int w, int h) {
    uint8_t code = 0;
    if (round_float_to_int(p.y) < 0)
        code |= (1 << 3);
    if (round_float_to_int(p.y) >= h)
        code |= (1 << 2);
    if (round_float_to_int(p.x) >= w)
        code |= (1 << 1);
    if (round_float_to_int(p.x) < 0)
        code |= (1 << 0);
    return code;
}

bool cohen_sutherland_frame(Vector2 *p1, Vector2 *p2, int w, int h) {
    // printf("  enter cohen_sutherland_frame\n");
    while (true) {
        // printf("    enter cohen while\n");
        // printf("      p1.x: %f  p1.y: %f\n", p1->x, p1->y);
        // printf("      p2.x: %f  p2.y: %f\n", p2->x, p2->y);
        uint8_t code1 = get_sutherland_code(*p1, w, h);
        uint8_t code2 = get_sutherland_code(*p2, w, h);
        // printf("      code1: %d  code2: %d\n", code1, code2);
        if ((code1 | code2) == 0) {
            // printf("      cohen frame: return true\n");
            return true;
        }
        if (code1 & code2) {
            return false;
        }

        Vector2 *outside_point = (code1) ? p1 : p2;
        uint8_t outside_code = (code1) ? code1 : code2;
        // printf("      out_pt.x: %f  out_pt.y: %f\n", outside_point->x, outside_point->y);
        // printf("      code: %d\n", outside_code);
        if (outside_code & 0b0100) {
            // en bas là!
            outside_point->x = ((p2->x - p1->x) * ((float)(h - 1) - p1->y) / (p2->y - p1->y)) + p1->x;
            outside_point->y = h - 1;
        } else if (outside_code & 0b1000) {
            // en haut
            outside_point->x = ((p2->x - p1->x) * ((float)(1) - p1->y) / (p2->y - p1->y)) + p1->x;
            outside_point->y = 1;
        } else if (outside_code & 0b0010) {
            // a droite
            outside_point->y = ((p2->y - p1->y) * ((float)(w - 1) - p1->x) / (p2->x - p1->x)) + p1->y;
            outside_point->x = w - 1;
        } else if (outside_code & 0b0001) {
            // a gauche
            outside_point->y = ((p2->y - p1->y) * ((float)(1) - p1->x) / (p2->x - p1->x)) + p1->y;
            outside_point->x = 1;
        }
        // printf("      out_pt.x: %f  out_pt.y: %f\n", outside_point->x, outside_point->y);
    }
}

void point_float_to_int(Vector2 p1, Vector2_int *p2) {
    p2->x = round_float_to_int(p1.x);
    p2->y = round_float_to_int(p1.y);
}
} // namespace Maths
