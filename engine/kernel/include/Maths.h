#pragma once

#include <iostream>
#include <istream>
#include <ostream>

namespace arch::core::math {
struct Vector3 {
  float x, y, z;

  Vector3() : x{0.0f}, y{0.0f}, z{0.0} {}
  Vector3(float value) : x{value}, y{value}, z{value} {}
  Vector3(float x, float y, float z) : x{x}, y{y}, z{z} {}

  Vector3 &operator+=(const Vector3 &rhs) {
    this->x += rhs.x;
    this->y += rhs.y;
    this->z += rhs.z;
    return *this;
  }

  // Pointer access for GRAPHICS APIS.
  const float *data() const { return &x; }

  friend std::ostream &operator<<(std::ostream &os, const Vector3 &v) {
    return os << v.x << " " << v.y << " " << v.z;
  }

  friend std::istream &operator>>(std::istream &is, Vector3 &v) {
    return is >> v.x >> v.y >> v.z;
  }
};

struct Vector2 {
  float x, y;

  Vector2() : x{0.0f}, y{0.0f} {}
  Vector2(float value) : x{value}, y{value} {}
  Vector2(float x, float y) : x{x}, y{y} {}

  Vector2 &operator+=(const Vector2 &rhs) {
    this->x += rhs.x;
    this->y += rhs.y;
    return *this;
  }

  friend Vector2 operator+(Vector2 &lhs, const Vector2 &rhs) {
    lhs += rhs;
    return lhs;
  }

  // Pointer access for GRAPHICS APIS.
  const float *data() const { return &x; }

  friend std::ostream &operator<<(std::ostream &os, const Vector2 &v) {
    return os << v.x << " " << v.y;
  }

  friend std::istream &operator>>(std::istream &is, Vector2 &v) {
    return is >> v.x >> v.y;
  }
};

struct Matrix4 {
  float m[4][4];

  Matrix4(float diagonal = 1.0f) {
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        m[i][j] = (i == j) ? diagonal : 0.0f;
      }
    }
  }

  static Matrix4 identity() { return Matrix4(1.0f); }

  float &operator()(int row, int col) { return m[row][col]; }

  const float &operator()(int row, int col) const { return m[row][col]; }
  const float *data() const { return &m[0][0]; }

  Matrix4 &operator*=(const Matrix4 &rhs) {
    Matrix4 result(0.0f);
    for (int row = 0; row < 4; ++row) {
      for (int col = 0; col < 4; ++col) {
        for (int k = 0; k < 4; ++k) {
          result.m[row][col] += this->m[row][k] * rhs.m[k][col];
        }
      }
    }
    *this = result;
    return *this;
  }

  friend std::ostream &operator<<(std::ostream &os, const Matrix4 &mat) {
    for (int i = 0; i < 4; ++i)
      for (int j = 0; j < 4; ++j)
        os << mat.m[i][j] << (i == 3 && j == 3 ? "" : " ");
    return os;
  }

  friend std::istream &operator>>(std::istream &is, Matrix4 &mat) {
    for (int i = 0; i < 4; ++i)
      for (int j = 0; j < 4; ++j)
        is >> mat.m[i][j];
    return is;
  }
};

inline Vector3 operator+(Vector3 lhs, const Vector3 &rhs) {
  lhs += rhs;
  return lhs;
}

inline Matrix4 operator*(Matrix4 lhs, const Matrix4 &rhs) {
  lhs *= rhs;
  return lhs;
}
} // namespace arch::core::math