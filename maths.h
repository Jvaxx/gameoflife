#include <ostream>
#ifndef MATHS_H
#include <cstdint>
#include <vector>

struct Vector2 {
    float x{};
    float y{};
};

struct Vector2_int {
    int x{};
    int y{};
};
namespace Maths {
int round_float_to_int(double f);

void point_float_to_int(Vector2 p1, Vector2_int *p2);

bool cohen_sutherland_frame(Vector2_int *p1, Vector2_int *p2, int w, int h);

void merge_sort(std::vector<int> &arr);

} // namespace Maths

template <typename T>
std::ostream &operator<<(std::ostream &out, const std::vector<T> &arr) {
    for (const auto &ele : arr) {
        out << ele << " ";
    }
    out << '\n';
    return out;
}

#define MATHS_H
#endif // !MATHS_H
