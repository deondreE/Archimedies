#pragma once
#include <cmath>
#include <immintrin.h>
#include <smmintrin.h>
#include <iostream>

namespace Engine{
	namespace Math {
        constexpr float PI = 3.14159265358979f;
        constexpr float TWO_PI = 6.28318530717958f;
        constexpr float HALF_PI = 1.57079632679489f;
        constexpr float DEG2RAD = PI / 180.0f;
        constexpr float RAD2DEG = 180.0f / PI;
        constexpr float EPSILON = 1e-6;

        inline float Radians(float deg) { return deg * DEG2RAD; }
        inline float Degrees(float rad) { return rad * RAD2DEG; }
        inline float Clamp(float v, float lo, float hi) {
            return v < lo ? lo : (v < hi ? hi : v);
        }
        inline bool NearlyEqual(float a, float b, float eps = EPSILON) {
            return std::fabs(a - b) < eps;
        }

		/// <summary>
		///  Vec 
		/// </summary>
        struct Vec2 {
            union {
                struct { float x, y; };
                float data[2];
            };

            constexpr Vec2(float x = 0.f, float y = 0.f) : x(x), y(y) {}

            Vec2 operator+(const Vec2& other) const { return { x + other.x, y + other.y}; }
            Vec2 operator-(const Vec2& other) const { return { x - other.x, y - other.y }; }
            Vec2 operator*(const Vec2& other) const { return { x * other.x, y * other.y}; }
            Vec2 operator*(float scalar) const { return { x * scalar, y * scalar }; }

            float Length() const { return std::sqrt(Dot(*this, *this)); }
            Vec2 Normalize() const {
                float len2 = x * x + y * y;
                if (len2 > EPSILON) return *this * (1.0f / std::sqrt(len2));
                return { 0, 0 };
            }

            static float Dot(const Vec2& a, const Vec2& b) {
                return a.x * b.x + a.y * b.y;
            }

            static float Cross(const Vec2& a, const Vec2& b) {
                return a.x * b.y - a.y * b.x;
            }
        };

        // =====================================================================
        // Vec3  (16-byte aligned for SIMD/cbuffer friendliness)
        // =====================================================================
        struct alignas(16) Vec3 {
            union {
                struct { float x, y, z; };
                float data[4];      // 4th is padding (w), kept at 0
                __m128 simd;
            };

            Vec3() : simd(_mm_setzero_ps()) {}
            Vec3(float x, float y, float z) : simd(_mm_setr_ps(x, y, z, 0.0f)) {}
            explicit Vec3(__m128 v) : simd(v) {}

            Vec3 operator+(const Vec3& o) const { return Vec3(_mm_add_ps(simd, o.simd)); }
            Vec3 operator-(const Vec3& o) const { return Vec3(_mm_sub_ps(simd, o.simd)); }
            Vec3 operator*(const Vec3& o) const { return Vec3(_mm_mul_ps(simd, o.simd)); }
            Vec3 operator*(float s) const {
                return Vec3(_mm_mul_ps(simd, _mm_set1_ps(s)));
            }

            static float Dot(const Vec3& a, const Vec3& b) {
                // _mm_dp_ps needs SSE4.1. Mask 0x71 => use xyz (0111), write to lane 0 (0001)
                return _mm_cvtss_f32(_mm_dp_ps(a.simd, b.simd, 0x71));
            }

            float Length() const { return std::sqrt(Dot(*this, *this)); }

            Vec3 Normalize() const {
                float len2 = Dot(*this, *this);
                if (len2 > EPSILON) return *this * (1.0f / std::sqrt(len2));
                return Vec3();
            }

            static Vec3 Cross(const Vec3& a, const Vec3& b) {
                // Standard SIMD cross via two shuffles.
                __m128 a_yzx = _mm_shuffle_ps(a.simd, a.simd, _MM_SHUFFLE(3, 0, 2, 1));
                __m128 b_yzx = _mm_shuffle_ps(b.simd, b.simd, _MM_SHUFFLE(3, 0, 2, 1));
                __m128 c = _mm_sub_ps(_mm_mul_ps(a.simd, b_yzx),
                    _mm_mul_ps(a_yzx, b.simd));
                // result currently in (zxy) order -> shuffle back to (xyz)
                return Vec3(_mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1)));
            }

