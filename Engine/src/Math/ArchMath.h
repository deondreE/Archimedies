#pragma once
#include <cmath>
#include <iostream>

namespace Engine{
	namespace Math {
		struct Vec3 {
			union {
				struct { float x, y, z;  };
				float data[3];
			};

			Vec3(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}

			Vec3 operator+(const Vec3& other) const { return { x + other.x, y + other.y, z + other.z }; }
            Vec3 operator-(const Vec3& other) const { return { x - other.x, y - other.y, z - other.z }; }
            Vec3 operator*(const Vec3& other) const { return { x * other.x, y * other.y, z * other.z }; }
			Vec3 operator*(float scalar) const { return { x * scalar, y * scalar, z * scalar }; }

			float Length() const { return std::sqrt(x * x + y * y + z * z); }

            Vec3 Normalize() const {
                float len = Length();
                if (len > 0) return *this * (1.0f / len);
                return { 0, 0, 0 };
            }

            static float Dot(const Vec3& a, const Vec3& b) {
                return a.x * b.x + a.y * b.y + a.z * b.z;
            }

            /*
            (i, j, k)
                i * j = k
                i * k = i
                k * i = j
            */
            static Vec3 Cross(const Vec3& a, const Vec3& b) {
                return {
                    a.y * b.z - a.z * b.y,
                    a.z * b.x - a.x * b.z,
                    a.x * b.y - a.y * b.x
                }; 
            }
		};

        struct alignas(16) Mat4 {
            float m[4][4];

            Mat4() {
                for (int i = 0; i < 4; i++)
                    for (int j = 0; j < 4; j++)
                        m[i][j] = (i == j) ? 1.0f : 0.0f;
            }

            static Mat4 Identity() { return Mat4(); }

            Mat4 operator*(const Mat4& other) const {
                Mat4 res;
                for (int i = 0; i < 4; i++) {
                    for (int j = 0; j < 4; j++) {
                        res.m[i][j] = m[i][0] * other.m[0][j] +
                            m[i][1] * other.m[1][j] +
                            m[i][2] * other.m[2][j] +
                            m[i][3] * other.m[3][j];
                    }
                }
                return res;
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
                return res;
            }
                 
            static Mat4 Perspective(float fovDeg, float aspect, float nearP, float farP) {
                float fovRad = fovDeg * (3.1415926535f / 180.0f);
                float tanHalfFov = std::tan(fovRad / 2.0f);
                Mat4 res = {};
                res.m[0][0] = 1.0f / (aspect * tanHalfFov);
                res.m[1][1] = 1.0f / tanHalfFov;
                res.m[2][2] = farP / (farP - nearP);
                res.m[2][3] = 1.0f;
                res.m[3][2] = -(farP * nearP) / (farP - nearP);
                return res;
            }
        };
	}
}