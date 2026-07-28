#pragma once

#include "ScriptEngine.h"
#include <thread>
#include <filesystem>
#include <atomic>

namespace Cora {
	class ScriptManager
	{
	public:
		struct Config {
			fs::path managedDllPath;
			fs::path scriptDir;
			bool enableWatcher = true;
		};

		bool Initialize(const Config& config) {
			_Config = config;
 
			if (!_Engine.Initialize(_Config.managedDllPath)) {
				return false;
			}

			if (_Engine.LoadScripts(_Config.scriptDir) != 0) {
				std::cerr << "[ScriptManager] Initial script load failed." << std::endl;
			}

			if (_Config.enableWatcher) {
				_IsWatching = true;
				_WatcherThread = std::thread([this]() {
					_Engine.StartWatcher(_Config.scriptDir.string());
				});
			}

			return true;
		} 

		void ScriptTick(float deltaTime)
		{
			_Engine.Update(deltaTime);
		}

		void ReloadScipts() {
			_Engine.LoadScripts(_Config.scriptDir());
		}

		void Shutdown() {
			_IsWatching = false
			_Engine.Shutdown();

			if (_WatcherThread.joinable()) {
				_WatcherThread.detach();
			}
		}

	private:
		ScriptEngine _Engine;
		Config _Config;
		std::thread _WatcherThread;
		std::atomic<bool> _IsWatching{ false };
	};
}


