#pragma once
#include "Timestep.h"
#include "archpch.h"

namespace Engine {
	class Camera {
	public:
		Camera(float fovDegrees, float aspectRatio, float nearClip, float farClip);

		void OnUpdate(Timestep ts);
		void OnResize(uint32_t width, uint32_t height);

		const Math::Mat4& GetViewProjection() const { return _ViewProjection;  }
		const Math::Vec3& GetPosition() const { return _Position; }
	private:
		void RecalculateView();
		void RecalculateProjection();

	private:
		Math::Vec3 _Position = { 0.0f, 0.0f, -5.0f };
		float _Yaw = 0.0f;   // radians, rotation around Y (left/right)
		float _Pitch = 0.0f; // radians, rotation around X (up/down)

		float _FOV;
		float _AspectRatio;
		float _NearClip;
		float _FarClip;

		float _MoveSpeed = 5.0f;
		float _LookSpeed = 1.5f;

		Math::Mat4 _View;
		Math::Mat4 _Projection;
		Math::Mat4 _ViewProjection;
	};
}

