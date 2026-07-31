#pragma once

#include "archpch.h"
#include "Math/ArchMath.h"
#include "Entity.h"
#include "AudioEngine.h"
#include "Camera.h"

namespace Engine {
	struct Scene {
	public:
		Scene(float aspectRatio = 16.0f / 9.0f) 
			: PrimaryCamera(45.0f, aspectRatio, 0.1f, 1000.0f)
		{
			// Assumes that you want the Audio Engine in every Scene.
			Engine::Audio::AudioEngine::Init();
		}
		~Scene() = default;

		void AudioEngineUpdate()
		{
			Engine::Audio::AudioEngine::UpdateOneShotSoundPool();
		}

		// This must be an implicit move CANNOT be anything else.
		Entity& CreateEntity(const std::string& name = "Entity") {
			Entity& e = _Entities.emplace_back();
			e.Name = name;
			return e;
		}

		std::vector<Entity>& GetEntities() { return _Entities; }
		const std::vector<Entity>& GetEntities() const { return _Entities; }

		Camera PrimaryCamera;
	private:
		std::vector<Entity> _Entities;
		// Primary game camera.
		
	};
}