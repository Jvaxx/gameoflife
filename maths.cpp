#include "maths.h"
#include "main.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <vector>
namespace Maths {
int round_float_to_int(double f) {
    return static_cast<int>(f + 0.5f);
}

uint8_t get_sutherland_code(Vector2_int p, int w, int h) {
    uint8_t code = 0;
    if (p.y < 0)
        code |= (1 << 3);
    if (p.y >= h)
        code |= (1 << 2);
    if (p.x >= w)
        code |= (1 << 1);
    if (p.x < 0)
        code |= (1 << 0);
    return code;
}

bool cohen_sutherland_frame(Vector2_int *p1, Vector2_int *p2, int w, int h) {
    // printf("  enter cohen_sutherland_frame\n");
    while (true) {
        // printf("    enter cohen while\n");
        // printf("      p1.x: %d  p1.y: %d\n", p1->x, p1->y);
        // printf("      p2.x: %d  p2.y: %d\n", p2->x, p2->y);
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

        Vector2_int *outside_point = (code1) ? p1 : p2;
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
        // printf("      out_pt.x: %d  out_pt.y: %d\n", outside_point->x, outside_point->y);
    }
}

Vector2_int pt_float_to_int(Vector2 &vect) {
    return {static_cast<int>(vect.x), static_cast<int>(vect.y)};
}

std::vector<Vector2_int> pt_float_to_int(std::vector<Vector2> &vect) {
    std::vector<Vector2_int> res(vect.size());
    for (size_t i{}; i < vect.size(); ++i) {
        res[i] = pt_float_to_int(vect[i]);
    }
    return res;
}

template <typename T>
std::ostream &operator<<(std::ostream &out, const std::vector<T> &arr) {
    for (const auto &ele : arr) {
        out << ele << " ";
    }
    out << '\n';
    return out;
}

template <typename T>
void merge(std::vector<T> &arr, int left, int right) {
    if (left >= right) {
        return;
    }
    int mid = left + (right - left) / 2;
    merge(arr, left, mid);
    merge(arr, mid + 1, right);

    std::vector<T> L(mid - left + 1);
    std::vector<T> R(right - mid);
    for (int i{}; i < mid - left + 1; ++i) {
        L[i] = arr[left + i];
    }
    for (int j{}; j < right - mid; ++j) {
        R[j] = arr[mid + j + 1];
    }
    int i{}, j{}, k{};
    while (i < mid - left + 1 && j < right - mid) {
        if (L[i] <= R[j]) {
            arr[left + k++] = L[i++];
        } else {
            arr[left + k++] = R[j++];
        }
    }
    while (i < mid - left + 1) {
        arr[left + k++] = L[i++];
    }
    while (j < right - mid) {
        arr[left + k++] = R[j++];
    }
}

void merge_sort(std::vector<int> &arr) {
    int n = arr.size();
    merge(arr, 0, n - 1);
}

std::vector<float> operator*(const std::vector<float> &v, const float m) {
    std::vector<float> res(v.size());
    for (int i{}; i < v.size(); ++i) {
        res[i] = v[i] * m;
    }
    return res;
}

std::ostream &operator<<(std::ostream &out, const Vector2 &arr) {
    out << "x: " << arr.x << "  " << arr.y;
    return out;
}

View_Matrix mat_w_to_scr(const Vector2 &cam_pos, float theta, float ppm, int screen_w, int screen_h) {
    // Matrice de transfo homogène (juste les deux premières lignes) Monde->Ecran
    // Combine les tranfo: Tr(screen_center) * Scale(ppm et inversion axe y) * Rot(-theta) * Tr(-cam_pos)
    float cos_t = std::cos(theta);
    float sin_t = std::sin(theta);
    float offs_x = screen_w * 0.5f;
    float offs_y = screen_h * 0.5f;

    return {
        cos_t * ppm, sin_t * ppm, offs_x - ppm * (cam_pos.x * cos_t + cam_pos.y * sin_t),
        sin_t * ppm, cos_t * -ppm, offs_y - ppm * (cam_pos.x * sin_t - cam_pos.y * cos_t)};
}

View_Matrix mat_scr_to_w(const Vector2 &cam_pos, float theta, float ppm, int screen_w, int screen_h) {
    // Matrice de transfo homogène (juste les deux premières lignes) Ecran->World
    // Combine les tranfo: Tr(cam_pos) * Rot(theta) * Scale(1/ppm et inversion axe y) * Tr(-Screen_center)
    float cos_t = std::cos(theta);
    float sin_t = std::sin(theta);
    float offs_x = screen_w * 0.5f;
    float offs_y = screen_h * 0.5f;
    float mpp{1 / ppm};

    return {
        cos_t * mpp, sin_t * mpp, cam_pos.x - mpp * (offs_x * cos_t + offs_y * sin_t),
        sin_t * mpp, cos_t * -mpp, cam_pos.y - mpp * (offs_x * sin_t - offs_y * cos_t)};
}

Vector2 transformed(const Vector2 &real, const View_Matrix &m3) {
    // Effecture la transformation m3 * real
    return {
        real.x * m3.m00 + real.y * m3.m01 + m3.m02,
        real.x * m3.m10 + real.y * m3.m11 + m3.m12,
    };
}
std::vector<Vector2> transformed(const std::vector<Vector2> &reals, const View_Matrix &m3) {
    std::vector<Vector2> res(reals.size());
    for (int i{}; i < reals.size(); ++i) {
        res[i] = {transformed(reals[i], m3)};
    }
    return res;
}
} // namespace Maths
