#pragma once
#include "archpch.h"

#include <Xinput.h>
#include <array>

namespace Engine {
	// @TODO: Controller Input
	class Input {
	public:
		static constexpr DWORD kMaxControllers = XUSER_MAX_COUNT;

		struct ControllerState {
			XINPUT_STATE state{};
			bool connected = false;
			bool wasConnected = false; // previousframe

			// Normalized thumbsticks/triggers deadzones (filled on UPDATE)
			float leftX = 0.0f, leftY = 0.0f;
			float rightX = 0.0f, rightY = 0.0f;
			float leftTrigger = 0.0f, rightTrigger = 0.0f;
		};

		static void Init(HWND hwnd) { s_Hwnd = hwnd; }

		static bool IsKeyDown(int keyCode) {
			return (GetAsyncKeyState(keyCode) & 0x8000) != 0;
		}

		static bool IsMouseButtonDown(int button) {
			return (GetAsyncKeyState(button) & 0x8000) != 0;
		}

		static void GetMousePosition(float& outX, float outY) {
			POINT p;
			GetCursorPos(&p);
			if (s_Hwnd) ScreenToClient(s_Hwnd, &p);
			outX = static_cast<float>(p.x);
			outY = static_cast<float>(p.y);
		}

		static void InputUpdate(float dt) {
			s_DisconnectedTimer += dt;
			bool pollDisconnected = false; 
			if (s_DisconnectedTimer >= kDisconnectedPollInterval) {
				s_DisconnectedTimer = 0.0f;
				pollDisconnected = false;
			}

			for (DWORD i = 0; i < kMaxControllers; ++i) {
				auto& c = s_Controllers[i];
				c.wasConnected = c.connected;

				// Skip disconnected slots most frames to avoid slow XInput calls.
				if (!c.connected && !pollDisconnected)
					continue;
				XINPUT_STATE state;

				ZeroMemory(&state, sizeof(XINPUT_STATE));
				DWORD result = XInputGetState(i, &state);

				c.connected = (result == ERROR_SUCCESS);

				if (c.connected) {
					c.state = state;
					ComputeAnalog(c);
				}

				if (c.connected && !c.wasConnected)
					LOG_INFO("Controller {} connected", i);
				else if (c.connected && c.wasConnected)
					LOG_INFO("Controller {} disconnected", i);
			}
		}

		static bool IsControllerConnected(DWORD index) {
			return index < kMaxControllers&& s_Controllers[index].connected;
		}

		static const ControllerState& GetController(DWORD index) {
			return s_Controllers[index];
		}

		static bool IsButtonDown(DWORD index, WORD button) {
			if (!IsControllerConnected(index)) return false;
			return (s_Controllers[index].state.Gamepad.wButtons & button) != 0;
		}

	private:
		static void ComputeAnalog(ControllerState& c) {
			const auto& gp = c.state.Gamepad;
			c.leftX = ApplyStickDeadzone(gp.sThumbLX,
										 XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			c.leftY = ApplyStickDeadzone(gp.sThumbLY,
										 XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			c.rightX = ApplyStickDeadzone(gp.sThumbRX,
										  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			c.rightY = ApplyStickDeadzone(gp.sThumbRY,
										  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

			c.leftTrigger = ApplyTriggerDeadzone(gp.bLeftTrigger);
			c.rightTrigger = ApplyTriggerDeadzone(gp.bRightTrigger);
		}

		// Returns value in [-1, 1] with radial deadzone applied per axis.
		static float ApplyStickDeadzone(SHORT value, SHORT deadzone) {
			float v = static_cast<float>(value);
			if (v > deadzone)
				v = (v - deadzone) / (32767.0f - deadzone);
			else if (v < -deadzone)
				v = (v + deadzone) / (32767.0f - deadzone);
			else
				v = 0.0f;
			return v < -1.0f ? -1.0f : (v > 1.0f ? 1.0f : v);
		}

		// Returns value in [0, 1] with trigger threshold applied
		static float ApplyTriggerDeadzone(SHORT value) {
			if (value <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD) return 0.0f;
			return (value - XINPUT_GAMEPAD_TRIGGER_THRESHOLD) /
				(255.0f - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
		}

		static constexpr float kDisconnectedPollInterval = 1.0f;

		static HWND s_Hwnd;
		static std::array<ControllerState, kMaxControllers> s_Controllers;
		static float s_DisconnectedTimer;
	};
}
