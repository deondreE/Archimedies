#include "archpch.h"
#include "Input.h"

namespace Engine{
	HWND Input::s_Hwnd = nullptr;
	std::array<Input::ControllerState, Input::kMaxControllers>
		Input::s_Controllers{};
	float Input::s_DisconnectedTimer = 0.0f;
}