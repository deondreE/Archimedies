#pragma once

namespace Engine {
	class Timestep {
	public:
		Timestep(float seconds = 0.0f) : _Seconds(seconds) {}

		operator float() const { return _Seconds; }
		float GetSeconds() const { return _Seconds; }
		float GetMilliseconds() const { return _Seconds * 1000.0f; }

		static Timestep Fixed(float hz) { return Timestep(1.0 / hz); }

		/*
		 * Returns the interpolation factor between two fixed steps.
		 * Used for: RenderPos = CurrentPhysPos * alpha + PreviousPhysPos * (1.0 - alpha)
		 */
		static float GetAlpha(float accumulator, float fixedInterval) {
			return accumulator / fixedInterval;
		}
	private:
		float _Seconds;
	};
}