#pragma once
#include "ArchMath.h"
#include <cassert>

namespace Engine {
	namespace Math {
        inline void RunMathTests() {
            // --- Identity is identity ---
            Mat4 I = Mat4::Identity();
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    assert(NearlyEqual(I.m[i][j], (i == j) ? 1.f : 0.f));

            // --- Zero is zero ---
            Mat4 Z = Mat4::Zero();
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    assert(NearlyEqual(Z.m[i][j], 0.f));

            // --- M * I == M ---
            Mat4 T = Mat4::Translation({ 3, 4, 5 });
            Mat4 TI = T * I;
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    assert(NearlyEqual(TI.m[i][j], T.m[i][j]));

            // --- Translation moves a point (row-vector: v * M) ---
            Vec4 p = T.Transform({ 1, 1, 1, 1 });
            assert(NearlyEqual(p.x, 4) && NearlyEqual(p.y, 5) && NearlyEqual(p.z, 6));

            // --- Scale scales each axis independently ---
            Mat4 S = Mat4::Scale({ 2, 3, 4 });
            Vec4 sp = S.Transform({ 1, 1, 1, 1 });
            assert(NearlyEqual(sp.x, 2) && NearlyEqual(sp.y, 3) && NearlyEqual(sp.z, 4));

            // --- RotationZ(90deg) maps +X -> +Y (row-vector) ---
            Mat4 Rz = Mat4::RotationZ(HALF_PI);
            Vec4 rx = Rz.Transform({ 1, 0, 0, 0 });
            assert(NearlyEqual(rx.x, 0) && NearlyEqual(rx.y, 1));

            // --- RotationY(90deg) maps +Z -> +X (row-vector) ---
            Mat4 Ry = Mat4::RotationY(HALF_PI);
            Vec4 rz = Ry.Transform({ 0, 0, 1, 0 });
            assert(NearlyEqual(rz.x, 1) && NearlyEqual(rz.z, 0));

            // --- Rotation preserves length ---
            Vec4 before{ 1, 2, 3, 0 };
            Vec4 after = Mat4::RotationEuler({ 0.3f, 0.7f, 1.1f }).Transform(before);
            float lb = std::sqrt(Vec4::Dot(before, before));
            float la = std::sqrt(Vec4::Dot(after, after));
            assert(NearlyEqual(lb, la, 1e-4f));

            // --- Perspective: point in front has positive clip w ---
            Mat4 P = Mat4::Perspective(45.f, 16.f / 9.f, 0.1f, 100.f);
            Vec4 clip = P.Transform({ 0, 0, 10, 1 });   // 10 units in front
            assert(clip.w > 0.0f);
            assert(clip.z >= 0.0f && clip.z <= clip.w); // inside [0,w] depth range

            // --- Vec2 cross sign (CCW positive) ---
            assert(Vec2::Cross({ 1, 0 }, { 0, 1 }) > 0.0f);

            // --- Ortho maps center to origin-ish ---
            Mat4 O = Mat4::Orthographic(0, 800, 0, 600, 0, 1);
            Vec4 c = O.Transform({ 400, 300, 0, 1 });
            assert(NearlyEqual(c.x, 0) && NearlyEqual(c.y, 0));

            // --- Quaternion identity rotates nothing ---
            Vec3 v{ 1, 2, 3 };
            Vec3 qv = Quat::Identity().Rotate(v);
            assert(NearlyEqual(qv.x, 1) && NearlyEqual(qv.y, 2) && NearlyEqual(qv.z, 3));

            // --- Quat rotation preserves length ---
            Quat q = Quat::FromAxisAngle({ 0, 1, 0 }, HALF_PI);
            Vec3 r = q.Rotate({ 0, 0, 1 });
            assert(NearlyEqual(v.Length(), v.Length())); // sanity
            assert(NearlyEqual(r.Length(), 1.0f, 1e-4f));

            // --- Quat(Y,90) maps +Z -> +X (must match Mat4::RotationY) ---
            assert(NearlyEqual(r.x, 1.0f, 1e-4f) && NearlyEqual(r.z, 0.0f, 1e-4f));

            // --- Quat->Mat4 agrees with Rotate ---
            Mat4 qm = q.ToMat4();
            Vec4 mr = qm.Transform({ 0, 0, 1, 0 });
            assert(NearlyEqual(mr.x, r.x, 1e-4f) && NearlyEqual(mr.z, r.z, 1e-4f));

            // --- Slerp endpoints ---
            Quat a = Quat::Identity();
            Quat b = Quat::FromAxisAngle({ 0, 1, 0 }, HALF_PI);
            Quat s0 = Quat::Slerp(a, b, 0.0f);
            Quat s1 = Quat::Slerp(a, b, 1.0f);
            assert(NearlyEqual(Quat::Dot(s0, a), 1.0f, 1e-4f));
            assert(NearlyEqual(std::fabs(Quat::Dot(s1, b)), 1.0f, 1e-4f));

            // --- Slerp midpoint is unit length ---
            Quat mid = Quat::Slerp(a, b, 0.5f);
            assert(NearlyEqual(mid.Length(), 1.0f, 1e-4f));

            // --- SIMD Vec3 cross matches known result ---
            Vec3 cx = Vec3::Cross({ 1, 0, 0 }, { 0, 1, 0 });
            assert(NearlyEqual(cx.x, 0) && NearlyEqual(cx.y, 0) && NearlyEqual(cx.z, 1));

            // --- SIMD Vec3 dot ignores padding lane ---
            assert(NearlyEqual(Vec3::Dot({ 1, 2, 3 }, { 4, 5, 6 }), 32.0f));
        }

	}
}