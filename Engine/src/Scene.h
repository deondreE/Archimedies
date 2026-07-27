#pragma once

#include "archpch.h"
#include "Math/ArchMath.h"
#include "Entity.h"

namespace Engine {
	struct Scene {
	public:
		Scene() = default;
		~Scene() = default;

		Entity& CreateEntity(const std::string& name = "Entity") {
			Entity e;
			e.Name = name;
			_Entities.push_back(e);
			return _Entities.back();
		}

		std::vector<Entity>& GetEntities() { return _Entities; }
		const std::vector<Entity>& GetEntities() const { return _Entities; }

	private:
		std::vector<Entity> _Entities;
	};
}