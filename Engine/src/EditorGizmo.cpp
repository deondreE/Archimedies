#include "archpch.h"
#include "EditorGizmo.h"
#include <algorithm>
#include <cmath>

namespace {
	float ScreenDot(const ImVec2& a, const ImVec2& b) { return a.x * b.x + a.y * b.y; }
	float ScreenLength(const ImVec2& a) { return sqrtf(a.x * a.x + a.y * a.y); }

	ImVec2 ScreenSub(const ImVec2& a, const ImVec2& b) { return ImVec2(a.x - b.x, a.y - b.y); }
	ImVec2 ScreenAdd(const ImVec2& a, const ImVec2& b) { return ImVec2(a.x + b.x, a.y + b.y); }
	ImVec2 ScreenMul(const ImVec2& a, float s) { return ImVec2(a.x * s, a.y * s); }

	// Perpendicular (rotate 90 degrees) - used for arrowhead wings and box corners.
	ImVec2 ScreenPerp(const ImVec2& a) { return ImVec2(-a.y, a.x); }
	ImVec2 ScreenNormalize(const ImVec2& a) {
		float l = ScreenLength(a);
		return l > 0.001f ? ImVec2(a.x / l, a.y / l) : ImVec2(0.0f, 0.0f);
	}

	ImU32 AxisBaseColor(int axis) {
		static const ImU32 AxisColor[3] = {
			IM_COL32(226, 61, 61, 255),
			IM_COL32(70, 200, 90, 255),
			IM_COL32(65, 130, 240, 255)
		};
		return AxisColor[axis];
	}

	ImU32 AxisLineColor(int axis, bool hovered, bool active) {
		if (active)  return IM_COL32(255, 235, 120, 255); // consistent "selected" gold across all axes
		if (hovered) {
			ImU32 base = AxisBaseColor(axis);
			// Lighten the base color rather than flashing pure white.
			int r = std::min(255, (int)((base >> IM_COL32_R_SHIFT & 0xFF) + 70));
			int g = std::min(255, (int)((base >> IM_COL32_G_SHIFT & 0xFF) + 70));
			int b = std::min(255, (int)((base >> IM_COL32_B_SHIFT & 0xFF) + 70));
			return IM_COL32(r, g, b, 255);
		}
		return AxisBaseColor(axis);
	}

	constexpr ImU32 kOutlineColor = IM_COL32(20, 20, 20, 160);
	constexpr ImU32 kCenterIdle = IM_COL32(230, 230, 230, 255);
	constexpr ImU32 kCenterHover = IM_COL32(255, 235, 120, 255);

	void DrawOutlinedLine(ImDrawList* dl, const ImVec2& a, const ImVec2& b, ImU32 col, float thickness) {
		dl->AddLine(a, b, kOutlineColor, thickness + 2.5f);
		dl->AddLine(a, b, col, thickness);
	}

	void DrawOutlinedCircleFilled(ImDrawList* dl, const ImVec2& center, float radius, ImU32 col) {
		dl->AddCircleFilled(center, radius + 1.5f, kOutlineColor, 20);
		dl->AddCircleFilled(center, radius, col, 20);
	}

	void DrawArrowhead(ImDrawList* dl, const ImVec2& tip, const ImVec2& dir, ImU32 col, float length, float width) {
		ImVec2 back = ScreenSub(tip, ScreenMul(dir, length));
		ImVec2 perp = ScreenMul(ScreenPerp(dir), width * 0.5f);
		ImVec2 left = ScreenAdd(back, perp);
		ImVec2 right = ScreenSub(back, perp);

		// Dark outline pass (slightly larger) then filled color on top.
		dl->AddTriangleFilled(tip, left, right, kOutlineColor);
		ImVec2 tipIn = ScreenSub(tip, ScreenMul(dir, 1.0f));
		dl->AddTriangleFilled(tipIn, ScreenAdd(left, ScreenMul(dir, 0.5f)), ScreenSub(right, ScreenMul(dir, -0.5f)), col);
	}

