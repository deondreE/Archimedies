#include "archpch.h"
#include "EditorGizmo.h"
#include <algorithm>
#include <cmath>

namespace {
	float ScreenDot(const ImVec2& a, const ImVec2& b) { return a.x * b.x + a.y * b.y; }
	float ScreenLength(const ImVec2& a) { return sqrtf(a.x * a.x + a.y * a.y); }

	ImVec2 ScreenSub(const ImVec2& a, const ImVec2& b) { return ImVec2(a.x - b.x, a.y - b.y); }

	ImVec2 ScreenNormalize(const ImVec2& a) {
		float l = ScreenLength(a);
		return l > 0.001f ? ImVec2(a.x / l, a.y / l) : ImVec2(0.0f, 0.0f);
	}

	ImU32 AxisColor(int axis, bool highlighted) {
		static const ImU32 cols[3] = {
			IM_COL32(230, 50, 50, 255),
			IM_COL32(50, 200, 50, 255),
			IM_COL32(50, 115, 240, 255)
		};
		return highlighted ? IM_COL32(255, 255, 255, 255) : cols[axis];
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
		float bestDist = 8.0f;

		for (int axis = 0; axis < 3; axis++) {
			if (_AxisScreenLen[axis] <= 0.0f) continue;
			ImVec2 end(origin.x + _AxisScreenDir[axis].x * size, origin.y + _AxisScreenDir[axis].y * size);

			if (_ActiveAxis == -1) {
				float d = DistanceSegment(mouse, origin, end);
				if (d < bestDist) { bestDist = d; hoveredAxis = axis; } // fixed: was `hoveredAxis;`
			}

			bool isActive = (_ActiveAxis == axis);
			ImU32 col = AxisColor(axis, isActive || hoveredAxis == axis);
			drawList->AddLine(origin, end, col, isActive ? 4.0f : 3.0f);
			drawList->AddCircleFilled(end, 5.0f, col);
		}

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
			if (!w2s(probe, probeScreen)) { _AxisScreenLen[axis] = 0.0f; continue; } // fixed: was missing

			ImVec2 delta = ScreenSub(probeScreen, origin); // fixed: was self-referencing garbage
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

			bool isActive = (_ActiveAxis == axis);
			ImU32 col = AxisColor(axis, isActive || hoveredAxis == axis);
			drawList->AddLine(origin, end, col, isActive ? 4.0f : 3.0f);
			drawList->AddRectFilled(ImVec2(end.x - 4, end.y - 4), ImVec2(end.x + 4, end.y + 4), col);
		}

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

		for (int axis = 0; axis < 3; axis++) {
			if (!ptsValid[axis]) continue;
			bool isActive = (_ActiveAxis == axis);
			ImU32 col = AxisColor(axis, isActive);
			for (int i = 0; i < segments; i++) {
				drawList->AddLine(pts[axis][i], pts[axis][i + 1], col, isActive ? 3.0f : 2.0f);
				if (_ActiveAxis == -1) {
					float d = ScreenLength(ScreenSub(mouse, pts[axis][i]));
					if (d < bestDist) { bestDist = d; hoveredAxis = axis; }
				}
			}
		}

		if (hoveredAxis != -1 && _ActiveAxis == -1) {
			for (int i = 0; i < segments; i++)
				drawList->AddLine(pts[hoveredAxis][i], pts[hoveredAxis][i + 1], IM_COL32(255, 255, 255, 255), 3.0f);
		}

		if (_ActiveAxis == -1 && hoveredAxis != -1 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
			&& !ImGui::IsAnyItemActive()) {
			_ActiveAxis = hoveredAxis;
			_DragStartAngle = atan2f(mouse.y - origin.y, mouse.x - origin.x);
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