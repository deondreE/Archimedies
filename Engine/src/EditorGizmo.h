#pragma once
#include "archpch.h"

namespace Engine {
	enum class GizmoMode {
		Translate,
		Rotate,
		Scale
	};

	using WorldScreenFn = std::function<bool(const Math::Vec3& world, ImVec2& outScreen)>;

	class EditorGizmo {
	public:
		bool Manipulate(
			const WorldScreenFn& worldToScreen,
			Math::Vec3& posistion,
			Math::Vec3& rotation,
			Math::Vec3& scale,
			GizmoMode mode,
			float gizmoScreenSize = 90.0f);

		void SetMode(GizmoMode mode) { _Mode = mode; }
		GizmoMode GetMode() const { return _Mode; }
		bool IsUsing() const { return _ActiveAxis != -1; }
	private:
		GizmoMode _Mode = GizmoMode::Translate;
		int _ActiveAxis = -1;

		ImVec2 _AxisScreenDir[3];
		float _AxisScreenLen[3];
		ImVec2 _OriginScreen;
		float _DragStartAngle = 0.0f;
		float _RotateArcStart = 0.0f;
		Math::Vec3 _DragStartVec{};

		bool ManipulateTranslate(const WorldScreenFn& w2s, Math::Vec3& position, float size);
		bool ManipulateScale(const WorldScreenFn& w2s, Math::Vec3& position, Math::Vec3& scale, float size);
		bool ManipulateRotate(const WorldScreenFn& w2s, const Math::Vec3& position, Math::Vec3& rotation, float size);
	};
}