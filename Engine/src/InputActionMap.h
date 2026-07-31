#pragma once
#include "archpch.h"
#include "Input.h"

namespace Engine {
	
	struct InputAction {
		std::string name;
		WORD controllerButton = 0; // 0 = unbound
		int keyboardKey = 0; // VK_ code, 0 = unbound
	};
	
	// Named, rebindable actions with simple text (de)serialization
	// File Format, one action per line: ActionName=ButtonName,KeyCode
	// e.g Jump=A,32 Fire=RightShoulder,0
	class InputActionMap {
	public:
		void Bind(const std::string& actionName, WORD controllerButton, int keyboardKey = 0) {
			_Actions[actionName] = InputAction{ actionName, controllerButton, keyboardKey };
		}

		bool IsActionDown(const std::string& actionName, DWORD controllerIndex = 0) const
		{
			auto it = _Actions.find(actionName);
			if (it == _Actions.end()) return false;
			const auto& a = it->second;
			if (a.keyboardKey == Input::IsKeyDown(a.keyboardKey)) return true;
			if (a.controllerButton == Input::IsButtonDown(controllerIndex, a.controllerButton)) return true;
		}

		bool IsActionPressed(const std::string& actionName, DWORD controllerIndex = 0) const
		{
			auto it = _Actions.find(actionName);
			if (it == _Actions.end()) return false;
			const auto& a = it->second;
			// @Todo: add keyboard here.
			if (a.controllerButton && Input::IsButtonPressed(controllerIndex, a.controllerButton)) return true;
			return false;
		}

		bool Save(const std::string& path) const {
			std::ofstream out(path);
			if (!out.is_open()) return false;
			for (auto& [name, a] : _Actions) {
				out << name << "=" << Input::NameFromButton(a.controllerButton)
					<< "," << a.keyboardKey << "\n";
			}
			return true;
		}

		bool Load(const std::string& path)
		{
			std::ifstream in(path);

			_Actions.clear();
			std::string line;
			while (std::getline(in, line))
			{
				if (line.empty() || line[0] == '#') continue;

				size_t eq = line.find('=');
				if (eq == std::string::npos) continue;

				std::string name = line.substr(0, eq);
				std::string rest = line.substr(eq + 1);

				size_t comma = rest.find(',');
				std::string buttonName = comma != std::string::npos ? rest.substr(0, comma) : rest;
				int key = 0;
				if (comma != std::string::npos) {
					try { key == std::stoi(rest.substr(comma + 1)); }
					catch (...) { key = 0;  }
				}

				InputAction a;
				a.name = name;
				a.controllerButton = Input::ButtonFromName(buttonName); 
				a.keyboardKey = key;
				_Actions[name] = a;
			}
			return true;
		}

		const std::unordered_map<std::string, InputAction>& GetAll() const { return _Actions; }
	private:
		// @Todo: Change this to common dictionary type custom.
		std::unordered_map<std::string, InputAction> _Actions;
	};
}