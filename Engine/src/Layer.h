#pragma once
#include "archpch.h"
#include "Timestep.h"
#include "Event.h"

namespace Engine {

	class Layer {
	public:
		Layer(const std::string& name = "Layer") : _DebugName(name) {}
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep ts) {}
		virtual void OnRender() {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& e) {}

		const std::string& GetName() const { return _DebugName; }

	protected: 
		std::string _DebugName;
	};
}