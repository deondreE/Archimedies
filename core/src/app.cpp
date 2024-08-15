#include "app.hpp"

namespace arc {
	void App::run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}
}