            static Vec3 Lerp(const Vec3& a, const Vec3& b, float t) {
                return a + (b - a) * t;
            }
        };

        // =====================================================================
        // Vec4  (matches HLSL float4 / cbuffer rows exactly)
        // =====================================================================
        struct alignas(16) Vec4 {
            union {
                struct { float x, y, z, w; };
                float data[4];
                __m128 simd;
            };

            constexpr Vec4(float x = 0.f, float y = 0.f, float z = 0.f, float w = 0.f)
                : x(x), y(y), z(z), w(w) {
            }
            Vec4(const Vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

            Vec4 operator+(const Vec4& o) const {
                Vec4 r; r.simd = _mm_add_ps(simd, o.simd); return r;
            }
            Vec4 operator*(float s) const {
                Vec4 r; r.simd = _mm_mul_ps(simd, _mm_set1_ps(s)); return r;
            }

            static float Dot(const Vec4& a, const Vec4& b) {
                return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
            }
        };
        struct Mat4;

        // =====================================================================
        // Quaternion  (unit quaternion for rotations, xyzw storage)
        // Rotation convention matches the row-vector Mat4 (v * M).
        // =====================================================================
        struct alignas(16) Quat {
            union {
                struct { float x, y, z, w; };
                __m128 simd;
            };

            constexpr Quat(float x = 0.f, float y = 0.f, float z = 0.f, float w = 1.f)
                : x(x), y(y), z(z), w(w) {
            }

            Mat4 ToMat4() const;
            static Quat Identity() { return { 0, 0, 0, 1 }; }

            // Hamilton product.  q = a * b  applies b first, then a.
            Quat operator*(const Quat& b) const {
                const Quat& a = *this;
                return {
                    a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                    a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                    a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
                    a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
                };
            }

            float Length() const { return std::sqrt(x * x + y * y + z * z + w * w); }

            Quat Normalize() const {
                float len = Length();
                if (len > EPSILON) {
                    float inv = 1.0f / len;
                    return { x * inv, y * inv, z * inv, w * inv };
                }
                return Identity();
            }

            Quat Conjugate() const { return { -x, -y, -z, w }; }

            // Rotate a vector by this (unit) quaternion.
            Vec3 Rotate(const Vec3& v) const {
                Vec3 u{ x, y, z };
                float s = w;
                // v' = 2(u.v)u + (s^2 - u.u)v + 2s(u x v)
                return u * (2.0f * Vec3::Dot(u, v))
                    + v * (s * s - Vec3::Dot(u, u))
                    + Vec3::Cross(u, v) * (2.0f * s);
            }

            // --- Constructors ---
            static Quat FromAxisAngle(const Vec3& axis, float rad) {
                Vec3 n = axis.Normalize();
                float h = rad * 0.5f;
                float s = std::sin(h);
                return { n.x * s, n.y * s, n.z * s, std::cos(h) };
            }

            // Euler (pitch=x, yaw=y, roll=z) matching Mat4::RotationEuler order (Z*X*Y)
            static Quat FromEuler(const Vec3& e) {
                return FromAxisAngle({ 0, 0, 1 }, e.z) *
                    FromAxisAngle({ 1, 0, 0 }, e.x) *
                    FromAxisAngle({ 0, 1, 0 }, e.y);
            }
          
            static float Dot(const Quat& a, const Quat& b) {
                return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
            }

            // Spherical linear interpolation — smooth, constant-speed rotation.
            static Quat Slerp(const Quat& a, Quat b, float t) {
                float d = Dot(a, b);
                // Take the shorter arc.
                if (d < 0.0f) { b = { -b.x, -b.y, -b.z, -b.w }; d = -d; }

                if (d > 0.9995f) {
                    // Nearly parallel -> linear interp + normalize (avoids div by ~0).
                    Quat r{ a.x + (b.x - a.x) * t,
                            a.y + (b.y - a.y) * t,
                            a.z + (b.z - a.z) * t,
                            a.w + (b.w - a.w) * t };
                    return r.Normalize();
                }
                float theta0 = std::acos(d);
                float theta = theta0 * t;
                float sin0 = std::sin(theta0);
                float s0 = std::cos(theta) - d * std::sin(theta) / sin0;
                float s1 = std::sin(theta) / sin0;
                return { a.x * s0 + b.x * s1,
                         a.y * s0 + b.y * s1,
                         a.z * s0 + b.z * s1,
                         a.w * s0 + b.w * s1 };
            }
        };

        // =====================================================================
       // Mat4  (row-major, ROW-VECTOR convention: v * M)
       // =====================================================================
        struct alignas(16) Mat4 {
            union {
                float m[4][4];
                __m128 row[4];
            };

            Mat4() {
                row[0] = _mm_setr_ps(1, 0, 0, 0);
                row[1] = _mm_setr_ps(0, 1, 0, 0);
                row[2] = _mm_setr_ps(0, 0, 1, 0);
                row[3] = _mm_setr_ps(0, 0, 0, 1);
            }

            static Mat4 Zero() {
                Mat4 r;
                r.row[0] = r.row[1] = r.row[2] = r.row[3] = _mm_setzero_ps();
                return r;
            }
            static Mat4 Identity() { return Mat4(); }

            // result row i = sum_k this[i][k] * other.row[k]
            Mat4 operator*(const Mat4& other) const {
                Mat4 res;
                for (int i = 0; i < 4; i++) {
                    __m128 r = _mm_mul_ps(_mm_set1_ps(m[i][0]), other.row[0]);
                    r = _mm_add_ps(r, _mm_mul_ps(_mm_set1_ps(m[i][1]), other.row[1]));
                    r = _mm_add_ps(r, _mm_mul_ps(_mm_set1_ps(m[i][2]), other.row[2]));
                    r = _mm_add_ps(r, _mm_mul_ps(_mm_set1_ps(m[i][3]), other.row[3]));
                    res.row[i] = r;
                }
                return res;
            }

            // Transform a row vector: v * M  (w handled explicitly)
            Vec4 Transform(const Vec4& v) const {
                __m128 r = _mm_mul_ps(_mm_set1_ps(v.x), row[0]);
                r = _mm_add_ps(r, _mm_mul_ps(_mm_set1_ps(v.y), row[1]));
                r = _mm_add_ps(r, _mm_mul_ps(_mm_set1_ps(v.z), row[2]));
                r = _mm_add_ps(r, _mm_mul_ps(_mm_set1_ps(v.w), row[3]));
                Vec4 out; out.simd = r; return out;
            }

            static Mat4 Translation(const Vec3& v) {
                Mat4 res;
                res.m[3][0] = v.x;
                res.m[3][1] = v.y;
                res.m[3][2] = v.z;
                return res;
            }

            static Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
                Vec3 zAxis = (target - eye).Normalize();
                Vec3 xAxis = Vec3::Cross(up, zAxis).Normalize();
                Vec3 yAxis = Vec3::Cross(zAxis, xAxis);

                Mat4 res;
                res.m[0][0] = xAxis.x; res.m[0][1] = yAxis.x; res.m[0][2] = zAxis.x;
                res.m[1][0] = xAxis.y; res.m[1][1] = yAxis.y; res.m[1][2] = zAxis.y;
                res.m[2][0] = xAxis.z; res.m[2][1] = yAxis.z; res.m[2][2] = zAxis.z;

                res.m[3][0] = -Vec3::Dot(xAxis, eye);
                res.m[3][1] = -Vec3::Dot(yAxis, eye);
                res.m[3][2] = -Vec3::Dot(zAxis, eye);
                res.m[3][3] = 1;
                return res;
            }

            static Mat4 Scale(const Vec3& scale) {
                Mat4 res;
                res.m[0][0] = scale.x;
                res.m[1][1] = scale.y;
                res.m[2][2] = scale.z;
                return res;
            }

            static Mat4 Scale(float scale) {
                return Scale({ scale, scale, scale });
            }

            static Mat4 RotationX(float radians) {
                float c = std::cos(radians), s = std::sin(radians);
                Mat4 r;
                r.m[1][1] = c; r.m[1][2] = s;
                r.m[2][1] = -s; r.m[2][2] = c;
                return r;
            }

            static Mat4 RotationY(float radians) {
                float c = std::cos(radians), s = std::sin(radians);
                Mat4 r;
                r.m[0][0] = c; r.m[0][2] = -s;
                r.m[2][0] = s; r.m[2][2] = c;
                return r;
            }

            static Mat4 RotationZ(float radians) {
                float c = std::cos(radians), s = std::sin(radians);
                Mat4 r;
                r.m[0][0] = c; r.m[0][1] = s;
                r.m[1][0] = -s; r.m[1][1] = c;
                return r;
            }

            // Combined Euler (pitch=x, yaw=Y, roll=Z), radians
            // Order: Roll * Pitch * Yaw applied to a row vector => v * (Z * X * Y).
            // This is a common "YXZ"-style FPS order. Adjust to taste if needed.
            static Mat4 RotationEuler(const Vec3& eulerRadians) {
                return RotationZ(eulerRadians.z) *
                    RotationX(eulerRadians.x) *
                    RotationY(eulerRadians.y);
            }

            static Mat4 RotationEulerDeg(const Vec3& eulerDeg) {
                return RotationEuler({ eulerDeg.x * DEG2RAD, eulerDeg.y * DEG2RAD, eulerDeg.z * DEG2RAD });
            }

            // DirectX-style LH perspective, [0,1] depth
            static Mat4 Perspective(float fovDeg, float aspect, float nearP, float farP) {
                float t = std::tan(fovDeg * DEG2RAD * 0.5f);
                Mat4 res = Zero();
                res.m[0][0] = 1.0f / (aspect * t);
                res.m[1][1] = 1.0f / t;
                res.m[2][2] = farP / (farP - nearP);
                res.m[2][3] = 1.0f;
                res.m[3][2] = -(farP * nearP) / (farP - nearP);
                return res;
            }

            // 2D / UI: off-center orthographic, DX-style [0,1] depth.
            // Great for sprites, HUD, editor gizmos.
            static Mat4 Orthographic(float left, float right, float bottom,
                float top, float n, float f) {
                Mat4 r = Zero();
                r.m[0][0] = 2.0f / (right - left);
                r.m[1][1] = 2.0f / (top - bottom);
                r.m[2][2] = 1.0f / (f - n);
                r.m[3][0] = -(right + left) / (right - left);
                r.m[3][1] = -(top + bottom) / (top - bottom);
                r.m[3][2] = -n / (f - n);
                r.m[3][3] = 1.0f;
                return r;
            }

            Mat4 Transposed() const {
                Mat4 r;
                for (int i = 0; i < 4; ++i)
                    for (int j = 0; j < 4; ++j)
                        r.m[i][j] = m[j][i];
                return r;
            }

            static Mat4 Transform2D(const Vec2& pos, float rotRad, const Vec2& scale) {
                return Scale({ scale.x, scale.y, 1.0f }) *
                    RotationZ(rotRad) *
                    Translation({ pos.x, pos.y, 0.0f });
            }
        };

        inline Mat4 Quat::ToMat4() const {
            float xx = x * x, yy = y * y, zz = z * z;
            float xy = x * y, xz = x * z, yz = y * z;
            float wx = w * x, wy = w * y, wz = w * z;

            Mat4 r;
            r.m[0][0] = 1 - 2 * (yy + zz);
            r.m[0][1] = 2 * (xy + wz);
            r.m[0][2] = 2 * (xz - wy);

            r.m[1][0] = 2 * (xy - wz);
            r.m[1][1] = 1 - 2 * (xx + zz);
            r.m[1][2] = 2 * (yz + wx);

            r.m[2][0] = 2 * (xz + wy);
            r.m[2][1] = 2 * (yz - wx);
            r.m[2][2] = 1 - 2 * (xx + yy);
            return r;
        }
	}
}