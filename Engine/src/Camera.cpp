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

	void Camera::OnUpdate(Timestep ts) {
		// Flat (Y-ignoring) forward/right for WASD ground movement
		Math::Vec3 forward = { sinf(_Yaw), 0.0f, cosf(_Yaw) };
		Math::Vec3 right = { cosf(_Yaw), 0.0f, -sinf(_Yaw) };

		float speed = _MoveSpeed * ts.GetSeconds();
		
		if (Input::IsKeyDown('W')) _Position = _Position + forward * speed;
		if (Input::IsKeyDown('S')) _Position = _Position - forward * speed;
		if (Input::IsKeyDown('D')) _Position = _Position + right * speed;
		if (Input::IsKeyDown('A')) _Position = _Position - right * speed;
		if (Input::IsKeyDown(VK_SPACE)) _Position.y += speed;
		if (Input::IsKeyDown(VK_SHIFT)) _Position.y -= speed;

		float turn = _LookSpeed * ts.GetSeconds();
		if (Input::IsKeyDown(VK_LEFT)) _Yaw -= turn;
		if (Input::IsKeyDown(VK_RIGHT)) _Yaw += turn;
		if (Input::IsKeyDown(VK_UP)) _Yaw += turn;
		if (Input::IsKeyDown(VK_DOWN)) _Yaw -= turn;

		RecalculateView();
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