#pragma once
#include "archpch.h"

namespace Engine::Component 
{
	class Component 
	{
	public:
		Component(std::string name = "Component") : _DebugName(name), _ID() {}
		virtual ~Component() = default;

		virtual void ComponentAwake()  {}
		virtual void ComponentUpdate() {}

		// Each component could have a specfic way it wants it's data to render.
		virtual void OnImGuiRender() {}

		const std::string& GetDebugName() const { return _DebugName; }
		const UUID GetComponentID() const { return _ID; }
	protected:
		UUID _ID;
		std::string _DebugName;
	};
}