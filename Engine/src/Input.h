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
			WORD prevButtons = 0;
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
				pollDisconnected = true;
			}

			for (DWORD i = 0; i < kMaxControllers; ++i) {
				auto& c = s_Controllers[i];
				c.wasConnected = c.connected;
				c.prevButtons = c.state.Gamepad.wButtons;

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
				else if (!c.connected && c.wasConnected)
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

		// True only on the frame the button transitions up -> down
		static bool IsButtonPressed(DWORD index, WORD button) {
			if (!IsControllerConnected(index)) return false;
			const auto& c = s_Controllers[index];
			bool now = (c.state.Gamepad.wButtons & button) != 0;
			bool before = (c.prevButtons & button) != 0;
			return now && !before;
		}

		// True only on the frame the button transitions down -> up
		static bool IsButtonReleased(DWORD index, WORD button) {
			if (!IsControllerConnected(index)) return false;
			const auto& c = s_Controllers[index];
			bool now = (c.state.Gamepad.wButtons & button) != 0;
			bool before = (c.prevButtons & button) != 0;
			return !now && before;
		}

		// --- Name <-> button lookup, used by InputActionMap for (de)serialization ---
		static WORD ButtonFromName(const std::string& name) {
			auto it = s_ButtonNames.find(name);
			return it != s_ButtonNames.end() ? it->second : 0;
		}

		static std::string NameFromButton(WORD button) {
			for (auto& [name, val] : s_ButtonNames)
				if (val == button) return name;
			return "None";
		}

	private:
		static void ComputeAnalog(ControllerState& c) {
			const auto& gp = c.state.Gamepad;
			c.leftX = ApplyStickDeadzone(gp.sThumbLX,
										 XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			c.leftY = ApplyStickDeadzone(gp.sThumbLY,
										 XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			c.rightX = ApplyStickDeadzone(gp.sThumbRX,
										  XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
			c.rightY = ApplyStickDeadzone(gp.sThumbRY,
										  XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

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

		static inline const std::unordered_map<std::string, WORD> s_ButtonNames = {
			{ "DPadUp",       XINPUT_GAMEPAD_DPAD_UP },
			{ "DPadDown",     XINPUT_GAMEPAD_DPAD_DOWN },
			{ "DPadLeft",     XINPUT_GAMEPAD_DPAD_LEFT },
			{ "DPadRight",    XINPUT_GAMEPAD_DPAD_RIGHT },
			{ "Start",        XINPUT_GAMEPAD_START },
			{ "Back",         XINPUT_GAMEPAD_BACK },
			{ "LeftThumb",    XINPUT_GAMEPAD_LEFT_THUMB },
			{ "RightThumb",   XINPUT_GAMEPAD_RIGHT_THUMB },
			{ "LeftShoulder", XINPUT_GAMEPAD_LEFT_SHOULDER },
			{ "RightShoulder",XINPUT_GAMEPAD_RIGHT_SHOULDER },
			{ "A",            XINPUT_GAMEPAD_A },
			{ "B",            XINPUT_GAMEPAD_B },
			{ "X",            XINPUT_GAMEPAD_X },
			{ "Y",            XINPUT_GAMEPAD_Y },
		};
	};
}