	void DrawBoxHandle(ImDrawList* dl, const ImVec2& center, float halfSize, ImU32 col) {
		ImVec2 mn(center.x - halfSize, center.y - halfSize);
		ImVec2 mx(center.x + halfSize, center.y + halfSize);
		dl->AddRectFilled(ImVec2(mn.x - 1.5f, mn.y - 1.5f), ImVec2(mx.x + 1.5f, mx.y + 1.5f), kOutlineColor, 2.0f);
		dl->AddRectFilled(mn, mx, col, 2.0f);
	}

	float DistanceSegment(const ImVec2& p, const ImVec2& a, const ImVec2& b) {
		ImVec2 seg = ScreenSub(b, a);
		float segLenSq = seg.x * seg.x + seg.y * seg.y; // fixed: was seg.y * seg.x
		float t = segLenSq > 0.0f ? ((p.x - a.x) * seg.x + (p.y - a.y) * seg.y) / segLenSq : 0.0f;
		t = std::clamp(t, 0.0f, 1.0f);
		ImVec2 closest(a.x + seg.x * t, a.y + seg.y * t);
		return ScreenLength(ScreenSub(p, closest));
	}
}

namespace Engine {

	bool EditorGizmo::Manipulate(const WorldScreenFn& worldToScreen, Math::Vec3& position,
		Math::Vec3& rotation, Math::Vec3& scale, GizmoMode mode, float gizmoScreenSize)
	{
		_Mode = mode;
		switch (mode) {
		case GizmoMode::Translate: return ManipulateTranslate(worldToScreen, position, gizmoScreenSize);
		case GizmoMode::Scale: return ManipulateScale(worldToScreen, position, scale, gizmoScreenSize); // fixed
		case GizmoMode::Rotate: return ManipulateRotate(worldToScreen, position, rotation, gizmoScreenSize);
		}
		return false;
	}

