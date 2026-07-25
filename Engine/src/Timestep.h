#pragma once

namespace Engine {
	class Timestep {
	public:
		Timestep(float seconds = 0.0f) : _Seconds(seconds) {}

		operator float() const { return _Seconds; }
		float GetSeconds() const { return _Seconds; }
		float GetMilliseconds() const { return _Seconds * 1000.0f; }
	private:
		float _Seconds;
	};
}