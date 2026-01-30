#ifndef MATHS_H
#include <cstdint>
#include <vector>

struct Vector2 {
    double x{};
    double y{};
};

struct Vector2_int {
    int x{};
    int y{};
};
namespace Maths {
int round_float_to_int(double f);
void point_float_to_int(Vector2 p1, Vector2_int *p2);
bool cohen_sutherland_frame(Vector2 *p1, Vector2 *p2, int w, int h);
template <typename T>
void sort(std::vector<T> &arr);
} // namespace Maths

#define MATHS_H
#endif // !MATHS_H
