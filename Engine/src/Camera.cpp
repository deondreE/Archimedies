#include "archpch.h"
#include "Camera.h"
#include "Input.h"

namespace Engine {
	
	Camera::Camera(float fovDegrees, float aspectRatio, float nearClip, float farClip) 
		: _FOV(fovDegrees), _AspectRatio(aspectRatio), _NearClip(nearClip), _FarClip(farClip)
	{
		RecalculateProjection();
		RecalculateView();
	}

	void Camera::OnResize(uint32_t width, uint32_t height) {
		if (height == 0) return; // minimized window, avoid divide by zero error;
		_AspectRatio = static_cast<float>(width) / static_cast<float>(height);
		RecalculateProjection();
	}

	void Camera::RecalculateView() {
		Math::Vec3 forward = {
			cosf(_Pitch) * sinf(_Yaw),
			sinf(_Pitch),
			cosf(_Pitch) * cosf(_Yaw)
		};
		Math::Vec3 target = _Position + forward;
		Math::Vec3 up = { 0.0f, 1.0f, 0.0f };

		_View = Math::Mat4::LookAt(_Position, target, up);
		_ViewProjection = _View * _Projection;
	}

	void Camera::RecalculateProjection() {
		_Projection = Math::Mat4::Perspective(_FOV, _AspectRatio, _NearClip, _FarClip);
		_ViewProjection = _View * _Projection;
	}
}