	bool EditorGizmo::ManipulateTranslate(const WorldScreenFn& w2s, Math::Vec3& position, float size) {
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImGuiIO& io = ImGui::GetIO();
		bool changed = false;

		ImVec2 origin;
		if (!w2s(position, origin)) return false;
		_OriginScreen = origin;

		const float probeDist = 1.0f;
		for (int axis = 0; axis < 3; axis++) {
			Math::Vec3 probe = position;
			if (axis == 0) probe.x += probeDist;
			else if (axis == 1) probe.y += probeDist;
			else probe.z += probeDist;

			ImVec2 probeScreen;
			if (!w2s(probe, probeScreen)) { _AxisScreenLen[axis] = 0.0f; continue; }

			ImVec2 delta = ScreenSub(probeScreen, origin);
			_AxisScreenDir[axis] = ScreenNormalize(delta);
			_AxisScreenLen[axis] = ScreenLength(delta);
		}

		ImVec2 mouse = io.MousePos;
		int hoveredAxis = -1;
		bool centerHovered = false;
		float bestDist = 8.0f;

		const float centerRadius = 6.0f;
		if (_ActiveAxis == -1 && ScreenLength(ScreenSub(mouse, origin)) < centerRadius + 2.0f) {
			centerHovered = true;
		}

		for (int axis = 0; axis < 3; axis++) {
			if (_AxisScreenLen[axis] <= 0.0f) continue;
			ImVec2 end(origin.x + _AxisScreenDir[axis].x * size, origin.y + _AxisScreenDir[axis].y * size);

			if (_ActiveAxis == -1 && !centerHovered) {
				float d = DistanceSegment(mouse, origin, end);
				if (d < bestDist) { bestDist = d; hoveredAxis = axis; }
			}
		}

		// Draw shafts first (outline pass baked into DrawOutlinedLine), then arrowheads on top.
		for (int axis = 0; axis < 3; axis++) {
			if (_AxisScreenLen[axis] <= 0.0f) continue;
			ImVec2 end(origin.x + _AxisScreenDir[axis].x * size, origin.y + _AxisScreenDir[axis].y * size);
			bool isActive = (_ActiveAxis == axis);
			bool isHovered = (hoveredAxis == axis);
			ImU32 col = AxisLineColor(axis, isHovered, isActive);

			DrawOutlinedLine(drawList, origin, end, col, isActive ? 3.5f : 2.5f);
			DrawArrowhead(drawList, end, _AxisScreenDir[axis], col, 14.0f, 9.0f);
		}
		DrawOutlinedCircleFilled(drawList, origin, centerRadius, centerHovered ? kCenterHover : kCenterIdle);

		if (_ActiveAxis == -1 && hoveredAxis != -1 && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemActive()) {
			_ActiveAxis = hoveredAxis;
			_DragStartVec = position;
		}

		if (_ActiveAxis != -1) {
			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
				float pxPerUnit = _AxisScreenLen[_ActiveAxis];
				if (pxPerUnit > 0.01f) {
					float moveScreen = ScreenDot(io.MouseDelta, _AxisScreenDir[_ActiveAxis]);
					float moveWorld = moveScreen / pxPerUnit;
					if (_ActiveAxis == 0) position.x += moveWorld;
					else if (_ActiveAxis == 1) position.y += moveWorld;
					else position.z += moveWorld;
					changed = moveScreen != 0.0f;
				}
			}
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) _ActiveAxis = -1;
		}

		return changed;
	}

	bool EditorGizmo::ManipulateScale(const WorldScreenFn& w2s, Math::Vec3& position, Math::Vec3& scale, float size) {
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImGuiIO& io = ImGui::GetIO();
		bool changed = false;

		ImVec2 origin;
		if (!w2s(position, origin)) return false;
		_OriginScreen = origin;

		const float probeDist = 1.0f;
		for (int axis = 0; axis < 3; axis++) {
			Math::Vec3 probe = position;
			if (axis == 0) probe.x += probeDist;
			else if (axis == 1) probe.y += probeDist;
			else probe.z += probeDist;

			ImVec2 probeScreen;
			if (!w2s(probe, probeScreen)) { _AxisScreenLen[axis] = 0.0f; continue; }

			ImVec2 delta = ScreenSub(probeScreen, origin);
			_AxisScreenDir[axis] = ScreenNormalize(delta);
			_AxisScreenLen[axis] = ScreenLength(delta);
		}

		ImVec2 mouse = io.MousePos;
		int hoveredAxis = -1;
		float bestDist = 8.0f;

		for (int axis = 0; axis < 3; axis++) {
			if (_AxisScreenLen[axis] <= 0.0f) continue;
			ImVec2 end(origin.x + _AxisScreenDir[axis].x * size, origin.y + _AxisScreenDir[axis].y * size);

			if (_ActiveAxis == -1) {
				float d = DistanceSegment(mouse, origin, end);
				if (d < bestDist) { bestDist = d; hoveredAxis = axis; }
			}
		}

		for (int axis = 0; axis < 3; axis++) {
			if (_AxisScreenLen[axis] <= 0.0f) continue;
			ImVec2 end(origin.x + _AxisScreenDir[axis].x * size, origin.y + _AxisScreenDir[axis].y * size);
			bool isActive = (_ActiveAxis == axis);
			bool isHovered = (hoveredAxis == axis);
			ImU32 col = AxisLineColor(axis, isHovered, isActive);

			DrawOutlinedLine(drawList, origin, end, col, isActive ? 3.5f : 2.5f);
			DrawBoxHandle(drawList, end, isActive ? 6.5f : 5.5f, col);
		}

		// Small center box for uniform (all-axis) scale.
		bool centerHovered = (_ActiveAxis == -1 && hoveredAxis == -1 &&
			ScreenLength(ScreenSub(mouse, origin)) < 8.0f);
		DrawBoxHandle(drawList, origin, 5.0f, centerHovered ? kCenterHover : kCenterIdle);

		if (_ActiveAxis == -1 && hoveredAxis != -1 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
			&& !ImGui::IsAnyItemActive()) {
			_ActiveAxis = hoveredAxis;
			_DragStartVec = scale;
		}

		if (_ActiveAxis != -1) {
			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
				float pxPerUnit = _AxisScreenLen[_ActiveAxis];
				if (pxPerUnit > 0.01f) {
					float moveScreen = ScreenDot(io.MouseDelta, _AxisScreenDir[_ActiveAxis]);
					float moveWorld = moveScreen / pxPerUnit;
					float* target = _ActiveAxis == 0 ? &scale.x : (_ActiveAxis == 1 ? &scale.y : &scale.z);
					*target = std::max(0.001f, *target + moveWorld);
					changed = moveScreen != 0.0f;
				}
			}
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) _ActiveAxis = -1;
		}
		return changed;
	}

	bool EditorGizmo::ManipulateRotate(const WorldScreenFn& w2s, const Math::Vec3& position, Math::Vec3& rotation, float size)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImGuiIO& io = ImGui::GetIO();
		bool changed = false;

		ImVec2 origin;
		if (!w2s(position, origin)) return false;
		_OriginScreen = origin;

		const int segments = 48;
		const float worldRadius = 1.0f;
		static const int axisPlane[3][2] = { {1, 2}, {0, 2}, {0, 1} };

		static ImVec2 pts[3][segments + 1];
		bool ptsValid[3] = { true, true, true };

		for (int axis = 0; axis < 3; axis++) {
			for (int i = 0; i <= segments; i++) {
				float t = (float)i / segments * 2.0f * (float)Math::PI;
				Math::Vec3 p = position;
				float a = worldRadius * cosf(t);
				float b = worldRadius * sinf(t);
				float* comps = &p.x;
				comps[axisPlane[axis][0]] += a;
				comps[axisPlane[axis][1]] += b;

				if (!w2s(p, pts[axis][i])) { ptsValid[axis] = false; break; }
			}
		}

		ImVec2 mouse = io.MousePos;
		int hoveredAxis = -1;
		float bestDist = 8.0f;

		if (_ActiveAxis == -1) {
			for (int axis = 0; axis < 3; axis++) {
				if (!ptsValid[axis]) continue;
				for (int i = 0; i < segments; i++) {
					float d = DistanceSegment(mouse, pts[axis][i], pts[axis][i + 1]);
					if (d < bestDist) { bestDist = d; hoveredAxis = axis; }
				}
			}
		}

		// Outline pass for every ring first, so colored strokes never get crossed by another ring's outline.
		for (int axis = 0; axis < 3; axis++) {
			if (!ptsValid[axis]) continue;
			for (int i = 0; i < segments; i++)
				drawList->AddLine(pts[axis][i], pts[axis][i + 1], kOutlineColor, 4.5f);
		}

		for (int axis = 0; axis < 3; axis++) {
			if (!ptsValid[axis]) continue;
			bool isActive = (_ActiveAxis == axis);
			bool isHovered = (hoveredAxis == axis);
			ImU32 col = AxisLineColor(axis, isHovered, isActive);
			float thickness = isActive ? 3.0f : (isHovered ? 2.6f : 2.0f);
			for (int i = 0; i < segments; i++)
				drawList->AddLine(pts[axis][i], pts[axis][i + 1], col, thickness);
		}

		// While actively dragging, fill a translucent wedge showing the angle swept this drag.
		if (_ActiveAxis != -1 && ptsValid[_ActiveAxis]) {
			float angleNow = atan2f(mouse.y - origin.y, mouse.x - origin.x);
			ImU32 fillCol = (AxisLineColor(_ActiveAxis, false, true) & 0x00FFFFFF) | IM_COL32(0, 0, 0, 60);
			drawList->PathLineTo(origin);
			drawList->PathArcTo(origin, size, _RotateArcStart, angleNow, 32);
			drawList->PathFillConvex(fillCol);
		}

		if (_ActiveAxis == -1 && hoveredAxis != -1 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
			&& !ImGui::IsAnyItemActive()) {
			_ActiveAxis = hoveredAxis;
			_DragStartAngle = atan2f(mouse.y - origin.y, mouse.x - origin.x);
			_RotateArcStart = _DragStartAngle;
		}

		if (_ActiveAxis != -1) {
			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
				float angleNow = atan2f(mouse.y - origin.y, mouse.x - origin.x);
				float delta = angleNow - _DragStartAngle;
				_DragStartAngle = angleNow;

				float* target = _ActiveAxis == 0 ? &rotation.x : (_ActiveAxis == 1 ? &rotation.y : &rotation.z);
				*target += delta;
				changed = delta != 0.0f;
			}
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) _ActiveAxis = -1;
		}

		return changed;
	}


}