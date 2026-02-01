#include <numbers>
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

struct View_Matrix {
    float m00, m01, m02;
    float m10, m11, m12;
};

namespace Maths {

Vector2_int pt_float_to_int(Vector2 &vect);
std::vector<Vector2_int> pt_float_to_int(std::vector<Vector2> &vect);

bool cohen_sutherland_frame(Vector2_int *p1, Vector2_int *p2, int w, int h);

void merge_sort(std::vector<int> &arr);

View_Matrix create_view_mat(const Vector2 &cam_pos, float theta, float ppm, int screen_w, int screen_h);

Vector2 world_to_scr(const Vector2 &real, const View_Matrix &m3);
std::vector<Vector2> world_to_scr(const std::vector<Vector2> &reals, const View_Matrix &m3);

} // namespace Maths

template <typename T>
std::ostream &operator<<(std::ostream &out, const std::vector<T> &arr) {
    for (const auto &ele : arr) {
        out << ele << " ";
    }
    return out;
}

#define MATHS_H
#endif // !MATHS_H
