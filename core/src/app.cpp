#include "app.hpp"

namespace arc {
	void App::run() {
		while (!window.shouldClose()) {
			glfwPollEvents();
		}
	}
}
