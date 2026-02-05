#include <numbers>
#include <ostream>
#ifndef MATHS_H
#include <cstdint>
#include <vector>

struct Vector2 {
    float x{};
    float y{};

    constexpr Vector2() = default;
    constexpr Vector2(float x_in, float y_in) : x{x_in}, y{y_in} {}

    constexpr Vector2 &operator+=(const Vector2 &in) noexcept {
        x += in.x;
        y += in.y;
        return *this;
    }

    constexpr Vector2 &operator-=(const Vector2 &in) noexcept {
        x -= in.x;
        y -= in.y;
        return *this;
    }

    constexpr Vector2 &operator*=(float scalaire) noexcept {
        x *= scalaire;
        y *= scalaire;
        return *this;
    }

    friend constexpr Vector2 operator+(Vector2 lhs, const Vector2 &rhs) noexcept {
        lhs += rhs;
        return lhs;
    }

    friend constexpr Vector2 operator-(Vector2 lhs, const Vector2 &rhs) noexcept {
        lhs -= rhs;
        return lhs;
    }

    friend constexpr Vector2 operator*(Vector2 lhs, float rhs) noexcept {
        lhs *= rhs;
        return lhs;
    }

    friend constexpr Vector2 operator*(float lhs, Vector2 rhs) noexcept {
        rhs *= lhs;
        return rhs;
    }
};

struct Vector2_int {
    int x{};
    int y{};
};

struct View_Matrix {
    float m00, m01, m02;
    float m10, m11, m12;
};
const float pi = std::numbers::pi_v<float>;

namespace Maths {

Vector2_int pt_float_to_int(Vector2 &vect);
std::vector<Vector2_int> pt_float_to_int(std::vector<Vector2> &vect);

bool cohen_sutherland_frame(Vector2_int *p1, Vector2_int *p2, int w, int h);

void merge_sort(std::vector<int> &arr);

View_Matrix mat_w_to_scr(const Vector2 &cam_pos, float theta, float ppm, int screen_w, int screen_h);
View_Matrix mat_scr_to_w(const Vector2 &cam_pos, float theta, float ppm, int screen_w, int screen_h);

Vector2 transformed(const Vector2 &real, const View_Matrix &m3);
std::vector<Vector2> transformed(const std::vector<Vector2> &reals, const View_Matrix &m3);

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
