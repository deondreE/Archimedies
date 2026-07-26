#pragma once
#include "archpch.h"

namespace Engine {
	// Direction points FROM the light TWOARDS the scene.
	struct DirectionalLight {
		Math::Vec3 Direction = { -2.0f, 1.0f, 5.0f };
		float Itensity = 0.25f;

		float Color[3] = { 1.0f, 0.45f, 1.0f };
		float AmbientStrength = 0.15f; // small ambient term so unlit faces aren't pure black.
	};
}