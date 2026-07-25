#pragma once
#include "archpch.h"

namespace Engine {
	// @TODO: Controller Input
	class Input {
	public:
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
	private:
			static HWND s_Hwnd;
	};
}
