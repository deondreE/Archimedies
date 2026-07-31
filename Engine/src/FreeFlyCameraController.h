#pragma once
#include "Camera.h" 
#include "Input.h"
#include "Timestep.h" 

namespace Engine {
	// Editor Camera Controls
	class FreeFlyCameraController {
	public:
		explicit FreeFlyCameraController(Camera& camera) : _Camera(camera) {}

		// @TODO: Should make this on fixed update maybe
		void OnUpdate(Timestep ts) {
			float yaw = _Camera.GetYaw();
			Math::Vec3 forward = { sinf(yaw), 0.0f, cosf(yaw) };
			Math::Vec3 right = { cosf(yaw), 0.0f, -sinf(yaw) };

			Math::Vec3 pos = _Camera.GetPosition();
			float speed = _MoveSpeed * ts.GetSeconds();

			if (Input::IsKeyDown('W')) pos = pos + forward * speed;
			if (Input::IsKeyDown('S')) pos = pos - forward * speed;
			if (Input::IsKeyDown('D')) pos = pos + right * speed;
			if (Input::IsKeyDown('A')) pos = pos - right * speed;
			if (Input::IsKeyDown(VK_SPACE)) pos.y += speed;
			if (Input::IsKeyDown(VK_SHIFT)) pos.y -= speed;

			float turn = _LookSpeed * ts.GetSeconds();
			float newYaw = yaw;
			if (Input::IsKeyDown(VK_LEFT)) yaw -= turn;
			if (Input::IsKeyDown(VK_RIGHT)) yaw += turn;
			if (Input::IsKeyDown(VK_UP)) yaw += turn;
			if (Input::IsKeyDown(VK_DOWN)) yaw -= turn;

			_Camera.SetPosition(pos);
			_Camera.SetYawPitch(newYaw, _Camera.GetPitch());
		} // 

	private:
		Camera& _Camera;
		float _MoveSpeed = 5.0f;
		float _LookSpeed = 2.0f;
	};
}